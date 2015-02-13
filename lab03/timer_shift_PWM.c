/*
 * ELEC 327 Spring15 Lab03
 * Coding up the mood ring - part 1
 *
 * The code below uses Timer A0 and Timer A1 to generate two
 * PWM signals, such that the LEDs connected to the corresponding
 * pins flashes in the fashion defined by the global variable
 * intensity[]. Interrupt routine is used to make the flashing
 * pattern continuous.
 *
 * Author: 			Zichao Wang
 * Date Modified:	Feb 9, 2015
 *
 */
#include <msp430g2553.h>		//must include so compiler knows what each variable means

/*   global variables  */
int i = 0;						   //counter for change LED intensity level
int count = 16;					   //total number of LED intensity level
int DIRECTION = 1;				   //LED intensity change direction
int counter = 1;				   //counter for ISA service
/**********************/

void main(void){

	/* WDT configuration  */
	WDTCTL = WDTPW+WDTTMSEL+WDTCNTCL+WDTSSEL+WDTIS1; // set to interval timer mode, select ACLK, divide by 512
	IE1 |= WDTIE; 									 // watchdog timer interrupt enabled
	/**********************/

	/* clock configuration*/
	BCSCTL1 = DIVA_1;				// ACLK divide by 2 to run 6KHz
	BCSCTL3 |= LFXT1S_2; 			// LFXT1 = VLO; select VLO as clock source
	IFG1 &= ~OFIFG; 				// Clear OSCFault flag, and allow VLO to be selected
	/**********************/

	/* pin configuration  */
	P2DIR |= BIT1 + BIT5; 			// pin2.1 and pin2.5 are selected as output
	P2SEL |= BIT1 + BIT5; 			// pin2.1 and pin2.5 are selected for PWM
	/**********************/

	/*timerA configuration*/
	TA1CCR0 = 16;              		// PWM period: 6kHz clock gives 6000/16=375Hz
	TA1CCR1 = 0;              		// PWM duty cycle. 0% initially
	TA1CCR2 = 16;              		// PWM duty cycle. 100% initially
	TA1CCTL0 = OUTMOD_7;			// TA0CCR0 toggle mode
	TA1CCTL1 = OUTMOD_7;			// TA0CCR1 toggle mode
	TA1CCTL2 = OUTMOD_7;			// TA0CCR2 toggle mode
	TA1CTL = TASSEL_1 + MC_1;		// Timer A control selects ACLK, and up mode
	/**********************/

	_bis_SR_register(GIE);			//global enable interrupt
	// infinite loop to change LED intensity continuously
	while (1) {
		if (i < count) {
			i++; 						// increment counter if less than 16
		}
		else {
			DIRECTION = -DIRECTION;		// reverse direction of LED light change
			i = 1;						// reset counter
		}
		TA1CCR1 = TA1CCR1 + DIRECTION;  //change duty cycle to the ith intensity level
		TA1CCR2 = TA1CCR2 - DIRECTION;	//change duty cycle to the ith intensity level
		_bis_SR_register(LPM3_bits);    //enter low power mode 3 and enable interrupt

	}

}

#pragma vector = WDT_VECTOR						//WDT interval mode interrupt. Frequency = 6000/512 = 11.72Hz
__interrupt void WTD_ISR(void){					//can name the actual function anything
	if (counter >= 12) {						//if statement to exit the ISA approx. 1 second a time
		counter = 0;							//reset counter
		_bic_SR_register_on_exit(LPM3_bits);  	//enter low power mode 3 and enable interrupt
	}
	counter++;									//increment counter
}

