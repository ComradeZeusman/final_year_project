#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Configuration
const char* ssid = "zeusman";
const char* password = "Mada@12345678";
const char* serverName = "http://192.168.1.178:3000";

#define MAX_RETRIES 3
#define RETRY_DELAY 2000
#define SERVER_TIMEOUT 10000

// Function declarations
void startCameraServer();
void sendDataToServer(String data);
bool checkBootCount();
bool checkMongoDBConnection();

bool checkMongoDBConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected - can't check MongoDB");
        return false;
    }
    
    Serial.println("Checking MongoDB connection...");
    HTTPClient http;
    String url = String(serverName) + "/status";
    Serial.println("URL: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
        String response = http.getString();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            bool isConnected = doc["mongoConnected"] | false;
            http.end();
            return isConnected;
        }
    }
    
    http.end();
    return false;
}

bool checkBootCount() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected - can't check boot count");
        return false;
    }
    
    Serial.println("Checking boot count...");
    HTTPClient http;
    String url = String(serverName) + "/bootCount";
    Serial.println("URL: " + url);
    
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("HTTP Response code: %d\n", httpCode);
    
    if (httpCode > 0) {
        String response = http.getString();
        Serial.println("Response: " + response);
        
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (error) {
            Serial.print("JSON Parse failed: ");
            Serial.println(error.c_str());
            http.end();
            return false;
        }
        
        int bootCount = doc["bootCount"] | 0;
        Serial.printf("Boot count from server: %d\n", bootCount);
        http.end();
        return bootCount == 0;
    }
    
    Serial.println("Failed to get boot count");
    http.end();
    return false;
}

void sendDataToServer(String data) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Disconnected");
        return;
    }

    Serial.println("Sending data to server: " + data);
    HTTPClient http;
    String url = String(serverName) + "/data";
    Serial.println("URL: " + url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(SERVER_TIMEOUT);

    int retries = 0;
    int httpResponseCode = -1;

    while (retries < MAX_RETRIES) {
        httpResponseCode = http.POST(data);
        Serial.printf("Attempt %d - Response code: %d\n", retries + 1, httpResponseCode);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Server response: " + response);
            break;
        }
        
        if (retries < MAX_RETRIES - 1) {
            Serial.printf("Retrying in %d ms...\n", RETRY_DELAY);
            delay(RETRY_DELAY);
        }
        retries++;
    }

    if (httpResponseCode <= 0) {
        Serial.println("All connection attempts failed");
    }
    
    http.end();
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    // Camera configuration
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

    if(psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    // Configure camera sensor
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }
  // Configure camera sensor
s->set_framesize(s, FRAMESIZE_QVGA);  // Smaller frame size
s->set_quality(s, 12);
s->set_brightness(s, 1);
s->set_contrast(s, 1);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    int wifiRetries = 0;
    
    while (WiFi.status() != WL_CONNECTED && wifiRetries < MAX_RETRIES) {
        delay(RETRY_DELAY);
        Serial.print(".");
        wifiRetries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        
        // Check MongoDB connection
        int mongoRetries = 0;
        bool mongoConnected = false;
        
        while (!mongoConnected && mongoRetries < MAX_RETRIES) {
            mongoConnected = checkMongoDBConnection();
            if (!mongoConnected) {
                Serial.printf("MongoDB connection attempt %d failed\n", mongoRetries + 1);
                delay(RETRY_DELAY);
                mongoRetries++;
            }
        }
        
        if (mongoConnected) {
            Serial.println("MongoDB connected");
            
            // Check boot count and send first boot data if needed
            if (checkBootCount()) {
                StaticJsonDocument<200> doc;
                doc["firstBoot"] = true;
                doc["deviceId"] = WiFi.macAddress();
                doc["bootCount"] = 1;
                
                String jsonString;
                serializeJson(doc, jsonString);
                sendDataToServer(jsonString);
            }
            
            // Start camera server only after all checks pass
            startCameraServer();
            
            Serial.print("Camera Ready! Use 'http://");
            Serial.print(WiFi.localIP());
            Serial.println("' to connect");
        } else {
            Serial.println("Failed to connect to MongoDB - not starting camera server");
        }
    } else {
        Serial.println("\nWiFi connection failed");
    }
}

void loop() {
    // Check WiFi connection periodically
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Connection Lost - Attempting to reconnect");
        WiFi.begin(ssid, password);
    }
    delay(10000);
}