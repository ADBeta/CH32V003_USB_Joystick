/******************************************************************************
* 
*
* C7
* D2 (A3)
* D3 (A4)
* D4 (A7)
*
* ADBeta (c)    27 Aug 2024
******************************************************************************/
#include "ch32v003fun.h"
#include <stdio.h>

#include "lib_gpioctrl.h"

#define BTN_TOP GPIO_PC7
#define POT_ROT GPIO_ADC_A3
#define POT_HOR GPIO_ADC_A4
#define POT_VER GPIO_ADC_A7

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
		uint8_t btn_top = gpio_digital_read(BTN_TOP);

		// Read the potentiometer values
		uint16_t pot_rot = gpio_analog_read(POT_ROT);
		uint16_t pot_hor = gpio_analog_read(POT_HOR);
		uint16_t pot_ver = gpio_analog_read(POT_VER);

		
		// Print the state values
		printf("%d\t%d\t%d\t%d\n", btn_top, pot_rot, pot_hor, pot_ver);

		// Wait for next loop
		Delay_Ms(50);
	}
}
