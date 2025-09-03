#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>

#define IN_SWITCH_PIN 12
#define OUT_SWITCH_PIN 13

#define GAS_SENSOR_PIN 34
#define PIR_SENSOR_PIN 35

#define RED_LED_PIN 4
#define YELLOW_LED_PIN 2
#define GREEN_LED_PIN 15

#define BUZZER_PIN 18
#define DHT22_PIN 5

#define RX_PIN 16
#define TX_PIN 17

#define ADC_RESOLUTION 4095

String botToken = "7290873016:AAHMe_2TrILCar166zWO9s4jJsuo9JZ8tzg";
String chatID = "7969041356";

const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbxJml6kIWjmjraKd3SkN68tFN-4r4lITzbSk5Nua2th0zKBcdPc3aa3GVl73-hY0klf/exec";

DHT dht(DHT22_PIN, DHT22);

liquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "กระจายบุญ";
const char* password = "25222524";

int numberOfPeople = 0;
int gasValue = 0;
bool pirState = false;

bool SurveillanceMode = false;
bool nightMode = false;

float temperature = 0.0;
float humidity = 0.0;

void setup() {
  Serial2.begin(9600);
  Serial2.setPins(RX_PIN, TX_PIN);
  delay(1000);

  dht.begin();

  lcd.begin();
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

  // Connect to WiFi
  connectWiFi();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(IN_SWITCH_PIN) == HIGH) {
    numberOfPeople++;
  }
  if (digitalRead(OUT_SWITCH_PIN) == HIGH) {
    numberOfPeople--;
    if (numberOfPeople < 0) {
      numberOfPeople = 0;
    }
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
    if (nightMode || SurveillanceMode) {
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);

      Serial2.println("TAKE_PHOTO_AND_SEND");
    } else {
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);

      Serial2.println("TAKE_PHOTO");
    }
  } 
  if (nightMode || SurveillanceMode) {
    digitalWrite(YELLOW_LED_PIN, HIGH);
  } else {
    digitalWrite(YELLOW_LED_PIN, LOW);
  }

  if (gasValue > 2000) {
    sendMessage("Gas leak detected! Value: " + String(gasValue));
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  checkWifi();


}


void sendMessage(String message) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
    http.begin(url);
    int httpResponseCode = http.GET();
    
    if(httpResponseCode > 0) {
      Serial.println("ส่งข้อความสำเร็จ: " + String(httpResponseCode));
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
    
    // แสดงความคืบหน้า
    /*
    display.fillRect(0, 10, SCREEN_WIDTH, 10, BLACK);
    display.setCursor(0, 10);
    display.printf("Attempt %d/20", attempts + 1);
    display.display();*/
    
    attempts++;
    
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
  } else {
    Serial.println("\nFailed to connect.");
    delay(1000);
  }
}

void sendDataToGoogleSheets(String time, float temperature, float tds, bool oxygenPumpState, bool heaterState, bool coolerState, bool filterState) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Prepare JSON payload
    String jsonPayload = "{\"time\":\"" + time +
                         "\",\"temperature\":" + String(temperature) +
                         ",\"tds\":" + String(tds) +
                         ",\"oxygenPumpState\":" + (oxygenPumpState ? "true" : "false") +
                         ",\"heaterState\":" + (heaterState ? "true" : "false") +
                         ",\"coolerState\":" + (coolerState ? "true" : "false") +
                         ",\"filterState\":" + (filterState ? "true" : "false") + "}";

    http.begin(googleScriptURL);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully to Google Sheets.");
      Serial.println(http.getString());
    } else {
      Serial.print("Error sending data: ");
      Serial.println(httpResponseCode);
    }


    http.end();
  } else {
    Serial.println("Wi-Fi not connected. Unable to send data.");
  }
}