#include "Arduino.h"
#include "knxduino.h"

// Select Board: Nucleo-64, Board part number: Nucleo G071RB, U(S)ART support: Enable (no generic 'Serial')

HardwareSerial Serial(USART1);  // use USART1 for KNXduino One board (PA9, PA10 pins)
//HardwareSerial Serial(USART2);  // use USART2 for Nucleo 64 G071RB board (PA2, PA3 pins)
int counter;

void changeListener(bool newValue) {
  Serial.print("new value from bus or app: ");
  Serial.println(newValue);
}

BinaryValue binaryGroup001(0, 0, 1, &changeListener); // binary group 0/0/1 defined

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start");

  attachKnxBusTimerUpdateIntHandle();
  bcu.addComObject(&binaryGroup001);
  // Change to KNXDUINO_NUCLEO_PINMAPPING when using Nucleo board instead of KNXduino One down below
  bcu.begin(2, 1, 1, KNXDUINO_ONE_PINMAPPING); // ABB, dummy something device
  bcu.setOwnAddress(15, 15, 208); // physical addr 15.15.208 hard-wired, TODO: EEPROM not implemented yet
}

void loop() {
  Serial.println(counter++);
  delay(5000);
  binaryGroup001.setValue(!binaryGroup001.getValue()); // toggle binary group 0/0/1
  bcu.loop();
}
