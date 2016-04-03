/*
 * Code for Drowsiness Detection
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include"driverlib/adc.h"
#include"driverlib/interrupt.h"
#include"driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

#define defaultThresh 2000
/*

 * Function Name: setup()

 * Input: none

 * Output: none

 * Description: Set crystal frequency and enable GPIO Peripherals

 * Example Call: setup();

 */
void setup(void)
{
	// Setup the frequency of Microcontroller to be 40MHz
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	// Enable Perpheral GPIOD for inputs
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	// Enable perepheral GPIOF for LEDs and Switches
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	// EnableADC Peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE,1,0,ADC_CTL_CH5|ADC_CTL_IE|ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 1);

	ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE,2,0,ADC_CTL_CH4|ADC_CTL_IE|ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 2);
}

/*

 * Function Name: switchPinConfig()

 * Input: none

 * Output: none

 * Description: GPIO PORTD Pins to be set as ADC input

 * Example Call: switchPinConfig();

 */
void switchPinConfig(void)
{
	// GPIO PORTD Pins to be set as ADC input
	GPIODirModeSet(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2|GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTD_BASE,GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

/*

 * Function Name: led_pin_config()

 * Input: none

 * Output: none

 * Description: Set PORTF Pin 1, Pin 2, Pin 3 as output. On this pin Red, Blue and Green LEDs are connected.

 * Example Call: led_pin_config();

 */

void led_pin_config(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}

/*

 * Function Name: setupTimerInterrupt()

 * Input: none

 * Output: none

 * Description: Setup Timer Interrupt to calculate the heart rate and eye blink values every 2 milisecond

 * Example Call: setupTimerInterrupt();

 */

void setupTimerInterrupt(void)
{
	uint32_t ui32Period;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	/*
	 * Time Period = 2 milisecond
	 * So , frequency = 500
	 */
	ui32Period = (SysCtlClockGet() / 500) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);
}


// IR and HR values
uint32_t irValue[1];
uint32_t hrValue[1];

int main(void)
{

	setup();
	switchPinConfig();
	led_pin_config();
	setupTimerInterrupt();

	while(1){

	}
}

// Variables used for heartbeat calculation

volatile int BPM;
volatile int Signal;
volatile int IBI = 600;
volatile int Pulse = false;
volatile int QS = false;
volatile int rate[10];
volatile unsigned long sampleCounter = 0;
volatile unsigned long lastBeatTime = 0;
volatile int P =defaultThresh;
volatile int T = defaultThresh;
volatile int thresh = defaultThresh;
volatile int amp = defaultThresh;
volatile int firstBeat = true;
volatile int secondBeat = false;

void calcBPM(int sig){
	int i;

	Signal = sig;

	sampleCounter += 2;
	int N = sampleCounter - lastBeatTime;

	if(Signal < thresh && N > (IBI/5)*3){
		if (Signal < T){
			T = Signal;
		}
	}
	if(Signal > thresh && Signal > P){
		P = Signal;
	}

	if (N > 250){
		if ( (Signal > thresh) && (Pulse == false) && (N > ((IBI/5)*3) )){
			Pulse = true;
			IBI = sampleCounter - lastBeatTime;
			lastBeatTime = sampleCounter;
			if(secondBeat){
				secondBeat = false;
				for(i=0; i<=9; i++){
					rate[i] = IBI;
				}
			}
			if(firstBeat){
				firstBeat = false;
				secondBeat = true;

				return;
			}
			long long runningTotal = 0;
			for(i=0; i<=8; i++){
				rate[i] = rate[i+1];
				runningTotal += rate[i];
			}
			rate[9] = IBI;
			runningTotal += rate[9];
			runningTotal /= 10;
			BPM = 60000/runningTotal;
			QS = true;
		}
	}
	if (Signal < thresh && Pulse == true){
		Pulse = false;
		amp = P - T;
		thresh = amp/2 + T;
		P = thresh;
		T = thresh;
	}
	/*if (N > 2500){
	    thresh = defaultThresh;
	    P = defaultThresh;
	    T = defaultThresh;
	    firstBeat = true;
	    secondBeat = false;
	    lastBeatTime = sampleCounter;
	  }*/

}


void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
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

	calcBPM(hrValue[0]);
}
