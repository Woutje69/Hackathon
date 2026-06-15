#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>  // Gecorrigeerde bibliotheek voor ESP8266 WiFi
#include <PubSubClient.h> // Bibliotheek nodig voor MQTT

// Declaratie van de methoden
void mqttConnect();
void callback(char* topic, byte* message, unsigned int length);
void initWiFi();
void initMQTTbroker();

const char* ssid = "EscapeGame_Groep16"; // SSID van het te gebruiken wifi-netwerk.
const char* paswoord = "EscapeGame_PASS"; // Paswoord van het te gebruiken wifi-netwerk
WiFiClient WiFiClient1; // Een object wifiClient1 van de klasse wifiClient.

// Instellingen MQTT-broker
const char* mqttBroker = "10.42.0.1"; 
const char* mqttClientName = "Afstandsbediening";
const char* SubCorrect = "EscapeGame/Correct"; // Naam van de topic waarop geabonneerd wordt (subscribe).
const char* PubCode = "EscapeGame/Code";       // Naam van de topic waarop data verstuurd wordt (publish).
const char* mqttPwd = "EscapeGame";           // Paswoord van de MQTT-broker.
const char* clientID = "EscapeGame_Afstandsbediening"; // Client id
PubSubClient mqttClient1(WiFiClient1);

// --- OLED Setup (correcte pinnen volgens pinout diagram) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    16  // GPIO16 = OLED_RST
#define OLED_SDA       4  // GPIO4  = OLED_SDA
#define OLED_SCL       5  // GPIO5  = OLED_SCL
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pin Setup ---
const int IR_RECEIVE_PIN = 14; // GPIO14 = Pin D5 op NodeMCU — vrij te gebruiken
const int BUZZER_PIN     = 12; // GPIO12 = Pin D6 op NodeMCU — vrij te gebruiken

// --- IR Codes ---
const unsigned long IR_CODES[] = {
  0xAD52FF00, // 0
  0xFF6897, // 1
  0xFF9867, // 2
  0xFFB04F, // 3
  0xFF30CF, // 4
  0xFF18E7, // 5
  0xFF7A85, // 6
  0xFF10EF, // 7
  0xFF38C7, // 8
  0xFF5AA5  // 9
};
IRrecv irrecv(IR_RECEIVE_PIN);
decode_results results;

// --- Puzzle ---
int enteredDigits[4];
int enteredCount = 0;
bool waitingForServer = false; // Houdt bij of we wachten op MQTT antwoord

// --- Buzzer (non-blocking, ESP8266) ---
struct BuzzerNote {
  int freq;
  int duration;
};

BuzzerNote correctNotes[] = {{1047,150},{1319,150},{1568,150},{2093,300}};
BuzzerNote wrongNotes[]   = {{300,200},{200,400}};

BuzzerNote* currentMelody = nullptr;
int melodyLength    = 0;
int melodyIndex     = 0;
unsigned long noteStartTime = 0;

//****************** BEGIN mqttConnect methode ******************
void mqttConnect() {
  while (!mqttClient1.connected()) {
    Serial.print("Probeer te verbinden met MQTT-broker...");
    if (mqttClient1.connect(mqttClientName)) { 
      Serial.println("Verbonden");
      mqttClient1.publish("Hallo", mqttClientName);
      mqttClient1.subscribe(SubCorrect); 
    } else {
      Serial.print("Mislukt, rc=");
      Serial.print(mqttClient1.state());
      Serial.println(" Probeer opnieuw binnen 5 seconden");
      delay(5000);
    }
  }
}

//****************** BEGIN callback methode ******************
void callback(char* topic, byte* message, unsigned int length) {
  String ontvangenMsg;
  for (int i = 0; i < length; i++) {
    ontvangenMsg += (char)message[i];
  }
  Serial.print("Message ontvangen van topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(ontvangenMsg);

  if (String(topic) == SubCorrect) {
    waitingForServer = false; // Antwoord ontvangen, we blokkeren de IR-ontvanger niet meer
    
    if (ontvangenMsg == "1") { 
      // Server zegt: CODE IS JUIST!
      Serial.println("SUCCES: Server keurt code goed!");
      startMelody(correctNotes, 4);
      
      // Wacht 3 seconden zodat spelers de code zien staan, maak daarna het scherm leeg
      delay(3000); 
      enteredCount = 0; 
    } else if (ontvangenMsg == "0") { 
      // Server zegt: CODE IS ONJUIST!
      Serial.println("FOUT: Server keurt code af.");
      startMelody(wrongNotes, 2);
      
      // Direct het scherm wissen voor de volgende poging
      enteredCount = 0; 
    }
  }
}

//****************** BEGIN initWiFi methode ******************
void initWiFi() {
  WiFi.begin(ssid, paswoord);
  Serial.println("Verbindt met WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Verbonden met ");
  Serial.println(ssid);
  Serial.print("Het gekregen IP-adres van het netwerk is: ");
  Serial.println(WiFi.localIP());
}

//****************** BEGIN initMQTTbroker methode ******************
void initMQTTbroker() {
  mqttClient1.setServer(mqttBroker, 1883);
  mqttClient1.setCallback(callback);
}

void playTone(int freq) {
  if (freq == 0) {
    analogWrite(BUZZER_PIN, 0);
  } else {
    analogWriteFreq(freq);
    analogWrite(BUZZER_PIN, 128);
  }
}

void stopTone() {
  analogWrite(BUZZER_PIN, 0);
}

void startMelody(BuzzerNote* melody, int length) {
  currentMelody = melody;
  melodyLength  = length;
  melodyIndex   = 0;
  noteStartTime = millis();
  playTone(melody[0].freq);
}

void updateBuzzer() {
  if (currentMelody == nullptr) return;
  if (millis() - noteStartTime >= (unsigned long)currentMelody[melodyIndex].duration) {
    melodyIndex++;
    if (melodyIndex >= melodyLength) {
      stopTone();
      currentMelody = nullptr;
      return;
    }
    noteStartTime = millis();
    playTone(currentMelody[melodyIndex].freq);
  }
}

int irCodeToDigit(unsigned long code) {
  for (int i = 0; i < 10; i++) {
    if (IR_CODES[i] == code) return i;
  }
  return -1;
}

// --- Display update ---
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Toon cijfers als er getypt is óf als we nog op de server wachten
  if (enteredCount > 0 || waitingForServer) {
    display.setTextSize(3);
    display.setCursor(28, 4);
    for (int i = 0; i < 4; i++) {
      if (i < enteredCount) {
        display.print(enteredDigits[i]);
      } else {
        display.print('_');
      }
    }
  } else {
    // Scherm blijft volledig leeg en zwart zolang er geen actieve invoer is
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initMQTTbroker();
  Wire.begin(OLED_SDA, OLED_SCL);

  pinMode(OLED_RESET, OUTPUT);
  digitalWrite(OLED_RESET, LOW);
  delay(20);
  digitalWrite(OLED_RESET, HIGH);
  delay(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 niet gevonden!");
    while (true) { yield(); }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 12);
  display.print("Escape Room!");
  display.display();
  delay(1500);

  irrecv.enableIRIn();

  pinMode(BUZZER_PIN, OUTPUT);
  analogWriteRange(255);
}

void loop() {
  if (!mqttClient1.connected()) {
    mqttConnect();
  }
  mqttClient1.loop();

  updateBuzzer();

  // Accepteer toetsen als we niet op de server wachten
  if (!waitingForServer && irrecv.decode(&results)) {
    unsigned long code = results.value;

    if (code != 0 && code != 0xFFFFFFFF) {
      int digit = irCodeToDigit(code);
      
      if (digit != -1) {
        if (enteredCount < 4) {
          enteredDigits[enteredCount++] = digit;
          
          Serial.print("Cijfer geregistreerd: "); Serial.println(digit);
          Serial.print("Huidige reeks: ");
          for (int i = 0; i < enteredCount; i++) Serial.print(enteredDigits[i]);
          Serial.println();
          
          updateDisplay();

          // VERZENDEN VIA MQTT ZODRA ER 4 CIJFERS ZIJN
          if (enteredCount == 4) {
            Serial.println("4 Cijfers bereikt! Versturen naar MQTT...");
            waitingForServer = true; // Blokkeer nieuwe IR-invoer tot antwoord binnen is
            delay(300); 
            
            // Jouw indeling inclusief spaties: "1 2 3 4"
            String payload = String(enteredDigits[0]) + " " +
                             String(enteredDigits[1]) + " " +
                             String(enteredDigits[2]) + " " +
                             String(enteredDigits[3]);
            
            mqttClient1.publish(PubCode, payload.c_str());
            Serial.print("Gepubliceerd op topic "); Serial.print(PubCode);
            Serial.print(" met waarde: "); Serial.println(payload);
          }
        }
      } else {
        Serial.print("Onbekende IR code: 0x");
        Serial.println(code, HEX);
      }
    }
    irrecv.resume();
  }

  updateDisplay();
  yield();
}