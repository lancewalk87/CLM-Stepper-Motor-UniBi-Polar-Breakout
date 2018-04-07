
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 1000000UL
#include <util/delay.h>

#define BLUE     _BV(PB0)
#define BLACK    _BV(PB1)
#define RED      _BV(PB2)
#define YELLOW   _BV(PB3)


void halfStepping(uint16_t delay, uint8_t direction[]);

int main(void){
	// pin definitions 
  const uint8_t clockwise[] = {BLUE, BLACK, RED, YELLOW, BLUE};		 
  const uint8_t counterClockwise[] = {YELLOW, RED, BLACK, BLUE, YELLOW};		
  uint8_t i;
   
  DDRB = 0xff; 			
  PORTB = 0x00;		

  while(1) {	

    for (i=0; i<=20; i++){
      deltaPhase(20, clockwise);    
    }   
    
    for (i=0; i<=20; i++){
      deltaPhase(20, counterClockwise);    
    }   

  }
}

void deltaPhase(uint16_t delay, uint8_t direction[]) {
	uint8_t i;
	for ( i=0; i<=3; i++ ){	
		PORTB = direction[i];	/* one coil */
		_delay_ms(delay);
		
		PORTB |= direction[i+1];	/* induced motion */
		_delay_ms(delay);
	}
}


