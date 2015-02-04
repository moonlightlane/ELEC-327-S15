/*
 * ELEC 327 Spring15 Lab02
 * PWM, Low Power Modes and PCBs
 *
 * The code below uses Timer A0 and Timer A1 to generate two
 * PWM signals, such that the LEDs connected to the corresponding
 * pins flashes in the fashion defined by the global variable
 * intensity[]. Interrupt routine is used to make the flashing
 * pattern continuous.
 *
 * Author: 			Zichao Wang
 * Date Modified:	Feb 4, 2015
 *
 */

#include <msp430g2553.h>		//must include so compiler knows what each variable means

/*   global variables  */
extern char intensity[] = {5, 30, 60, 95}; //brightness levels (duty cycles)
extern int i = 0;						   //pointer, to be used to change the duty cycle in interrupt routine
/**********************/

void main(void){
	/* clock configuration*/
	WDTCTL = WDTPW + WDTHOLD;	//Stop WDT
	BCSCTL2 &= ~XTS;			//Disable DCO
	BCSCTL3 |= LFXT1S_2; 		// LFXT1 = VLO; select VLO as clock source
	IFG1 &= ~OFIFG; 			// Clear OSCFault flag, and allow VLO to be selected
	/**********************/

	/* pin configuration  */
	P2DIR |= BIT2 + BIT4 + BIT5; //+ BIT1; //Green and red LED
	P2SEL |= BIT2 + BIT4 + BIT5; //+ BIT1; //Green and red LED selected for PWM
	/**********************/

	/* timer configuration*/
	TA1CCR0 = 100;              	//PWM period: 12kHz clock gives 12000/500=24Hz=1/24s period
	TA1CCR1 = 50;              		//PWM duty cycle. 50% initially. To change the value: value = duty cycle * period
	TA1CCR2 = 50;              		//PWM duty cycle. 50% initially. To change the value: value = duty cycle * period
	TA1CCTL0 = CCIE+ OUTMOD_7;		// TA0CCR0 toggle mode
	TA1CCTL1 = CCIE+ OUTMOD_7;		// TA0CCR1 toggle mode
	TA1CCTL2 = CCIE+ OUTMOD_7;		// TA0CCR2 toggle mode
	TA1CTL = TASSEL_1 + MC_1;		//Timer A control selects ACLK, and up mode
	/**********************/

	/* lesson learnt: this part must be the last part of the main function!!! */
	_bis_SR_register(LPM3_bits + GIE);  //enter low power mode 3 and enable interrupt

	__enable_interrupt();				//global interrupt enable
}

#pragma vector = TIMER1_A0_VECTOR	//will use the "TIMER1_A0_VECTOR" interrupt, since we use Timer A1 throughout the code
__interrupt void Timer_A(void){		//can name the actual function anything
	/*loop through the brightness values defined externally*/
	while (i < 4) {
		i++;						//increment pointer
		TA1CCR1 = intensity[i];		//change duty cycle to the ith intensity level
		TA1CCR2 = intensity[i];		//change duty cycle to the ith intensity level
		__delay_cycles(600000);		//delay a few milliseconds.
	}
	/*change pointer back to 0 when the pointer equals or is greater than 4*/
	if (i >= 4) {
		i = 0;
	}

}

