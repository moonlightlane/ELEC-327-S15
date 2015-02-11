/*
 * ELEC 327 Spring15 Lab03
 * Coding up the mood ring
 *
 * BLUE LED: PORT2.1, RED LED: PORT2.5
 *
 * Author: 			Zichao Wang
 * Date Modified:	Feb 9, 2015
 *
 */
#include <msp430g2553.h>		//must include so compiler knows what each variable means

/*   global variables  */
int i = 0;			//counter for change LED intensity level
int DIRECTION = 1;	//LED intensity change direction
int counter = 1;	//counter for ISA service
int step = 16;		//step of intensity change
int VALUE;			//value to store temperature
int MIN_TEMP;		//minimum temperature
int MAX_TEMP;		//maximum temperature
int TEMP_RANGE;		//temperature range read from sensor
int RANGE = 16;		//actual value range for duty cycle
int a;				//variable to convert from TEMP_RANGE to RANGE
int b;				//variable to convert from TEMP_RANGE to RANGE
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
	TA1CCR0 = 80;              // PWM period: 6kHz clock gives 6000/16=375Hz
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
	__delay_cycles(500);				//wait for a while for reference voltage to stable
	ADC10CTL0 |= ADC10SC + ENC; 		//ADC10 start conversion
	while (ADC10CTL1 & ADC10BUSY);		//wait for conversion to finish
	ADC10CTL0 &= ~ENC;					//disable ADC
	MIN_TEMP = ADC10MEM + 1;			//get minimum temperature
	MAX_TEMP = MIN_TEMP + 16;			//add 16 steps to get max temperature
	//TEMP_RANGE = MAX_TEMP - MIN_TEMP;	//calculate temperature range from sensor reading
	//a = RANGE/TEMP_RANGE;				//set up conversion constant a
	//b = -MIN_TEMP*RANGE/TEMP_RANGE;	//set up conversion constant b
	/**********************/

	// infinite loop to change LED intensity continuously
	while (1) {
		_bis_SR_register(GIE + LPM3_bits); 	//global interrupt enable and enter low power mode
		ADC10CTL0 |= ADC10SC + ENC; 		// ADC10 start conversion
		_bis_SR_register(GIE + LPM3_bits); 		//global interrupt enable and enter low power mode
		if (VALUE >= MAX_TEMP) {
			TA1CCR1 = TA1CCR0;  			//change duty cycle to the ith intensity level
			TA1CCR2 = 0;					//change duty cycle to the ith intensity level
		}
		else if (VALUE <= MIN_TEMP) {
			TA1CCR1 = 0;  					//change duty cycle to the ith intensity level
			TA1CCR2 = TA1CCR0;				//change duty cycle to the ith intensity level
		}
		else {
			TA1CCR1 = TA1CCR0 * (VALUE - MIN_TEMP)/RANGE;  		//change duty cycle to the ith intensity level
			TA1CCR2 = TA1CCR0 * (MAX_TEMP - VALUE)/RANGE;	//change duty cycle to the ith intensity level
		}
		_bis_SR_register(LPM3_bits);  		//enter low power mode 3 and enable interrupt

	}

}

#pragma vector = WDT_VECTOR					//WDT interval mode interrupt. Frequency = 6000/512 = 11.72Hz
__interrupt void WDT_ISR(void){				//can name the actual function anything
	if (counter >= 3) {						//if statement to exit the ISA approx. 0.25 second a time (4Hz)
		_bic_SR_register_on_exit(LPM3_bits);//enter low power mode 3 and enable interrupt
		counter = 0;						//reset counter

	}
	counter++;								//increment counter
}

#pragma vector = ADC10_VECTOR				//ADC10 interrupt. Triggered when adc finishes sampling and conversion
__interrupt void ADC_ISR(void){				//can name the actual function anything
	ADC10CTL0 &= ~ENC;						//disable conversion
	VALUE = ADC10MEM;						//get the value of ADC
	_bic_SR_register_on_exit(LPM3_bits);  	//enter low power mode 3 and enable interrupt
}
