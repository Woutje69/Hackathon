#include "WiFi.h"          //Bibliotheek nodig voor de wifi.
#include <PubSubClient.h>  //bibliotheek nodig voor MQTT.
// In de library manager moet PubSubClient geïnstalleerd zijn.
#define knopROOD 23
#define knopBLAUW 22
#define knopGROEN 15
#define knopGEEL 4

#define ledROOD 21
#define ledBLAUW 19
#define ledGROEN 18
#define ledGEEL 5

int knopArr[4] = {knopROOD, knopBLAUW, knopGROEN, knopGEEL};
int ledArr[4] = {ledROOD, ledBLAUW, ledGROEN, ledGEEL};
String kleurArr[4] = {"ROOD", "BLAUW", "GROEN", "GEEL"};
String strVolgorde = "";
int teller = 0;
//Declaratie van de methoden
void mqttConnect();
void initWiFi();
void initMQTTbroker();
//Vervang onderstaande wifi instellingen.
const char* ssid = "EscapeGame_Groep16";//SSID van het te gebruiken wifi-netwerk.
const char* paswoord = "EscapeGame_PASS";//Paswoord van het te gebruiken wifi-netwerk
WiFiClient WiFiClient1; //Een object wifiClient1 van de klasse wifiClient.

//Instellingen MQTT-broker
const char* mqttBroker = "10.42.0.1";// De te gebruiken MQTT-broker.
const char* mqttClientName = "Knoppen"; //Naam van de MQTT-client
const char* pubVolgorde = "EscapeGame/Volgorde"; //Naam van de topic waarop data verstuurd wordt (publish).
const char* mqttPwd = "EscapeGame";//Paswoord van de MQTT-broker.
const char* clientID = "EscapeGame_Knoppen";//Client id usernamen+0001
PubSubClient mqttClient1(WiFiClient1);
//****************** BEGIN mqttConnect methode ******************
void mqttConnect() {
  while (!mqttClient1.connected()) {
    Serial.print("Probeer te verbinden met MQTT-broker...");
    if (mqttClient1.connect(mqttClientName)) {
      Serial.println("Verbonden");
      digitalWrite(ledGROEN, HIGH);
      delay(1000);
      digitalWrite(ledGROEN, LOW);
      mqttClient1.publish("Hallo", mqttClientName);
      
    } else {
      Serial.print("Mislukt, rc=");
      Serial.print(mqttClient1.state());
      Serial.println(" Probeer opnieuw binnen 5 seconden");
      delay(5000);
    }
  }
}
//****************** EINDE mqttConnect methode ******************

//****************** BEGIN initWiFi methode ******************
void initWiFi() {
  WiFi.begin(ssid, paswoord);  // Verbinding maken met het draadloze netwerk
  // met het meegegeven SSID en paswoord.
  Serial.println("Verbindt met WiFi ..");
  while (WiFi.status() != WL_CONNECTED)  //Controle of er verbinding is.
  {
    Serial.print('.');  //Zolang er geen verbinding is print een puntje.
    delay(500);         // Een halve seconde wachten.
  }
  // Verbonden met wifi. De nodige info naar de serial monitor sturen.
  Serial.print("Verbonden met");
  Serial.println(ssid);
  Serial.print("Het gekregen IP-adres van het netwerk is: ");
  Serial.println(WiFi.localIP());  //Print het gekregen IP-adres.
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());         //Geef de signaalsterkte met het wifi netwerk.
  WiFi.setHostname("ESP32 Node Wout");  //Instellen van de hostname
  Serial.print("Het MAC-adres is: ");
  Serial.println(WiFi.macAddress());
}
//****************** EINDE initWiFi methode ******************
//****************** BEGIN initMQTTbroker methode ******************
void initMQTTbroker() {
  mqttClient1.setServer(mqttBroker, 1883);
}
//****************** EINDE initMQTTbroker methode ******************
//****************** BEGIN setup methode ******************
void setup() {
  Serial.begin(115200);  //Instellen serieële communicatie.
  initWiFi();            //Instellen van de wifi.
  initMQTTbroker();      //Instellen van de MQTT broker.
  pinMode(knopBLAUW, INPUT_PULLUP);
  pinMode(knopROOD, INPUT_PULLUP);
  pinMode(knopGROEN, INPUT_PULLUP);
  pinMode(knopGEEL, INPUT_PULLUP);
  pinMode(ledROOD, OUTPUT);
  pinMode(ledBLAUW, OUTPUT);
  pinMode(ledGROEN, OUTPUT);
  pinMode(ledGEEL, OUTPUT);
}
//****************** EINDE setup methode ******************
//****************** BEGIN loop methode ******************
void loop() {
  //Controle of de ESP32 nog verbonden is met de MQTT-broker.
  if (!mqttClient1.connected()) {
    mqttConnect();
  }
  //Dit moet reggelmatig worden aangeroepen om de client in staat te stellen
  // inkomende berichten te verwerken en de verbinding met de server te behouden.
  mqttClient1.loop();

    for (int i = 0; i < 4; i++) {
    if(digitalRead(knopArr[i]) == LOW) {
      if(digitalRead(ledArr[i]) == LOW) {
        digitalWrite(ledArr[i], HIGH);
        strVolgorde += kleurArr[i] + " ";
        teller++;
        Serial.println(strVolgorde);
      }
      delay(50);
    }
  }

  if(teller == 4) {
    mqttClient1.publish(pubVolgorde, strVolgorde.c_str());
    teller = 0;
    strVolgorde = "";
    digitalWrite(ledROOD, LOW);
    digitalWrite(ledBLAUW, LOW);
    digitalWrite(ledGROEN, LOW);
    digitalWrite(ledGEEL, LOW);
  }
  else if (teller > 4) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(ledArr[i], LOW);
    }
  }
  delay(200);
}
//****************** EINDE loop methode ******************
