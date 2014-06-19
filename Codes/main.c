										   /*****************************************************************************
 This programme counts encoder pulses from the right motor's position encoder using at Timer 1 as counter 1 at 
 its external clock pin,  compares them to a required distance and stops robot after covering predefined distance.  
 *******************************************************************************/

#include <intrins.h>
#include "p89v51rx2.H"


sbit LB=P1^0;  //LEFT BACK Pin
sbit LF=P1^1;  //LEFT FRONT Pin
sbit RF=P1^2;  //RIGHT FRONT Pin
sbit RB=P3^4;  //RIGHT BACK Pin
sbit left_velocity=P1^3; //Left velocity control pin. 
sbit right_velocity=P1^4; //Right velocity control pin. 

unsigned int right_shaft_count=0; //variable used to store pulse count from right position encoder
unsigned int count=0;
unsigned int speed=0;

sbit RS=P2^6; //Register Select
sbit RW=P2^5; //Read Write
sbit EN=P2^4; //Enable line
sbit buzzer=P2^7; //Buzzer Pin


const unsigned char LCD_CLEAR 	  = 0x01;	/* Clear display screen */  
const unsigned char CURSOR_HOME	  = 0x02;	/* Return Home (First line first block) */
const unsigned char DISPLAY_ON 	  = 0x0F;	/* Display ON cursor Blinking */
const unsigned char LINE1 	  = 0x80;	/* Address for Line 1 */ 
const unsigned char LINE2	  = 0xC0;	/* Address for Line 2 */

const unsigned char CURSOR_LEFT   = 0x10;	/* Decrement cursor (shift cursor to left) */
const unsigned char CURSOR_RIGHT  = 0x14;	/* Increment cursor (shift cursor to right) */
const unsigned char DISPLAY_LEFT  = 0x18;	/* Shift the entire display to left */ 
const unsigned char DISPLAY_RIGHT = 0x1C;	/* Shift the entire display to right */
const unsigned char DOFF_COFF 	  = 0x08;	/* Display OFF cursor OFF */
const unsigned char DON_COFF 	  = 0x0C;	/* Display ON cursor OFF */

const unsigned char TEST 	  = 0xC6;	/* TEST */

 void delay_ms(unsigned int ms)		// Delay Function
{
 unsigned int i, j;
 
 for(i=0;i<ms;i++)
 	for(j=0;j<53;j++);
} 

void pulse(void)	 //Generating Hi-Low Pulse for Enable Pin
{
 EN=1;
 delay_ms(4);
 EN=0;
}

void LCD_CMD(unsigned char cmd)		 //Function To Send Command
{
 P2=(0x00|0x80);
 EN=0;								
 RS=0;								// Command Register Select
 //RW=0;

 P2=(((cmd&0xF0)/0x10)|0x80);		// Send Higher Bits
 pulse();

 P2=((cmd&0x0F)|0x80);				// Send Lower Bits
 pulse();
}

void LCD_INIT(void)
{
 EN=0;
 RS=0;				// Command Register Select
 //RW=0;
 P2=(0x02|0x80);		// 4 bit mode
 pulse();
 LCD_CMD(LCD_CLEAR);		 // Clear LCD 
 LCD_CMD(DISPLAY_ON);
 LCD_CMD(CURSOR_HOME);
}

void LCD_CHAR(unsigned char dat)   //Function for Character write
{
 P2=(0x40|0x80);
 EN=0;
 RS=1;					// Data Register Select
 //RW=0;
 P2=((((dat&0xF0)/0x10)|0x40)|0x80);	  // Send Higher Bits
 pulse();

 P2=(((dat&0x0F)|0x40)|0x80);			  // Send Lower Bits
 pulse();
}

void LCD_WRITE(unsigned char *dat)		  // Function to write Character String
{
 unsigned char i;

 for(i=0;i<16;i++)
 {
 	if(*dat!='\0')
	{
		LCD_CHAR(*dat);					 // Sending each character of string to print
		dat++;
	}
	else
	{
		break;
	}
 }
}



//initializing timer/counter1 as counter in mode 2
void  timer_setup(void)   
	{
 	TMOD=0x61;  // Timer 1 in 8 bit external counter mode
 	TH1=0; // reset counter value to 0       
 	TL1=0;	// reset counter value to 0  
	TH0=0;
	TL0=0;
	}

void int_setup(void)
	{
	 IEN0=0x02;		   // Enable timer interrupt.
	}

void forward(void)
	{
	RF=1;
	LF=1;
	RB=0;
	LB=0;
	}

void calc_speed(void)
	{
	speed = (unsigned int)((right_shaft_count*5.44));	  //5.44 mm shaft multiplyied by no of count.
	}

void disp_speed (void)
	{
	unsigned char speed_arry[3];			  // 3digit arry for velocity
	unsigned char dat=0;
	unsigned int val=0;
	LCD_CMD(LINE1);
	val=speed;
												// for hundred 
	speed_arry[2]=(val/100)+48;
	speed_arry[1]=val/10;
	speed_arry[1]=(speed_arry[1]%10)+48;		 // for tens
	speed_arry[0]= (val % 10) + 48;				 // for unit

	LCD_CMD(TEST);								 //  to set cursor position

	dat=speed_arry[2];
	LCD_CHAR(dat);
	dat=speed_arry[1];
	LCD_CHAR(dat);
	dat='.'	;
	LCD_CHAR(dat);
	dat=speed_arry[0];
	LCD_CHAR(dat);

	}

void int1_isr(void)interrupt 1
	{
	EA=0;		  // desable interrupt 
	TR0=0;		   // stop timer 0
	count++;		 // increment variable 
	if (count == 20)  // if this is 20 menance the 1 second completed so calc speed and display it
		{
		
		count=0;
		calc_speed();
		disp_speed();
		TH1=0;
		TL1=0;
		}
	TH0=0x3C;			// set again timer for 50 ms 
	TL0=0xAF;
	EA=1;				// enable interrupt	
	TR0=1;				// start timet again
	TF0=0;				// reset timer interrupt flag
	}


void main()
	{
 	unsigned int distance=0;
 	unsigned int reqd_shaft_count_int=0;
 	LCD_INIT();			  // LCD initialization 
	LCD_CMD(DON_COFF);	  // Curssor off
 	timer_setup();  	//Timer  initialization  Timer 1 as a counter and Timer 0 as Timer 
	int_setup();		// Enable timer interrrupt
 
  
	left_velocity=0; //setting this pin to one sets the motor to run at maximum velocity. Here enable pin of Motor driver is always on unlike in PWM mode.
 	right_velocity=1;//setting this pin to one sets the motor to run at maximum velocity. Here enable pin of Motor driver is always on unlike in PWM mode
 
	LCD_CMD(LINE1);						 //	 Curssor at 1st position line 1
	LCD_WRITE("***FireBird V***");		 //  Wel-Come msg.
	LCD_CMD(LINE2);
	LCD_WRITE("Vel. mesurement");
	delay_ms(5000);
	LCD_CMD(LINE2);						  
	LCD_WRITE("Vel =      cm/s");
					  
 	TR1=1;	//start timer 1 as a Counter
	TR0=1;  //Start timer 1 
	EA=1;	// Enable interrupt for timer 0
 	forward();  //set the robot to move forward     
   
 	while(1)
		{
		right_shaft_count=TL1;	   // Move count no in to variable
		}
	}							//main ends