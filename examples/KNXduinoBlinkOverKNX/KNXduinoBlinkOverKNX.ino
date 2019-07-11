#include "Arduino.h"
#include "knxduino.h"

HardwareSerial Serial(USART1);  // use USART1 for KNXduino One board
int counter;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Start");

  attachKnxBusTimerUpdateIntHandle();
  bcu.begin(2, 1, 1); // ABB, dummy something device
  bcu.setOwnAddress(0xFFC0); // addr hard-wired, TODO: EEPROM not implemented yet

  // Disable telegram processing by the lib
  if (userRam.status & BCU_STATUS_TL)
      userRam.status ^= BCU_STATUS_TL | BCU_STATUS_PARITY;
}

void loop() {
  Serial.println(counter++);

  if (knxBus.telegramReceived()) {
        Serial.print("Telegram: ");
        for (int i = 0; i < knxBus.telegramLen; ++i) {
            if (i) Serial.print(" ");
            Serial.print(knxBus.telegram[i], HEX);
        }
        Serial.println();

        knxBus.discardReceivedTelegram();
  }

  // Blink on 0/0/1 KNX group address

  unsigned char sendTelBuffer[32];
  sendTelBuffer[0] = 188;
  sendTelBuffer[1] = 17;
  sendTelBuffer[2] = 10;
  sendTelBuffer[3] = 0;
  sendTelBuffer[4] = 1;
  sendTelBuffer[5] = 225;
  sendTelBuffer[6] = 0;
  sendTelBuffer[7] = 128;
  knxBus.sendTelegram(sendTelBuffer, 8);
  delay(1000);  // in real app, do not block loop in order to process all received telegrams

  sendTelBuffer[0] = 188;
  sendTelBuffer[1] = 17;
  sendTelBuffer[2] = 10;
  sendTelBuffer[3] = 0;
  sendTelBuffer[4] = 1;
  sendTelBuffer[5] = 225;
  sendTelBuffer[6] = 0;
  sendTelBuffer[7] = 129;
  knxBus.sendTelegram(sendTelBuffer, 8);
  delay(1000);  // in real app, do not block loop in order to process all received telegrams

}
