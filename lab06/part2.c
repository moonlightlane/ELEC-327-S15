/*
ELEC 327 S15 Lab06: software debouncing and simon PCB
Author: Zichao Wang
Date  : March 30th, 2015

This code is the 3rd part of the lab, which completes a simplified version
of the simon game. In this implementation, the game makes use of 2 buttons,
1 LED and 1 buzzer. The game proceeds as follows:

1) 	default pattern is [1,2,1,1,2,2]. When you press the button in this 
	sequence correctly, AND is faster than the previous sequence, the LED
	becomes brighter and buzzer frequency becomes higher. If you do not press
	as fast as or slower than the previous sequence, the LED and buzzer will
	remain their previous brightness and frequency, respectively. The brightness 
	for LED and frequency for buzzer are all 8 steps. After 8 correct sequence 
	inputs, LED brightness and buzzer frequency all reach their respective maixmum,
	and will not increase any more beyond this point.
2)	When you press the pattern incorrectly, the LED and buzzer immediately
	go off.
3)	If you press the two buttons simultaneously for 2 seconds, the game is 
	restarted.
4)	If you press the two buttons simultaneously for 2 restart sequence (repeat 
	step 3 twice), the reset mode is entered. In this mode, you can change the
	default pattern by pressing the buttons to create a new sequence.
*/


 
#include "msp430g2553.h"
 
#define S1 0x0001
#define S2 0x0002

/* button press variables */ 
int buttonNumber = 0;
int Time = 0;
int previousTime = 4096;
int endOfSequence = 0;
int startOfSequence = 0;
int RSTCnt = 1;
int counter = 0;

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
 		}
	}
}
      
/* WDT interrupt routine for update LED and buzzer, restart and reset pattern */
#pragma vector = WDT_VECTOR
__interrupt void wdt_isr(void) {
 	P1IFG &= ~BIT3; // Clear the button interrupt flag
 	P1IE |= BIT3; 	// Re-enable interrupt for the button on P1.3
 	P1IFG &= ~BIT2;
 	P1IE |= BIT2;

 	/* update LED and buzzer */
 	if (pointer == 1 & startOfSequence == 1) { // start of the sequence
 		Time = 0; 			// reset time
 		startOfSequence = 0;// reset start of sequence flag
 	}
 	Time ++;	// increment time
 	if (pointer == 0 & endOfSequence == 1) { // end of the sequence
 		endOfSequence = 0;	// reset end of sequence flag
 		if (Time < previousTime) { // if this sequence is pressed faster than the previous one
 			TA0CCR1 = TA0CCR0/step*(led_buzzer_pointer+1); // led increment brightness
			if (TA0CCR1 >= PERIOD) {
				TA0CCR1 = PERIOD;	// set LED to brightest when it reaches maximum duty cycle
			}					
		 	TA1CCR0 = periods[led_buzzer_pointer]; // increment buzzer frequency
		 	led_buzzer_pointer ++;	// go to next higher frequency
		 	if (led_buzzer_pointer >= sizeof(periods)/sizeof*(periods)-1) {
		 		led_buzzer_pointer = sizeof(periods)/sizeof*(periods)-1; // set buzzer to highest frequency if highest frequency is reached
		 	}
 		}
		previousTime = Time; // record time pressed for this sequence
 	}
 	/* restart the sequence */
 	if (RSTCnt<=7){
		if ((P1IN & (BIT2+BIT3))==0x00) {
			if(++counter==63){ 			// wait for 63*32 = 2016ms ~= 2s
				// toggle LED
				// reset everything here
				P1OUT ^= BIT6;
				counter=0;			
				RSTCnt++;				
				pointer=0;				
				TA1CCR0=0;
				led_buzzer_pointer=0;
				startOfSequence=0;
				endOfSequence = 0;
				previousTime = 4096;
			}
		}
		else{
			// toggle LED and reset reset counter
			P1OUT ^= BIT6;
			RSTCnt=1;
		}
	}
	else if(RSTCnt>7){                              //BONUS
		if ((P1IN & (BIT2+BIT3))==0x00) {
			P1OUT ^= BIT6;
			// RESET PATTERN WANTED
		}
		else{
			P1OUT ^= BIT6;
		}
		RSTCnt=1;
	}
	else{
	}
	
}