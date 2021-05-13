#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"

extern "C"
{
#include "crypto/base64.h"
}
#include "mbedtls/md.h"

#define LED (2)
#define BUTTON_PIN (18)

String serverName = "https://api.bitopro.com/v3";
const char *userName = "crazypatoto@gmail.com";
const char *apiKey = "0787acb2920e12548712fba483f878f8";
const char *apiSecret = "$2a$12$vO.1ftM34Vogzder7QtLYewOH3TqvvQupFJTalBQx00iKBw9dSkPi";

const char *ntpServer = "time.google.com";
// const char *ssid = "IVAM_Darren";
// const char *password = "0966591107";
const char *ssid = "IVAM_Darren";
const char *password = "0966591107";
const char *ssid2 = "Darren_Wifi";
const char *password2 = "0966591107";
double prev_price = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

time_t getTimestamp()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time from NTP Server");
  }
  time_t now;
  time(&now);
  return now;
}

int64_t getTimestampMilliSeconds()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time from NTP Server");
  }
  time_t now;
  time(&now);
  return (int64_t)now * (int64_t)1000 + millis() % 1000;
}

void getHMACSHA384(const char *key, const char *payload, byte *hmacResult)
{
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA384;
  const size_t payloadLength = strlen(payload);
  const size_t keyLength = strlen(key);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1); //1 for HMAC
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'a' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'a' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}

void removeNewLine(unsigned char *input)
{
  for (size_t i = 0; i < strlen((char *)input); i++)
  {
    if (input[i] == '\n')
    {
      //puts("\\n DETECTED!!");
      for (size_t j = i; j < strlen((char *)input); j++)
      {
        input[j] = input[j + 1];
      }
    }
  }
}

void IRAM_ATTR button_ISR()
{
  static uint32_t prev_pressed_time = 0;
  if (xTaskGetTickCount() - prev_pressed_time > 200)
  {
    Serial.println("button pressed");
    prev_pressed_time = xTaskGetTickCount();
  }
}

void setup()
{
  int trial_count = 0;
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(BUTTON_PIN, button_ISR, FALLING);
  pinMode(LED, OUTPUT);
  Serial.begin(115200);

  lcd.begin(-1, -1);
  //lcd.backlight();

  ledcSetup(0, 250, 8);
  ledcAttachPin(13, 0);

  delay(1000);

  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    if (trial_count++ > 10)
    {
      WiFi.begin(ssid2, password2);
      delay(1000);
      Serial.println("Connecting to WiFi2..");
      continue;
    }
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  lcd.clear();

  Serial.println("Configuring NTP Server");
  configTime(0, 0, ntpServer);

  Serial.println(getCurrencyPrice("btc"));
}

void loop()
{
  if ((WiFi.status() == WL_CONNECTED))
  { //Check the current connection status
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("RSSI:");
    lcd.print(WiFi.RSSI());
    //Serial.print("RSSI:");
    //Serial.print(WiFi.RSSI());
    //Serial.print("\r\n");

    HTTPClient http;
    http.setConnectTimeout(10000);
    //String path = serverName + "/tickers/btc_usdt";
    String path = serverName + "/accounts/balance";
    http.begin(path); //Specify the URL

    DynamicJsonDocument bitopro_payload_json(1024);
    bitopro_payload_json["identity"] = userName;
    bitopro_payload_json["nonce"] = getTimestampMilliSeconds();
    unsigned char bitopro_payload[100];
    serializeJson(bitopro_payload_json, bitopro_payload);
    //serializeJson(bitopro_payload_json, Serial);

    size_t bitopro_payload_encoded_length;
    unsigned char *bitopro_payload_encoded = base64_encode((const unsigned char *)bitopro_payload, strlen((const char *)bitopro_payload), &bitopro_payload_encoded_length);
    removeNewLine(bitopro_payload_encoded);

    byte bitopro_signature_bytes[48];
    getHMACSHA384(apiSecret, (const char *)bitopro_payload_encoded, bitopro_signature_bytes);
    char bitopro_signature[97];
    array_to_string(bitopro_signature_bytes, 48, bitopro_signature);

    http.addHeader(F("X-BITOPRO-APIKEY"), apiKey, true, false);
    http.addHeader(F("X-BITOPRO-PAYLOAD"), (char *)bitopro_payload_encoded, false, false);
    http.addHeader(F("X-BITOPRO-SIGNATURE"), bitopro_signature, false, false);

    //    Serial.println((char *)bitopro_payload_encoded);
    //    Serial.println(bitopro_signature);
    free(bitopro_payload_encoded);

    //Serial.println("Start http.GET()");
    int httpCode = http.GET(); //Make the request
    //Serial.println("Finsih");

    //    String payload = http.getString();
    //    Serial.print("HttpCode: ");
    //    Serial.println(httpCode);
    //    Serial.print("Payload: ");
    //    Serial.println(payload);
    if (httpCode == 200)
    { //Check for the returning code

      String payload = http.getString();
      //Serial.println(httpCode);
      //Serial.println(payload);

      StaticJsonDocument<2048> JSONBuffer;
      StaticJsonDocument<64> filter;
      filter["data"][0]["amount"] = true;
      filter["data"][0]["currency"] = true;

      if (deserializeJson(JSONBuffer, payload, DeserializationOption::Filter(filter)) == DeserializationError::Ok)
      {
        Serial.println("parsed success");
        JsonArray array = JSONBuffer["data"].as<JsonArray>();
        int total = 0;
        for (JsonVariant v : array) {
          double amount = v["amount"];
          if (amount > 0) {
            String currency = v["currency"];
            Serial.print(currency);
            Serial.print(": ");
            Serial.println(amount, 5);
            total += getCurrencyPrice(currency) * amount;
          }
        }
        Serial.println(total);
                lcd.setCursor(0, 0);
                //lcd.print("BTC:   ");
                lcd.print(total);
        //Serial.println(dataValues);
        //deserializeJson(JSONBuffer, dataValues);

        //        double price = JSONBuffer["lastPrice"];
        //        Serial.println(price);
        //        lcd.setCursor(0, 0);
        //        lcd.print("BTC:   ");
        //        lcd.print(price, 2);

        //        if (prev_price > price)
        //        {
        //          //ledcWriteTone(0, 250);
        //          //ledcWrite(0,5);
        //        }
        //        else if (prev_price < price)
        //        {
        //          //ledcWriteTone(0, 1000);
        //          //ledcWrite(0,5);
        //        }
        //        prev_price = price;
        //        digitalWrite(LED, HIGH);
        //        delay(100);
        //        digitalWrite(LED, LOW);
        //        ledcWrite(0, 0);
      }
    }

    else
    {
      //Serial.println("Error on HTTP request");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HTTP Request");
      lcd.setCursor(10, 1);
      lcd.print("Error");
    }
    http.end(); //Free the resources
  }
  else
  {
    Serial.println("WiFi Connection Error!");
    lcd.clear();
    lcd.setCursor(0, 0);
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

double getCurrencyPrice(String currency) {
  String path = serverName + "/tickers/" + currency + "_twd";
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.begin(path); //Specify the URL
  int httpCode = http.GET(); //Make the request
  if (httpCode == 200)//Check for the returning code
  {
    String payload = http.getString();
    StaticJsonDocument<128> JSONBuffer;
    StaticJsonDocument<32> filter;
    filter["data"]["lastPrice"] = true;
    if (deserializeJson(JSONBuffer, payload, DeserializationOption::Filter(filter)) == DeserializationError::Ok)
    {
      Serial.println("parse ok");
      double price = JSONBuffer["data"]["lastPrice"];
      return price;
    }
  }
  Serial.println(http.getString());
  return -1.0f;
}
