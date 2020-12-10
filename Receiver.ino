/*******************************************************************************
 *
 * LoRa GPS Tracker by Wio Terminal
 * Live tracking on OpenStreetMap with GPS over LoRa
 * Please find more details at Hackster.io:
 * https://www.hackster.io/idreams/lora-gps-tracker-by-wio-terminal-5d8647
 * 
 ******************************************************************************/
#include <wiring_private.h>
#include <TJpg_Decoder.h>
#include <rpcWiFi.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include <RH_RF95.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include <SPI.h>
#include "RTC_SAMD51.h"
#include "DateTime.h"
//----------------------------
//RTC
RTC_SAMD51 rtc;
DateTime now;
char timeBuffer[16];
char dateBuffer[16];
//----------------------------
//LoRa
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    //RH_RF95<Uart> rf95(Serial1);	//for Hardware serial
    static Uart Serial3(&sercom3, PIN_WIRE_SCL, PIN_WIRE_SDA, SERCOM_RX_PAD_1, UART_TX_PAD_0);
    RH_RF95 <Uart> rf95(Serial3);
#endif

#define SEND_TO_SERVER
//#define SD_LOG
#define BUZZER
#define MAP_WIDTH 320
#define MAP_HEIGHT 192
const char* WIFI_SSID = "Your_AP";
const char* PASSWORD = "Please_Input_Your_Password_Here";

String prevCoarseLatitude, prevCoarseLongitude;
String latitude = "N/A";
String longitude = "N/A";

String TempData = "N/A";
String HumData = "N/A";
String DeviceName = "";

#define BUFSIZE 50000 
uint8_t* jpgbuffer;
unsigned long jp = 0;

const int maxZoomLevel = 17;
const int minZoomLevel = 12;
int ZoomLevel = 16;
int QualityJPG = 60;  //Map Quality

File myFile;
TFT_eSPI tft = TFT_eSPI();

//---------------------------------------------------------------------
//"http://osm-static-maps.herokuapp.com/?geojson=[{'type':'Point','coordinates':[113.943668,22.575171]}]&height=192&width=320&zoom=16&type=jpeg&attribution=Wio Terminal Tracker&quality=90"

bool getDatafromLink(){
  return getData("http://osm-static-maps.herokuapp.com/?geojson=[{'type':'Point','coordinates':[" + longitude + "," + latitude + "]}]&height=" + String(MAP_HEIGHT) + "&width=" + String(MAP_WIDTH) + "&zoom=" + String(ZoomLevel) + "&type=jpeg&attribution=Wio Terminal Tracker&quality=" + String(QualityJPG), jpgbuffer);
  }
//---------------------------------------------------------------------
void playTone(int tone, int duration) {
  analogWrite(WIO_BUZZER, tone);
  delay(duration);
  analogWrite(WIO_BUZZER, 0);
}
//---------------------------------------------------------------------
bool Send_to_my_Server(String date_format, String time_format, String lat_format, String lon_format){
  if ((WiFi.status() == WL_CONNECTED)) {

  char server[] = "yourserver.com";
  int port = 80; // HTTP
  WiFiClient client;
  if (client.connect(server, port))
    {
      Serial.println("connected");
      String str = "GET /wio_store_data.php?";
      str += "date=";
      str += date_format;
      str += "&time=";
      str += time_format;
      str += "&lat=";
      str += lat_format;
      str += "&lon=";
      str += lon_format;
      client.print(str);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(server);
      client.println("Connection: close");
      client.println();
      client.stop();
      return true;
    }
    else {
      Serial.println("Unable to connect");
      return false;
    }
  }
  else return false;
}    
//--------------------------------------------------------------------- 
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}
//---------------------------------------------------------------------
void ShowMessage(String msg, int Cursor) {
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(0, Cursor);
  tft.print("                                                   ");
  tft.setCursor((320 - tft.textWidth(msg))/2, Cursor);
  tft.print(msg);
}
//--------------------------------------------------------------------- 
void setup() {
  //jpgbuffer = (uint8_t *) malloc (BUFSIZE);
  Serial.begin(115200);
  //
  rtc.begin();
  //DateTime now = DateTime(F(__DATE__), F(__TIME__));
  now = DateTime(F(__DATE__), F(__TIME__));
  rtc.adjust(now);
  //
  pinMode(WIO_BUZZER, OUTPUT);
  //
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  pinPeripheral(PIN_WIRE_SCL, PIO_SERCOM_ALT);
  pinPeripheral(PIN_WIRE_SDA, PIO_SERCOM_ALT);
  delay(100);
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(FMB12);
//----------------------------------------------
 tft.setCursor((320 - tft.textWidth("LoRa GPS Tracker"))/2, 96);
 tft.print("LoRa GPS Tracker");
 tft.setTextFont(1);
 tft.setCursor(183, 100);
 tft.print("by Wio Terminal");
//----------------------------------------------
  delay(1000);
  #ifdef SD_LOG
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("SD Card init failed!");
    ShowMessage("SD Card init failed!", 196);
    while (1);
  }
  Serial.println("SD Card init OK.");
  ShowMessage("SD Card init OK.", 196);
  delay(1000);
  #endif
  
  if (!rf95.init())
  {
    ShowMessage("LoRa init failed!", 196);
    while (1);
  }
  else
  {
    ShowMessage("LoRa init OK.", 196);
  }
  delay(1000);
  

  ShowMessage("Connecting to Wi-Fi...", 196);

 
  WiFi.begin(WIFI_SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  ShowMessage("Connected!", 196);
  delay(1000);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);	
	

  rf95.setFrequency(868.0);
  ShowMessage("Waiting for receiving LoRa packet...", 196);

}
//---------------------------------------------------------------
void LoRaEvent()
{     
  bool isMapNeedUpdate = false;
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      int StrLength = String((char*)buf).length();
      String incomingString = String((char*)buf);
      Serial.println(incomingString);
      String recTest = incomingString.substring(0,3);
      Serial.println(recTest);
      if(recTest == "SOS"){
        ShowMessage("[S.O.S] Message Received.", 196);
        #ifdef BUZZER
          int addr_str = incomingString.indexOf(',');
          String RCV = incomingString.substring(0,addr_str);
          int addr_midd = incomingString.indexOf(',', addr_str + 1);
          String RCV_int = incomingString.substring(addr_str + 1, addr_midd);
          playTone(230, RCV_int.toInt());
        #endif
      }else{
        int addr_start = incomingString.indexOf(',');
        String LatRCV = incomingString.substring(0,addr_start);
        int addr_mid = incomingString.indexOf(',', addr_start + 1);
        String LonRCV = incomingString.substring(addr_start+1,addr_mid);
        //-----------------------------------------------------------------
        int addr_mid2 = incomingString.indexOf(',', addr_mid + 1);
        String TempD = incomingString.substring(addr_mid + 1, addr_mid2);
        //Serial.println(TempD);
        int addr_mid3 = incomingString.indexOf(',', addr_mid2 + 1);
        String HumD = incomingString.substring(addr_mid2 + 1, addr_mid3);
        //Serial.println(HumD);
        int addr_mid4 = incomingString.indexOf(',', addr_mid3 + 1);
        String DevName = incomingString.substring(addr_mid3 + 1, addr_mid4);
        //Serial.println(DevName);
        TempData = TempD;
        HumData = HumD;
        DeviceName = DevName; //not use in this project
        //-----------------------------------------------------------------
        latitude = LatRCV;
        longitude = LonRCV;
        String coarseLatitude = latitude;
        String coarseLongitude = longitude;
        if(coarseLatitude != prevCoarseLatitude){
          isMapNeedUpdate = true;
        }
        prevCoarseLatitude = coarseLatitude;
            
        if(coarseLongitude != prevCoarseLongitude){            
          isMapNeedUpdate = true;
        }
        prevCoarseLongitude = coarseLongitude;
              
        if(isMapNeedUpdate){

          #ifdef SEND_TO_SERVER
            now = rtc.now();
            sprintf(dateBuffer, "%04u-%02u-%02u", now.year(), now.month(), now.day());      
            sprintf(timeBuffer, "%02u:%02u:%02u", now.hour(), now.minute(), now.second());
            if (Send_to_my_Server(String(dateBuffer), String(timeBuffer), latitude, longitude)){
                ShowMessage("Send to server successfully.", 196);
		//delay(1000);
            }
          #endif
          
          #ifdef SD_LOG
            now = rtc.now();
            sprintf(dateBuffer, "%04u-%02u-%02u", now.year(), now.month(), now.day());      
            sprintf(timeBuffer, "%02u:%02u:%02u", now.hour(), now.minute(), now.second());
            //myFile = SD.open(String(dateBuffer), FILE_WRITE);
            myFile = SD.open(String(dateBuffer) + ".txt", FILE_APPEND);
            if (myFile){
              String DataSD = String(dateBuffer) + "," + String(timeBuffer) + "," + latitude + "," + longitude + "," + TempData + "," + HumData ;
              myFile.println(DataSD);
              myFile.close();
              ShowMessage("Save on SD card successfully.", 196);
            }
          #endif
          
          #ifdef BUZZER
            playTone(230, 30);
          #endif
          UpdateMap();
        } 
    }
    }
}
}
//---------------------------------------------------------------
void ScreenINF()
{
  now = rtc.now();
  sprintf(dateBuffer, "%04u-%02u-%02u", now.year(), now.month(), now.day());      
  sprintf(timeBuffer, "%02u:%02u:%02u", now.hour(), now.minute(), now.second());

  //----------------------------------------------------------------
  tft.drawFastVLine(105,208,30,TFT_WHITE);
  tft.drawFastVLine(210,208,30,TFT_WHITE);
  tft.drawFastHLine(2,223,316,TFT_WHITE);
  //----------------------------------------------------------------
  tft.drawRect(2,208,316,30,TFT_WHITE);
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  String nulltxt = "        ";
  tft.setCursor((105 - tft.textWidth(nulltxt))/2, 212);
  tft.print(nulltxt);
  tft.setCursor((105 - tft.textWidth(longitude))/2, 212);
  tft.print(longitude);

  tft.setCursor(105 + (105 - tft.textWidth(nulltxt))/2, 212);
  tft.print(nulltxt);
  tft.setCursor(105 + (105 - tft.textWidth(String(timeBuffer)))/2, 212);
  tft.print(String(timeBuffer));

  tft.setCursor(210 + (105 - tft.textWidth(nulltxt))/2, 212);
  tft.print(nulltxt);
  tft.setCursor(210 + (105 - tft.textWidth(TempData + " C"))/2, 212);
  tft.print(TempData + " C");

  //-----------------------------------------------------------------
  tft.setCursor((105 - tft.textWidth(nulltxt))/2, 227);
  tft.print(nulltxt);
  tft.setCursor((105 - tft.textWidth(latitude))/2, 227);
  tft.print(latitude);

  tft.setCursor(105 + (105 - tft.textWidth(nulltxt))/2, 227);
  tft.print(nulltxt);
  tft.setCursor(105 + (105 - tft.textWidth(String(dateBuffer)))/2, 227);
  tft.print(String(dateBuffer));

  tft.setCursor(210 + (105 - tft.textWidth(nulltxt))/2, 227);
  tft.print(nulltxt);
  tft.setCursor(210 + (105 - tft.textWidth(HumData + " %"))/2, 227);
  tft.print(HumData + " %");
}
//---------------------------------------------------------------
void ButtonEvent(){
	if (digitalRead(WIO_5S_PRESS) == LOW) {
    Serial.println("5 Way Press.");
    uint8_t data[] = "SOS,500,1";
    if(rf95.send(data, sizeof(data))){
      rf95.waitPacketSent();
      ShowMessage("[S.O.S] Message Sent.", 196);
     Serial.println("LoRa Packet Sent.");
	  #ifdef BUZZER
          	playTone(230, 30);
	  #endif 
	}}
   if (digitalRead(WIO_KEY_A) == LOW) {
    Serial.println("A Key pressed");
    if (ZoomLevel > minZoomLevel)
    {
      ZoomLevel--;
      Serial.println("ZoomLevel: ");
      Serial.println(ZoomLevel);
      if(UpdateMap()){Serial.println("Image Downloaded.");}
    }
   }
   else if (digitalRead(WIO_KEY_B) == LOW) {
    Serial.println("B Key pressed");
    if (ZoomLevel != 16){
      ZoomLevel = 16;
      Serial.println("ZoomLevel: ");
      Serial.println(ZoomLevel);
      if(UpdateMap()){Serial.println("Image Downloaded.");}
    }
   }
   else if (digitalRead(WIO_KEY_C) == LOW) {
    Serial.println("C Key pressed");
    if (ZoomLevel < maxZoomLevel)
    {
      ZoomLevel++;
      Serial.println("ZoomLevel: ");
      Serial.println(ZoomLevel);
      if(UpdateMap()){Serial.println("Image Downloaded.");}
    }
   }
   delay(200);
}
//-------------------------------------------------
bool UpdateMap(){
  jpgbuffer = (uint8_t *) malloc (BUFSIZE);
  jp = 0;
  
  uint32_t t = millis();
  bool loaded_ok = getDatafromLink();
  
  t = millis() - t;
  if (loaded_ok) {
    #ifdef BUZZER
      playTone(230, 30);
    #endif
    
    Serial.print("Downloaded in ");
    Serial.print(t);
    Serial.println(" ms."); 
    t = millis();
    TJpgDec.drawJpg(0, 0, jpgbuffer, jp);
    t = millis() - t;
    Serial.print("Decoding and rendering: ");
    Serial.print(t);
    Serial.println(" ms.");
    ShowMessage("Downloaded in " + String(t) + " ms.", 196);
    return true;
  }else{return false;} 
}
//------------------------------------------------------------------------------------ 
bool getData(String url, uint8_t* jpgbuffer) {
  Serial.println("Downloading file from " + url);
  if ((WiFi.status() != WL_CONNECTED)) {
    return false;
  }
    ShowMessage("Connecting to Server...", 196);
    free(jpgbuffer);
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        int total = http.getSize();
        Serial.println();
        Serial.print("TOTAL: ");
        Serial.println(total);
        //jpgbuffer = (uint8_t *) malloc (total);
        int len = total;
        uint8_t buff[128] = { 0 };
        WiFiClient * stream = http.getStreamPtr();
        while (http.connected() && (len > 0 || len == -1)) {
          size_t size = stream->available();
          if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            memcpy (jpgbuffer + jp, buff, c);
            jp = jp + c;
            if (len > 0) {
              len -= c;
            }
          }
          yield();
        }
        ShowMessage("Connection closed.", 196);
        Serial.println();
        Serial.print("[HTTP] connection closed or file end.\n");
        //return false
      }
    }
    else {
      Serial.printf("[HTTP] GET..failed, error: %s\n", http.errorToString(httpCode).c_str());
      ShowMessage("Error: " + String(http.errorToString(httpCode).c_str()), 196);
      http.end();
      return false;
    }
    http.end();
    return true; 
  }
//---------------------------------------------------------------

void loop() {
  ButtonEvent();
  LoRaEvent();
  ScreenINF();
}
//---------------------------------------------------------------
void SERCOM3_0_Handler()
{
  Serial3.IrqHandler();
}
void SERCOM3_1_Handler()
{
  Serial3.IrqHandler();
}
void SERCOM3_2_Handler()
{
  Serial3.IrqHandler();
}
void SERCOM3_3_Handler()
{
  Serial3.IrqHandler();
}
  
