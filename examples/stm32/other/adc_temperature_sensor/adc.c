/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Thomas Otto <tommi@viadmin.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stm32/rcc.h>
#include <stm32/flash.h>
#include <stm32/gpio.h>
#include <stm32/usart.h>
#include <stm32/adc.h>

void usart_setup(void)
{
	/* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_USART1EN);

	/* Setup GPIO pin GPIO_USART1_TX/GPIO9 on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
	              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

void gpio_setup(void)
{
	/* Enable GPIOB clock. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

	/* Set GPIO6/7 (in GPIO port B) to 'output push-pull' for the LEDs. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
	              GPIO_CNF_OUTPUT_PUSHPULL, GPIO6);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
	              GPIO_CNF_OUTPUT_PUSHPULL, GPIO7);
}

void adc_setup(void)
{
	int i;
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN);

	/* make shure it didnt run during config */
	adc_off(ADC1);

	/* we configure everything for one single conversion */	
	adc_disable_scan_mode(ADC1);
	adc_set_single_conversion_mode(ADC1);
	adc_enable_discontinous_mode_regular(ADC1);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	/* we want read out the temperature sensor so we have to enable it */
	adc_enable_temperature_sensor(ADC1);
	adc_set_conversion_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC); 
	
	adc_on(ADC1);
	/* wait for adc starting up*/
        for (i = 0; i < 80000; i++);    /* Wait (needs -O0 CFLAGS). */

	adc_reset_calibration(ADC1);
	adc_calibration(ADC1);	
}

void my_usart_print_int(u32 usart, int value)
{
	s8 i;
	u8 nr_digits = 0;
	char buffer[25];

	if (value < 0) {
		 usart_send(usart, '-');
		 value = value * -1;
	}
	
	while (value > 0) {
		buffer[nr_digits++] = "0123456789"[value%10];
		value = value/10;
	}
	
	for (i=nr_digits; i>=0; i--) {
		usart_send(usart, buffer[i]);
	}
}

int main(void)
{
	u8 channel_array[16];
	u16 temperature;

	rcc_clock_setup_in_hse_16mhz_out_72mhz();
	gpio_setup();
	usart_setup();
	adc_setup();

	gpio_clear(GPIOB, GPIO7);	/* LED1 on */
	gpio_set(GPIOB, GPIO6);		/* LED2 off */

	/* Send a message on USART1. */
	usart_send(USART1, 's');
	usart_send(USART1, 't');
	usart_send(USART1, 'm');
	usart_send(USART1, '\r');
	usart_send(USART1, '\n');

	/* Select the channel we want to convert. 16=temperature_sensor */
	channel_array[0] = 16;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	
	/* If the ADC_CR2_ON bit is already set -> setting it another time starts the conversion */
	adc_on(ADC1);
	
	/* Waiting for end of conversion */
	while (!(ADC_SR(ADC1) & ADC_SR_EOC));
	
	temperature = ADC_DR(ADC1);

	/* thats actually not the real temparature - you have to compute it like described in the datasheet */	
	my_usart_print_int(USART1, temperature);

	gpio_clear(GPIOB, GPIO6); /* LED2 on */

	while(1); /* Halt. */

	return 0;
}
