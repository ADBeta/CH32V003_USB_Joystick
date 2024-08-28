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


typedef struct {
	uint16_t min;
	uint16_t max;
	uint16_t cur;

} joystick_axis_t;


static joystick_axis_t axis_rot = {200, 800};
static joystick_axis_t axis_hor = {200, 800};
static joystick_axis_t axis_ver = {200, 800};


void get_joystick_values(const GPIO_ANALOG_CHANNEL chan, joystick_axis_t *axis)
{
	axis->cur = gpio_analog_read(chan);
	if(axis->cur > axis->max)
	{
		axis->max = axis->cur;
	} else if (axis->cur < axis->min)
	{
		axis->min = axis->cur;
	}
}

uint8_t get_joystick_percent(const joystick_axis_t *axis)
{
	float slope = 100.0 / (axis->max - axis->min);
	return slope * (axis->cur - axis->min);
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

		printf("rot: %d\n", get_joystick_percent(&axis_rot));
		printf("hor: %d\n", get_joystick_percent(&axis_hor));
		printf("ver: %d\n\n", get_joystick_percent(&axis_ver));


		// Print the state values
		//printf("%d\t%d\t%d\t%d\n", btn_top, pot_rot, pot_hor, pot_ver);

		// Wait for next loop
		Delay_Ms(50);
	}
}
