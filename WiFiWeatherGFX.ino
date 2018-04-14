
// Graphics
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "my_icons.h"
#define X_ICON 2
#define Y_ICON 16
#define W_ICON 40
#define H_ICON 40

#define X_WIFI 0
#define Y_WIFI 0
#define W_WIFI 8
#define H_WIFI 8

// Display
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

#include <SPI.h>
#include <Wire.h>

// DH11 Sensor
#include <DHTesp.h>
#include <math.h>
DHTesp dht;
DHTesp dht_2;

// WiFi
#include <ESP8266WiFi.h>
// #include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

const char* ssid = "Vinovinovino";
const char* password = "Pucci1981";
const char* nowcast_url  = "http://api.openweathermap.org/data/2.5/weather?q=Marina%20Del%20Rey,us&mode=json&APPID=de3b9d61b8be217fc92029bcc69780aa&units=metric";
const char* forecast_url = "http://api.openweathermap.org/data/2.5/forecast?q=Marina%20Del%20Rey,us&mode=json&APPID=de3b9d61b8be217fc92029bcc69780aa&units=metric";
//const char* forecast_url = "http://api.openweathermap.org/data/2.5/weather?q=Los%20Angeles,us&mode=json&APPID=de3b9d61b8be217fc92029bcc69780aa&units=metric";


HTTPClient http;
// ESP8266WiFiMulti WiFiMulti;
unsigned long respond_time = 0; // time that store the request moment
unsigned long wait_time = 1000; // How ofter do the request

//json
#include <ArduinoJson.h>
String nowcast_respond, forecast_respond;

// For testing icons
//int codes[]={0,800,801,802,-10,700,701,900,200,201,0,0,0,0,0};
//int counter=0;

bool WiFi_isconnected, WiFi_isanswer, WiFi_coldrun;

// Time
#include <Time.h>
#include <TimeLib.h>

//time_t weatherTime;

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

// Not in use
uint8_t* get_icon(const char* bits)
{
  // Convert bitmap into uint8_t bitmap
  // This is very unefficient way to do stuff
  
  uint8_t *bitmap;  
  bitmap = new uint8_t[sizeof(bits)];

  memcpy(bitmap, bits, sizeof(bits));

  //for( int i=0; i <  sizeof(bits); i++)
 // {
  //  bitmap[i] = uint8_t(bits[i]);
 // }
  
  return bitmap;
}

void symbolTable()
{
   // Symbol test
    display.setCursor(0,0);
    for (int i=0; i< 1000; i++)
    {  
      display.print(i);
      display.print(" ");
      display.write(i);  
      display.print(" ");    
      if ( (i % 3) == 0 ) display.println(); 
      if ( (i % 24) == 0 ){
        display.display();
        delay(8000);
        display.setCursor(0,0);
        display.clearDisplay();
      }     
      
    }
    // Symbol test end
}

String deg()
{
  // Return degree symbol 
  return String(char(247));
  //return String(char(265));
  //return String(char(9));
}

String uptime()
{
   // Return uptime string
   unsigned long uptime = (millis()- respond_time) / 1000; 
   return String(uptime);
}

void textDHT()
{
    // Display text from DHT sensors
  
    // Sensor 1
    float humidity = round(dht.getHumidity());
    float temperature = round(dht.getTemperature());

    String temperature_str = String(int(temperature));
    String humidity_str = String(int(humidity));
    String Sensor1 = "" + temperature_str + deg() + "," + humidity_str + "%";

    // Sensor 2
    humidity = round(dht_2.getHumidity());
    temperature = round(dht_2.getTemperature());

    temperature_str = String(int(temperature));
    humidity_str = String(int(humidity));
    String Sensor2 = "2:" + temperature_str + deg() +", " + humidity_str + "%";

    String time_str = String(millis());

    display.setTextSize(1);  
    display.setCursor(0,8);  display.print(dht.getStatusString());
    if (dht.getStatus() == 0)
    {
      display.print(" DH11 ");
      display.print(Sensor1);
      display.print(" " + uptime());    
    //display.println(Sensor2);
    //display.setCursor(0,0);
    //display.println(time_str);
    }
}

void textWeather()
{
  // Display text from Weather
  // Parse Json string and put info on display
  Serial.println("Processing the nowcast");
  //Serial.println(nowcast_respond);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(nowcast_respond);

  if(root.success())
  {
    JsonObject& weather = root["weather"][0];
    JsonObject& main    = root["main"];
  
    // Temperature
    const char* __main = weather["main"];
    float temp     = main["temp"];    
    float temp_max = main["temp_max"];    
    float temp_min = main["temp_min"];    
    
    temp     = round(temp);
    temp_max = round(temp_max);
    temp_min = round(temp_min);

    // Serial.println(__main);
    display.setTextSize(2);
    display.setCursor(69+2, 18); display.print(temp,0); display.print(deg());   
    display.setCursor(44+4, 38); 
    display.print(temp_min,0);display.print("-");display.print(temp_max,0); display.print(deg());   
    /*display.setCursor(80, 16);//32 48
                               display.print(temp_max,0); display.print(deg());   
    display.setCursor(80, 32); display.print(temp,0); display.print(deg());   
    display.setCursor(80, 48); display.print(temp_min,0); display.print(deg());   
    */

    // Icon
    //int code = root["cod"];
    int code = weather["id"];
    


    // For testing icons
    //code = codes[counter++];
    //if(counter > 10) counter = 0;
    
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

    display.setTextSize(1);
    display.setCursor(0, 56); display.print(__main); 
    display.setCursor(104, 0);  display.print(code);

    time_t weatherTime = root["dt"];
    display.setCursor(10, 0);     
    display.print(timeStr(weatherTime));
     }
  else
  {
    display.setTextSize(2);
    display.setCursor(71, 18);
    display.print("???");
    Serial.println("root parsing error");
  }  
}

void parseForecast()
{
  // Parse forecast string
  Serial.println("Processing the forecast");
  //Serial.println(forecast_respond);
  //mf();
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(forecast_respond);
  //mf();
  if(root.success())
  {
   
    //JsonObject& list = root["list"]; // get the list

    Serial.println("Getting size:");
    int walk = min(4, int(root["list"].size()));
   
    Serial.println("Size: " + String(root["list"].size()));
  
    for (int i = 0; i<walk; i++)
    {
      JsonObject& weather = root["list"][i];
      weather.printTo(Serial);
      Serial.println();
    }
  }
  else
  {
    Serial.println("Parsing failed");
    
  }
  
}

void textWiFi()
{
  //display.setCursor(0,0);
  //display.setTextSize(1);
  if((WiFi.status() == WL_CONNECTED)) {
  //display.print("ONLINE");
   display.drawBitmap(X_WIFI, Y_WIFI, wifi_connected, W_WIFI, H_WIFI, WHITE);  // TODO - CHANGE
  }
  else
  {
  //display.print("ERROR");  
  display.drawBitmap(X_WIFI, Y_WIFI, wifi_error, W_WIFI, H_WIFI, WHITE);  // TODO - CHANGE
   
  }
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

bool wifi_nowcast()
{
    // Get current weather
    return wifi_get(nowcast_url, &nowcast_respond);  
}

bool wifi_forecast()
{
    // Get weather forecast
    //forecast_respond = wifi_get_string(forecast_url); 
    //mf();  
    //return (forecast_respond.length()>0);   
    bool recived = wifi_get(forecast_url, &forecast_respond);
    if(recived)
    {
      int count = 0;
      for(int c=0; c<5; c++) // find only 4 items 
      {
        count = forecast_respond.indexOf("\"dt\"", count+1);        
      }
      //mf();
      if(count > 2)
      forecast_respond = forecast_respond.substring(0, count-2) + "]}";
      //mf();
      //Serial.println(forecast_respond);
    }
    
    return recived;
}

String wifi_get_string(const char* u) { return String(); }

bool wifi_get(const char* url, String * answer)
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

String timeStr(time_t t)
{
  // Return string time from time
  // ToDo - return for LA, maybe with clock...
  String timeStr;  
  
  //timeStr = String(year(t))+"."+String(month(t))+"."+String(day(t))+" "+String(hour(t))+":"+String(minute(t))+":"+String(second(t));
  timeStr = String(monthShortStr(month(t)))+" "+String(day(t))+" "+String(hour(t))+":"+String(minute(t));
  return timeStr;
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
    dht_2.setup(13); // Connect temp DHT sensor to GPIO 13 which is D7

    //setup Graphix
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    // Clear the buffer.
    display.clearDisplay();
    display.setTextColor(WHITE);

   // symbolTable();

    // Setup WiFi   
    WiFi_coldrun = true; // Check if we just startes the device

    // Setup json    

    mf();
}

void loop()
{
    //mf();   
    display.clearDisplay();
    textWiFi();
    textDHT();
    blinki(1);
    digitalWrite(LED_BUILTIN,HIGH);        
    
    if((WiFi.status() == WL_CONNECTED)) {

      if ((millis() >= respond_time + wait_time) || WiFi_coldrun) // We will wait first wait_time because millis and respond_time will be 0 on start
      {        
        Serial.print("WiFi update: " + String(millis()) + " >= " + String(respond_time + wait_time));  Serial.println("");
        WiFi_isanswer = wifi_nowcast();         
        //mf();
        if (WiFi_isanswer)
        {
          respond_time = millis();
          WiFi_coldrun = false;

          // basic information is recived, get forecast           
          if (wifi_forecast()){ parseForecast(); }
          //mf();
          //else{}
        }
      }
      // if aswer - json put on display
      // if not - continue
    }
    else
    {
      // Connecting
      WiFi_isconnected = wifi_connect();
      // Connected
      // Or not connected
    }
    
   if (WiFi_isanswer)  
   {
    textWeather();
   }
    blinki(2);    
    display.display();
    mf();
    delay(1000);    
    
}
