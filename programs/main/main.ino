//**************************************************************//
//  Name    : LCD/Stepper/74HC595, config                                
//  Author  : Lance Walker  
//  Date    : 07 Apr, 2018                                    
//  Version : 1.0.0v                                             
//  Notes   : Components: 
//          : Arduino 1602 Keypad Shield Module 16x2 (https://www.dfrobot.com/wiki/index.php/File:DFR0009-PIN2.png)
//          : Bipolar Stepper w/ULN2003A (https://www.diodes.com/assets/Datasheets/ULN200xA.pdf)
//          : 74HC595 Shift Register (http://www.ti.com/lit/ds/symlink/sn74hc595.pdf) 
//  Details : I/O: (18)
//          :   ATMega328: 10 
//          :   74HC595: 0               
//****************************************************************

// IMPORTS 
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Pin Definitions: 
  // Coil 1: 
#define phase00x1     7  // PD7                     
#define phase00x2     6  // PD6
  // Coil 2: 
#define phase11x1     5  // PD5                    
#define phase11x2     4  // PD4               

#define t int(5000*uint8_t(1))                   
#define stepsPerRevolution 400   

// device: (74HC595 IO expansion)
  // interface: 
#define Q_STCP      0  // storage: clk in (16Mhz), data latch 
#define Q_SHCP      1  // main: clk in (16Mhz), sck pin 
#define Q_DS        7  // serial data, data pin
  // 74HC595 Extensions 
  byte addr_serialIn; byte addr_serialOut; 
  byte addrList[10] = {
    0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 
    0xE0, 0xC0, 0x80, 0x00, 0xE0
  };
// device: (LiquidCry stal)
#define DBE     8  
#define DRS     9
#define DB4     10
#define DB5     11
#define DB6     12
#define DB7     13
  // init LCD:
  LiquidCrystal lcd(DBE, DRS, DB4, DB5, DB6, DB7);
  int lcd_key     = 0;  
  int adc_key_in  = 0; 
  // native keyboard (All report to initSerialOut()
  #define btnRIGHT      0
  #define btnUP         1
  #define btnDOWN       2
  #define btnLEFT       3
  #define btnSELECT     4
  #define btnNONE       5

// device IO: (other)
#define AN0     A0  

// EEPROM (1KB capacity)
byte localCache[] = {}; 
enum cacheManager {
  eepromWrite,
  eepromCrc, 
  eepromClear
};

bool invert = false; 
uint8_t phase[] = {phase00x1, phase00x2, phase11x1, phase11x2};  

byte indCoilx00[4][4] = {
  {1, 0, 0, 0}, // => output: 1111101000
  {1, 0, 1, 0}, // => output: 1111110010
  {0, 0, 1, 0}, // => output: 1010
  {0, 1, 1, 0}  // => output: 1101110
}; // inversion 

// Hardware Config: 
void setup() {
  Serial.begin(9600); // serialization      
  if (Serial) {
    Serial.print("init");
    // write EEPROM Cache 
    for (int i = 0; i < EEPROM.length(); i++) {
       localCache[i] = retrieveLocalAddr(i); 
    }
    Serial.print("EEPROM Cache Written: ");
  }

  // LCD config: 
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Init UI: button"); 
  
  // Motor config: 
  pinMode(phase00x1, OUTPUT);
  pinMode(phase00x2, OUTPUT);
  
  pinMode(phase11x1, OUTPUT);
  pinMode(phase11x2, OUTPUT);
}

void loop() {  
  // primary thread managers:
    // lcdManager();  
    motorControl(); 
}
// End: Hardware Config 

// Device Delegation:
void initSerialOut(int DSPin, int CLKPin, byte dataAddr) { // transmit serial data  
  int addrState; 
  // configure BUS outputs: 
  pinMode(DSPin, OUTPUT);
  pinMode(CLKPin, OUTPUT); 

  digitalWrite(DSPin, 0);
  digitalWrite(CLKPin, 0);

  int i=0;
  for (i=7; i>=0; i--) {
    digitalWrite(CLKPin, 0);

    if (dataAddr & (i<<1)) {
      addrState = 1; 
    } else {
      addrState = 0; 
    }
    
    digitalWrite(DSPin, addrState);
    digitalWrite(CLKPin, 1);
    digitalWrite(DSPin, 0);
  }

  // cleanup 
  digitalWrite(CLKPin, 0);
}

void initSerialIn() { // receive serial data 
  
}

void lcdManager() {
  lcd.setCursor(9,1);
  lcd.print(millis()/1000); 

  lcd.setCursor(0,1);
  lcdShouldListenForEvent();
}

int read_LCD_buttons() { // LCD: UI Configuration 
  adc_key_in = analogRead(0);

  if (adc_key_in > 1000) return btnNONE;

  // control threshold
  if (adc_key_in < 50) return btnRIGHT; 
  if (adc_key_in < 250) return btnUP;
  if (adc_key_in < 450) return btnDOWN;
  if (adc_key_in < 650) return btnLEFT;
  if (adc_key_in < 850) return btnSELECT; 

  // default:
  return btnNONE; 
}

void motorControl(bool isLeft) {
  for (int i = 0; i < 4; i++) { // index: row

    if (isLeft) {
     for (int j = 4; j > 0; i--) { // index: column
      digitalWrite(phase[i], indCoilx00[i][j]);  
     }
    } else {
      for (int j = 0; j < 4; i++) { // index: column
        digitalWrite(phase[i], indCoilx00[i][j]);  
      }
    }
  }
}

void eepromManager(cacheManager caller, byte data) {
  switch (caller) {
    case eepromWrite: {
      if (data) {
        EEPROM.write(sizeof(localCache), data);
      }
      break; 
    }
    
    case eepromCrc: {
      Serial.print("CRC32 of EEPROM data: 0x");
      Serial.println(sizeof(localCache), HEX);
      break; 
    }
    
    case eepromClear: {
      for (int i = 0; i < sizeof(localCache); i++) {
        EEPROM.write(i, 0);
      }
      Serial.print("EEPROM Cache OverWritten: ");
      Serial.println();
      break; 
    }
  }
}

byte retrieveLocalAddr(int addr) {
  byte Byte = EEPROM.read(addr);
    return Byte; 
}
// End: Device Delegation 

// Event Handling
void lcdShouldListenForEvent() {
  lcd_key = read_LCD_buttons();
  switch (lcd_key) {
    case btnRIGHT: {
      lcd.print("RIGHT");
      break; 
    }

    case btnLEFT: {
      lcd.print("LEFT"); 
      break; 
    }

    case btnUP: {
      lcd.print("UP");
      break; 
    }

    case btnDOWN: {
      lcd.print("DOWN");
      break; 
    }

    case btnSELECT: {
      lcd.print("SELECT"); 
      break; 
    }

    case btnNONE: {
      lcd.print("NONE");
      break; 
    }
  }
}
// End: Event Handling 

