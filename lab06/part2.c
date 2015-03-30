/* This code demonstrates the use of both switches, labelled S1 (RESET) and S2 (P1.3) on the MSP430 Launchpad.
 * The code implements a software debounce using the WDT and also indicates a long press i.e. switch pressed
 * for more than 1.5 seconds
 * Switch S1 uses the RST/NMI pin as an input
 * Switch S2 uses the P1.3 general purpose I/O pin
 * The P1.0 Red LED1 is used to indicate when S2 (P1.3) is pressed (Active Pressed).
 * The P1.6 Green LED2 is used to indicate when S1 (RESET) is pressed (Active Pressed).
 * These LEDs are turned off when the switches are released.
 * The Watchdog Timer (WDT) is used both to debounce the switches and to time the length of the press. The debounce
 * is achieved by using the delay provided by the WDT to re-enable the Switch interrupts only after the bouncing has
 * finished. The WDT interrupt service routine also increments separate counters (one for each switch) when the
 * switches are pressed. When the count exceeds a certain time the P1.0 LED (for Switch 1) and the P1.6 LED (for
 * Switch 2) are turned on to indicate a long press.
 * In order to run the code using the RESET pin as a switch input you need to cycle the power on the LaunchPad.
 */
 
#include "msp430g2553.h"
 
#define S1 0x0001
#define S2 0x0002

/* button press variables */ 
unsigned char PressCountS1 = 0;
unsigned char PressCountS2 = 0;
unsigned char Pressed = 0;
unsigned char PressRelease = 0;
int buttonNumber = 0;

/* button pattern variables */
char pattern[] = {1,2,1,1,2,2}; // define pattern
int pointer; // pointer to loop through the pattern

/* buzzer variables */
int periods[] = {1000000/261.63,1000000/293.66,1000000/329.63,1000000/349.23,
   				 1000000/392.00,1000000/440.00,1000000/493.88,1000000/523.25}; // frequencies

/* LED variables */
int PERIOD = 240;
int step = 8;
int intensity;
int led_buzzer_pointer = 0;

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
	// The CPU is free to do other tasks, or go to sleep
	__bis_SR_register(LPM0_bits | GIE);
}

/* set P1.3 and P1.2 to be button inputs. WORKS. */
void button_init() {
	P1DIR &= ~BIT3; // Set button pin as an input pin
	P1OUT |= BIT3; // Set pull up resistor on for button
	P1REN |= BIT3; // Enable pull up resistor for button to keep pin high until pressed
	P1IES |= BIT3; // Enable Interrupt to trigger on the falling edge (high (unpressed) to low (pressed) transition)
	P1IFG &= ~BIT3; // Clear the interrupt flag for the button
	P1IE |= BIT3; // Enable interrupts on port 1 for the button

	P1DIR &= ~BIT2; 
	P1OUT |= BIT2; 
	P1REN |= BIT2; 
	P1IES |= BIT2; 
	P1IE |= BIT2; 
}

/* TA1 setup for LED and buzzer. WORKS. */
void buzzer_init() {
  P2DIR |= BIT2; //
  P2SEL |= BIT2; // BIT2 is TA1.1 PWM output. This one is currently used
  P2DIR |= BIT5; //
  P2SEL |= BIT5; // BIT2 is TA1.2 PWM output. This one is currently unused

  TA1CCR0 = 440;
  TA1CCR1 = 220; // divde by 2, 50%: loudest
  TA1CCR2 = 0; // placeholder. not yet used
  TA1CCTL1 = OUTMOD_6;
  TA1CCTL2 = OUTMOD_6;
  TA1CTL = TASSEL_2 + MC_1; // SMCLK, upmode
}

/* set P1.6 to be LED outputs. WORKS. */
void led_init() {
	P1DIR |= BIT6; // Set the LEDs on P1.6 as output (PWM TA0.1 output)
	P1SEL |= BIT6; // Select P1.6 as PWM output
	//P1OUT = ~BIT6;
	P2DIR |= BIT1; // P2.1 LED and P1.0 to test push button functionality
	P2OUT &= ~BIT1;
	P1DIR |= BIT0;
	P1OUT &= ~BIT0;
	TA0CCR0 = PERIOD; // set period
	TA0CCR1 = PERIOD; // set to brightest initially
	//TA0CCR2 = periods[which_period]; // placeholder. not yet used
	TA0CCTL1 = OUTMOD_6;
	//TA0CCTL2 = OUTMOD_6;
	TA0CTL = TASSEL_2 + MC_1; // SMCLK, upmode	
}

/* initialize watchdog timer and interrupt */
void WDT_init(){

	// The Watchdog Timer (WDT) will be used to debounce s1 and s2
	WDTCTL = WDTPW | WDTHOLD; //WDT password + Stop WDT + detect RST button falling edge
	IFG1 &= ~WDTIFG; // Clear the WDT interrupt flags
	IE1 |= WDTIE; // Enable the WDT interrupt
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
	/* interrupt routine for button P1.3 */
 	if (P1IFG & BIT3) {
 		P1IE &= ~BIT3; 			// Disable Button interrupt to avoid bounces
 		P1IFG &= ~BIT3; 		// Clear the interrupt flag for the button
 		if (P1IES & BIT3) { 	// Falling edge detected
 			P2OUT |= BIT1; 		// Turn on P2.1 red LED to indicate switch 2 is pressed
 			Pressed = S2; 		// Set Switch 2 Pressed flag
 			PressCountS2 = 0; 	// Reset Switch 2 long press count
 	  		buttonNumber = S2;  // record the button number for pattern checking
 	  		if (pattern[pointer] == buttonNumber) {
		 		pointer ++;
		 		P1OUT |= BIT0; // check if entered if statement
		 	}
		 	else {
				TA0CCR1 = 0; // turn off LED
				TA1CCR0 = 0; // turn off buzzer
				pointer = 0; // reset pointer 	
				P1OUT &= ~BIT0; // check if entered else statement			
		 	}
		 	if (pointer > sizeof(pattern)-1) {
		 		pointer = 0;
				TA0CCR1 = TA0CCR0/step*(led_buzzer_pointer+1); // led increment brightness
				TA1CCR0 = periods[led_buzzer_pointer]; // buzzer increment frequency
		 		led_buzzer_pointer ++; // increment led_buzzer_pointer pointer		
		 	}
 		}
 		else { // Rising edge detected
 			P2OUT &= ~BIT1; // Turn off P1.0 and P1.6 LEDs
 			Pressed &= ~S2; // Reset Switch 2 Pressed flag
 			PressRelease |= S2; // Set Press and Released flag
 		}
 		P1IES ^= BIT3; // Toggle edge detect
 		IFG1 &= ~WDTIFG; // Clear the interrupt flag for the WDT
 		WDTCTL = WDT_MDLY_32; // Restart the WDT with the same NMI status as set by the NMI interrupt
 	}
 	/* interrupt routine for button P1.2 */
 	else { // add other port 1 interrupts
		if (P1IFG & BIT2) {
 			P1IE &= ~BIT2; 			// Disable Button interrupt to avoid bounces
 			P1IFG &= ~BIT2; 		// Clear the interrupt flag for the button
 			if (P1IES & BIT2) { 	// Falling edge detected
 				P2OUT |= BIT1; 		// Turn on P2.1 red LED to indicate switch 1 is pressed
 				Pressed = S1; 		// Set Switch 1 Pressed flag
 				PressCountS1 = 0; 	// Reset Switch 2 long press count
	  			buttonNumber = S1; 	// record the button number for pattern checking
	  			if (pattern[pointer] == buttonNumber) {
			 		pointer ++;
			 		P1OUT |= BIT0; // check if entered if statement
			 	}
			 	else {
					TA0CCR1 = 0; // turn off LED
					TA1CCR0 = 0; // turn off buzzer
					pointer = 0; // reset pointer 	
					P1OUT &= ~BIT0; // check if entered else statement			
			 	}
			 	if (pointer > sizeof(pattern)-1) {
			 		pointer = 0;
					TA0CCR1 = TA0CCR0/step*(led_buzzer_pointer+1); // led increment brightness
					TA1CCR0 = periods[led_buzzer_pointer]; // buzzer increment frequency
			 		led_buzzer_pointer ++; // increment led_buzzer_pointer pointer		
			 	}
 			}
 			else { // Rising edge detected
 				P2OUT &= ~BIT1; // Turn off P2.1 LEDs
 				Pressed &= ~S1; // Reset Switch 1 Pressed flag
 				PressRelease |= S1; // Set Press and Released flag
 			}
 			P1IES ^= BIT2; // Toggle edge detect
 			IFG1 &= ~WDTIFG; // Clear the interrupt flag for the WDT
 			WDTCTL = WDT_MDLY_32; // Restart the WDT 
 		}
	}
}
      
// WDT is used to debounce s1 and s2 by delaying the re-enable of the P1IE interrupts
// and to time the length of the press
#pragma vector = WDT_VECTOR
__interrupt void wdt_isr(void) {
 	if (Pressed & S1) { // Check if switch 1 is pressed
 		if (++PressCountS1 == 62 ) { // Long press duration 62*32ms ~= 2s
 			P1OUT ^= BIT0; // Turn on the P1.1 LED to indicate long press
 		}
 	}
 
	if (Pressed & S2) { // Check if switch 2 is pressed
 		if (++PressCountS2 == 62 ) {// Long press duration 62*32ms ~= 2s
 			P1OUT ^= BIT0; // Turn on the P1.2 LED to indicate long press
 		}
 	}
 
 	P1IFG &= ~BIT3; // Clear the button interrupt flag (in case it has been set by bouncing)
 	P1IE |= BIT3; // Re-enable interrupt for the button on P1.3
 	P1IFG &= ~BIT2;
 	P1IE |= BIT2;
}