/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * <JNSJOH021> <VDYLIS001>
 * 16/08/2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include<softPwm.h>//for pwm functions
#include <signal.h> //For keyboard interrupt

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance
int HH,MM,SS;

void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	
	//Set up the LEDS
	for(int i; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT);
		digitalWrite(LEDS[i], LOW);//set all LEDS initially off
	}
	
	//Set Up the Seconds LED for PWM
	//Write your logic here
	pinMode(1, PWM_OUTPUT);
	softPwmCreate(1,0,60);
			
	printf("LEDS done\n");
	
	//Set up the Buttons
	for(int j; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	//Attach interrupts to Buttons
	//Write your logic here
	wiringPiISR(30, INT_EDGE_RISING, &hourInc);
	wiringPiISR(5, INT_EDGE_RISING, &minInc);

	printf("BTNS done\n");
	printf("Setup done\n");
}

/*void cleanGPIO(void){

	digitalWrite(LEDS[0],0);
        for(int i=0; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
            pinMode(LEDS[i], INPUT);
	}
	pinMode(SECS,INPUT);
	exit(1);
}*/
/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();
	//signal(SIGINT,cleanGPIO);
	//Set random time (3:04PM)
	//You can comment this file out later
	//wiringPiI2CWriteReg8(RTC, HOUR, 0x07);
	//wiringPiI2CWriteReg8(RTC, MIN, 0x00);
	//wiringPiI2CWriteReg8(RTC, SEC, 0x00);
	toggleTime();
	wiringPiI2CWriteReg8(RTC, SEC, 0x80);

	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		//Write your logic here
		hours = hFormat(hexCompensation(wiringPiI2CReadReg8(RTC, HOUR)));
		mins = hexCompensation(wiringPiI2CReadReg8(RTC, MIN));
		secs = wiringPiI2CReadReg8(RTC, SEC) - 0x80;

		//Function calls to toggle LEDs
		//Write your logic here
		lightHours(hours);
		lightMins(mins);
		secPWM(secs);
		// Print out the time we have stored on our RTC
		printf("The current time is: %d:%d:%x\n",hours,mins, secs);

		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here	

	int unit = units;

	digitalWrite(25,unit/8);
	unit%=8;
	digitalWrite(22,unit/4);
	unit%=4;
	digitalWrite(21,unit/2);
	unit%=2;
	digitalWrite(27,unit);

}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here
        int unit = units;

        digitalWrite(0,unit/32);
        unit%=32;
        digitalWrite(2,unit/16);
        unit%=16;
        digitalWrite(3,unit/8);
        unit%=8;
        digitalWrite(7,unit/4);
	unit%=4;
	digitalWrite(4,unit/2);
	unit%=2;
	digitalWrite(6,unit);
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
	softPwmWrite(1, units);
	
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 1 triggered, %d\n",hours);
		//Fetch RTC Time
		hours = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		//Increase hours by 1, ensuring not to overflow

		//if(hours <=0x17){
		//	hours = hours + 0x01;
		//}
		//else{
		//	hours =0x00;
		//}
		hours=hFormat(hours +1);
		hours = decCompensation(hours);
		//Write hours back to the RTC
		wiringPiI2CWriteReg8(RTC, HOUR,hours);
		}

	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 2 triggered, %d\n", mins);
		//Fetch RTC Time
		mins = hexCompensation(wiringPiI2CReadReg8(RTC, MIN));
		hours = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		//Increase minutes by 1, ensuring not to overflow
		mins +=1;
		if (mins==60){
			mins = 0;
			hours = hFormat(hours+1);
		}
		mins = decCompensation(mins);
		hours = decCompensation(hours);
		
		//Write minutes back to the RTC
		wiringPiI2CWriteReg8(RTC, MIN, mins);
		wiringPiI2CWriteReg8(RTC, HOUR, hours);

	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
