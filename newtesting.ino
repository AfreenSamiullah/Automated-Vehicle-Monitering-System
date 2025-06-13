#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_camera.h>
#include <mbedtls/base64.h>

// === CONFIGURATION === //

// Wi-Fi credentials
const char* ssid = "LCS CONTROLS"; // Your Wi-Fi SSID
const char* password = "LCSW!F!@)23"; // Your Wi-Fi password

// Flask server details
const char* server_host = "vision-api-flask-app-766077244496.us-central1.run.app";
const int https_port = 443;
const char* server_path = "/";

// Camera configuration for ESP32-CAM (AI-Thinker module)
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

#define FLASH_LED_PIN 4
#define TRIGGER_INPUT_PIN 13

// === GLOBAL VARIABLES === //

WiFiClientSecure client;
unsigned long last_time = 0;

// === SETUP === //

void setup() {
  pinMode(TRIGGER_INPUT_PIN, INPUT); // On ESP32
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(3000);
  digitalWrite(FLASH_LED_PIN, LOW);
  Serial.begin(115200);
  delay(3000);
  Serial.println("ESP32-CAM starting...");

  // Initialize camera
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
  config.frame_size = FRAMESIZE_SVGA; // 800x600
  config.jpeg_quality = 12; // 0-63, lower is higher quality
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(500);
  digitalWrite(FLASH_LED_PIN, LOW);
  delay(500);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(500);
  digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("\nWi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Skip SSL verification for simplicity (not recommended for production)
  client.setInsecure();
}

// === BASE64 ENCODING === //

String encode_base64(uint8_t* data, size_t len) {
  size_t encoded_len = 0;
  mbedtls_base64_encode(NULL, 0, &encoded_len, data, len); // Get required buffer size
  unsigned char* encoded = (unsigned char*)malloc(encoded_len);
  if (!encoded) {
    Serial.println("Base64 encode: Memory allocation failed");
    return "";
  }
  int ret = mbedtls_base64_encode(encoded, encoded_len, &encoded_len, data, len);
  if (ret != 0) {
    Serial.printf("Base64 encode failed: %d\n", ret);
    free(encoded);
    return "";
  }
  String result = String((char*)encoded);
  free(encoded);
  return result;
}

// === SEND IMAGE TO SERVER === //

void send_image_to_server() {
  // Capture image
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(2000);
  digitalWrite(FLASH_LED_PIN, LOW);
  delay(3000);
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.printf("Captured image: %u bytes\n", fb->len);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);
  digitalWrite(FLASH_LED_PIN, LOW);
  delay(200);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);
  digitalWrite(FLASH_LED_PIN, LOW);

  // Encode image to base64
  String base64_image = encode_base64(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  if (base64_image == "") {
    Serial.println("Base64 encoding failed");
    return;
  }
  Serial.println("Image encoded to base64");

  // Create JSON payload with only base64 image
  StaticJsonDocument<128> doc;
  doc["image"] = base64_image;
  String payload;
  serializeJson(doc, payload);

  // Send HTTPS POST request
  HTTPClient https;
  String url = String("https://") + server_host + server_path;
  https.begin(url);
  https.addHeader("Content-Type", "application/json");
  Serial.println("Sending POST request...");
  int http_code = https.POST(payload);
  if (http_code > 0) {
    Serial.printf("HTTP response code: %d\n", http_code);
    if (http_code == HTTP_CODE_OK) {
      String response = https.getString();
      Serial.println("Server response:");
      Serial.println(response);

      // Parse response
      StaticJsonDocument<256> resp_doc;
      DeserializationError error = deserializeJson(resp_doc, response);
      if (error) {
        Serial.printf("JSON parse error: %s\n", error.c_str());
      } else {
        const char* detected_text = resp_doc["detected_text"];
        const char* drive_url = resp_doc["drive_url"];
        Serial.printf("Detected text: %s\n", detected_text ? detected_text : "N/A");
        Serial.printf("Drive URL: %s\n", drive_url ? drive_url : "N/A");
      }
    }
  } else {
    Serial.printf("HTTP POST failed: %s\n", https.errorToString(http_code).c_str());
  }
  https.end();
}

// === LOOP === //

bool last_trigger_state = LOW;

void loop() {
  bool current_trigger_state = digitalRead(TRIGGER_INPUT_PIN);

  // Detect rising edge (LOW -> HIGH)
  if (current_trigger_state == HIGH && last_trigger_state == LOW) {
    Serial.println("Rising edge detected, capturing photo...");

    if (WiFi.status() == WL_CONNECTED) {
      send_image_to_server();
    } else {
      Serial.println("Wi-Fi disconnected, attempting to reconnect...");
      WiFi.reconnect();
      delay(5000);
    }
    delay(5000);
  }

  last_trigger_state = current_trigger_state;
  delay(10); // prevent CPU hogging
}
