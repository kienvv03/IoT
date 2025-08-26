#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

#define WIFI_SSID "Kvv" 
#define WIFI_PASSWORD "66668888" 

#define FIREBASE_HOST "https://esp32-166c4-default-rtdb.firebaseio.com"
#define FIREBASE_API_KEY "AIzaSyC3tOt-MRumNTeDClxxvhQQ05mLlVhDBjo"

#define SS_PIN 21
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

unsigned long lastScanTime = 0;
const unsigned long scanInterval = 2000; 

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID RC522 ready.");

  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 30) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.println("IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("Failed to connect to WiFi");
    return;
  }

  
  const char* ntpServers[] = {"pool.ntp.org", "asia.pool.ntp.org", "time.google.com", "time.windows.com"};
  bool ntpSynced = false;
  for (int i = 0; i < 4; i++) {
    configTime(7 * 3600, 0, ntpServers[i]);
    Serial.print("Trying NTP server: ");
    Serial.println(ntpServers[i]);
    delay(5000); 
    time_t now = time(nullptr);
    if (now > 100000) {
      Serial.println("NTP time synced: " + String(ctime(&now)));
      ntpSynced = true;
      break;
    }
  }
  if (!ntpSynced) {
    Serial.println("Failed to sync NTP time with all servers. Using default timestamp.");
  }
}

void loop() {
  if (millis() - lastScanTime >= scanInterval) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      // Đọc ID thẻ
      String tagID = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        tagID += String(rfid.uid.uidByte[i], HEX);
      }
      tagID.toUpperCase();
      Serial.println("Tag ID: " + tagID);

      
      time_t now = time(nullptr);
      String timeStr;
      if (now < 100000) {
        timeStr = "2025-07-09T14:40:00"; 
        Serial.println("Using default timestamp");
      } else {
        char timeBuf[20];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", localtime(&now));
        timeStr = String(timeBuf);
      }

      
      String path = "/scans/scan_" + String(millis()) + ".json?auth=" + FIREBASE_API_KEY;
      String url = String(FIREBASE_HOST) + path;
      String payload = "{\"tag_id\":\"" + tagID + "\",\"timestamp\":\"" + timeStr + "\"}";

      
      HTTPClient http;
      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      Serial.print("Sending data to Firebase: " + path);
      int httpCode = http.PATCH(payload);
      if (httpCode == HTTP_CODE_OK) {
        Serial.println(" - Success");
      } else {
        Serial.println(" - Failed, HTTP code: " + String(httpCode));
        Serial.println("Response: " + http.getString());
      }
      http.end();

      lastScanTime = millis();
      rfid.PICC_HaltA(); 
    }
  }
}

