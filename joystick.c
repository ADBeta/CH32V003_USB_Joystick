/******************************************************************************
* 
*
* C7
* D2 (A3)
* D3 (A4)
* D4 (A7)
*
* ADBeta (c)    28 Aug 2024
******************************************************************************/
#include "ch32v003fun.h"
#include <stdio.h>

#include "lib_gpioctrl.h"

#define BTN_TOP_PIN  GPIO_PC7
#define AXIS_ROT_PIN GPIO_ADC_A3
#define AXIS_HOR_PIN GPIO_ADC_A4
#define AXIS_VER_PIN GPIO_ADC_A7

#define AXIS_MAXIMUM    255
#define AXIS_HYST_CEIL  148
#define AXIS_HYST_MIDL  128
#define AXIS_HYST_FLOOR 108


typedef struct {
	// ADC Rave min/max/current values
	uint16_t min;
	uint16_t max;
	uint16_t cur;

	// Output value from mapping to a new range and applying deadzone
	uint16_t mapped;
} joystick_axis_t;


static joystick_axis_t axis_rot = {50,  950};
static joystick_axis_t axis_hor = {200, 800};
static joystick_axis_t axis_ver = {200, 800};


/// @brief Gets the Axis potentiometer value directly from the ADC, adjust
/// minimum and maximum ends of travel if they are exceeded
/// @param GPIO_ANALOG_CHANNEL, gpioctrl Analog pin to read from
/// @param joystick_axis_t, axis to update
/// @return none
void get_joystick_values(const GPIO_ANALOG_CHANNEL chan, joystick_axis_t *axis)
{
	// Get the current value directly from the ADC Channel
	axis->cur = gpio_analog_read(chan);

	// TODO: Make this neater
	// Adjust the ends of travel if they are exceeded
	if(axis->cur > axis->max)
	{
		axis->max = axis->cur;
	} else if (axis->cur < axis->min)
	{
		axis->min = axis->cur;
	}
}

/// @brief maps the raw ADC joystick values to a new range between 
/// 0 and AXIS_MAXIMUM. Also applies deadzones
/// @param joystick_axis_t, axis to remap
/// @return none
void get_joystick_mapped(joystick_axis_t *axis)
{
	// Map the ADC Value into the new range
	float slope = (float)AXIS_MAXIMUM / (axis->max - axis->min);
	axis->mapped = slope * (axis->cur - axis->min);

	// Apply deadzone checks
	if(axis->mapped >= AXIS_HYST_FLOOR && axis->mapped <= AXIS_HYST_CEIL)
		axis->mapped = AXIS_HYST_MIDL;
}


int main()
{
	SystemInit();
	
	// PC7 is a Digital Input, pulled HIGH
	gpio_set_mode(GPIO_PC7, INPUT_PULLUP);

	// PD2, PD3 and PD4 are Analog Input
	gpio_set_mode(GPIO_A3, INPUT_ANALOG);
	gpio_set_mode(GPIO_A4, INPUT_ANALOG);
	gpio_set_mode(GPIO_A7, INPUT_ANALOG);

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
