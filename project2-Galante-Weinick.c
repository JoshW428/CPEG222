/*=================CPEG222===================
 * Program:		Project #2
 * Authors: 		James Galante, Josh Weinick
 * Date: 		3/14/2017
 * Description:
 *
 * This porject is a timer/countdown
 * you pick the amount of time you want with the buttons
 * and either count up from there, or create a countdown
 * you can also pause and play
 *

============================================*/

#include <p32xxxx.h>
#include <plib.h>
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
#define DELAY2 400000



// The refresh rate of the segments
#define REFRESH_RATE 200   // ~1/100 secs


/****************************************************/
/* Port Definition                           */
/*    You should add your port definition if necessary*/
/****************************************************/

// Buttons
#define Btn1	PORTGbits.RG6
#define Btn2    PORTGbits.RG7
#define Btn3    PORTAbits.RA0

#define LED1 LATGbits.LATG12
#define LED2 LATGbits.LATG13
#define LED3 LATGbits.LATG14
#define LED4 LATGbits.LATG15


// 7 Segment Display pmod for using the bottom row JC & JD jumpers
// Segments

#define SegA1 	PORTBbits.RB15
#define SegB1	PORTDbits.RD5
#define SegC1 	PORTDbits.RD4
#define SegD1 	PORTBbits.RB14
#define SegE1	PORTDbits.RD1
#define SegF1	PORTDbits.RD2
#define SegG1	PORTDbits.RD3


// 7 segment display pmod for using the bottom row of JA & JB jumpers
#define SegA2 	PORTBbits.RB7
#define SegB2	PORTBbits.RB8
#define SegC2	PORTBbits.RB9
#define SegD2 	PORTBbits.RB10
#define SegE2	PORTEbits.RE4
#define SegF2	PORTEbits.RE5
#define SegG2	PORTEbits.RE6


// Display selection. 0 = right, 1 = left (Cathode)
#define DispSel1 PORTDbits.RD12

// Display selection. 0 = right, 1 = left (Cathode)
#define DispSel2 PORTEbits.RE7


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
//enum for is it counting or not

enum State {
    PAUSE, PLAY
};


//Subroutine, add yours for the project
void displayDigit(unsigned char, unsigned char, unsigned char); // Display one digit among two digits
void slowDownDisplay(unsigned char, unsigned char, unsigned char); // Frequency control function
void updateSSD();


// look-up table for the numbers
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
    0b0000000 //clear
};

//variable definition
unsigned int display_value_left1 = 0; // The initial displayed value left 2
unsigned int display_value_right1 = 0; // The initial displayed value right 2

unsigned int display_value_left2 = 0; // The initial displayed value left 2
unsigned int display_value_right2 = 0; // The initial displayed value right 2

int Buttonlock = 0;
int switchCase;


enum Mode_min mode_min = Left1; //Initial mode is left
enum Mode_sec mode_sec = Left2; //Initial mode is left
enum State state = PLAY;
enum Mode_LEFT_RIGHT LEFT_OR_RIGHT = LEFT;

main() {
    //initialization
    TRISG = 0x00C0;
    //TRISA = 0x00C0;
    AD1PCFG = 0xFFFF;
    TRISE = 0x0000;

    PORTA = 0x0000;
    PORTG = 0x0000;

    PORTE = 0x0001;

    TRISB = 0x0000; // set PORTB as output
    TRISD = 0x0000; // set PORTE as output
    //TRISG = 0x40; // set PORTG as output
    PORTD = 0; // initialize PORTE to 0
    PORTB = 0; // initialize PORTG to 0

    DDPCONbits.JTAGEN = 0; //off for button 3

    //  Configure device for max performance
    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    OpenCoreTimer(SECOND); // configure core timer with rate defined above


    LED1 = 1;
    LED2 = 1;
    LED3 = 1;
    LED4 = 1;

    //set to zero
    display_value_left1 = 0;
    display_value_right1 = 0;
    display_value_left2 = 0;
    display_value_right2 = 0;

    while (1) {


        //***************INTIAL STATE CODE 00:00***************// mode 1


        while (1) {
            LED1 = 1; //remove later
            LED2 = 1; //remove later
            LED3 = 1; //remove later
            LED4 = 1; //remove later

            updateSSD();


            while (Btn3 && Buttonlock) { //buffer when switching modes
                updateSSD();
            }
            Buttonlock = 0; //reset button lock

            if (!Buttonlock) {
                if (Btn1) {
                    switchCase = 5; //switch to mode 5
                    Buttonlock = 1;
                    break; //leaves initial while loop
                }

                if (Btn2) {
                    switchCase = 4; //switch to mode 4
                    Buttonlock = 1;
                    break; ////leaves initial while loop
                }

                if (Btn3) {
                    switchCase = 2; //switch to mode 2
                    Buttonlock = 1;
                    break; //leaves initial while loop
                }
            }

        }


        switch (switchCase) {
                //**************************************************************************//    
            case 2: //mode 2
                while (1) {
                    updateSSD();

                    LED1 = 0;
                    LED2 = 1;
                    LED3 = 0;
                    LED4 = 0;

                    while (Btn3 && Buttonlock) { //waits for button 3 to be lifted while switching modes
                        updateSSD();
                    }


                    if (Btn3 && !Buttonlock) { //button 3 press
                        switchCase = 3;
                        Buttonlock = 1;
                        break;

                    } else if (!Btn3 && !Btn2 && !Btn1 && Buttonlock) {
                        updateSSD();

                        Buttonlock = 0;
                        //break;
                    }


                    if (Btn2 && !Buttonlock) { //button 2 press, increment up
                        updateSSD();

                        Buttonlock = 1;

                        display_value_right1++;

                        if (display_value_right1 > 9) {

                            display_value_right1 = 0;
                            display_value_left1++;

                            if (display_value_left1 > 9) {
                                display_value_left1 = 0;
                                display_value_right1 = 0;
                            }
                        }

                    } else if (!Btn2 && !Btn3 && !Btn1 && Buttonlock) //unlock button
                    {
                        updateSSD();
                        Buttonlock = 0;
                    }



                    if (Btn1&&!Buttonlock) { //button 1 press, increment down
                        updateSSD;

                        Buttonlock = 1;

                        display_value_right1--;

                        if (display_value_left1 == 0 && display_value_right1 > 9) {
                            display_value_right1 = 9;
                            display_value_left1--;

                            display_value_left1 = 9;
                            display_value_right1 = 9;
                        }


                        if (display_value_left1 != 0 && display_value_right1 > 9) {

                            display_value_right1 = 9;
                            display_value_left1--;


                        }

                    } else if (!Btn1 && !Btn3 && !Btn2 && Buttonlock) //unlock button
                    {
                        updateSSD();

                        Buttonlock = 0;
                    }

                }

                //**************************************************************************//    
            case 3: //mode 3
                while (1) {
                    updateSSD();

                    LED1 = 1;
                    LED2 = 0;
                    LED3 = 0;
                    LED4 = 0;


                    if (Btn3 && !Buttonlock) { //button 3 press
                        Buttonlock = 1;
                        break; //return to initial state

                    } else if (!Btn3 && !Btn2 && !Btn1 && Buttonlock) {
                        updateSSD();
                    }


                    if (Btn2 && !Buttonlock) { //button 2 press, increment up
                        updateSSD();

                        Buttonlock = 1;

                        display_value_right2++;

                        if (display_value_right2 > 9) {

                            display_value_right2 = 0;
                            display_value_left2++;

                            if (display_value_left2 > 5) {
                                display_value_left2 = 0;
                                display_value_right2 = 0;
                            }
                        }

                    } else if (!Btn2 && !Btn3 && !Btn1 && Buttonlock) //unlock
                    {
                        updateSSD();

                        Buttonlock = 0;
                        //break;
                    }



                    if (Btn1&&!Buttonlock) { //button 1 press, increment down
                        updateSSD;

                        Buttonlock = 1;

                        display_value_right2--;

                        if (display_value_left2 == 0 && display_value_right2 > 9) {
                            display_value_right2 = 9;
                            display_value_left2--;

                            display_value_left2 = 5;
                            display_value_right2 = 9;
                        }


                        if (display_value_left2 != 0 && display_value_right2 > 9) {

                            display_value_right2 = 9;
                            display_value_left2--;


                        }

                    } else if (!Btn1 && !Btn3 && !Btn2 && Buttonlock) //unlock, button
                    {
                        updateSSD();

                        Buttonlock = 0;
                        //break;
                    }

                }
                break;

                //**************************************************************************//    
            case 4: //mode 4

                while (1) {
                    updateSSD();

                    LED1 = 1;
                    LED2 = 1;
                    LED3 = 0;
                    LED4 = 0;



                    //One second time is started here
                    //pause and play code is also inside the timer itself
                    OpenCoreTimer(SECOND);
                    while (!mCTGetIntFlag()) {
                        updateSSD();

                        if (Btn3) { //reset
                            updateSSD();
                            break;
                        }

                        if (Btn2 && !Buttonlock) { //pause
                            //pause, play timer
                            state = PAUSE;
                            while (Btn2 && state == PAUSE) {
                                updateSSD();
                            }

                            while (1) {
                                updateSSD();
                                if (state == PAUSE && Btn2) { //play
                                    updateSSD();
                                    state = PLAY;
                                    break;
                                }
                            }
                        }
                        while (Btn2) {
                            updateSSD();
                        }
                    }
                    // wait for flag to set
                    mCTClearIntFlag(); // clear CoreTimer flag  */
                    UpdateCoreTimer(SECOND); // updating core timer.
                    if (!Btn2) {
                        display_value_right2++; //adding every second
                    }

                    //count up on the timer
                    if (display_value_right2 > 9) {
                        display_value_right2 = 0;
                        display_value_left2++;

                        if (display_value_left2 > 5) {
                            display_value_left2 = 0;
                            display_value_right2 = 0;

                            display_value_right1++;
                            if (display_value_right1 > 9) {
                                display_value_right1 = 0;
                                display_value_left1++;

                                if (display_value_left1 > 9) {
                                    display_value_left1 = 0;
                                    display_value_right1 = 0;
                                }
                            }
                        }
                    }


                    if (Btn3 && !Buttonlock) { //button 3 press
                        updateSSD();
                        Buttonlock = 1;
                        break; //return to initial state

                    } else if (!Btn3 && !Btn2 && !Btn1 && Buttonlock) {
                        updateSSD();
                        Buttonlock = 0;
                    }


                    //turn off button lock when nothing is pressed
                    if (!Btn3 && !Btn2 && !Btn1 && Buttonlock) {
                        updateSSD();
                        Buttonlock = 0;
                    }





                }

                break;

                //**************************************************************************//    
            case 5:
                while (1) {
                    updateSSD();

                    LED1 = 0;
                    LED2 = 0;
                    LED3 = 1;
                    LED4 = 1;

                    //One second time is started here
                    //pause and play code is also inside the timer itselfF
                    OpenCoreTimer(SECOND);
                    while (!mCTGetIntFlag()) {
                        updateSSD();

                        if (Btn3) {
                            updateSSD();
                            break;
                        }

                        if (Btn1 && !Buttonlock) {
                            //pause, play timer
                            state = PAUSE;
                            while (Btn1 && state == PAUSE) {
                                updateSSD();
                            }

                            while (1) {
                                updateSSD();
                                if (state == PAUSE && Btn1) {
                                    updateSSD();
                                    state = PLAY;
                                    break;
                                }
                            }
                        }
                        while (Btn1) {
                            updateSSD();
                        }
                    }
                    // wait for flag to set
                    mCTClearIntFlag(); // clear CoreTimer flag  */
                    UpdateCoreTimer(SECOND); // updating core timer.
                    if (!Btn1) {
                        display_value_right2--; //adding every second
                    }


                    //when 00:00 is achieved, stop
                    if (display_value_right1 == 0 &&
                            display_value_right2 == 0 &&
                            display_value_left1 == 0 &&
                            display_value_left2 == 0) {
                        display_value_right1 = 0;
                        display_value_right2 = 0;
                        display_value_left1 = 0;
                        display_value_left2 = 0;

                        //wait for reset
                        while (!Btn3) {
                            updateSSD();
                        }
                    }


                    //count down on the timer
                    if (display_value_right2 > 9) {
                        display_value_right2 = 9;
                        display_value_left2--;

                        if (display_value_left2 > 9) {
                            display_value_left2 = 5;
                            display_value_right2 = 9;

                            display_value_right1--;
                            if (display_value_right1 > 9) {
                                display_value_right1 = 9;
                                display_value_left1--;

                                if (display_value_left1 > 9) {
                                    display_value_left1 = 9;
                                    display_value_right1 = 9;
                                }
                            }
                        }
                    }




                    if (Btn3 && !Buttonlock) { //button 3 press
                        updateSSD();
                        Buttonlock = 1;
                        break; //return to initial state

                    } else if (!Btn3 && !Btn2 && !Btn1 && Buttonlock) {
                        updateSSD();
                        Buttonlock = 0;
                        //break;
                    }



                    //turn off button lock when nothing is pressed
                    if (!Btn3 && !Btn2 && !Btn1 && Buttonlock) {

                        updateSSD();
                        Buttonlock = 0;
                    }





                }

                break;

                //**************************************************************************//      
        }







        //*********************************************//

    }
}

//update function that allows the SSD to show the correct numbers at the correct times
void updateSSD() {
    slowDownDisplay(mode_min == Left1, number[display_value_left1], LEFT_OR_RIGHT == RIGHT); // controls left, left
    slowDownDisplay(mode_sec == Left2, number[display_value_left2], LEFT_OR_RIGHT == LEFT); // controls right, left
    slowDownDisplay(mode_min == Right1, number[display_value_right1], LEFT_OR_RIGHT == RIGHT); // controls left, right
    slowDownDisplay(mode_sec == Right2, number[display_value_right2], LEFT_OR_RIGHT == LEFT); // controls right, right
}

//display digit
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

// debouncing keys & display
void slowDownDisplay(unsigned char display_sel, unsigned char value, unsigned char side) {
    unsigned int slow = 0;
    while (slow < REFRESH_RATE) {
        displayDigit(display_sel, value, side);
        slow++;
    }
}