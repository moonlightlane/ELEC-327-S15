/*
 * ELEC 327 Spring15 Lab03
 * Coding up the mood ring - part 2
 *
 * RED LED: PORT2.1, BLUE LED: PORT2.5
 *
 * Mood ring code.
 *
 * Red and blue LED brightness change with temperature.
 * Temperature is calibrated to current room temperature (see "min temp setup")
 * for more detail. Blue LED is in its brightest and red LED is turned off when
 * at room temperature. When temperature starts going up, blue LED becomes
 * dimmer, and red LED becomes brighter.
 *
 * Author: 			Zichao Wang
 * Date Modified:	Feb 10, 2015
 *
 */
#include <msp430g2553.h>

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
	TA1CCR0 = 80;              	// PWM period: 6kHz clock gives 6000/16=375Hz
	TA1CCR1 = 0;              	// PWM duty cycle. 0% initially
	TA1CCR2 = 0;              	// PWM duty cycle. 100% initially
	TA1CCTL0 = OUTMOD_7;		// TA0CCR0 toggle mode
	TA1CCTL1 = OUTMOD_7;		// TA0CCR1 toggle mode
	TA1CCTL2 = OUTMOD_7;		// TA0CCR2 toggle mode
	TA1CTL = TASSEL_1 + MC_1;	// Timer A control selects ACLK, and up mode
	/**********************/

	/* ADC configuration  */
	ADC10CTL0 &= ~ENC;												// disable conversion
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
	MAX_TEMP = MIN_TEMP + RANGE;		//add 16 steps to get max temperature
	/**********************/

	// infinite loop to change LED intensity continuously
	while (1) {
		_bis_SR_register(GIE + LPM3_bits); 					//global interrupt enable and enter low power mode
		ADC10CTL0 |= ADC10SC + ENC; 						// ADC10 start conversion
		_bis_SR_register(GIE + LPM3_bits); 					//global interrupt enable and enter low power mode
		if (VALUE >= MAX_TEMP) {
			TA1CCR1 = TA1CCR0;  							//change red LED to its brightest if too hot
			TA1CCR2 = 0;									//turn off blue LED if too hot
		}
		else if (VALUE <= MIN_TEMP+1) {
			TA1CCR1 = 0;  									//turn off red LED if too cold
			TA1CCR2 = TA1CCR0;								//change blue LED to its brightest if too cold
		}
		else {
			TA1CCR1 = TA1CCR0 * (VALUE - MIN_TEMP)/RANGE;  	//change brightness of red LED according to temperature change
			TA1CCR2 = TA1CCR0 * (MAX_TEMP - VALUE)/RANGE;	//change brightness of blue LED according to temperature change
		}
		_bis_SR_register(LPM3_bits);  						//enter low power mode 3 and enable interrupt

	}

}


// WDT interrupt routine
#pragma vector = WDT_VECTOR					//WDT interval mode interrupt. Frequency = 6000/512 = 11.72Hz
__interrupt void WDT_ISR(void){
	if (counter >= 3) {						//if statement to exit the ISA approx. 0.25 second a time (4Hz)
		_bic_SR_register_on_exit(LPM3_bits);//enter low power mode 3 and enable interrupt
		counter = 0;						//reset counter

	}
	counter++;								//increment counter
}

// ADC interrupt routine
#pragma vector = ADC10_VECTOR				//ADC10 interrupt. Triggered when adc finishes sampling and conversion
__interrupt void ADC_ISR(void){
	ADC10CTL0 &= ~ENC;						//disable conversion
	VALUE = ADC10MEM;						//get the value of ADC
	_bic_SR_register_on_exit(LPM3_bits);  	//enter low power mode 3 and enable interrupt
}
