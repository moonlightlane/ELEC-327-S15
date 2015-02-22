/*
 * ELEC 327 Spring15 Lab04
 * Pseudorandom number generator
 *
 * This function generates a 5 bit pseudorandom number
 * using the concept of linear feedback shifting register
 * (lfsr). The polynomial used is x^5 + x^3 + 1. The 
 * range of the pseudorandom number is between 0 and 31.
 * 
 * @argument: 
 * seed   -- an integer, the start state of lfsr
 *
 * @return:
 * output -- a pseudorandom number between 0 and 31.
 *
 * Author: 			Zichao Wang
 * Date Modified:	Feb 17, 2015
 *
 */


#include <stdio.h>
int rand32(int seed) {
	static char lfsr[5]; // value for linear feedback shift register
	static unsigned bit3; // third bit
	static unsigned bit5; // fifth bit
	static int output = 0; // integer value of the output
	if (output == 0) {
		if (seed != 0) { // calculate the values for lfsr when seed is provided
			lfsr[4] = seed%2;
			lfsr[3] = (seed/2)%2;
			lfsr[2] = (seed/4)%2;
			lfsr[1] = (seed/8)%2;
			lfsr[0] = (seed/16)%2;
		}
		else { // return 0 if no seed is set
			return 0;
		}
	}

		bit5 = lfsr[4]; // get bit5 and bit3 for xor function
		bit3 = lfsr[2];
		lfsr[4] = lfsr[3]; // shifting lfsr 1 bit to the right
		lfsr[3] = lfsr[2];
		lfsr[2] = lfsr[1];
		lfsr[1] = lfsr[0];
		lfsr[0] = bit5 ^ bit3; // xor function
		output = lfsr[0]*16 + lfsr[1]*8 + lfsr[2]*4 + lfsr[3]*2 + lfsr[4]-1; // convert to decimal output
	return output;
}

void main () {
	int seed = 29;
	int idx = 0;
	int answer;
	while (idx < 33) {
		answer = rand32(seed);
		printf("%d\n",answer);
		idx ++ ;
	}
}