// Graphics
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "my_icons.h"

// Display
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);
#include <SPI.h>
#include <Wire.h>
bool displayStatus = true;

#define DISPLAY_DIM_LEVEL 150
#define DISPLAY_OFF_LEVEL 30

// DH11 Sensor
#include <DHTesp.h>
#include <math.h>
DHTesp dht;

// WiFi
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "constant.h"
#define CITY "Marina%20Del%20Rey"
//#define CITY "Miami"
//#define CITY "New%20Orleans"
const char* ssid = SSDI;
const char* password = PASS;
const char* nowcast_url  = "http://api.openweathermap.org/data/2.5/weather?q=" CITY ",us&mode=json&APPID=" APPID "&units=metric";
const char* forecast_url = "http://api.openweathermap.org/data/2.5/forecast?q=" CITY ",us&mode=json&APPID=" APPID "&units=metric";
//const char* forecast_url = "http://api.openweathermap.org/data/2.5/weather?q=Los%20Angeles,us&mode=json&APPID=" APPID "&units=metric";

HTTPClient http;
unsigned long respond_time = 0; // time that store the request moment
//#define WAIT_TIME 5000 // How ofter do the request (todo change to DEFINE)
//#define UPDATE_TIME 5000 // How do we wait before the next loop

#define WAIT_TIME 250000 // How ofter do the request (todo change to DEFINE) 5 min
#define UPDATE_TIME 5000 // How do we wait before the next loop 5 sec

//json
#include <ArduinoJson.h>
float forecast_max_temp, forecast_min_temp;

bool WiFi_isconnected, WiFi_isanswer, WiFi_coldrun;

// Time
#include <Time.h>
#include <TimeLib.h>

struct Frame{
  String update_time;
  int weather_code;
  String dht_string;
  String temp;
  String temp_range;
  String weather_str;
  bool umbrella;
  float rain;
} frame;


/// SUPPORTING FUNCTION
// Memory
extern "C" {
#include "user_interface.h"
}
void mf()
{
    // Print free memory
    Serial.print("freeMemory = ");
    Serial.println(system_get_free_heap_size());
}

void blinki(int n)
{  
  // Blink LED n time
  unsigned long dtime = 100;
  for (int i = 1; i <= n; i++)
  {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(dtime);
    digitalWrite(LED_BUILTIN,LOW);    
    delay(dtime);
  }
}

inline String deg()
{
  // Return degree symbol 
  return String(char(247)); // 265, 9
}

inline String uptime()
{
   // Return uptime string
   return String((millis()- respond_time) / 1000);
}

String timeStr(time_t t)
{
  // Return string time from time
  // ToDo - return for LA, maybe with clock...
  String timeStr;  
  
  //timeStr = String(year(t))+"."+String(month(t))+"."+String(day(t))+" "+String(hour(t))+":"+String(minute(t))+":"+String(second(t));
  timeStr = String(monthShortStr(month(t)))+" "+String(day(t))+" "+String(hour(t))+":"+String(minute(t));
  return timeStr;
}

void clearFrame()
{
  frame.update_time = "";
  frame.weather_code = 0;
  frame.dht_string = "";
  frame.temp = "";
  frame.temp_range = "";
  frame.weather_str = "";
  frame.rain = 0;
}

/// WIFI CONNECT FUNCTIONS
bool wifi_connect()
{
  // Connect to WiFi, using 10 attemps. Return status of connection
  int count=0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  bool WiFi_status = (WiFi.status() == WL_CONNECTED);
  while (!WiFi_status and count < 10) {
    delay(500);
    count++;    
  }

  if(WiFi_status)
  {
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("No connection");
  }
  
  return WiFi_status;
}

/// DHT FUNCTIONS
void getDHT()
{
    // Display text from DHT sensors
  
    // Sensor 1
    float humidity = round(dht.getHumidity());
    float temperature = round(dht.getTemperature());

    String temperature_str = String(int(temperature));
    String humidity_str = String(int(humidity));
    String Sensor1 = "" + temperature_str + deg() + "," + humidity_str + "%";

    String time_str = String(millis());

    frame.dht_string = dht.getStatusString();
    if (dht.getStatus() == 0)
    {
      frame.dht_string += " DH11 " + Sensor1;
    }
    Serial.println(frame.dht_string);
}

/// GET AND PARSE JSON FUNCTIONS
bool getJson(const char* url, String * answer)
{
    // Get json string by http client. Return if the string was returned.
    
    Serial.println(url);
    http.begin(url); //HTTPS ?
    int httpCode = http.GET();
    
    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            *answer = http.getString();
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    return (httpCode == HTTP_CODE_OK);
}

bool getNowcast()
{
  
  // Get current weather
  String respond;
  Serial.println("Getting nowcast");    
  if(getJson(nowcast_url, &respond))
  {
    Serial.println("Processing the nowcast");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(respond);
    if(root.success())
    {
      Serial.println(respond);
      JsonObject& weather = root["weather"][0];
      JsonObject& main    = root["main"];

      //Status
      const char* weather_str = weather["main"];
      int code = weather["id"];
  
      // Temperature      
      float temp     = main["temp"];    
      float temp_max = main["temp_max"];    
      float temp_min = main["temp_min"];    

      //Time 
      time_t weatherTime = root["dt"];

      // Rain
      frame.rain = 0;
      JsonObject& rain = weather["rain"];          
      if (rain.success())
      {
         float r = rain["3h"];
         frame.rain = r;
      }

      frame.weather_code = code;
      frame.weather_str = weather_str;
      frame.temp = String(round(temp)) + deg();
      frame.temp_range = String(round(temp_min)) + "-" + String(round(temp_max)) + deg();
      frame.update_time = timeStr(weatherTime);
      return true;
    } 
    else
    {
      Serial.println("Error processing nowcast");
      frame.weather_code = -1;
      frame.weather_str = "Processing error";
      frame.temp = "???";
      frame.temp_range = "";
    }
  }
  else
  {
    Serial.println("Error getting nowcast json");
  }
  return false;
}

bool getForecast()
{
    // Get weather forecast
    String respond;
    float temp_max = -999;
    float temp_min =  999;
    float temp_get;
    float rain_get = 0;
    
    Serial.println("Getting forecast");
    
    if(getJson(forecast_url, &respond))
    {
      Serial.println("Redusing forecast");
      int count = 0;
      for(int c=0; c<5; c++) // find only 4 items 
      {
        count = respond.indexOf("\"dt\"", count+1);        
      }
      //mf();
      if(count > 2)
      respond = respond.substring(0, count-2) + "]}";
      //mf();
      Serial.println(respond);

      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(respond);
      //mf();
      if(root.success())
      {
        Serial.println("Parsing forecast");
        Serial.println("Getting size:");
        int walk = min(4, int(root["list"].size()));       
        Serial.println("Size: " + String(root["list"].size()));
      
        for (int i = 0; i<walk; i++)
        {
          JsonObject& weather = root["list"][i];     
          JsonObject& main = weather["main"];          
          JsonObject& rain = weather["rain"];          
          int code = weather["id"];
                    
          temp_get = main["temp"];
          temp_max = max(temp_max, temp_get);
          temp_min = min(temp_min, temp_get);         

          if (code >= 900)
          {
            const char* weather_str = weather["main"];
            frame.weather_code = code;
            frame.weather_str = weather_str;
          }
          // list.rain.3h
          // list.wind.speed
          if (rain.success())
          {
             float r = rain["3h"];
             rain_get += r;
          }
          weather.printTo(Serial);
          Serial.println();
        }

        frame.rain += rain_get;
        frame.temp_range = String(round(temp_min)) + "-" + String(round(temp_max)) + deg();
        // Serial.print(frame.rain); Serial.print(" | " + frame.temp_range);
      }
      else
      {
        Serial.println("Parsing forecast error");
      }

      
      return true;
    }
    return false;
}

/// DISPLAY FUNCTIONS
void displayFrame()
{
    display.setTextSize(2);
    display.setCursor(69+2, 18); display.print(frame.temp);   
    display.setCursor(44+4, 38); display.print(frame.temp_range);

    int code = frame.weather_code;

    Serial.print("Rain: "); Serial.println(frame.rain);
    if(frame.rain >= 0.1 && code < 900)
    {
      display.drawBitmap(X_ICON, Y_ICON, umbrella_icon, W_ICON, H_ICON, WHITE); 
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(12, 24);
      display.print(String(frame.rain)); 
      display.setTextColor(WHITE);
    }
    else
    {    
      if (code == 800) // clear
      {
        display.drawBitmap(X_ICON, Y_ICON, clear_icon, W_ICON, H_ICON, WHITE);  
      }
      else if(code == 801 ) // partial cloud
      {
        display.drawBitmap(X_ICON, Y_ICON, partial_cloud_icon, W_ICON, H_ICON, WHITE); 
      }
      else if(code > 801 && code < 805) // cloud
      {
        display.drawBitmap(X_ICON, Y_ICON, cloud_icon, W_ICON, H_ICON, 1); 
      }
      else if(code >= 900 ) // Additional or extrime
      {
        display.drawBitmap(X_ICON, Y_ICON, alert_icon, W_ICON, H_ICON, WHITE); // TODO - CHANGE
      }    
      else if(code >= 700 && code < 800) // Atmosphere
      {
        display.drawBitmap(X_ICON, Y_ICON, hase_icon, W_ICON, H_ICON, WHITE); 
      }
      else if(code >= 200 && code < 700) // All sort of rain, show, thunder
      {
        display.drawBitmap(X_ICON, Y_ICON, rain_icon, W_ICON, H_ICON, WHITE); 
      }
      else // Unknown
      {
        display.drawBitmap(X_ICON, Y_ICON, unknown_icon, W_ICON, H_ICON, WHITE);  // TODO - CHANGE
      }
    }

    display.setTextSize(1);    
    display.setCursor(0, 56); display.print(frame.weather_str); 
    display.setCursor(104, 0); display.print(code);
    display.setCursor(10, 0);  display.print(frame.update_time);
}

void displayWiFi()
{
  if((WiFi.status() == WL_CONNECTED)) {
   display.drawBitmap(X_WIFI, Y_WIFI, wifi_connected, W_WIFI, H_WIFI, WHITE);  // TODO - CHANGE
  }
  else
  {
  display.drawBitmap(X_WIFI, Y_WIFI, wifi_error, W_WIFI, H_WIFI, WHITE);  // TODO - CHANGE   
  }

  display.setTextSize(1);  
  display.setCursor(104, 8);  
  display.print(uptime());
}

void displayDHT()
{
  display.setTextSize(1);  
  display.setCursor(0,8);  
  display.print(frame.dht_string);
}

void setContrast()
{
  int A0_read = analogRead(A0);
  uint8_t brightness = map(A0_read, 0, 1024, 0, 255);

  if (brightness >DISPLAY_OFF_LEVEL )
  {
    wakeDisplay();
   if (brightness > DISPLAY_DIM_LEVEL )
      display.dim(false);
    else
      display.dim(true);     
  }
  else  
     sleepDisplay();
   
  Serial.print("Photoresistor: "); Serial.print(A0_read); Serial.print(" - "); Serial.println(brightness);
}

void sleepDisplay() {
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  displayStatus = false;
}

void wakeDisplay() {
  if (!displayStatus)
  {
    display.ssd1306_command(SSD1306_DISPLAYON);
    displayStatus = true;
  }  
}

void setup()
{
    blinki(5);
    // Setup Serial
    Serial.begin(115200);
    delay(500); // Give some time to connest to the Serial
  
    pinMode(LED_BUILTIN,OUTPUT);
    // Setup DHT11
    dht.setup(2); // Connect DHT sensor to GPIO 2 which is D4
    //dht.setup(2); // Connect DHT sensor to GPIO 2 which is D4
    //dht_2.setup(13); // Connect temp DHT sensor to GPIO 13 which is D7

    //setup Graphix
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    // Clear the buffer.
    display.clearDisplay();
    display.display();
    display.setTextColor(WHITE);

   // symbolTable();

    // Setup WiFi   
    WiFi_coldrun = true; // Check if we just startes the device

    // Setup json    
    clearFrame();
    mf();
}

void loop()
{
    setContrast();
    blinki(2);
    digitalWrite(LED_BUILTIN,LOW);
    
    getDHT();
        
    if((WiFi.status() == WL_CONNECTED)) {

      if ((millis() >= respond_time + WAIT_TIME) || WiFi_coldrun) // We will wait first wait_time because millis and respond_time will be 0 on start
      {        
        Serial.print("WiFi update: " + String(millis()) + " >= " + String(respond_time + WAIT_TIME));  Serial.println("");
        //WiFi_isanswer = wifi_nowcast();         
        //mf();
        if (getNowcast())
        {
          respond_time = millis();
          WiFi_coldrun = false;

          getForecast(); // basic information is recived, get forecast           
        }
      }
    }

    display.clearDisplay();
    displayWiFi();
    displayFrame();
    displayDHT();
    display.display();
    
    mf();
    delay(UPDATE_TIME);       
}
