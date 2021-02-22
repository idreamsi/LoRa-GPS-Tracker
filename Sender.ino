/*******************************************************************************
 *
 * LoRa GPS Tracker with Wio Terminal
 * Live tracking on OpenStreetMap with GPS over LoRa
 * Please find more details at Hackster.io:
 * https://www.hackster.io/idreams/lora-gps-tracker-by-wio-terminal-5d8647
 * 
 ******************************************************************************/
#include <TinyGPS++.h>
#include <RH_RF95.h>
#include "DHT.h"
#include "OneButton.h"
#include "wiring_private.h"		// for pinPeripheral() function

#define USE_XIAO

#ifdef USE_XIAO
    #define BUTTON_INPUT 9
    #define DHTPIN 1
    int BuzzerPin = 0;
#else //Arduino Pro Micro
    #define BUTTON_INPUT A1
    #define DHTPIN 16
    int BuzzerPin = 14;
#endif

OneButton button(BUTTON_INPUT, true);
bool Send_GPS_LoRa = 1;
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);

String DeviceName ="N1";
const long interval = 15000; 
unsigned long previousMillis = 0;

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    static Uart SSerial(&sercom2, PIN_WIRE_SCL, PIN_WIRE_SDA, SERCOM_RX_PAD_1, UART_TX_PAD_0);
    RH_RF95 <Uart> rf95(SSerial);
#else
    #include <SoftwareSerialUSB.h>
    SoftwareSerial SSerial(8, 9);// RX, TX
    RH_RF95<SoftwareSerial> rf95(SSerial);
#endif

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;

#define GPSserial Serial1
//---------------------------------------------------------
void SendLoRaPacket(String dataString){
  if (rf95.send((uint8_t *)dataString.c_str(), dataString.length())){
    rf95.waitPacketSent();
    SerialUSB.println("LoRa Packet Sent.");
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
  SerialUSB.begin(115200);
  pinMode(BuzzerPin, OUTPUT);
  GPSserial.begin(GPSBaud);
  dht.begin();
  
#ifdef USE_XIAO  
  pinPeripheral(PIN_WIRE_SCL, PIO_SERCOM_ALT);
  pinPeripheral(PIN_WIRE_SDA, PIO_SERCOM_ALT);
#endif  
  if (!rf95.init()) {
	SerialUSB.println("LoRa init failed");
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
    if (GPSserial.available() > 0){
      if (gps.encode(GPSserial.read())){   
          if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            displayInfo();
        }
  }}}
  button.tick();

 if (rf95.available()) {
    SerialUSB.println("LoRa Available");
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      int StrLength = String((char*)buf).length();
      String incomingString = String((char*)buf);
      SerialUSB.println(incomingString);
      String recTest = incomingString.substring(0,3);
      SerialUSB.println(recTest);
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
    SerialUSB.println(F("No GPS detected: check wiring."));
    while(true);
  }
}
//---------------------------------------------------------
void displayInfo()
{
  SerialUSB.println();
  SerialUSB.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    SerialUSB.print(gps.location.lat(), 6);
    SerialUSB.print(F(","));
    SerialUSB.print(gps.location.lng(), 6);

    int temp = dht.readTemperature();
    int humidity = dht.readHumidity();

    SerialUSB.println();
    SerialUSB.print("Temperature: ");
    SerialUSB.println(temp);
    SerialUSB.print("Humidity:");
    SerialUSB.println(humidity);
    //---------------------------------
    String NewData = (String(gps.location.lat(), 6)) + "," + (String(gps.location.lng(), 6)) + "," + String(temp) + "," + String(humidity) + "," + DeviceName + ".";
    SerialUSB.println("DATA is: ");
    SerialUSB.println(NewData);
    SendLoRaPacket(NewData);   
  }
}
//--------------------------------------------
void doubleclick()
{
    String data = "SOS,1000,1";
    SendLoRaPacket(data);
    playTone(10);
    SerialUSB.println("SOS Button Pressed.");
}

//-------------------------------------------
#ifdef USE_XIAO
void SERCOM2_Handler()
{
  SSerial.IrqHandler();
}
#endif
