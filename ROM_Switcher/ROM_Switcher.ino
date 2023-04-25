/*
  First test for ROM switch
  8 MHz internal, Pin Mapping: Clockwise, LTO: disable, tinyNeoPixel poer: Port B millis(): enable
*/

#include <EEPROM.h>

#define button 11;
#define A16 0;	// B0
#define A17 1;	// B1
#define A18 2;	// B2
#define A19 3;	// A7

#define ADDR 0

// Character for seven segment display
byte display[] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111, // 9
  0b01110111, // A
  0b01111100, // b
  0b01011000, // c
  0b01011110, // d
  0b01111001, // E
  0b01110001, // F
  0b01011100, // o
  0b01001000, // =
  0b01001001, // =-
  0b00000000  // blank
};

byte tmp;
byte romIdx;               // ROM Index (selected ROM)
long buttonTime;           // Keystroke duration
long lastAction;           // Start button pressed
byte lastButtonState = 0;  // Status of the last keystroke

// Key query
// return value
// 1 = Key pressed shorter 50 ms
// 2 = Key was pressed
// 3 = Key was pressed for a long time (>2 seconds)
// 4 = Key was pressed for a very long time (>5 seconds)
byte getButton() {
  if ((PINB & (1 << 3)) == 0) { // Key pressed
    if (lastButtonState == 0) { // not pressed before
      lastButtonState = 1;
      lastAction = millis();
    } else { // Key is still pressed
      if ((millis() - lastAction) > 5000) lastButtonState = 5;
    }
  } else {                     // Key released
    if (lastButtonState > 0) { // Key was pressed
      lastButtonState = 0;
      buttonTime = millis() - lastAction;
      if (buttonTime > 5000) {
        return 4;
      } else if (buttonTime > 2000) {
        return 3;
      } else if (buttonTime > 50) {
        return 2;
      } else {
        return 1;
      }
    }
  }
  return lastButtonState;
}

// print char
void setDisplay(byte id) {
  if (id < 20) PORTA = (PORTA & 0x80) | display[id];
}

// Select ROM bank, activate
void setAddress(byte id) {
  PORTB = (PORTB & 0b11111000) | (id & 0b00000111);
  if ((id & 0b00001000) == 0) {
    PORTA = PORTA & 0b01111111;
  } else {
    PORTA = (PORTA & 0b01111111) | 0b10000000;
  }
}

void setup() {
  DDRA = 0xFF;
  DDRB = 0x07; 	               // 0b00000111;
  romIdx = EEPROM.read(ADDR);  // read default index from eeprom
  romIdx &= 0x0F;
}

void loop() {
  setDisplay(romIdx & 0x0F);   // show selected bank
  setAddress(romIdx);          // activate selected bank
  tmp = getButton();
  if (tmp == 2) {              // key pressed, switch to the next bank
    romIdx++;
    romIdx &= 0x0F;
  } else if (tmp == 3) {       // key pressed for a long time, reset bank to 0
    romIdx = 0;
  } else if (tmp == 4) {       // key pressed very long, set current bank as default
    EEPROM.update(ADDR, romIdx);
    setDisplay(18);            // signal put the bank on display
    delay(500);
    setDisplay(19);
    delay(500);
  }
  delay(10);
}
