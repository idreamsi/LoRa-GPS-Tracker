/*******************************************************************************
 *
 * LoRa GPS Tracker by Wio Terminal
 * Live tracking on OpenStreetMap with GPS over LoRa
 * Please find more details at instructables:
 * https://www.hackster.io/idreams/
 * 
 ******************************************************************************/
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <RH_RF95.h>
#include "DHT.h"
#include "OneButton.h"

int BuzzerPin = 14;
#define BUTTON_INPUT A1
OneButton button(BUTTON_INPUT, true);

bool Send_GPS_LoRa = 1;
#define DHTPIN 16 
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

String DeviceName ="N1";
int DELAY_TIME = 15000; 
const long interval = 15000; 
unsigned long previousMillis = 0;

#ifdef __AVR__
    SoftwareSerial SSerial(8, 9); // RX, TX
    RH_RF95<SoftwareSerial> rf95(SSerial);
#endif

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

#define ss Serial1		//for gps module
//---------------------------------------------------------
void SendLoRaPacket(String dataString){  
  uint8_t data[dataString.length()];
  dataString.toCharArray(data, dataString.length());
  if (rf95.send(data, sizeof(data))){
    rf95.waitPacketSent();
    Serial.println("LoRa Packet Sent.");
  }
}
//---------------------------------------------------------
void playTone(int duration) {
  digitalWrite(BuzzerPin, HIGH);
  delay(duration);
  digitalWrite(BuzzerPin, LOW);
}
//---------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  pinMode(BuzzerPin, OUTPUT);
  ss.begin(GPSBaud);
  dht.begin();
  if (!rf95.init()) {
        Serial.println("init failed");
        while (1);
  }
  rf95.setFrequency(868.0);
  button.attachDoubleClick(doubleclick);
}
//---------------------------------------------------------
void loop()
{
  unsigned long currentMillis = millis();
  if (Send_GPS_LoRa){
    if (ss.available() > 0){
      if (gps.encode(ss.read())){   
        //while (ss.available() > 0)
          if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            displayInfo();
        }
  }}}
  button.tick();

 if (rf95.available()) {
    Serial.println("LoRa Available");
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      int StrLength = String((char*)buf).length();
      String incomingString = String((char*)buf);
      Serial.println(incomingString);
      String recTest = incomingString.substring(0,3);
      Serial.println(recTest);
      if(recTest == "SOS"){
        int addr_str = incomingString.indexOf(',');
        String RCV = incomingString.substring(0,addr_str);
        int addr_midd = incomingString.indexOf(',', addr_str + 1);
        String RCV_int = incomingString.substring(addr_str + 1, addr_midd);
        playTone(RCV_int.toInt());
      }
    }
   }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }

}
//---------------------------------------------------------
void displayInfo()
{
  Serial.println();
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    //---------------------------------
    int temp = dht.readTemperature();
    int humidity = dht.readHumidity();
    Serial.println();
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity:");
    Serial.println(humidity);
    //---------------------------------
    String NewData = (String(gps.location.lat(), 6)) + "," + (String(gps.location.lng(), 6)) + "," + String(temp) + "," + String(humidity) + "," + DeviceName + ".";
    Serial.println("DATA is: ");
    Serial.println(NewData);
    SendLoRaPacket(NewData);   
  }
}
//--------------------------------------------
void doubleclick()
{
    SendLoRaPacket("SOS,1000,1");
    playTone(10);
    Serial.println("Double Click");
}
