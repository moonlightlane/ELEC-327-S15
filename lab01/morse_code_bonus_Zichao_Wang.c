/*
 * ELEC 327 Spring 15
 * Lab 1 BONUS code.
 *
 * Flash the LED in Morse code pattern as fast as possible.
 *
 * The two messages provided are "SOS" and "WZC".
 * You are welcome to change the message.
 *
 * Author: Zichao Wang
 */


#include "msp430g2553.h"		//must include so compiler knows what each variable means

/****** morse code for "SOS". Space separates the letters *******/
char MESSAGE[] = {'.','.','.',' ', '-','-','-',' ', '.','.','.'};
/****************************************************************/

// TODO: change this to whatever 3 letter sequence you like. Here is the sequence "WZC".
//char MESSAGE[] = {'.','-','-',' ', '-','-','.','.',' ', '-','.','-','.'};

int i;

void main(void){
    WDTCTL = WDTPW + WDTHOLD;	// Stop WDT
    
    if (CALBC1_16MHZ == 0xFF || CALDCO_16MHZ == 0xff)
        while(1); // Erased calibration data? Trap!
    
    BCSCTL1 = CALBC1_16MHZ; // Set the DCO to 16 MHz
    DCOCTL = CALDCO_16MHZ; // And load calibration data
    
    P1OUT &= BIT0; // Set P1 output high
    P1DIR |= BIT0; // Switch P1 to output direction
    
    while (1) {
        //__delay_cycles(500000); // half a second time
        //P1OUT ^= BIT0; // toggle the LED so the LED blinks 1 clock cycle, rest, and repeat the cycle
    	for (i = 1; i <= sizeof MESSAGE; i++){
    		if (MESSAGE[i-1] == '.') {
    			P1OUT ^= BIT0; // turn on the LED
    			__delay_cycles(1); // delay for 1 clock cycle
    			P1OUT ^= BIT0; // turn off the LED
    			__delay_cycles(1); // delay for 1 clock cycle
    			//i ++; // increment the pointer
    		}
    		else if (MESSAGE[i-1] == '-') {
    			P1OUT ^= BIT0; // turn on the LED
    			__delay_cycles(2); // delay for 2 clock cycle
    		    P1OUT ^= BIT0; // turn off the LED
    		    __delay_cycles(1); // delay for 2 clock cycle
       			//i ++; // increment the pointer
    		}
    		else {
    			P1OUT &= ~BIT0; // turn off the LED
    			__delay_cycles(2); // delay for 2 clock cycle
    		}

    	}
    	P1OUT &= ~BIT0; // turn off the LED
    	__delay_cycles(7); // delay for 7 clock cycle
    }
    
}
