/*
  First test for ROM switch
  8 MHz internal, Pin Mapping: Clockwise, LTO: disable, tinyNeoPixel poer: Port B millis(): enable
*/

#include <EEPROM.h>

//#define debug

#define button 11;
#define A16 0;	// B0
#define A17 1;	// B1
#define A18 2;	// B2
#define A19 3;	// A7

#define ADDR 0

#define IF_KEY_PRESSED (PINB & (1 << 3)) == 0
#define ELAPSED_TIME (millis() - lastAction)
#define NOW millis()

#define TIME_DEBOUNCE 50
#define TIME_LONG_PRESSED 1000
#define TIME_LONG_RELEASED 2000
#define TIME_VERY_LONG_PRESSED 3000

#define STATE_LONG_RELEASED 1
#define STATE_NO_KEY_PRESSED 2
#define STATE_PRESSED 3
#define STATE_LONG_PRESSED 4
#define STATE_VERY_LONG_PRESSED 5
#define STATE_NOW_PRESSED 6

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

#define BLANK 19

byte tmp;
byte romIdx;                                  // ROM Index (selected ROM)
byte romLastIdx = 17;
byte eepromAddr;                              // ROM Index (selected ROM)
long buttonPressedTime = 0;                   // Keystroke duration
long lastAction = NOW;                        // Start button pressed
byte lastButtonState = STATE_NO_KEY_PRESSED;  // Status of the last keystroke
byte saveEeprom = 1;

// Key query
byte getButton() {
  if (ELAPSED_TIME > TIME_DEBOUNCE ) {

    if (IF_KEY_PRESSED) {                                                                           // Key pressed
      //if ((lastButtonState == STATE_NO_KEY_PRESSED) || (lastButtonState == TIME_LONG_RELEASED)) { // not pressed before
      if (lastButtonState < STATE_PRESSED) {                                                        // not pressed before
        lastButtonState = STATE_NOW_PRESSED;
        lastAction = NOW;
      } else {                                                                                      // Key is still pressed
        if (ELAPSED_TIME > TIME_VERY_LONG_PRESSED) lastButtonState = STATE_VERY_LONG_PRESSED;
      }
    } else {                                                                                        // Key not pressed
      //if ((lastButtonState != STATE_NO_KEY_PRESSED) && (lastButtonState != TIME_LONG_RELEASED)) { // Key was pressed [Taste war gedrückt, wurde eben losgelassen]
      if (lastButtonState >= STATE_PRESSED) {                                                       // Key was pressed
        lastButtonState = STATE_NO_KEY_PRESSED;
        buttonPressedTime = ELAPSED_TIME;
        lastAction = NOW;
        if (buttonPressedTime > TIME_VERY_LONG_PRESSED) {
          return STATE_VERY_LONG_PRESSED;
        } else if (buttonPressedTime > TIME_LONG_PRESSED) {
          return STATE_LONG_PRESSED;
        } else {
          return STATE_PRESSED;
        }
      } else {                                                                                       // Key is not pressed and was not pressed
        if (ELAPSED_TIME > TIME_LONG_RELEASED) lastButtonState = STATE_LONG_RELEASED;
      }
    }
    return lastButtonState;

  } else {                                                                                           // in debounce time
    return STATE_NO_KEY_PRESSED;
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
  if (eepromAddr < 2 || eepromAddr > 126) { // Address must be even and between 2..126
    eepromAddr = 2;
    EEPROM.update(ADDR, eepromAddr);
  }
  tmp = EEPROM.read(eepromAddr + 1);        // Increase write counter for cell
  tmp--;
  EEPROM.write(eepromAddr + 1, tmp);
  if (tmp == 0) {                           // switch to the next cell on overflow
    eepromAddr += 2;
    if (eepromAddr > 126) eepromAddr = 2;
    EEPROM.update(ADDR, eepromAddr);        // Set pointer to current cell
  }
  //eepromAddr = 0;
  romIdx &= 0x0F;
  setDisplay(BLANK);
  delay(250);
#ifdef debug
  setDisplay(eepromAddr);
  delay(250);
#endif
  EEPROM.update(eepromAddr, romIdx);        // Save ROM index
  setDisplay(BLANK);
  delay(100);
  setDisplay(eepromAddr);
  delay(250);
  setDisplay(BLANK);
  delay(150);
}

void setup() {
  DDRA = 0xFF;
  DDRB = 0x07; 	                            // 0b00000111;
  eepromAddr = EEPROM.read(ADDR);
  if (eepromAddr > 0b01111110) {
    setAddress(0);
    eepromAddr = 2;
    EEPROM.write(ADDR, 2);

  } else {
    eepromAddr &= 0b01111110;
    romIdx = EEPROM.read(eepromAddr);       // read default index from eeprom
    romIdx &= 0x0F;
    setAddress(romIdx);
#ifdef debug
    setDisplay(BLANK);
    delay(250);
#endif
    setDisplay(eepromAddr >> 4);
    delay(250);
    setDisplay(eepromAddr & 0x0F);
    delay(250);
#ifdef debug
    setDisplay(BLANK);
    delay(250);
    setDisplay(romIdx);
    delay(250);
    setDisplay(BLANK);
    delay(250);
#endif
  }
}

void loop() {
  if (romIdx != romLastIdx) {
    setAddress(romIdx);          // activate selected bank
    setDisplay(romIdx & 0x0F);   // show selected bank
    romLastIdx = romIdx;
  }

  tmp = getButton();
  //setDisplay(tmp);
  if (tmp == STATE_LONG_RELEASED) {
    if (saveEeprom == 0) {
      saveEeprom = 1;
      writeEEPROM();
      setDisplay(romIdx & 0x0F);
    }
  } else if (tmp == STATE_PRESSED) {             // key pressed, switch to the next bank
    romIdx++;
    romIdx &= 0x0F;
    saveEeprom = 0;
  } else if (tmp == STATE_LONG_PRESSED) {        // key pressed for a long time, set bank to 0 (my ROM vX0)
    romIdx = 0;
    saveEeprom = 0;
  } else if (tmp == STATE_VERY_LONG_PRESSED) {   // key pressed very long, set bank to 8 (my ROM 128k7)
    romIdx = 8;
    saveEeprom = 0;
  }
}
