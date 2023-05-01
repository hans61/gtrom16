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
byte romLastIdx = 17;
byte eepromAddr;           // ROM Index (selected ROM)
long buttonTime;           // Keystroke duration
long lastAction;           // Start button pressed
byte lastButtonState = 0;  // Status of the last keystroke
byte saveEeprom = 1;

// Key query
// return value
// 2 = Key pressed shorter 50 ms
// 3 = Key was pressed
// 4 = Key was pressed for a long time (>1 seconds)
// 5 = Key was pressed for a very long time (>3 seconds)
// 6 = Key released for a long time (>2 seconds)
byte getButton() {
  if ((millis() - lastAction) > 50 ) {
    if ((PINB & (1 << 3)) == 0) { // Key pressed
      if (lastButtonState == 0) { // not pressed before
        lastButtonState = 1;
        lastAction = millis();
      } else { // Key is still pressed
        if ((millis() - lastAction) > 5000) lastButtonState = 5;
      }
    } else {                     // Key released
      if (lastButtonState > 0) { // Key was pressed [Taste war gedrückt, wurde eben losgelassen]
        lastButtonState = 0;
        buttonTime = millis() - lastAction;
        lastAction = millis();
        if (buttonTime > 3000) {
          return 5;
        } else if (buttonTime > 1000) {
          return 4;
        } else if (buttonTime > 50) {
          return 3;
        } else {
          //lastButtonState = 1;
          return 2;
        }
      } else {                 // Key is not pressed and was not pressed
        if ((millis() - lastAction) > 2000) lastButtonState = 6;
        //        buttonTime = millis() - lastAction;
        //        if (buttonTime > 3000) {
        //          return 1;
        //        }
      }
    }
    return lastButtonState;
  } else {                  // Debounce time
    return 0;
  }
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

void writeEEPROM() {
  if (eepromAddr < 2 || eepromAddr > 128) { // Address must be even and between 2..128
    eepromAddr = 2;
    EEPROM.update(ADDR, eepromAddr);
  }
  tmp = EEPROM.read(eepromAddr + 1);        // Increase write counter for cell
  tmp--;
  EEPROM.update(eepromAddr + 1, tmp);
  if (tmp == 0) {                           // switch to the next cell on overflow
    eepromAddr += 2;
    if (eepromAddr > 128) eepromAddr = 2;
    EEPROM.update(ADDR, eepromAddr);        // Set pointer to current cell
  }
  EEPROM.update(eepromAddr, romIdx);        // Save ROM index
}

void setup() {
  DDRA = 0xFF;
  DDRB = 0x07; 	                            // 0b00000111;
  eepromAddr = EEPROM.read(ADDR) & 0b01111110;
  romIdx = EEPROM.read(eepromAddr);         // read default index from eeprom
  romIdx &= 0x0F;
  setAddress(romIdx);
}

void loop() {
  if (romIdx != romLastIdx) {
    setAddress(romIdx);          // activate selected bank
    setDisplay(romIdx & 0x0F);   // show selected bank
    romLastIdx = romIdx;
  }

  tmp = getButton();
  if (tmp == 6) {
    if (saveEeprom == 0) {
      saveEeprom = 1;
      writeEEPROM();
      setDisplay(18);            // signal put the bank on display
      delay(500);
      setDisplay(19);
      delay(500);
      setDisplay(romIdx & 0x0F);
    }
  } else if (tmp == 3) {        // key pressed, switch to the next bank
    romIdx++;
    romIdx &= 0x0F;
    saveEeprom = 0;
    //writeEEPROM();
  } else if (tmp == 4) {        // key pressed for a long time, set current bank as default
    //romIdx = 0;
  } else if (tmp == 5) {        // key pressed very long, reset bank to 0
    //romIdx = 0;
  }
  //delay(10);
}
