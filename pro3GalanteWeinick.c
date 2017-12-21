/*=================CPEG222===================
 * Program:		Project #3
 * Authors: 		James Galante, Josh Weinick
 * Date: 		3/23/2017
 * Description:
 *
 *   This program creates a simple calculator and displays
 *   it on the SSD 
 * 
 * 

============================================*/

#include <p32xxxx.h>
#include <plib.h>
#include <stdio.h>
#include <math.h>
// Configuration Bit settings (Don't touch them if not necessary)
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
#define SYS_FREQ     80000000L

#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF//not changed
#pragma config POSCMOD = HS, FNOSC = PRIPLL,   FPBDIV = DIV_2// set PBCLK as 40MHZ


#define SYS_FREQ            (80000000L)                     // 80MHz system clock
#define TOGGLES_PER_SEC     10                               // No. of LED Toggles per second
#define CORE_TICK_RATE	    (SYS_FREQ/2/TOGGLES_PER_SEC)  // 4 million 4MHz

#define SECOND (CORE_TICK_RATE*10)     //exactly one second


// The speed control
#define DELAY 100
#define DELAY2 4500000



// The refresh rate of the segments
#define REFRESH_RATE 200   // ~1/100 secs


/****************************************************/
/*          Port Definition                           */
/*    You should add your port definition if necessary*/
/****************************************************/

// Buttons
#define Btn1  PORTGbits.RG6
#define Btn2  PORTGbits.RG7
#define Btn3  PORTAbits.RA0

#define LED1 LATGbits.LATG12
#define LED2 LATGbits.LATG13
#define LED3 LATGbits.LATG14
#define LED4 LATGbits.LATG15


// 7 Segment Display pmod for using the bottom row JC & JD jumpers
// Segments
#define SegA1 	LATBbits.LATB15
#define SegB1	LATDbits.LATD5
#define SegC1 	LATDbits.LATD4
#define SegD1 	LATBbits.LATB14
#define SegE1	LATDbits.LATD1
#define SegF1	LATDbits.LATD2
#define SegG1	LATDbits.LATD3



// 7 segment display pmod for using the bottom row of JA & JB jumpers
#define SegA2 	LATBbits.LATB7
#define SegB2	LATBbits.LATB8
#define SegC2	LATBbits.LATB9
#define SegD2 	LATBbits.LATB10
#define SegE2	LATEbits.LATE4
#define SegF2	LATEbits.LATE5
#define SegG2   LATEbits.LATE6


//// KYPD Pmod attached to MX7 jumper JF
#define Col4 LATFbits.LATF12
#define Col3 LATFbits.LATF5
#define Col2 LATFbits.LATF4
#define Col1 LATFbits.LATF13
#define Row4 PORTEbits.RE9
#define Row3 PORTAbits.RA1
#define Row2 PORTAbits.RA4
#define Row1 PORTAbits.RA5


// Display selection. 0 = right, 1 = left (Cathode)
//#define DispSel1 PORTDbits.RD12
#define DispSel1 LATDbits.LATD12

// Display selection. 0 = right, 1 = left (Cathode)
//#define DispSel2 PORTEbits.RE7
#define DispSel2 LATEbits.LATE7


//left sssd

enum Mode_min {
    Left1, Right1
};

//right ssd

enum Mode_sec {
    Left2, Right2
};

//enum for which ssd it is

enum Mode_LEFT_RIGHT {
    LEFT, RIGHT
};


//defining boolean//
typedef int bool;
#define true 1
#define false 0

//Subroutine//
void displayDigit(unsigned char, unsigned char, unsigned char); // Display one digit among two digits
void slowDownDisplay(unsigned char, unsigned char, unsigned char); // Frequency control function
void updateSSD();
void clearSSD();

int Poll_KYPD(); //main keypad func
int Poll_KYPD_ROW(); //polls rows
int Poll_KYPD_COL(int col); //polls columnss

void wait(int time, bool updateSSD);

int setOperand();
void addition();
void subtraction();
void multiplication();
void division();

void equalsPress(int mode);
void overflowError();
void underflowError();
void divisonError();
void resetCalc();

void hexDecimal();


// look-up table for the numbers //
unsigned char number[] = {
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
    0b1110111, // 10 - A
    0b1111100, // 11 - B
    0b0111001, // 12 - C
    0b1011110, // 13 - D
    0b1111001, // 14 - E
    0b1110001, // 15 - F
    0b0000000, //16 -clear

    0b1110110, //17 - overflow - H
    0b0111000, //18 - overflow - L
    0b1111001 //19 - divide by zero - E


};

//variable definition //
unsigned int display_value_thousands = 0; // The initial displayed value left 2
unsigned int display_value_hundreds = 0; // The initial displayed value right 2

unsigned int display_value_tens = 0; // The initial displayed value left 2
unsigned int display_value_ones = 0; // The initial displayed value right 2

int Row = 9; //arbitrary value when row is not clicked
int Col = -1; //arbitrary value when column is not clicked
int keyPressed = -1; //actual button pressed on board int value

int operand1 = 0;
int operand2 = 0;

/*modes:
 * 0 - reset/start mode
 * 1-addition
 * 2-subtraction
 * 3-multiplication
 * 4-division
 * 5-overflow/underflow */
int mathMode = 0;

enum Mode_min mode_min = Left1; //Initial mode is left
enum Mode_sec mode_sec = Left2; //Initial mode is left
enum Mode_LEFT_RIGHT LEFT_OR_RIGHT = LEFT;

main() {

    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    INTEnableSystemMultiVectoredInt(); // enable multi-vector interrupts


    DDPCONbits.JTAGEN = 0; //Shuts off JTAG
    PORTSetPinsDigitalOut(IOPORT_G, BIT_12 | BIT_13 | BIT_14 | BIT_15); //Set LD1 through LD4 as outputs
    PORTClearBits(IOPORT_G, BIT_12 | BIT_13 | BIT_14 | BIT_15); //Starts with all 4 LEDs off

    PORTSetPinsDigitalIn(IOPORT_G, BIT_6 | BIT_7); //Set Btn1 and Btn2 as inputs
    PORTSetPinsDigitalIn(IOPORT_A, BIT_0 /*|BIT_1|BIT_4|BIT_5*/); //Set Btn3 as an input

    PORTSetPinsDigitalOut(IOPORT_B, BIT_7 | BIT_8 | BIT_9 | BIT_10 | BIT_14 | BIT_15); // Set PMOD Port A as output for SSD1R
    PORTSetPinsDigitalOut(IOPORT_E, BIT_4 | BIT_5 | BIT_6 | BIT_7); // Set PMOD Port B as output for SSD1R
    PORTSetPinsDigitalOut(IOPORT_D, BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_5 | BIT_12); // Set PMOD Ports C and D as output for SSD2

    PORTSetPinsDigitalOut(IOPORT_F, BIT_12 | BIT_13 | BIT_4 | BIT_5);
    PORTSetPinsDigitalIn(IOPORT_E, BIT_9);
    PORTSetPinsDigitalIn(IOPORT_A, BIT_1 | BIT_4 | BIT_5);

    LED1 = 1; //initial state of LEDs
    LED2 = 0;
    LED3 = 0;
    LED4 = 0;

    clearSSD(); //set SSD to Zero

    while (1) {

        updateSSD(); //turn on ssd
        keyPressed = Poll_KYPD(); //poll the keypad

        //********************************************************//
        //all operations for key clicks here
        if (keyPressed != 10 && keyPressed != 11 && keyPressed
                != 12 && keyPressed != 13 && keyPressed != 14 && keyPressed != 15) { // a number is clicked, not an operation
            if (operand1 != 0 && operand2 != 0) { //used when preforming multiple operations
                operand1 = setOperand();
                operand2 = 0;
                clearSSD();
            }


            if (LED1 == 1 && LED2 == 1 && LED3 == 0 && LED4 == 0) { //if a hex number is showing clear and go back to decimal
                LED1 = 1;
                LED2 = 0;
                LED3 = 0;
                LED4 = 0;
                clearSSD();
            }

            //push all the digits to the left
            display_value_thousands = display_value_hundreds;
            display_value_hundreds = display_value_tens;
            display_value_tens = display_value_ones;
            display_value_ones = keyPressed;

        } else if (keyPressed == 10) { //preform addition

            if (operand1 == 0) {
                operand1 = setOperand();
                clearSSD();
            } else {
                operand2 = setOperand();
                equalsPress(mathMode);

            }
            mathMode = 1; //addition 

        } else if (keyPressed == 11) { //prefrom subtraction

            if (operand1 == 0) {
                operand1 = setOperand();
                if (LED4 == 1) {
                    operand1 = -1 * operand1;
                }
                clearSSD();
            } else {
                operand2 = setOperand();
                equalsPress(mathMode);

            }
            mathMode = 2; //subtraction


        } else if (keyPressed == 12) { //preform multiplication

            if (operand1 == 0) {
                operand1 = setOperand();
                clearSSD();
            } else {
                operand2 = setOperand();
                equalsPress(mathMode);

            }
            mathMode = 3; //multiplication

        } else if (keyPressed == 13) { //preform Division

            if (operand1 == 0) {
                operand1 = setOperand();
                clearSSD();
            } else {
                operand2 = setOperand();
                equalsPress(mathMode);

            }
            mathMode = 4; //Division

        } else if (keyPressed == 15) { //convert to hex/decimal
            hexDecimal();

        } else if (keyPressed == 14) { //equals press
            operand2 = setOperand();
            equalsPress(mathMode);

            operand1 = 0; //reset
            operand2 = 0; //reset 

        }


        //********************************************************//

        if (display_value_tens == 0 && display_value_hundreds == 16) { //handle a leading zero error
            display_value_tens = 16;
        }

        updateSSD();
    }

}

/**********************************
 * Equals Press
 * 
 * parameters:
 *  mode - math mode, i.e addition, division etc
 * 
 * returns:
 *  void
 * 
 * purpose:
 *  preform math
 * 
 **********************************/
void equalsPress(int mode) {
    //mode 1 - addition
    //mode 2 - subtraction
    //mode 3 - multiplication
    //mode 4 - division

    if (mode == 1) { //addition
        addition();

    } //end addition

    if (mode == 2) { //subtraction
        subtraction();

    } //end subtraction


    if (mode == 3) { //multiplication
        multiplication();
    } //end multiplication

    if (mode == 4) { //division
        division();
    } //end division



}

/**********************************
 * Additon()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  void 
 * 
 * purpose:
 *  prefrom addition
 * 
 **********************************/
void addition() {
    int output; //dummy varible 
    if (LED4 == 1) { //if "negative light" is one make value negative
        operand1 = operand1*-1;
    }
    output = operand1 + operand2;

    if (output > 9999) { //handle overflow
        overflowError();

    } else if (output < -9999) { //handle underflow
        underflowError();

    } else {

        int tempOut = output; //temp var
        if (output < 0) { //output negative?
            tempOut = tempOut * -1;
            LED4 = 1;
        } else {
            LED4 = 0;
        }

        int outputDigits[] = {0, 0, 0, 0}; //array that represents each SSD

        int nArray = 3; //counter
        while (tempOut > 0) { //seperate the digits and store in the array
            int digit = tempOut % 10;
            outputDigits[nArray] = digit;
            tempOut /= 10;

            nArray--;

        }

        //set the SSDs equal to the values of the array
        display_value_ones = outputDigits[3];
        display_value_tens = outputDigits[2];
        display_value_hundreds = outputDigits[1];
        display_value_thousands = outputDigits[0];

        if (display_value_thousands == 0) { //remove leading zeros
            display_value_thousands = 16;
            if (display_value_hundreds == 0) {
                display_value_hundreds = 16;
                if (display_value_tens == 0) {
                    display_value_tens == 16;
                }
            }
        }
    }

}

/**********************************
 * subtraction
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  void
 * 
 * purpose:
 *  preform subtraction
 * 
 **********************************/
void subtraction() {
    int output;
    output = operand1 - operand2;

    if (output > 9999) { //overflow error
        overflowError();
    } else if (output < -9999) { //underflow error
        underflowError();
    } else {

        int tempOut = output; //temp var
        if (output < 0) { //output negative?
            tempOut = tempOut * -1;
            LED4 = 1;
        } else {
            LED4 = 0;
        }

        int outputDigits[] = {0, 0, 0, 0}; //array that represents each SSD

        int nArray = 3; //counter
        while (tempOut > 0) { //seperate the digits and store in the array
            int digit = tempOut % 10;
            outputDigits[nArray] = digit;
            tempOut /= 10;

            nArray--;


        }


        //set the SSDs equal to the values of the array
        display_value_ones = outputDigits[3];
        display_value_tens = outputDigits[2];
        display_value_hundreds = outputDigits[1];
        display_value_thousands = outputDigits[0];

        if (display_value_thousands == 0) { //remove leading zeros
            display_value_thousands = 16;
            if (display_value_hundreds == 0) {
                display_value_hundreds = 16;
                if (display_value_tens == 0) {
                    display_value_tens == 16;
                }
            }
        }

    }


}

/**********************************
 * multiplication
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  void
 * 
 * purpose:
 *  preform multiplication
 * 
 **********************************/
void multiplication() {

    int output; //dummy var
    if (LED4 == 1) { //
        operand1 = operand1*-1;
    }
    output = operand1 * operand2;

    if (output > 9999) { //overflow handler
        overflowError();
    } else if (output < -9999) { //handle underflow
        underflowError();
    } else {

        int tempOut = output;
        if (output < 0) { //output negative?
            tempOut = tempOut * -1;
            LED4 = 1;
        } else {
            LED4 = 0;
        }


        int outputDigits[] = {0, 0, 0, 0}; //array that represents each SSD

        int nArray = 3; //counter
        while (tempOut > 0) { //seperate the digits and store in the array
            int digit = tempOut % 10;
            outputDigits[nArray] = digit;
            tempOut /= 10;

            nArray--;


        }

        //set the SSDs equal to the values of the array
        display_value_ones = outputDigits[3];
        display_value_tens = outputDigits[2];
        display_value_hundreds = outputDigits[1];
        display_value_thousands = outputDigits[0];

        if (display_value_thousands == 0) { //remove leading zeros
            display_value_thousands = 16;
            if (display_value_hundreds == 0) {
                display_value_hundreds = 16;
                if (display_value_tens == 0) {
                    display_value_tens == 16;
                }
            }
        }

    }



}

/**********************************
 * division()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  void
 * 
 * purpose:
 *preform division  
 * 
 **********************************/
void division() {

    int output;
    if (LED4 == 1) { //if "negative light" is one make value negative
        operand1 = operand1*-1;
    }
    output = operand1 / operand2; //division 

    if (output > 9999) { //overflow error
        overflowError();
    } else if (output < -9999) { //underflow error
        underflowError();
    } else {

        int tempOut = output; //dummy var
        if (output < 0) { //turn negative light on off
            tempOut = tempOut * -1;
            LED4 = 1;
        } else {
            LED4 = 0;
        }


        int outputDigits[] = {0, 0, 0, 0}; //array that represents each SSD

        int nArray = 3; //counter
        while (tempOut > 0) { //seperate the digits and store in the array
            int digit = tempOut % 10;
            outputDigits[nArray] = digit;
            tempOut /= 10;

            nArray--;


        }

        //set the SSDs equal to the values of the array
        display_value_ones = outputDigits[3];
        display_value_tens = outputDigits[2];
        display_value_hundreds = outputDigits[1];
        display_value_thousands = outputDigits[0];

        if (display_value_thousands == 0) { //remove leading zeros
            display_value_thousands = 16;
            if (display_value_hundreds == 0) {
                display_value_hundreds = 16;
                if (display_value_tens == 0) {
                    display_value_tens == 16;
                }
            }
        }

    }


}

/**********************************
 * setOperand()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  integer (operand)
 * 
 * purpose:
 *  This gathers the numbers on the display and turns them into one
 *  integer value. 
 * 
 **********************************/
int setOperand() {

    if (display_value_tens == 16) { //change off value to 0
        display_value_tens = 0;
    }
    if (display_value_hundreds == 16) { //change off value to 0
        display_value_hundreds = 0;
    }
    if (display_value_thousands == 16) { //change off value to 0
        display_value_thousands = 0;
    }

    int w = display_value_ones;
    int x = display_value_tens * 10;
    int y = display_value_hundreds * 100;
    int z = display_value_thousands * 1000;

    int operand = z + y + x + w; //add digits with multiples
    w = x = y = z = 0;

    if (operand == 0) { //look for division error
        divisonError();
    }

    return operand;


}

/**********************************
 * overflowError()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  void
 * 
 * purpose:
 *  handle when the ssd when a math function exceeds 9999
 * 
 **********************************/
void overflowError() {
    mathMode = 5;
    LED1 = LED2 = LED3 = LED4 = 0; //turn off LEDs

    //blink ssd wait for btn1 to reset
    while (!Btn1) {
        updateSSD();
        int i = 0;
        while (i < 400 && !Btn1) {

            display_value_thousands = 17;
            display_value_hundreds = 17;
            display_value_tens = 17;
            display_value_ones = 17;
            i++;
            updateSSD();
        }
        if (Btn1) {
            resetCalc();
            break;
        }

        wait(400, false);

    }
    resetCalc();

}

/**********************************
 * underflowError()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  handle when the ssd when a math function goes below -9999
 * 
 **********************************/
void underflowError() {
    mathMode = 5;
    LED1 = LED2 = LED3 = LED4 = 0;

    //blink ssd wait for btn1 to reset
    while (!Btn1) {
        updateSSD();
        int i = 0;
        while (i < 400 && !Btn1) {
            display_value_thousands = 18;
            display_value_hundreds = 18;
            display_value_tens = 18;
            display_value_ones = 18;
            i++;
            updateSSD();
        }
        if (Btn1) {
            resetCalc();
            break;
        }


        wait(400, false);

    }
}

/**********************************
 * divisonError()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  handles the ssd when dividing by zero
 * 
 **********************************/
void divisonError() {
    mathMode = 5;
    LED1 = LED2 = LED3 = LED4 = 0;

    //blink ssd wait for btn1 to reset
    while (!Btn1) {
        updateSSD();
        int i = 0;
        while (i < 400 && !Btn1) {
            display_value_thousands = 19;
            display_value_hundreds = 19;
            display_value_tens = 19;
            display_value_ones = 19;
            i++;
            updateSSD();
        }
        if (Btn1) {
            resetCalc();
            break;
        }

        wait(400, false);

    }
}

/**********************************
 * HexDecimal()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  converts between hex and decimal formal and updates display accordingly
 * 
 **********************************/
void hexDecimal() {
    int q; //quotient
    int decimal;

    //change off values to zero
    if (display_value_tens == 16) {
        display_value_tens = 0;
    }
    if (display_value_hundreds == 16) {
        display_value_hundreds = 0;
    }
    if (display_value_thousands == 16) {
        display_value_thousands = 0;
    }

    if (LED2 != 1) { //convert to hex
        LED1 = 1;
        LED2 = 1;
        LED3 = 0;
        LED4 = 0;

        int i = 0;
        int temp;

        int hexNumber[4] = {0, 0, 0, 0};
        q = ((display_value_thousands * 1000)+(display_value_hundreds * 100) + (display_value_tens * 10) + display_value_ones);

        while (q != 0) {
            temp = q % 16;
            hexNumber[i] = temp;
            q = q / 16;
            i++;
        }//while

        display_value_ones = hexNumber[0];
        display_value_tens = hexNumber[1];
        display_value_hundreds = hexNumber[2];
        display_value_thousands = hexNumber[3];


    } else { //convert to decimal
        LED1 = 1;
        LED2 = 0;
        LED3 = 0;
        LED4 = 0;


        decimal = ((display_value_thousands * (pow(16, 3)))+(display_value_hundreds * (pow(16, 2))) + (display_value_tens * 16) + display_value_ones);
        display_value_thousands = decimal / 1000;
        display_value_hundreds = (decimal % 1000) / 100;
        display_value_tens = ((decimal % 1000) % 100) / 10;
        display_value_ones = ((decimal % 1000) % 100) % 10;
    }

    if (display_value_thousands == 0) { //remove leading zeros
        display_value_thousands = 16;
        if (display_value_hundreds == 0) {
            display_value_hundreds = 16;
            if (display_value_tens == 0) {
                display_value_tens == 16;
            }
        }
    }




}

/**********************************
 * wait()
 * 
 * parameters:
 *  integer (time) - how long of a delay
 *  bool (willLightDisplay) - determines if ssd display will be one
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  a delay function
 * 
 **********************************/
void wait(int time, bool willLightDisplay) {

    int i = 0;

    if (mathMode == 5) { //overflow-underflow-division errors

        while (i < 400 && !Btn1) {
         
                if (willLightDisplay == true) {
                    updateSSD();
                } else {

                    display_value_thousands = 16;
                    display_value_hundreds = 16;
                    display_value_tens = 16;
                    display_value_ones = 16;

                    updateSSD();
                }
                i++;
        }
        if (Btn1) {
            resetCalc();
        }

        
    } else {

        for (i = 0; i < time; i++) {
            if (willLightDisplay == true) {
                updateSSD();
            } else {
                display_value_thousands = 16;
                display_value_hundreds = 16;
                display_value_tens = 16;
                display_value_ones = 16;
                updateSSD();
            }

        }


    }
}

/**********************************
 * resetCalc()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  resets calculator to initial state  
 * 
 **********************************/
void resetCalc() {
    display_value_thousands = 16;
    display_value_hundreds = 16;
    display_value_tens = 16;
    display_value_ones = 0;

    LED1 = 1;
    LED4 = LED2 = LED3 = 0;

    operand1 = 0;
    operand2 = 0;

    mathMode = 0;
}

/**********************************
 * poll_kypd_row()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  poll the row of the kypd
 * 
 **********************************/
int Poll_KYPD_ROW() {
    updateSSD();

    if (Btn1 == 1) {

        while (Btn1) {
            updateSSD();
        }
        resetCalc();

    } else {
        if (!Row1) { //if a button in row 1 is pressed
            Row = 1;
            while (!Row1) {
                updateSSD();
            } //wait for button to be lifted to continue
            return Row;

        } else if (!Row2) { //if a button in row 2 is pressed
            Row = 2;
            while (!Row2) {
                updateSSD();
            } //wait for button to be lifted to continue
            return Row;

        } else if (!Row3) { //if a button in row 3 is pressed
            Row = 3;
            while (!Row3) {
                updateSSD();
            } //wait for button to be lifted to continue
            return Row;


        } else if (!Row4) { //if a button in row 4 is pressed
            Row = 4;
            while (!Row4) {
                updateSSD();
            } //wait for button to be lifted to continue
            return Row;


        } else {
            Row = 9;

            return Row;

        }

        updateSSD();
        //return Row;
    }
}

/**********************************
 *  Poll_KYPD_COL()
 * 
 * parameters:
 *  int (col) - column
 * 
 * returns:
 *  int (col)
 * 
 * purpose:
 *  find the column
 * 
 **********************************/
int Poll_KYPD_COL(int col) {
    updateSSD();
    switch (col) //pull the appropriate column pin LOW
    {
        case 0: //col1
            Col1 = 0;
            Col2 = 1;
            Col3 = 1;
            Col4 = 1;

            break;
        case 1: //col2
            Col1 = 1;
            Col2 = 0;
            Col3 = 1;
            Col4 = 1;
            break;
        case 2: //col3
            Col1 = 1;
            Col2 = 1;
            Col3 = 0;
            Col4 = 1;
            break;
        case 3: //col4
            Col1 = 1;
            Col2 = 1;
            Col3 = 1;
            Col4 = 0;
            break;
        default:
            break;
    }


    updateSSD();
    return col; //return column pressed
}

/**********************************
 * Poll_KYPD()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  int (pressedButton) - what keypad was pressed
 * 
 * purpose:
 *  to find what key was pressed
 * 
 **********************************/
int Poll_KYPD() {
    int pressedButton; //what keypad button pressed
    updateSSD();

    int key[4][4] = {//2d array for keypad
        {1, 4, 7, 0}, //col 1
        {2, 5, 8, 15}, //col 2
        {3, 6, 9, 14}, //col 3
        {10, 11, 12, 13} //col 4
    };


    int row; //what row clicked
    int col; //what col clicked


    while (Row == 9) { //iterate through the columns until the col and row correspond correctly
        Col++;

        if (Col > 3) { //only 4 cols
            Col = 0;
        }

        row = Poll_KYPD_ROW(); //find row 
        col = Poll_KYPD_COL(Col); //find col

    }

    if (col == 0) { //brute force method of fixing col 4
        col = 4;
    }

    pressedButton = key[col - 1][row - 1]; //-1 b/c index starts at zero


    Row = 9; //reset to arbitrary values
    Col = -1; //reset to arbitrary values

    return pressedButton; //return keypad value

}

/**********************************
 * clearSSD()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  puts display back to zero
 * 
 **********************************/
void clearSSD() {
    display_value_thousands = 16; //off
    display_value_hundreds = 16; //off
    display_value_tens = 16; //off
    display_value_ones = 0;

}

/**********************************
 * updateSSD()
 * 
 * parameters:
 *  none
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  called whenever the display is changed
 * 
 **********************************/
void updateSSD() {
    slowDownDisplay(mode_min == Left1, number[display_value_thousands], LEFT_OR_RIGHT == RIGHT); // controls left, left
    slowDownDisplay(mode_sec == Left2, number[display_value_tens], LEFT_OR_RIGHT == LEFT); // controls right, left
    slowDownDisplay(mode_min == Right1, number[display_value_hundreds], LEFT_OR_RIGHT == RIGHT); // controls left, right
    slowDownDisplay(mode_sec == Right2, number[display_value_ones], LEFT_OR_RIGHT == LEFT); // controls right, right
}

/**********************************
 * displayDigit()
 * 
 * parameters:
 *  char (display_sel) - which display
 *  char (value) - what number is displayed
 *  char (side) - which side of the display do you want to show
 * 
 * returns:
 *  none
 * 
 * purpose:
 *  pick a ssd to display and what to display
 * 
 **********************************/
void displayDigit(unsigned char display_sel, unsigned char value, unsigned char side) {

    if (side == LEFT) {
        DispSel1 = display_sel;
        SegA1 = value & 1;
        SegB1 = (value >> 1) & 1;
        SegC1 = (value >> 2) & 1;
        SegD1 = (value >> 3) & 1;
        SegE1 = (value >> 4) & 1;
        SegF1 = (value >> 5) & 1;
        SegG1 = (value >> 6) & 1;


    }

    if (side == RIGHT) {
        DispSel2 = display_sel;
        SegA2 = value & 1;
        SegB2 = (value >> 1) & 1;
        SegC2 = (value >> 2) & 1;
        SegD2 = (value >> 3) & 1;
        SegE2 = (value >> 4) & 1;
        SegF2 = (value >> 5) & 1;
        SegG2 = (value >> 6) & 1;

    }

}

/**********************************
 * slowDownDisplay()
 * 
 * parameters:
 *  char (display_sel) - which display
 *  char (value) - what number is displayed
 *  char (side) - which side of the display do you want to show
 *
 * returns:
 * none
 *  
 * purpose:
 *  delay updating of display 
 * 
 **********************************/
void slowDownDisplay(unsigned char display_sel, unsigned char value, unsigned char side) {
    unsigned int slow = 0;
    while (slow < REFRESH_RATE) {
        displayDigit(display_sel, value, side);
        slow++;
    }
}
