/*******************************************************************************
 *
 * LoRa GPS Tracker with Wio Terminal
 * Live tracking on OpenStreetMap with GPS over LoRa
 * Please find more details at Hackster.io:
 * https://www.hackster.io/idreams/lora-gps-tracker-by-wio-terminal-5d8647
 * 
 ******************************************************************************/
#include <wiring_private.h>
#include <SDConfigFile.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include <RH_RF95.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "RTC_SAMD51.h"
#include "DateTime.h"
#include "rpcWiFi.h"
#include <TJpg_Decoder.h>

String _VERSION = "v1.1";
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

const char CONFIG_FILE[] = "config.txt";

//#define SEND_TO_SERVER
//#define SD_LOG
//#define BUZZER
#define MAP_WIDTH 320		//Map Width
#define MAP_HEIGHT 192		//Map Height
//const char* WIFI_SSID = "Your_AP";
//const char* PASSWORD = "Please_Input_Your_Password_Here";

String prevCoarseLatitude, prevCoarseLongitude;
String latitude = "N/A";	//To store latitude
String longitude = "N/A";	//To store longitude

String TempData = "N/A";	//To store Temperature
String HumData = "N/A";		//To store Humidity
String DeviceName = "";		//Not used in this project.

int BUFSIZE;
uint8_t* jpgbuffer;
unsigned long jp = 0;

const int maxZoomLevel = 17;	//Max Zoom Level
const int minZoomLevel = 12;	//Min Zoom Level	
//int ZoomLevel = 16;    			//Default Zoom level	
//int QualityJPG = 60;  			//Map Quality

File myFile;
TFT_eSPI tft = TFT_eSPI();

//----------------------------
double HOME_LAT;                          // Enter Your Latitude and Longitude here
double HOME_LNG;                         // to track how far away the "Sender" is away from Home\\ 
//----------------------------
boolean didReadConfig;
char* WIFI_SSID;
char* PASSWORD;
boolean BUZZER;
boolean SEND_TO_SERVER;
boolean SD_LOG;
int QualityJPG;
int ZoomLevel;
String HomeLAT;
String HomeLNG;
String BufferSize;
String MapType;
float LoRaFrequency;
char* MyServer;
boolean DISTANCE_CALC;

boolean readConfiguration();
//----------------------------
double DistanceBetween2P(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795;
}
//---------------------------------------------------------------------
double StringToDouble(String & str)
{
  return atof(str.c_str());
}
//---------------------------------------------------------------------
//"http://osm-static-maps.herokuapp.com/?geojson=[{'type':'Point','coordinates':[113.943668,22.575171]}]&height=192&width=320&zoom=16&type=jpeg&attribution=Wio Terminal Tracker&quality=90"
//http://idreams.ir/osm/map.php?center=35.661261,51.379574&zoom=17&size=320x192&maptype=cedar&markers=35.661261,51.379574,bullseye&quality=100
//http://maps.google.com/?q=lat,lng


bool getDatafromLink(){
  //return getData("http://osm-static-maps.herokuapp.com/?geojson=[{'type':'Point','coordinates':[" + longitude + "," + latitude + "]}]&height=" + String(MAP_HEIGHT) + "&width=" + String(MAP_WIDTH) + "&zoom=" + String(ZoomLevel) + "&type=jpeg&attribution=Wio Terminal Tracker&quality=" + String(QualityJPG), jpgbuffer);
  return getData("http://idreams.ir/osm/map.php?center="+ latitude + "," + longitude + "&zoom=" + String(ZoomLevel) + "&size=" + String(MAP_WIDTH) + "x" + String(MAP_HEIGHT) + "&maptype=" + MapType + "&markers="+ latitude + "," + longitude + ",bullseye&quality=" + String(QualityJPG), jpgbuffer);
  //return getData("http://idreams.ir/osm/map.php?center="+ latitude + "," + longitude + "&zoom=" + String(ZoomLevel) + "&size=" + String(MAP_WIDTH) + "x" + String(MAP_HEIGHT) + "&maptype=osmfr" + "&markers="+ latitude + "," + longitude + ",bullseye&quality=" + String(QualityJPG), jpgbuffer);
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

  //char server[] = "yourserver.com";
  int port = 80; // HTTP
  WiFiClient client;
  if (client.connect(MyServer, port))
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
      client.println(MyServer);
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
 tft.setCursor(152, 100);
 tft.print("by Wio Terminal " + _VERSION);
//----------------------------------------------
  delay(1000);
  
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("SD Card init failed!");
    ShowMessage("SD Card init failed!", 196);
    while (1);
  }
  Serial.println("SD Card init OK.");
  ShowMessage("SD Card init OK.", 196);
  delay(1000);
  ShowMessage("Read configuration file from SD.", 196);
	
	
  didReadConfig = readConfiguration();
  if (!didReadConfig) {
	Serial.println("Read configuration file failed!");
    ShowMessage("Read configuration file failed!", 196);
    while (1);
  }else{
    Serial.println("Configuration file loaded.");
	  ShowMessage("Configuration file loaded.", 196);
  }
  delay(1000);
  
  if (!rf95.init())
  {
    ShowMessage("LoRa init failed!", 196);
    while (1);
  }
  else
  {
    ShowMessage("LoRa init OK.", 196);
  }

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  //rf95.setTxPower(13, false);
  
  //Select the correct frequency. https://www.thethingsnetwork.org/docs/lorawan/frequencies-by-country/ 
  //rf95.setFrequency(868.0);
  //rf95.setFrequency(434.0);
  rf95.setFrequency(LoRaFrequency);
  
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
        if (BUZZER){
          int addr_str = incomingString.indexOf(',');
          String RCV = incomingString.substring(0,addr_str);
          int addr_midd = incomingString.indexOf(',', addr_str + 1);
          String RCV_int = incomingString.substring(addr_str + 1, addr_midd);
          playTone(230, RCV_int.toInt());
        }
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

          if (SEND_TO_SERVER){
            now = rtc.now();
            sprintf(dateBuffer, "%04u-%02u-%02u", now.year(), now.month(), now.day());      
            sprintf(timeBuffer, "%02u:%02u:%02u", now.hour(), now.minute(), now.second());
            if (Send_to_my_Server(String(dateBuffer), String(timeBuffer), latitude, longitude)){
                ShowMessage("Send to server successfully.", 196);
				//delay(1000);
            }
          }
          
          if (SD_LOG){
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
          }
          
          if (BUZZER){
            playTone(230, 30);
          }
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
  tft.setCursor((105 - tft.textWidth(latitude))/2, 212);
  tft.print(latitude);

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
  tft.setCursor((105 - tft.textWidth(longitude))/2, 227);
  tft.print(longitude);

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
	  if (BUZZER){
          	playTone(230, 30);
	  } 
	}}
   if (digitalRead(WIO_KEY_A) == LOW) {
    Serial.println("A Key pressed");
    if (ZoomLevel > minZoomLevel)
    {
      ZoomLevel--;
      Serial.println("ZoomLevel: ");
      Serial.println(ZoomLevel);
      if(UpdateMap()){Serial.println("Image Downloaded.");ShowMessage("Zoom Level: " + String(ZoomLevel), 196);}
    }
   }
   else if (digitalRead(WIO_KEY_B) == LOW) {
    Serial.println("B Key pressed");
    if (ZoomLevel != 16){
      ZoomLevel = 16;
      Serial.println("ZoomLevel: ");
      Serial.println(ZoomLevel);
      if(UpdateMap()){Serial.println("Image Downloaded.");ShowMessage("Zoom Level: " + String(ZoomLevel), 196);}
    }
   }
   else if (digitalRead(WIO_KEY_C) == LOW) {
    Serial.println("C Key pressed");
    if (ZoomLevel < maxZoomLevel)
    {
      ZoomLevel++;
      Serial.println("ZoomLevel: ");
      Serial.println(ZoomLevel);
      if(UpdateMap()){Serial.println("Image Downloaded.");ShowMessage("Zoom Level: " + String(ZoomLevel), 196);}
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
	  
    if (DISTANCE_CALC){
      float Distance_To_Home;                                       // variable to store Distance to Home  
      double GPSlat = (StringToDouble(latitude));                  // variable to store latitude
      double GPSlng = (StringToDouble(longitude));                   // variable to store longitude 
      Distance_To_Home = (unsigned long) DistanceBetween2P(GPSlat, GPSlng, HOME_LAT, HOME_LNG);
      ShowMessage("Distance to HOME: " + String(int(Distance_To_Home)) + " m", 196);
	}  
	  
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
boolean readConfiguration() {
  const uint8_t CONFIG_LINE_LENGTH = 127;
  SDConfigFile cfg;
  
  // Open the configuration file.
  if (!cfg.begin(CONFIG_FILE, CONFIG_LINE_LENGTH)) {
    Serial.print("Failed to open configuration file: ");
    Serial.println(CONFIG_FILE);
    return false;
  }
  // Read each setting from the file.
  while (cfg.readNextSetting()) {
    if (cfg.nameIs("WifiSSID")) {
	WIFI_SSID = cfg.copyValue();
    } else if (cfg.nameIs("WifiPASS")) {
	PASSWORD = cfg.copyValue();
    } else if (cfg.nameIs("SendToServer")) {
	SEND_TO_SERVER = cfg.getBooleanValue();
    } else if (cfg.nameIs("MyServer")) {
	MyServer = cfg.copyValue();
    } else if (cfg.nameIs("SDLog")) {
	SD_LOG = cfg.getBooleanValue();
    } else if (cfg.nameIs("Buzzer")) {
	BUZZER = cfg.getBooleanValue();
    } else if (cfg.nameIs("ZoomLevel")) {
	ZoomLevel = cfg.getIntValue();
    } else if (cfg.nameIs("QualityJPG")) {
	QualityJPG = cfg.getIntValue();
    } else if (cfg.nameIs("BufferSIZE")) {
	BUFSIZE = cfg.getIntValue();
	Serial.print("BufferSize: ");
	Serial.println(BUFSIZE);
    } else if (cfg.nameIs("MapType")) {
	MapType = cfg.copyValue();
    }else if (cfg.nameIs("HomeLAT")) {
	HomeLAT = cfg.copyValue();
	HOME_LAT = (StringToDouble(HomeLAT));
	Serial.print("LAT: ");
	Serial.println(HOME_LAT);
    } else if (cfg.nameIs("HomeLNG")) {
	HomeLNG = cfg.copyValue();
	HOME_LNG = (StringToDouble(HomeLNG));
	Serial.print("LNG: ");
	Serial.println(HOME_LNG);
    }else if (cfg.nameIs("DistanceCalc")) {
	DISTANCE_CALC = cfg.getBooleanValue();
    }else if (cfg.nameIs("LoRaFrequency")) {
	LoRaFrequency = float(cfg.getIntValue());
	Serial.print("LoRaFrequency: ");
	Serial.println(LoRaFrequency);		
    }else {
      // report unrecognized names.
	Serial.print("Unknown name in config: ");
	Serial.println(cfg.getName());
    }
  }
  
  // clean up
  cfg.end();
  
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
  
