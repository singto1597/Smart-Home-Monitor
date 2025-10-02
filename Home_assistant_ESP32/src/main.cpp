#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>

#define IN_SWITCH_PIN 12
#define OUT_SWITCH_PIN 13

#define GAS_SENSOR_PIN 34
#define PIR_SENSOR_PIN 14

#define RED_LED_PIN 4
#define YELLOW_LED_PIN 2
#define GREEN_LED_PIN 15

#define BUZZER_PIN 18
#define DHT22_PIN 5

#define RX_PIN 16
#define TX_PIN 17

#define ADC_RESOLUTION 4095

String botToken = "8216612749:AAE_0eLRSXB5_kN-YEXyfhh2lmXBIkBTg74";
String chatID = "7969041356";

const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbzb6h6ei0SyeSQP1Sag8hLeZc_iStKUBuwNqvCAc-dPuyUUveXW3So-PdeZQO5apO4h/exec";

DHT dht(DHT22_PIN, DHT22);

LiquidCrystal_I2C lcd(0x27, 16, 2);

void connectWiFi();
void checkWifi();
void sendMessage(String message);
void sendDataToGoogleSheets();
void checkTelegramMessages();


const char* ssid = "กระจายบุญ";
const char* password = "25222524";

int numberOfPeople = 0;
int gasValue = 0;
bool pirState = false;

bool SurveillanceMode = false;
bool nightMode = false;

float temperature = 0.0;
float humidity = 0.0;

unsigned long lastTriggerTime = 0;
unsigned long cooldown = 10000;

unsigned long previousMillis = 0;
const unsigned long interval = 60000; 

unsigned long lastBotTime = 0;
const unsigned long botInterval = 10000;
long lastUpdateId = 0;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial2.setPins(RX_PIN, TX_PIN);
  delay(1000);

  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Home Assistant");

  pinMode(IN_SWITCH_PIN, INPUT);
  pinMode(OUT_SWITCH_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  connectWiFi();
  sendMessage("Smart Home Monitor is Ready");
  lcd.clear();
  lcd.setCursor(0, 0);
}

void loop() {
  if (digitalRead(IN_SWITCH_PIN) == HIGH) {
    numberOfPeople++;
    tone(BUZZER_PIN, 1000);
    delay(200);
  }
  if (digitalRead(OUT_SWITCH_PIN) == HIGH) {
    numberOfPeople--;
    if (numberOfPeople < 0) {
      numberOfPeople = 0;
    }
    tone(BUZZER_PIN, 800);
    delay(200);
  }

  if (numberOfPeople == 0) {
    SurveillanceMode = true;
  } else {
    SurveillanceMode = false;
  }

  pirState = digitalRead(PIR_SENSOR_PIN);
  gasValue = analogRead(GAS_SENSOR_PIN);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (pirState) {
    unsigned long now = millis();
    if (now - lastTriggerTime > cooldown) {
      lastTriggerTime = now;
      
      if (nightMode || SurveillanceMode) {
        digitalWrite(RED_LED_PIN, HIGH);
        digitalWrite(GREEN_LED_PIN, LOW);
        tone(BUZZER_PIN, 500);
        delay(200);
        noTone(BUZZER_PIN);

        digitalWrite(RED_LED_PIN, LOW);
        Serial2.println("TAKE_PHOTO_AND_SEND");
      } else {
        digitalWrite(RED_LED_PIN, LOW);
        digitalWrite(GREEN_LED_PIN, HIGH);
        delay(200);
        digitalWrite(GREEN_LED_PIN, LOW);

        Serial2.println("TAKE_PHOTO");
      }
    }
  }
  else {
    digitalWrite(RED_LED_PIN, LOW);
    tone(BUZZER_PIN, 0);
  }
  if (nightMode || SurveillanceMode) {
    digitalWrite(YELLOW_LED_PIN, HIGH);
  } else {
    digitalWrite(YELLOW_LED_PIN, LOW);
  }
  
  if (gasValue > 3500) {
    sendMessage("Gas leak detected! Value: " + String(gasValue));
    tone(BUZZER_PIN, 1200);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(1000);
    tone(BUZZER_PIN, 0);
    digitalWrite(RED_LED_PIN, LOW);
  }
  
  checkWifi();
  if (millis() - lastBotTime > botInterval) {
    checkTelegramMessages();
    lastBotTime = millis();
  }
  lcd.setCursor(0, 0);
  lcd.printf("In:%d ", numberOfPeople);
  lcd.setCursor(8, 0);
  lcd.printf("Gas:%4d", gasValue);
  lcd.setCursor(0, 1);
  lcd.printf("T:%2.1fC H:%2.1f%%", temperature, humidity);
  
}


void sendMessage(String message) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String encodedMessage = urlEncode(message);
    String url = "https://api.telegram.org/bot" + botToken +
                 "/sendMessage?chat_id=" + chatID +
                 "&text=" + encodedMessage;
    http.begin(url);
    int httpResponseCode = http.GET();
    
    if(httpResponseCode > 0) {
      Serial.println("ส่งข้อความสำเร็จ: " + String(httpResponseCode));
      Serial.println(http.getString());
    } else {
      Serial.println("ไม่สามารถส่งข้อความได้, รหัสข้อผิดพลาด: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("ไม่ได้เชื่อมต่อ Wi-Fi");
  }
}

void checkWifi(){
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);

  }
  else if (WiFi.status() == WL_CONNECTED){

  }
}

void connectWiFi() {;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(ssid, password);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(250);
    digitalWrite(YELLOW_LED_PIN, LOW);
    delay(250);
    Serial.print(".");    
    attempts++;
    
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
  } else {
    Serial.println("\nFailed to connect.");
    delay(1000);
  }
}


void checkTelegramMessages() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Checking Msg...");
  lcd.setCursor(0, 1);
  lcd.print("Wait a moment");
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?offset=" + String(lastUpdateId + 1);
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);

      StaticJsonDocument<2048> doc; 
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        JsonArray result = doc["result"].as<JsonArray>();
        for (JsonObject msg : result) {
          lastUpdateId = msg["update_id"].as<long>();

          String text = msg["message"]["text"].as<String>();
          String chat_id = msg["message"]["chat"]["id"].as<String>();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(text);
          lcd.setCursor(0, 1);
          lcd.print("Sending...");
          if (text == "/status") {
            String reply = "People: " + String(numberOfPeople) +
                           "\nGas: " + String(gasValue) +
                           "\nTemp: " + String(temperature) + "C" +
                           "\nHumidity: " + String(humidity) + "%";
            sendMessage(reply);
          }
          else if (text == "/nightmode") {
            nightMode = !nightMode;
            sendMessage("Night mode toggled: " + String(nightMode ? "ON" : "OFF"));
          }
        }
      }
    }
    http.end();
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Success");
  lcd.clear();
  lcd.setCursor(0, 0);
}