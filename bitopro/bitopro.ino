#include <WiFi.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"

#define LED (2)

const char *ssid2 = "EN301_shie";
const char *password2 = "en301en301";
//const char *ssid2 = "DAZ";
//const char *password2 = "0966591107";
const char *ssid = "Darren_Wifi";
const char *password = "0966591107";
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
double prev_price =0;

void setup()
{
  int trial_count = 0;
  Serial.begin(115200);
  
  pinMode(LED,OUTPUT);
  lcd.begin(-1, -1);             
  //lcd.backlight();
  
  ledcSetup(0, 250, 8);   
  ledcAttachPin(13, 0);
  
  delay(1000);
  
  WiFi.begin(ssid, password);
  lcd.setCursor(0,0);
  lcd.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    if(trial_count++>10){
      WiFi.begin(ssid2, password2);
    }
    delay(1000);   
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");  
  lcd.clear();
}

void loop()
{ 
  if ((WiFi.status() == WL_CONNECTED))
  { //Check the current connection status
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("RSSI:");
    lcd.print(WiFi.RSSI());
    Serial.print("RSSI:");
    Serial.print(WiFi.RSSI());
    Serial.print("\r\n");
    HTTPClient http;

    http.setConnectTimeout(5000);
    http.begin("https://api.bitopro.com/v3/tickers/eth_twd"); //Specify the URL
    int httpCode = http.GET();                                //Make the request

    if (httpCode == 200)
    { //Check for the returning code

      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);

      StaticJsonDocument<512> JSONBuffer;

      if (deserializeJson(JSONBuffer, payload) == DeserializationError::Ok)
      {
        Serial.println("parsed success");
        String dataValue = JSONBuffer["data"];
        deserializeJson(JSONBuffer, dataValue);
        double price = JSONBuffer["lastPrice"];
        Serial.println(price);
        lcd.setCursor(0,0);
        lcd.print("ETH:     ");
        lcd.print(price,2);

        if(prev_price > price){          
          ledcWriteTone(0, 250);
          ledcWrite(0,127);    
        }else if(prev_price < price){          
          ledcWriteTone(0, 2000);
          ledcWrite(0,127);
        }
        prev_price = price;
        digitalWrite(LED,HIGH);
        delay(100);
        digitalWrite(LED,LOW);
        ledcWrite(0,0);
      }
    }

    else
    {
      Serial.print("HttpCode: ");
      Serial.print(httpCode);
      Serial.println("Error on HTTP request");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("HTTP Request");
      lcd.setCursor(10,1);
      lcd.print("Error");
    }

    http.end(); //Free the resources
  }
  else
  {
    Serial.println("WiFi Connection Error!");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Reconnecting...");
    WiFi.begin();
    int WLcount = 0;
    int UpCount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount < 200)
    {
      delay(100);
      Serial.printf(".");
      if (UpCount >= 60) // just keep terminal from scrolling sideways
      {
        UpCount = 0;
        Serial.printf("\n");
      }
      ++UpCount;
      ++WLcount;
    }
  }
}
