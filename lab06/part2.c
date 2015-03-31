
 
#include "msp430g2553.h"
 
#define S1 0x0001
#define S2 0x0002

/* button press variables */ 
int buttonNumber = 0;
int Time = 0;
int previousTime = 4096;
int endOfSequence = 0;
int startOfSequence = 0;

/* button pattern variables */
char pattern[] = {1,2,1,1,2,2}; // define pattern
int pointer = 0; // pointer to loop through the pattern

/* buzzer variables */
float periods[] = {1000000/261.63,1000000/293.66,
				   1000000/329.63,1000000/349.23,
   				   1000000/392.00,1000000/440.00,
   				   1000000/493.88,1000000/523.25}; // frequencies

/* LED variables */
int PERIOD = 240;
int step = 8;
int intensity;
int led_buzzer_pointer = 0;

/* setup functions */
void button_init(void);
void buzzer_init(void);
void led_init(void);
void WDT_init(void);
 
void main (void)
{
	WDTCTL = WDTPW + WDTHOLD; // Stop WDT
	buzzer_init();
	button_init();
	led_init();
	WDT_init();
	__bis_SR_register(LPM0_bits | GIE);
}

/* set P1.3 and P1.2 to be button inputs. WORKS. */
void button_init() {
	// set P1.2 and P1.3 as button inputs.
	// P1.2 is S1 and P1.3 is S2.
	P1DIR &= ~BIT3; 	// Set P1.3 as input
	P1OUT |= BIT3; 
	P1REN |= BIT3; 		// keep pin high until pressed
	P1IES |= BIT3; 		// Enable Interrupt 
	P1IFG &= ~BIT3; 	// Clear the interrupt flag
	P1IE |= BIT3; 		// Enable interrupts on port 1 

	P1DIR &= ~BIT2; 
	P1OUT |= BIT2; 
	P1REN |= BIT2; 
	P1IES |= BIT2; 
	P1IFG &= ~BIT2; 
	P1IE |= BIT2; 
}

/* TA1 setup for LED and buzzer. WORKS. */
void buzzer_init() {
  P2DIR |= BIT2;
  P2SEL |= BIT2; 			// Set BIT2 as TA1.1 PWM output.
  P2DIR |= BIT5; 		
  P2SEL |= BIT5; 			// Set BIT2 as TA1.2 PWM output.

  TA1CCR0 = 0;	 			// not playing anything initially	
  TA1CCR1 = 220; 			// divde by 2, 50%: loudest
  TA1CCR2 = 0; 
  TA1CCTL1 = OUTMOD_6;
  TA1CCTL2 = OUTMOD_6;
  TA1CTL = TASSEL_2 + MC_1; // SMCLK + upmode
}

/* set P1.6 to be LED outputs. WORKS. */
void led_init() {
	P1DIR |= BIT6; 				// Set the LEDs on P1.6 as output (PWM TA0.1 output)
	P1SEL |= BIT6; 				// Select P1.6 as PWM output
	TA0CCR0 = PERIOD; 			// set period
	TA0CCR1 = 0; 				// set duty cycle. Set to dark initially
	TA0CCTL1 = OUTMOD_6;
	TA0CTL = TASSEL_2 + MC_1; 	// SMCLK + upmode	
}

/* initialize watchdog timer and interrupt */
void WDT_init(){
	WDTCTL = WDTPW | WDTHOLD; //WDT password + Stop WDT + detect RST button falling edge
	//IFG1 &= ~WDTIFG; // Clear the WDT interrupt flags
	IE1 |= WDTIE; // Enable the WDT interrupt
	WDTCTL = WDT_MDLY_32;
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
	/* interrupt routine for button P1.3 */
 	if (P1IFG & BIT3) {
 		P1IE &= ~BIT3; 				// Disable Button interrupt to avoid bounces
 		P1IFG &= ~BIT3; 			// Clear the interrupt flag for the button
 		if (P1IES & BIT3) { 		// Falling edge detected
 	  		buttonNumber = S2;  	// record the button number for pattern checking
 	  		/* check pattern correctness */
 	  		if (pattern[pointer] == buttonNumber) {
		 		pointer ++;			// if correct, go to next pattern in the sequence
		 		startOfSequence = 1;
		 	}
		 	/* reset everything for incorrect press */
		 	else {	
				TA0CCR1 = 0; 		// turn off LED (duty cycle = 0)
				TA1CCR0 = 0; 		// turn off buzzer (frequency = 0)
				pointer = 0; 		// reset pointer 	
				P1OUT &= ~BIT0; 	// check if entered else statement			
		 	}
		 	/* start the sequence again if the entire sequence is pressed correctly */
		 	if (pointer > sizeof(pattern)-1) {
		 		pointer = 0;		// go to the start of pattern
		 		endOfSequence = 1;	// set end of sequence flag
		 	}
 		}
 		P1IES ^= BIT3; // Toggle edge detect
 		//IFG1 &= ~WDTIFG; // Clear the interrupt flag for the WDT
 		//WDTCTL = WDT_MDLY_32; // Restart the WDT with the same NMI status as set by the NMI interrupt
 	}
 	/* interrupt routine for button P1.2 */
 	else { // add other port 1 interrupts
		if (P1IFG & BIT2) {
 			P1IE &= ~BIT2; 				// Disable Button interrupt
 			P1IFG &= ~BIT2; 			// Clear the interrupt flag
 			if (P1IES & BIT2) { 		// Falling edge detected
	  			buttonNumber = S1; 		// record the button number for pattern checking
	  			/* check pattern correctness */
	  			if (pattern[pointer] == buttonNumber) {
			 		pointer ++;			// go to next pattern
			 		startOfSequence = 1;// set start of sequence flag
			 	}
			 	/* reset everything for incorrect press */
			 	else {
					TA0CCR1 = 0; 		// turn off LED (duty cycle = 0)
					TA1CCR0 = 0; 		// turn off buzzer (frequency = 0)
					pointer = 0; 		// reset pointer 	
			 	}
			 	/* start the sequence again if the entire sequence is pressed correctly */
			 	if (pointer > sizeof(pattern)-1) {
			 		pointer = 0;
			 		endOfSequence = 1;
			 	}
 			}
 			P1IES ^= BIT2; // Toggle edge detect
 			//IFG1 &= ~WDTIFG; // Clear the interrupt flag for the WDT
 			//WDTCTL = WDT_MDLY_32; // Restart the WDT 
 		}
	}
}
      
// WDT is used to debounce s1 and s2 by delaying the re-enable of the P1IE interrupts
// and to time the length of the press
#pragma vector = WDT_VECTOR
__interrupt void wdt_isr(void) {
 
 	P1IFG &= ~BIT3; // Clear the button interrupt flag
 	P1IE |= BIT3; // Re-enable interrupt for the button on P1.3
 	P1IFG &= ~BIT2;
 	P1IE |= BIT2;

 	if (pointer == 1 & startOfSequence == 1) { // start of the sequence
 		Time = 0;
 		startOfSequence = 0;
 	}
 	Time ++;
 	if (pointer == 0 & endOfSequence == 1) { // end of the sequence
 		endOfSequence = 0;
 		if (Time < previousTime) {
 			TA0CCR1 = TA0CCR0/step*(led_buzzer_pointer+1); // led increment brightness
			if (TA0CCR1 >= PERIOD) {
				TA0CCR1 = PERIOD;
			} // set brightness to maximum if maximum is reached					
		 	TA1CCR0 = periods[led_buzzer_pointer];
		 	led_buzzer_pointer ++;
		 	if (led_buzzer_pointer >= sizeof(periods)/sizeof*(periods)-1) {
		 		led_buzzer_pointer = sizeof(periods)/sizeof*(periods)-1;
		 	}
 		}
		previousTime = Time;
 	}

}