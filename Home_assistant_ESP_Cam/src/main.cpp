#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include "SD_MMC.h"


// กำหนดขา Serial2 ของ ESP32-CAM
#define RX_PIN 3   // U0R (GPIO3) ใช้อ่านจาก TX ของ ESP32
#define TX_PIN 1   // U0T (GPIO1) ใช้ส่งกลับไปหา ESP32 (ถ้าต้องการตอบ)

String botToken = "7290873016:AAHMe_2TrILCar166zWO9s4jJsuo9JZ8tzg";
String chatID = "7969041356";

const char* ssid = "กระจายบุญ";
const char* password = "25222524";

const char * photoPrefix = "/photo_";
int photoNumber = 0;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  Serial.begin(115200);         // Debug Monitor
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Serial2 สำหรับรับจาก ESP32
  Serial.println("ESP32-CAM ready to receive commands...");
  connectWiFi();

    camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  #if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  #endif

  // camera init
  esp_err_t err = esp_camera_init( & config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s -> id.PID == OV3660_PID) {
    s -> set_vflip(s, 1); // flip it back
    s -> set_brightness(s, 1); // up the brightness just a bit
    s -> set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s -> set_framesize(s, FRAMESIZE_QVGA);

  #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s -> set_vflip(s, 1);
  s -> set_hmirror(s, 1);
  #endif

  Serial.println("Initialising SD card");
  if (!SD_MMC.begin()) {
    Serial.println("Failed to initialise SD card!");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("SD card slot appears to be empty!");
    return;
  }

}

void loop() {
  if (Serial2.available()) {
    String command = Serial2.readStringUntil('\n');  // อ่านทีละบรรทัด
    command.trim(); // ตัดช่องว่าง/เว้นบรรทัดออก

    if (command.length() > 0) {
      Serial.print("Received command: ");
      Serial.println(command);

      // ตัวอย่าง: ตรวจสอบคำสั่ง
      if (command == "TAKE_PHOTO") {
        Serial.println("📷 Received request: TAKE_PHOTO");
        takePhoto();
      } 
      else if (command == "TAKE_PHOTO_AND_SEND") {
        Serial.println("📷 Received request: TAKE_PHOTO_AND_SEND");
        sendPhotoToTelegram(botToken, chatID);
      }
      else {
        Serial.println("❓ Unknown command");
      }
    }
  }
  checkWifi();
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
    digitalWrite(2, HIGH);
    delay(250);
    digitalWrite(2, LOW);
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

void takePhoto(){
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  savePhotoToSD(fb);
  esp_camera_fb_return(fb);
}

String sendPhotoToTelegram(String token, String chat_id) {
  const char* myDomain = "api.telegram.org";
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return "Camera capture failed";
  }

  WiFiClientSecure client;
  client.setInsecure(); // ข้ามการตรวจสอบใบรับรอง SSL

  if (!client.connect(myDomain, 443)) {
    Serial.println("Connection to Telegram failed");
    esp_camera_fb_return(fb);
    return "Connection failed";
  }

  // สร้าง header + tail
  String boundary = "ESP32CAM";
  String head = "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" +
                chat_id + "\r\n--" + boundary +
                "\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint16_t imageLen = fb->len;
  uint16_t totalLen = head.length() + tail.length() + imageLen;

  client.println("POST /bot" + token + "/sendPhoto HTTP/1.1");
  client.println("Host: " + String(myDomain));
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println();
  client.print(head);

  // ส่งภาพทีละ chunk
  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n += 1024) {
    if (n + 1024 < fbLen) client.write(fbBuf, 1024);
    else client.write(fbBuf, fbLen % 1024);
    fbBuf += 1024;
  }

  client.print(tail);

  savePhotoToSD(fb); 

  esp_camera_fb_return(fb); // คืน buffer

  // อ่าน response แบบง่าย ๆ
  String response = "";
  while (client.connected()) {
    if (client.available()) {
      response = client.readStringUntil('\n');
      break;
    }
  }

  client.stop();
  Serial.println("Telegram response: " + response);
  return response;
}

void savePhotoToSD(camera_fb_t * fb){
  String photoFileName = photoPrefix + String(photoNumber) + ".jpg";
  fs::FS & fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", photoFileName.c_str());

  File file = fs.open(photoFileName.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb -> buf, fb -> len);
    Serial.printf("Saved file to path: %s\n", photoFileName.c_str());
    ++photoNumber;
  }
  file.close();
}