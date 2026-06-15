#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h" //Bibliotheek nodig voor de wifi.
#include <PubSubClient.h> //bibliotheek nodig voor MQTT.
// In de library manager moet PubSubClient geïnstalleerd zijn.

#define RST_PIN         21           // Configurable, see typical pin layout above
#define SS_PIN          23          // Configurable, see typical pin layout above
int pinsVanSegmenten[7] = {32, 14, 13, 12, 27, 15, 33};

//Declaratie van de methoden
void mqttConnect();
void callback(char* topic, byte* message, unsigned int length);
void initWiFi();
void initMQTTbroker();
void segments(int A, int B, int C, int D, int E, int F, int G);

//Vervang onderstaande wifi instellingen.
const char* ssid = "EscapeGame_Groep16";//SSID van het te gebruiken wifi-netwerk.
const char* paswoord = "EscapeGame_PASS";//Paswoord van het te gebruiken wifi-netwerk

WiFiClient WiFiClient1; //Een object wifiClient1 van de klasse wifiClient.
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

//Instellingen MQTT-broker
const char* mqttBroker = "10.42.0.1";// De te gebruiken MQTT-broker.
const char* mqttClientName = "RFID_SCANNER"; //Naam van de MQTT-client
const char* subNumber = "EscapeGame/Number"; //Naam van de topic waarop geabonneerd wordt (subscribe).
const char* pubScannedCode = "EscapeGame/ScannedCode"; //Naam van de topic waarop data verstuurd wordt (publish).
const char* mqttPwd = "EscapeGame";//Paswoord van de MQTT-broker.
const char* clientID = "EscapeGame_Scanner";//Client id usernamen+0001
PubSubClient mqttClient1(WiFiClient1);

void segments(int A, int B, int C, int D, int E, int F, int G) {
  digitalWrite(pinsVanSegmenten[0], A);
  digitalWrite(pinsVanSegmenten[1], B);
  digitalWrite(pinsVanSegmenten[2], C);
  digitalWrite(pinsVanSegmenten[3], D);
  digitalWrite(pinsVanSegmenten[4], E);
  digitalWrite(pinsVanSegmenten[5], F);
  digitalWrite(pinsVanSegmenten[6], G);
}

//****************** BEGIN mqttConnect methode ******************
void mqttConnect()
{
  while (!mqttClient1.connected())
  {
    Serial.print("Probeer te verbinden met MQTT-broker...");
    if (mqttClient1.connect(mqttClientName))
    {
      Serial.println("Verbonden");
      mqttClient1.publish("Hallo", mqttClientName);
      //Plaats hier alle topics waarvan je data wil ontvangen.
      mqttClient1.subscribe(subNumber);
      //...
    }
    else
    {
      Serial.print("Mislukt, rc=");
      Serial.print(mqttClient1.state());
      Serial.println(" Probeer opnieuw binnen 5 seconden");
      delay(5000);
    }
  }
}
//****************** EINDE mqttConnect methode ******************
//****************** BEGIN callback methode ******************
void callback(char* topic, byte* message, unsigned int length)
{
  String ontvangenMsg;
  for (int i=0; i<length; i++)
  {
    ontvangenMsg += (char)message[i];
  }
  Serial.print("Message ontvangen van topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(ontvangenMsg);
  //Controle welke topic er ontvangen is:
  if(String(topic) == subNumber)
  {
    if (ontvangenMsg == ("1"))
    {
      segments(0, 1, 1, 0, 0, 0, 0); //1
    }
    else if (ontvangenMsg == ("2"))
    {
      segments(1, 1, 0, 1, 1, 0, 1); //2
    }
    else if (ontvangenMsg == ("3"))
    {
      segments(1, 1, 1, 1, 0, 0, 1); //3
    }
    else if (ontvangenMsg == ("4"))
    {
      segments(0, 1, 1, 0, 0, 1, 1); //4
    }
    else if (ontvangenMsg == ("0")) { //uit
      segments(0,0,0,0,0,0,0);
    }
  //Controleer hier op andere ontvangen topics
  }
}
//****************** EINDE callback methode ******************

//****************** BEGIN initWiFi methode ******************
void initWiFi()
{
  WiFi.begin(ssid,paswoord);// Verbinding maken met het draadloze netwerk
  // met het meegegeven SSID en paswoord.
  Serial.println("Verbindt met WiFi ..");
  while (WiFi.status() != WL_CONNECTED)//Controle of er verbinding is.
  {
    Serial.print('.');//Zolang er geen verbinding is print een puntje.
    delay(500);// Een halve seconde wachten.
  }
  // Verbonden met wifi. De nodige info naar de serial monitor sturen.
  Serial.print("Verbonden met");
  Serial.println(ssid);
  Serial.print("Het gekregen IP-adres van het netwerk is: ");
  Serial.println(WiFi.localIP());//Print het gekregen IP-adres.
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());//Geef de signaalsterkte met het wifi netwerk.
  WiFi.setHostname("ESP32 Node Tom"); //Instellen van de hostname
  Serial.print("Het MAC-adres is: ");
  Serial.println(WiFi.macAddress());
}
//****************** EINDE initWiFi methode ******************
//****************** BEGIN initMQTTbroker methode ******************
void initMQTTbroker()
{
  mqttClient1.setServer(mqttBroker, 1883);
  //Als er een topic binnenkomt wordt de callback methode uitgevoerd.
  mqttClient1.setCallback(callback);
}
//****************** EINDE initMQTTbroker methode ******************

//*****************************************************************************************//
void setup() {
  Serial.begin(115200);                                           // Initialize serial communications with the PC
  initWiFi();//Instellen van de wifi.
  initMQTTbroker();//Instellen van de MQTT broker.
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                              // Init MFRC522 card
  Serial.println(F("Lezen van de kleurkaarten"));    //shows in serial that it is ready to read
  for (int i = 0; i < 7; i++) {
    pinMode(pinsVanSegmenten[i], OUTPUT);
  }
}

//*****************************************************************************************//
void loop() {
  if(!mqttClient1.connected())
  {
    mqttConnect();
  }
  //Dit moet reggelmatig worden aangeroepen om de client in staat te stellen
  // inkomende berichten te verwerken en de verbinding met de server te behouden.
  mqttClient1.loop();

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;

  //-------------------------------------------

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("**Card Detected:**"));

  //-------------------------------------------

  Serial.print(F("Kleur: "));

  block = 4;
  len = 18;



  //---------------------------------------- GET KLEUR

  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //PRINT + SEND KLEUR
  for (uint8_t i = 0; i < 6; i++) {
    Serial.write(buffer2[i] );
  }
  mqttClient1.publish(pubScannedCode, (char*)buffer2);


  //----------------------------------------

  Serial.println(F("\n**End Reading**\n"));

  delay(1000); //change value if you want to read cards faster

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
//*****************************************************************************************//
