/******************************************************************************
* USB Joystick handler for a 3 Axis & 1 Button Jotstick + 4 Auxillary buttons
* and a Left-handed and Right-handed mode selection
*
* Pinout:
*   Handedness Switch      C0
*
*   Joystick Button        D5
*   Auxiliary Button 1     C1
*	Auxiliary Button 2     C2
*   Auxiliary Button 3     C3
*   Auxiliary Button 4     C4
*
*   Horizontal             A2 (A1)
*   Vertical               A1 (A0)
*   Rotation               D6 (A6)
*
* Notes:
*   USB Gamepad Joystick Axis must be in range between -128 and 128
*   Send 4 Bytes to the USB Buffer, [0] [1] and [2] are Axis. [3] is an 8bit
*   button mask (8 Buttons Max)
*
*   A Switch on the device can switch it between Right and Left Handed modes
*
* (c) ADBeta 2024
* Sep 2024    Ver0.9
******************************************************************************/
#include "ch32v003fun.h"
#include "rv003usb.h"
#include <stdio.h>

#include "lib_gpioctrl.h"

/*** Joystick Pinout *********************************************************/
#define SW_HANDEDNESS  GPIO_PC0 
#define BTN_JST_PIN    GPIO_PD5
#define BTN_AUX_1_PIN  GPIO_PC4
#define BTN_AUX_2_PIN  GPIO_PC3
#define BTN_AUX_3_PIN  GPIO_PC2
#define BTN_AUX_4_PIN  GPIO_PC1

#define AXIS_HOR_PIN GPIO_ADC_A1
#define AXIS_VER_PIN GPIO_ADC_A0
#define AXIS_ROT_PIN GPIO_ADC_A6

/*** Constant Variables ******************************************************/
// Linear Mapping variables, Magic numbers to speed up execution
// INPUT is   (1.0 * (axis->max - axis->min)
// OUTPUT is  (1.0 * (127 - (-128));
#define AXIS_MAP_INPUT_RANGE  ((float)(axis->max - (float)axis->min))
#define AXIS_MAP_OUTPUT_RANGE ((float)255.0)
// USB Gamepads expect analog variables between -128 and 128
#define AXIS_MAP_MINIMUM    -128
#define AXIS_MAP_MAXIMUM     128

// Mapping output Hysteresis variables
#define AXIS_HYST_CEIL   15
#define AXIS_HYST_MIDL   0
#define AXIS_HYST_FLOOR -15


/*** Type definitions ********************************************************/
// Left / Right Handedness
typedef enum {
	RIGHT_HANDED_MODE,
	LEFT_HANDED_MODE,
} handedness_t;

// Joystick Axis
typedef struct {
	// ADC Raw min/max/current values
	uint16_t min;
	uint16_t max;
	uint16_t cur;

	// Output value from mapping to a new range and applying deadzone
	int8_t mapped;
} joystick_axis_t;

/*** Global Variables ********************************************************/
// Joystick Axis
static joystick_axis_t g_axis_hor = {190, 810};
static joystick_axis_t g_axis_ver = {190, 810};
static joystick_axis_t g_axis_rot = {50,  950};

// Button bit mask. Up to 8 Buttons
uint8_t g_button_mask = 0x00;


/*** Aux function definitions ************************************************/
/// @brief Gets the Axis potentiometer value directly from the ADC, adjust
/// minimum and maximum ends of travel if they are exceeded
/// @param GPIO_ANALOG_CHANNEL, gpioctrl Analog pin to read from
/// @param joystick_axis_t, axis to update
/// @return none
void get_joystick_values(const GPIO_ANALOG_CHANNEL chan, joystick_axis_t *axis)
{
	// Get the current value directly from the ADC Channel
	axis->cur = gpio_analog_read(chan);

	// Adjust the ends of travel if they are exceeded
	if(axis->cur > axis->max)
		axis->max = axis->cur;
	else if (axis->cur < axis->min)
		axis->min = axis->cur;
}

/// @brief maps the raw ADC joystick values to a new range between 
/// 0 and AXIS_MAXIMUM. Also applies deadzones
/// @param joystick_axis_t, axis to remap
/// @return none
void get_joystick_mapped(joystick_axis_t *axis)
{
	// Overall process of linear mapping the ADC Values
	// float slope = 1.0 * (128 - (-128)) / (1024 - 0);
	// float output = -128 + slope * (input - 0) - 128;

	// Simplified Mapping the ADC Value into the new range
	const float slope = AXIS_MAP_OUTPUT_RANGE / AXIS_MAP_INPUT_RANGE;
	axis->mapped = slope * (axis->cur - axis->min) - AXIS_MAP_MAXIMUM;

	// Apply deadzone checks
	if(axis->mapped >= AXIS_HYST_FLOOR && axis->mapped <= AXIS_HYST_CEIL)
		axis->mapped = AXIS_HYST_MIDL;
}


/*** Main ********************************************************************/
int main()
{
	SystemInit();

	// Ensures USB re-numeration after bootloader or reset
	Delay_Ms(1);
	usb_setup();

	// Handedness Switch is a floating input
	gpio_set_mode(SW_HANDEDNESS, INPUT_FLOATING);

	// All Buttons are digital input, pulled HIGH
	gpio_set_mode(BTN_JST_PIN,   INPUT_PULLUP);
	gpio_set_mode(BTN_AUX_1_PIN, INPUT_PULLUP);
	gpio_set_mode(BTN_AUX_2_PIN, INPUT_PULLUP);
	gpio_set_mode(BTN_AUX_3_PIN, INPUT_PULLUP);
	gpio_set_mode(BTN_AUX_4_PIN, INPUT_PULLUP);

	// All Axes are analog input
	gpio_set_mode(GPIO_A1, INPUT_ANALOG);  // Horizontal
	gpio_set_mode(GPIO_A0, INPUT_ANALOG);  // Vertical
	gpio_set_mode(GPIO_A6, INPUT_ANALOG);  // Rotation

	// Initiliase the ADC to use 24MHz clock, and Sample for 73 Clock Cycles
	gpio_init_adc(ADC_CLOCK_DIV_2, ADC_SAMPLE_CYCLES_73);
	

	// Loop forever getting joystick data
	while(1)
	{
		// Read the buttons state(s). Invert them as they are PULLUP
		g_button_mask = 0x00                          |
			(!gpio_digital_read(BTN_JST_PIN))         |
			(!gpio_digital_read(BTN_AUX_1_PIN) << 1)  |
			(!gpio_digital_read(BTN_AUX_2_PIN) << 2)  |
			(!gpio_digital_read(BTN_AUX_3_PIN) << 3)  |
			(!gpio_digital_read(BTN_AUX_4_PIN) << 4);
	

		// Read the potentiometer values
		get_joystick_values(AXIS_ROT_PIN, &g_axis_rot);
		get_joystick_values(AXIS_HOR_PIN, &g_axis_hor);
		get_joystick_values(AXIS_VER_PIN, &g_axis_ver);

		
		// Invert Horizontal and Vertical values if the Handedness switch reads
		// RIGHT_HANDED_MODE (0). 
		// Do nothing if Switch is in LEFT_HANDED_MODE (1) (default)
		if((handedness_t)gpio_digital_read(SW_HANDEDNESS) == RIGHT_HANDED_MODE)
		{
			g_axis_hor.cur = (g_axis_hor.max - g_axis_hor.cur) + g_axis_hor.min;
			g_axis_ver.cur = (g_axis_ver.max - g_axis_ver.cur) + g_axis_ver.min;
		}


		// Map the analog values to -128 to 127
		get_joystick_mapped(&g_axis_rot);
		get_joystick_mapped(&g_axis_hor);
		get_joystick_mapped(&g_axis_ver);

		// Wait for next loop (50FPS Refresh) 
		Delay_Ms(20);
	}
}


/*** USB Functions ***********************************************************/
void usb_handle_user_in_request(struct usb_endpoint * e, 
								uint8_t * scratchpad,
								int endp, 
								uint32_t sendtok, 
								struct rv003usb_internal * ist )
{
	// If this endpoint is the one we want:
	if(endp)
	{
		// Create a buffer to send to the USB Host, using the Axis and Button
		// variables
		const uint8_t joystick_data[4] = {
			(uint8_t)g_axis_hor.mapped,
			(uint8_t)g_axis_ver.mapped,
			(uint8_t)g_axis_rot.mapped,
			g_button_mask
		};

		// Send that buffer to the USB Host
		// (4 bytes, CRC Enabled, using send token)
		usb_send_data(joystick_data, 4, 0, sendtok);
	}

	// If it's a control transfer, send empty to NACK
	else usb_send_empty(sendtok);
}
