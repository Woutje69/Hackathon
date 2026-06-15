#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const int IR_RECEIVE_PIN = 14; // GPIO14 = Pin D5
IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
  Serial.println("IR Ontvanger Test Gestart. Druk op je afstandsbediening...");
}

void loop() {
  if (irrecv.decode(&results)) {
    // Dit print de exacte code in hexadecimaal formaat naar de monitor
    Serial.print("Ontvangen code: 0x");
    Serial.println(results.value, HEX);
    
    irrecv.resume();
  }
  delay(100);
}