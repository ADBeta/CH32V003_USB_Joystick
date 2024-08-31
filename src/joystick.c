/******************************************************************************
* USB Joystick handler for a 3 Axis, 1 Button Jotstick + 
*
* Pinout:
*   BTN        D5
*   Rotation   D6 (A6)
*   Horizontal A1 (A0)
*   Vertical   A2 (A1)
*
* Notes:
*   USB Gamepad Joystick Axis must be in range between -128 and 128
*   (Which bytes in buffer????
*   USB Gamepad Buttons (???)
*
* (c) ADBeta 2024
* Sep 2024    Ver0.0
******************************************************************************/
#include "ch32v003fun.h"
#include <stdio.h>

#include "lib_gpioctrl.h"

// Gamepad Pinouts
#define BTN_TOP_PIN  GPIO_PD5
#define AXIS_ROT_PIN GPIO_ADC_A6
#define AXIS_HOR_PIN GPIO_ADC_A1
#define AXIS_VER_PIN GPIO_ADC_A0

// Linear Mapping variales, Magic numbers to speed up execution
// INPUT is   (1.0 * (axis->max - axis->min)
// OUTPUT is  (1.0 * (127 - (-128));
#define AXIS_MAP_INPUT_RANGE  (1.0 * (axis->max - axis->min))
#define AXIS_MAP_OUTPUT_RANGE ((float)255.0)
// USB Gamepads expect analog variables between -128 and 128
#define AXIS_MAP_MINIMUM    -128
#define AXIS_MAP_MAXIMUM     128

// Mapping output Hysteresis variables
#define AXIS_HYST_CEIL   15
#define AXIS_HYST_MIDL   0
#define AXIS_HYST_FLOOR -15


/*** Type definitions ********************************************************/
typedef struct {
	// ADC Rave min/max/current values
	uint16_t min;
	uint16_t max;
	uint16_t cur;

	// Output value from mapping to a new range and applying deadzone
	int8_t mapped;
} joystick_axis_t;

static joystick_axis_t axis_rot = {50,  950};
static joystick_axis_t axis_hor = {200, 800};
static joystick_axis_t axis_ver = {200, 800};


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
	
	// BTN is a Digital Input, pulled HIGH
	gpio_set_mode(BTN_TOP_PIN, INPUT_PULLUP);

	// All Axes are Analog Input
	gpio_set_mode(GPIO_A6, INPUT_ANALOG);
	gpio_set_mode(GPIO_A1, INPUT_ANALOG);
	gpio_set_mode(GPIO_A0, INPUT_ANALOG);

	// Initiliase the ADC to use 24MHz clock, and Sample for 73 Clock Cycles
	gpio_init_adc(ADC_CLOCK_DIV_2, ADC_SAMPLE_CYCLES_73);
	

	while(1)
	{
		// Read the buttons state
		uint8_t btn_top = gpio_digital_read(BTN_TOP_PIN);

		// Read the potentiometer values
		get_joystick_values(AXIS_ROT_PIN, &axis_rot);
		get_joystick_values(AXIS_HOR_PIN, &axis_hor);
		get_joystick_values(AXIS_VER_PIN, &axis_ver);

		get_joystick_mapped(&axis_rot);
		get_joystick_mapped(&axis_hor);
		get_joystick_mapped(&axis_ver);

		printf("rot: %d\n", axis_rot.mapped);
		printf("hor: %d\n", axis_hor.mapped);
		printf("ver: %d\n\n", axis_ver.mapped);


		// Print the state values
		//printf("%d\t%d\t%d\t%d\n", btn_top, pot_rot, pot_hor, pot_ver);

		// Wait for next loop
		Delay_Ms(50);
	}
}
