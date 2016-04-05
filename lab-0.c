/*
 * Code for Drowsiness Detection
 * COnnections :
 * Heart Rate Sensor:
 * Blac Wire - GND
 * White - VBUS
 * Grey - PD3
 *
 * TCRT500
 * A0(red tape)  - PD2
 * GND(yell) - GND
 * VCC(red) - +3.3V
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

#define defaultThresh 2500
#define numSamples 60 // NO of samples to take average from
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

	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0|GPIO_PIN_4;

	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
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
volatile int rate[numSamples];
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

	if(Signal < thresh /*&& N > (IBI/5)*3*/){
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
				for(i=0; i<=numSamples-1; i++){
					rate[i] = IBI;
				}
			}
			if(firstBeat){
				firstBeat = false;
				secondBeat = true;

				return;
			}
			long long runningTotal = 0;
			for(i=0; i<numSamples-1; i++){
				rate[i] = rate[i+1];
				runningTotal += rate[i];
			}
			rate[numSamples-1] = IBI;
			runningTotal += rate[numSamples-1];
			runningTotal /= numSamples;
			BPM = 60000/runningTotal;
			QS = true;
		}
	}
	if (Signal < thresh && Pulse == true){
		Pulse = false;
		amp = P - T;
		thresh = (amp*3)/4 + T;
		P = thresh;
		T = thresh;
	}
	if (N > 2500){
	    thresh = defaultThresh;
	    P = defaultThresh;
	    T = defaultThresh;
	    firstBeat = true;
	    secondBeat = false;
	    lastBeatTime = sampleCounter;
	  }

}
#define numBlinkSamples 10
volatile int ready=false;
volatile int blinkTime = -1;
volatile int midval = 0;
volatile int lowval = 0;
volatile int highval = 0;
volatile int lowcount = 0;
volatile int highcount = 0;
volatile int blinkDuration[numBlinkSamples];
volatile int curClosedCount = 0;
volatile int curClosedSum = 0;
volatile int iterator = 0;
volatile float averageBlinkDuration;
volatile int curThreshold = 1000;
volatile int avgThreshold = 200;
volatile int alreadyOpen = false;
int count = 0;
int countLimit = 5;

void calcEyeBlink(int sig){
	// Caliberation
	if(!ready){
		if (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00){
			//pressed
			if(lowcount>=1000)
				return;
			lowval = ((lowval * lowcount) + sig)/ (lowcount+1);
			lowcount+=1;
		}else if(lowcount>=1000){
			highval = ((highval * highcount) + sig) / (highcount+1);
			highcount+=1;
			if(highcount >=1000){
				ready = true;
				midval = (highval + lowval)/2;
			}
		}
	}else
	{
		if(GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00){
			lowval = 0;
			lowcount = 0;
			highval = 0;
			highcount = 0;
			ready = false;
			midval = 0;
			blinkTime = -1;
			curClosedCount = 0;
			curClosedSum = 0;
			iterator = 0;
			count = 0;
			return;
		}

		if(sig < midval){
			GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2,4);
			if (alreadyOpen && count < countLimit){
				count ++;
				return;
			}
			//closed eye
			curClosedCount += 1;
			if (curClosedCount >= curThreshold){
				GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_3,2);
			}
			alreadyOpen = false;
			count = 0;
		}
		else{
			GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2,0);
			if (!alreadyOpen && count < countLimit){
				count ++;
				return;
			}

			if(alreadyOpen)
				return;

			printf("%f\n", averageBlinkDuration);
			alreadyOpen = true;
			iterator = (iterator + 1)%numBlinkSamples;
			curClosedSum += curClosedCount - blinkDuration[iterator];
			blinkDuration[iterator] = curClosedCount;
			averageBlinkDuration = curClosedSum/(float)numBlinkSamples;
			curClosedCount = 0;
			if (averageBlinkDuration >= avgThreshold){
				GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_3,2);
			} else {
				GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_3,8);
			}
			count = 0;
		}
	}
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
	// BPM is in the variable BPM Now
	calcEyeBlink(irValue[0]);
	// blinkTime has the average blink time. -1 means no value
}
