#include <HTTPClient.h>
#include <WiFi.h>
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

void wifi_connect_and_fetch_data()
{
  // Connect to Wifi
  M5.EPD.Clear(true);
  int wifi_attempt_count = 0;
  WiFi.begin(ssid, pass);
  canvas1.setTextSize(10);
  canvas1.drawString("Attempting wifi connection", 0, 0);
  canvas1.pushCanvas(0, 0, UPDATE_MODE_GC16);
  while (WiFi.status() != WL_CONNECTED) {     

    if (wifi_attempt_count % 10 == 0)
    {
      //Flash the screen to know we are still alive and trying to connect to wifi
      //Only do this once every 5 seconds
      M5.EPD.Clear(true);
      canvas1.setTextSize(10);
      canvas1.drawString("Attempting wifi connection", 0, 0);
      canvas1.pushCanvas(0, 0, UPDATE_MODE_GC16);

      //Restart the wifi connect attempt..
      //FIXME: This still isn't great. We should probably check for an error status on the wifi connect attempt (assuming such a result is supplied by the Wifi API)
      WiFi.begin(ssid, pass);
      
      delay(500);
      
      //FIXME: Put in an upper bound on attempts before giving up and just displaying the previously captured (or default) info
      //        -- This is critical for the "low power" approach          
    }

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

void wifi_disconnect()
{
  WiFi.disconnect();
}

void draw_topbar()
{
  const char *datenow = json_object_dotget_string(json_value_get_object(json_root), "currentobservation.Date");
  const char *area = json_object_dotget_string(json_value_get_object(json_root), "location.areaDescription");

  // Wifi network indicator
  canvas1.createRender(20, 256);
  canvas1.setTextSize(20);
  canvas1.setTextDatum(CL_DATUM);
  canvas1.drawString(datenow, 20, 20);

  // Area
  canvas1.createRender(20, 256);
  canvas1.setTextSize(20);
  canvas1.setTextDatum(CR_DATUM);
  canvas1.drawString(area, 520, 20);
}

void draw_weathernow()
{
  const char *tempnow = json_object_dotget_string(json_value_get_object(json_root), "currentobservation.Temp");
  const char *descnow  = json_object_dotget_string(json_value_get_object(json_root), "currentobservation.Weather");

  // Weather Symbol
  canvas1.setTextDatum(BL_DATUM);
  canvas1.createRender(100, 256);
  canvas1.setTextSize(100);
  canvas1.setTextDatum(CC_DATUM);

  char *symbol;
  if      (!strcmp(descnow, "Chance Flurries")) symbol = snowy;
  else if (!strcmp(descnow, "Flurries")) symbol = snowy;
  //
  else if (!strcmp(descnow, "Fair"))         symbol = sunny;
  else if (!strcmp(descnow, "Clear"))        symbol = sunny;
  else if (!strcmp(descnow, "Mostly Clear")) symbol = sunny;
  else if (!strcmp(descnow, "Mostly Sunny")) symbol = sunny;
  else if (!strcmp(descnow, "Sunny"))        symbol = sunny;
  //
  else if (!strcmp(descnow, "Partly Cloudy")) symbol = cloudy;
  else if (!strcmp(descnow, "Cloudy"))        symbol = cloudy;
  else if (!strcmp(descnow, "Overcast"))      symbol = cloudy;
  else if (!strcmp(descnow, "Mostly Cloudy")) symbol = cloudy;
  //
  else if (!strcmp(descnow, "Chance Rain")) symbol = rainy;
  else if (!strcmp(descnow, "Chance Showers")) symbol = rainy;
  else if (!strcmp(descnow, "Rain Likely")) symbol = rainy;
  else if (!strcmp(descnow, "Mostly Cloudy then Slight Chance Rain")) symbol = rainy;
  else if (!strcmp(descnow, "Slight Chance Rain")) symbol = rainy;
  //
  else symbol = unknown;
  canvas1.drawString(symbol, 270, 240);

  // Temp degrees
  canvas1.setTextSize(100);
  canvas1.setTextDatum(CC_DATUM);
  canvas1.drawString(tempnow, 270, 350);

  // Desc of now
  canvas1.createRender(40, 256);
  canvas1.setTextSize(40);
  canvas1.setTextDatum(CC_DATUM);
  canvas1.drawString(descnow, 270, 440);
}

void draw_forecast() {
  const JSON_Array *pname = json_object_dotget_array(json_value_get_object(json_root), "time.startPeriodName");
  const JSON_Array *temps = json_object_dotget_array(json_value_get_object(json_root), "data.temperature");
  const JSON_Array *desc_short= json_object_dotget_array(json_value_get_object(json_root), "data.weather");
  const JSON_Array *desc_long= json_object_dotget_array(json_value_get_object(json_root), "data.text");

  char buf[300];
  int i;
  for (i = 0; i < json_array_get_count(pname) && i < 5; i++) {
    const char *periodtime = json_array_get_string(pname, i);
    const char *periodtemp = json_array_get_string(temps, i);
    const char *perioddesc = json_array_get_string(desc_short, i);
    sprintf(buf, "%s: %s°F / %s\n", periodtime, periodtemp, perioddesc);
    canvas1.createRender(28, 256);
    canvas1.setTextSize(28);
    canvas1.setTextDatum(CC_DATUM);
    canvas1.drawString(buf, 270, 500 + (i * 38));
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

  canvas1.createRender(20, 256);
  canvas1.setTextSize(20);

  canvas1.setTextDatum(CL_DATUM);
  canvas1.drawString("Inside: " + String(temStr) + "°F" , 20, 940);

  canvas1.setTextDatum(CR_DATUM);
  canvas1.drawString("Humidity: " +  String(humStr)+ "%" , 520, 940);
}







void draw()
{
  wifi_connect_and_fetch_data();
  //Clear the screen after "connect and fetch".
  //The connect and fetch function handles clearing the screen on its own during wifi status"ing"
  M5.EPD.Clear(true);
  
  draw_topbar();
  draw_weathernow();
  draw_forecast();
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
  M5.EPD.Clear(true);
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
