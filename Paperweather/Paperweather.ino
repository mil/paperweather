#include <HTTPClient.h>
#include <WiFi.h>
#include "time.h"
#include "binaryttf.h"
#include "config.h"
#include "M5EPD.h"
#include "parson.h"

M5EPD_Canvas canvas1(&M5.EPD);
HTTPClient client;
JSON_Value *json_root = NULL;

char *sunny = "";
char *cloudy = "";
char *snowy = "";
char *rainy = "";
char *tstormy = "";
char *unknown = "U";

String timeString;

#define DEFAULT_TEXT_SIZE 30
#define XRES 540
#define YRES 960
#define XMID XRES / 2
#define YMID YRES / 2

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

void addText(String text, int x, int y, int size)
{
  canvas1.setTextDatum(BL_DATUM);
  canvas1.createRender(size, 256);
  canvas1.setTextSize(size);
  canvas1.setTextDatum(CC_DATUM);
  canvas1.drawString(text, x, y);
}

void addText(String text, int x, int y)
{
  addText(text, x, y, DEFAULT_TEXT_SIZE);
}

void wifi_connect_and_fetch_data()
{
  // Connect to Wifi
  int wifi_attempt_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (wifi_attempt_count % 5 == 0)
    {
      //Flash the screen to know we are still alive and trying to connect to wifi
      //Only do this once every 5 seconds
      
      //Restart the wifi connect attempt..
      //FIXME: This still isn't great. We should probably check for an error status on the wifi connect attempt (assuming such a result is supplied by the Wifi API)
      Serial.println("Re-trying wifi connection: " + String(wifi_attempt_count));
      WiFi.begin(ssid, pass);    

      if (wifi_attempt_count >= 30)
      {
        //FIXME: Display previous (or default) data??
        M5.EPD.Clear(true);
        delay(100);
        addText(String("Failed to connect to wifi"), XMID, YMID - 30);
        canvas1.pushCanvas(0, 0, UPDATE_MODE_GC16);

        delay(1000 * refreshintervalseconds);
        wifi_attempt_count = -1;
        M5.EPD.Clear(true);
        
      }
      else
      {
        addText(String("Attempting wifi connection"), XMID, YMID - 30);
      }
      canvas1.drawString("SSID: " + String(ssid), XMID, YMID);
      canvas1.drawString("Pass: " + String(pass), XMID, YMID+DEFAULT_TEXT_SIZE);
      canvas1.pushCanvas(0, 0, UPDATE_MODE_GC16);
    }
    
    delay(1000);
    wifi_attempt_count++;
  }  

  // Fetch weather JSON
  client.begin(weatherurl);
  int httpResponseCode = client.GET();
  String noaabuffer = "{}"; 
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    noaabuffer = client.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // Parse XML
  json_root = json_parse_string(noaabuffer.c_str());
}

void getNtpTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){
    timeString = "UnknownTime";    
    return;
  }
  timeinfo.tm_hour -= 5;
  char buf[40];  
  strftime(buf, 40, "%m/%d/%y %H:%M", &timeinfo);
  timeString = String(buf);

  // FIXME: Maybe add some smart logic to look at current real time and last 
  //        report time and try to synchronize our next sleep so we always have
  //        up to date data as it is published

}

void wifi_disconnect()
{
  WiFi.disconnect();
}

void draw_lastUpdated()
{
  const char *datenow = json_object_dotget_string(json_value_get_object(json_root), "creationDateLocal"); 
  addText(String("Last Updated: " + String(datenow)), XMID, 910, 20);
}

void draw_batteryleft()
{
  char buf[20];
  uint32_t vol = M5.getBatteryVoltage();
  float battery = (float)(vol - 3300) / (float)(4350 - 3300);
  if(battery <= 0.01)
  {
      battery = 0.01;
  }
  if(battery > 1)
  {
      battery = 1;
  }
  uint8_t px = battery * 25;
  sprintf(buf, "Battery: %d%%", (int)(battery * 100));
  addText(String(buf) , XMID, 40, 20);
}

void draw_topbar()
{
  const char *datenow = json_object_dotget_string(json_value_get_object(json_root), "currentobservation.Date");
  const char *area = json_object_dotget_string(json_value_get_object(json_root), "location.areaDescription");
  
  addText(timeString, 20, 20, 20);

  // Area
  addText(area, 520, 20, 20);  
}

void draw_weathericon()
{
  const char *tempnow = json_object_dotget_string(json_value_get_object(json_root), "currentobservation.Temp");
  const char *descnow  = json_object_dotget_string(json_value_get_object(json_root), "currentobservation.Weather");

  // Weather Symbol
  canvas1.setTextDatum(BL_DATUM);
  canvas1.createRender(100, 256);
  canvas1.setTextSize(100);
  canvas1.setTextDatum(CC_DATUM);

  char *symbol;
  if (strstr(descnow,"Flur") != NULL) symbol = snowy;
  else if (strstr(descnow,"Snow") != NULL) symbol = snowy;
  else if (strstr(descnow,"Winter") != NULL) symbol = snowy;  
  else if (strstr(descnow,"Blizzard") != NULL) symbol = snowy;
  //
  else if (strstr(descnow,"Fair") != NULL) symbol = sunny;
  else if (strstr(descnow,"Clear") != NULL) symbol = sunny;
  else if (strstr(descnow,"Sun") != NULL) symbol = sunny;
  //
  else if (strstr(descnow,"Cloud") != NULL) symbol = cloudy;
  else if (strstr(descnow,"Overcast") != NULL) symbol = cloudy;
  //
  else if (strstr(descnow,"Showers") != NULL) symbol = rainy;
  else if (strstr(descnow,"Rain") != NULL) symbol = rainy;
  // 
  else if (strstr(descnow,"Thun") != NULL) symbol = tstormy; 
  //
  else symbol = unknown;
  if (symbol == unknown)
  {
    canvas1.setTextSize(20);
    canvas1.drawString(String(descnow), XMID, 210);
  }
  else
  {
    canvas1.drawString(symbol, XMID, 210);
  }

  // Temp degrees
  canvas1.setTextSize(100);
  canvas1.setTextDatum(CC_DATUM);
  canvas1.drawString(String(tempnow)+" °F", XMID, 320);

  // Desc of now
  canvas1.createRender(40, 256);
  canvas1.setTextSize(40);
  canvas1.setTextDatum(CC_DATUM);
  canvas1.drawString(descnow, XMID, 410);
}

void draw_forecast() {
  const JSON_Array *pname = json_object_dotget_array(json_value_get_object(json_root), "time.startPeriodName");
  const JSON_Array *temps = json_object_dotget_array(json_value_get_object(json_root), "data.temperature");
  const JSON_Array *desc_short= json_object_dotget_array(json_value_get_object(json_root), "data.weather");
  const JSON_Array *desc_long= json_object_dotget_array(json_value_get_object(json_root), "data.text");

  char buf[300];
  char buf2[50];
  int i;
  for (i = 0; i < json_array_get_count(pname) && i < 5; i++) {
    const char *periodtime = json_array_get_string(pname, i);
    const char *perioddesc = json_array_get_string(desc_short, i);
    const char *highTemp = json_array_get_string(temps, (2*i)+1);
    const char *lowTemp = json_array_get_string(temps, (2*i));
    sprintf(buf, "%s: %s / %s °F", periodtime, highTemp, lowTemp);
    sprintf(buf2, "%s", perioddesc);
    addText(buf, XMID, 460 + (i*87), 30);    
    addText(buf2, XMID, 460 + (i*87)+30, 25);
  }
}


void draw_sensors()
{
  M5.SHT30.UpdateData();
  float tem = M5.SHT30.GetTemperature() * (9.0/5.0) + 32;
  float hum = M5.SHT30.GetRelHumidity();
  char temStr[10];
  char humStr[10];
  dtostrf(tem, 2, 2 , temStr);
  dtostrf(hum, 2, 2 , humStr);

  addText(String("Inside: " + String(temStr) + "°F"), 20, 940, 20);
  addText(String("Humidity: " +  String(humStr)+ "%" ), 520, 940, 20);
}

void draw()
{
  canvas1.fillCanvas(0);
  wifi_connect_and_fetch_data();
  M5.EPD.Clear(true);
  canvas1.fillCanvas(0);
  
  getNtpTime();
  draw_topbar();
  draw_batteryleft();
  draw_weathericon();
  draw_forecast();
  draw_lastUpdated();
  draw_sensors();

  canvas1.pushCanvas(0, 0, UPDATE_MODE_GC16);

  wifi_disconnect();
  json_value_free(json_root);
}

void setup()
{
  M5.begin();
  M5.TP.SetRotation(90);
  M5.EPD.SetRotation(90);
  M5.SHT30.Begin();
  M5.RTC.begin();
  canvas1.loadFont(binaryttf, sizeof(binaryttf)); // Load font files from binary data
  canvas1.createCanvas(540, 960);
}

void loop()
{
  draw();
  delay(1000 * refreshintervalseconds);
}
