/* Dylan Houn (dhoun002@ucr.edu)
 * Lab Section = B21
 * Assignment = Custom Lab Assignment
 * 
 * I acknowledge all content contained herein, excluding template 
 * or example code, is my original work.
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.c"
#include "keypad.h"
#include "bit.h"

//Global
volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;
unsigned char seconds = 0;
unsigned char minutes = 0;
unsigned char hours = 0;
unsigned char second = 0;
unsigned char minute = 0;
unsigned char hour = 0;
unsigned char i = 0;
unsigned char alarminutes = 0;
unsigned char alarhours = 3;
unsigned char alarminute = 0;
unsigned char alarhour = 0;


// Timer
void TimerISR() {
	TimerFlag = 1;
}

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00;
}

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;
	if(_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


//PWM
void set_PWM(double frequency) {
	static double current_frequency;
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		else {TCCR3B |= 0x03;}
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		else if (frequency > 31250) { OCR3A = 0x0000; }
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0;
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}



// State Machine
enum STATES {Init, IncTime, SetTime, SetAlarm, RingAlarm, Snooze}States;

void tick() {
	switch(States) {
		case Init:
			States = IncTime;
			break;
		case IncTime:
			if(hours == alarhours && hour == alarhour && minutes == alarminutes && minute == alarminute) {
				i = 0;
				States = RingAlarm;
			}
			else if(GetKeypadKey() == 0x42) {
				i = 1;
				States = SetTime;
			}
			else if(GetKeypadKey() == 0x41) {
				i = 1;
				States = SetAlarm;
			}
			else {
				States = IncTime;
			}
			break;
		case SetTime:
			if(i >= 6) {
				States = IncTime;
			}
			else {
				States = SetTime;
			}
			break;
		case SetAlarm:
			if(i >= 6) {
				States = IncTime;
			}
			else {
				States = SetAlarm;
			}
			break;
		case RingAlarm:
			if(GetKeypadKey() == 0x43) {
				alarhours = 3;
				States = IncTime;
			}
			else if (GetKeypadKey() == 0x44) {
				States = Snooze;
			}
			else {
				States = RingAlarm;
			}
			break;
		case Snooze:
			States = IncTime;
			break;
	}
	
	switch(States) {
		case Init:
			hours = 0;
			minutes = 0;
			seconds = 0;
			hour = 0;
			minute = 0;
			second = 0;
			break;
		case IncTime:
			PWM_on();
			PWM_off();
			second++;
			if(second == 10) {
				seconds++;
				second = 0;
			}
			if(seconds == 6) {
				minute++;
				seconds = 0;
			}
			if(minute == 10) {
				minutes++;
				minute = 0;
			}
			if(minutes == 6) {
				hour++;
				minutes = 0;
			}
			if(hour == 10) {
				hours++;
				hour = 0;
			}
			if(((hours * 10) + hour) == 24) {
				hours = 0;
				hour = 0;
			}
			LCD_Cursor(10);
			LCD_WriteData(' ');
			LCD_Cursor(11);
			LCD_WriteData(' ');
			LCD_Cursor(12);
			LCD_WriteData(' ');
			LCD_Cursor(13);
			LCD_WriteData(' ');
			LCD_Cursor(8);
			LCD_WriteData(second + '0');
			LCD_Cursor(7);
			LCD_WriteData(seconds + '0');
			LCD_Cursor(5);
			LCD_WriteData(minute + '0');
			LCD_Cursor(4);
			LCD_WriteData(minutes + '0');
			LCD_Cursor(2);
			LCD_WriteData(hour + '0');
			LCD_Cursor(1);
			LCD_WriteData(hours + '0');
			break;
			
		case SetTime:
			LCD_Cursor(i);
			if(i == 1) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 2)) {
					i++;
					LCD_WriteData(GetKeypadKey());
					hours = GetKeypadKey() - '0';
				}
			}
			else if(i == 2) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 3)) {
					i += 2;
					LCD_WriteData(GetKeypadKey());
					hour = GetKeypadKey() - '0';
				}
				else if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 9) && hours < 2) {
					i += 2;
					LCD_WriteData(GetKeypadKey());
					hour = GetKeypadKey() - '0';
				}
			}
			else if(i == 4) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 5)) {
					i++;
					LCD_WriteData(GetKeypadKey());
					minutes = GetKeypadKey() - '0';
				}
			}
			else if(i == 5) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 9)) {
					i++;
					LCD_WriteData(GetKeypadKey());
					minute = GetKeypadKey() - '0';
				}
			}
			break;
			
		case SetAlarm:
			LCD_Cursor(i);
			if(i == 1) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 2)) {
					i++;
					LCD_WriteData(GetKeypadKey());
					alarhours = GetKeypadKey() - '0';
				}
			}
			else if(i == 2) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 3)) {
					i += 2;
					LCD_WriteData(GetKeypadKey());
					alarhour = GetKeypadKey() - '0';
				}
				else if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 9) && hours < 2) {
					i += 2;
					LCD_WriteData(GetKeypadKey());
					alarhour = GetKeypadKey() - '0';
				}
			}
			else if(i == 4) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 5)) {
					i++;
					LCD_WriteData(GetKeypadKey());
					alarminutes = GetKeypadKey() - '0';
				}
			}
			else if(i == 5) {
				if((GetKeypadKey() - '0' >= 0) && (GetKeypadKey() - '0' <= 9)) {
					i++;
					LCD_WriteData(GetKeypadKey());
					alarminute = GetKeypadKey() - '0';
				}
			}
			break;
		
		case RingAlarm:
			PWM_on();
			set_PWM(261.63);
			if(i == 0) {
				LCD_Cursor(10);
				LCD_WriteData('W');
				LCD_Cursor(11);
				LCD_WriteData('A');
				LCD_Cursor(12);
				LCD_WriteData('K');
				LCD_Cursor(13);
				LCD_WriteData('E');
				++i;
			}
			else {
				LCD_Cursor(10);
				LCD_WriteData('U');
				LCD_Cursor(11);
				LCD_WriteData('P');
				LCD_Cursor(12);
				LCD_WriteData('!');
				LCD_Cursor(13);
				LCD_WriteData('!');
				i--;
			}
			second++;
			if(second == 10) {
				seconds++;
				second = 0;
			}
			if(seconds == 6) {
				minute++;
				seconds = 0;
			}
			if(minute == 10) {
				minutes++;
				minute = 0;
			}
			if(minutes == 6) {
				hour++;
				minutes = 0;
			}
			if(hour == 10) {
				hours++;
				hour = 0;
			}
			if(((hours * 10) + hour) == 24) {
				hours = 0;
				hour = 0;
			}
			break;
		case Snooze:
			alarminutes++;
			if(alarminutes == 6) {
				alarhour++;
				alarminutes = 0;
			}
			if(alarhour == 10) {
				alarhours++;
				alarhour = 0;
			}
			if(((alarhours * 10) + alarhour) == 24) {
				alarhours = 0;
				alarhour = 0;
			}
		break;
			
	}
}
int main(void)
{
	DDRA = 0xF0; PORTA = 0x0F;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	States = Init;
	LCD_init();
	TimerSet(1000);
	TimerOn();
	PWM_on();
	for(i = 1; i < 9; ++i) {
		if((i == 3) || (i == 6)) {
			LCD_Cursor(i);
			LCD_WriteData(':');
		}
		else {
			LCD_Cursor(i);
			LCD_WriteData('0');
		}
	}
	
    while (1) 
    {
		tick();
		while(!TimerFlag);
		TimerFlag = 0;
    }
}

