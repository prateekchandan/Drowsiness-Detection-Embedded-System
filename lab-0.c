#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include"driverlib/adc.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE,1,0,ADC_CTL_CH5|ADC_CTL_IE|ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 1);

	ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE,2,0,ADC_CTL_CH4|ADC_CTL_IE|ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 2);
}


void switchPinConfig(void)
{
	// GPIO PORTA Pin 5 and Pin6
	GPIODirModeSet(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2|GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

uint32_t irValue[1];
//int irValueD = 0;
uint32_t hrValue[1];
int main(void)
{

	setup();

	switchPinConfig();

	while(1){
		//irValueA = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5);
		//irValueD = GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_6);

		ADCIntClear(ADC0_BASE, 1);
		ADCProcessorTrigger(ADC0_BASE, 1);
		ADCIntClear(ADC0_BASE, 2);
		ADCProcessorTrigger(ADC0_BASE, 2);
		while(!ADCIntStatus(ADC0_BASE, 1, false) && !ADCIntStatus(ADC0_BASE, 2, false))
		{
		}


		//read the ADC value from the ADC Sample Sequencer 1 FIFO
		ADCSequenceDataGet(ADC0_BASE, 1, irValue);
		ADCSequenceDataGet(ADC0_BASE, 2, hrValue);

		SysCtlDelay(10);


	}


}
