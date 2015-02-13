/*
 * ELEC 327 Spring15 Lab03
 * Coding up the mood ring - bonus
 *
 * BLUE LED: PORT2.1, RED LED: PORT2.5
 *
 * Heartbeat cycle "mood ring".
 *
 * The intensities of both red and blue LED gradually change from 0, to their
 * respective maximum brightness, and back to 0. This pattern is repeated at
 * a frequency of 0.5Hz. The maximum brightness of each of the LED is controlled
 * by the temperature. The maximum and minimum temperature is set before the LEDs
 * are turned on. When detected temperature equals or is smaller than minimum
 * temperature, maximum brightness of blue LED is set to duty period (it cannot
 * be bigger), and maximum brightness of red LED is set to 0 (turned off).
 * Similarly, when detected temperature equals or is greater than maximum
 * temperature, maximum brightness of blue LED is set to 0, and maximum brightness
 * of red LED is set to duty period. When detected temperature is between max and min
 * temperature range, maximum brightness of red LED goes up with temperature, and
 * maximum brightness of blue LED goes down with temperature.
 *
 * Author: 			Zichao Wang
 * Date Modified:	Feb 12, 2015
 *
 */
#include <msp430g2553.h>

/*   global variables  */
int counter = 1;	//counter for ISA service
int VALUE;			//value to store temperature
int MIN_TEMP;		//minimum temperature
int MAX_TEMP;		//maximum temperature
int RED_STEP = 0;	//step of brightness change for red LED
int BLUE_STEP = 0;	//step of brightness change for blue LED
int TA1CCR1_MAX;	//maximum brightness for red LED
int TA1CCR2_MAX;	//maximum brightness for blue LED
/**********************/

void main(void){

	/* WDT configuration  */
	WDTCTL = WDTPW+WDTTMSEL+WDTCNTCL+WDTSSEL+WDTIS1; // set to interval timer mode, select ACLK, divide by 512
	IE1 |= WDTIE; 									 // watchdog timer interrupt enabled
	/**********************/

	/* clock configuration*/
	BCSCTL1 &= ~XTS;
	BCSCTL1 = DIVA_1;				// ACLK divide by 2 to run 6KHz
	BCSCTL3 |= LFXT1S_2; 			// LFXT1 = VLO; select VLO as clock source
	IFG1 &= ~OFIFG; 				// Clear OSCFault flag, and allow VLO to be selected
	/**********************/

	/* pin configuration  */
	P2DIR |= BIT1 + BIT5; 			// pin2.1 and pin2.5 are selected as output
	P2SEL |= BIT1 + BIT5; 			// pin2.1 and pin2.5 are selected for PWM
	/**********************/

	/*timerA configuration*/
	TA1CCR0 = 80;             	// PWM period: 6kHz clock gives 6000/16=375Hz
	TA1CCR1 = 0;              	// PWM duty cycle. 0% initially
	TA1CCR2 = 0;              	// PWM duty cycle. 100% initially
	TA1CCTL0 = OUTMOD_7;		// TA0CCR0 toggle mode
	TA1CCTL1 = OUTMOD_7;		// TA0CCR1 toggle mode
	TA1CCTL2 = OUTMOD_7;		// TA0CCR2 toggle mode
	TA1CTL = TASSEL_1 + MC_1;	// Timer A control selects ACLK, and up mode
	/**********************/

	/* ADC configuration  */
	ADC10CTL0 &= ~ENC;
	ADC10CTL0 |= ADC10ON + ADC10IE + REFON + SREF_1 + ADC10SHT_3; 	// ADC10 enabled, conversion enabled, and interrupt enable
	ADC10CTL1 |= INCH_10 + ADC10SSEL_1 + CONSEQ_0; 					// channel 10, temperature sensor selected, ACLK selected, single-channel-conversion selected
	/**********************/

	/*   min temp setup   */
	/* Temperature calibration. Get the first reading from ADC as the
	 * minimum temperature. Based on that, and based on experiments, offsets
	 * are added to both minimum temperature and maximum temperature for
	 * optimal observation effects.
	 */
	__delay_cycles(500);				//wait for a while for reference voltage to stable
	ADC10CTL0 |= ADC10SC + ENC; 		//ADC10 start conversion
	while (ADC10CTL1 & ADC10BUSY);		//wait for conversion to finish
	ADC10CTL0 &= ~ENC;					//disable ADC
	MIN_TEMP = ADC10MEM+2;				//get minimum temperature
	MAX_TEMP = MIN_TEMP + 8;			//add 16 steps to get max temperature
	/**********************/

	// infinite loop to change LED intensity continuously
	while (1) {
		_bis_SR_register(GIE + LPM3_bits); 						//global interrupt enable and enter low power mode
		ADC10CTL0 |= ADC10SC + ENC; 							// ADC10 start conversion
		_bis_SR_register(LPM3_bits); 							//global interrupt enable and enter low power mode
		if (VALUE >= MAX_TEMP) {								//when temperature is too high
			TA1CCR1_MAX = TA1CCR0;  							//set maximum brightness of red LED
			TA1CCR2_MAX = 0;									//set maximum brightness of blue LED
		}
		else if (VALUE <= MIN_TEMP+2) {							//when temperature is too low
			TA1CCR1_MAX = 0;  									//set maximum brightness of red LED
			TA1CCR2_MAX = TA1CCR0;								//set maximum brightness of blue LED
		}
		else {													//when temperature is in max and min range set previously
			TA1CCR1_MAX = TA1CCR0 * (VALUE - MIN_TEMP)/RANGE;  	//set maximum brightness of red LED
			TA1CCR2_MAX = TA1CCR0 * (MAX_TEMP - VALUE)/RANGE;	//set maximum brightness of blue LED
		}
		RED_STEP = TA1CCR1_MAX/12;								//set step of brightness change for red LED
		BLUE_STEP = TA1CCR2_MAX/12;								//set step of brightness change for blue LED
		_bis_SR_register(LPM3_bits);  							//enter low power mode 3 and enable interrupt

	}

}

#pragma vector = WDT_VECTOR					//WDT interval mode interrupt. Frequency = 6000/512 = 11.72Hz
__interrupt void WDT_ISR(void){				//can name the actual function anything
	if (counter < 13) {						//for the first 12 counts
		TA1CCR1 = TA1CCR1 + RED_STEP;  		//increase duty cycle of red LED
		TA1CCR2 = TA1CCR2 + BLUE_STEP;		//increase duty cycle of blue LED
	}
	else  {									//for the remaining counts
		TA1CCR1 = TA1CCR1 - RED_STEP;  		//decrease duty cycle of red LED
		TA1CCR2 = TA1CCR2 - BLUE_STEP;		//decrease duty cycle of blue LED
	}
	if (counter > 23) {						//if statement to exit the ISA approx. 2 seconds a time (0.5Hz) (11.72/24 = approx. 0.5)
		TA1CCR1 = 0;						//set brightness of red LED back to zero
		TA1CCR2 = 0;						//set brightness of blue LED back to zero
		counter = 0;						//reset counter
		_bic_SR_register_on_exit(LPM3_bits);//enter low power mode 3 and enable interrupt
	}
	counter++;								//increment counter
}

#pragma vector = ADC10_VECTOR				//ADC10 interrupt. Triggered when adc finishes sampling and conversion
__interrupt void ADC_ISR(void){				//can name the actual function anything
	ADC10CTL0 &= ~ENC;						//disable conversion
	VALUE = ADC10MEM;						//get the value of ADC
	_bic_SR_register_on_exit(LPM3_bits);  	//enter low power mode 3 and enable interrupt
}
