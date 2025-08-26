#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Keypad.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define SS_PIN 5    // GPIO 5 cho MFRC522 SDA
#define RST_PIN 22  // GPIO 22 cho MFRC522 RST
#define TFT_CS 15   // GPIO 15 cho ILI9341 CS
#define TFT_DC 2    // GPIO 2 cho ILI9341 DC
#define TFT_RST 4   // GPIO 4 cho ILI9341 RST
#define SPI_MOSI 23 // VSPI_MOSI
#define SPI_MISO 19 // VSPI_MISO
#define SPI_SCK 18  // VSPI_SCK
#define MAX_UID_LENGTH 7
#define WIFI_SSID "Kvv"
#define WIFI_PASSWORD "66668888"
#define FIREBASE_API_KEY "AIzaSyC3tOt-MRumNTeDClxxvhQQ05mLlVhDBjo"
#define FIREBASE_URL "https://esp32-166c4-default-rtdb.firebaseio.com"

MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7*3600, 60000);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {13, 14, 12, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

enum State {
  CHECK_IN,
  MAIN_MENU,
  WORK_MENU,
  TASK_MENU,
  WORKING,
  CHECK_OUT
};
State currentState = CHECK_IN;

struct Task {
  int completed = 0;
  int failed = 0;
  bool available = false;
};
Task lapRap, suaChua, dongGoi;
byte currentUID[MAX_UID_LENGTH];
byte uidLength = 0;
String workerName = "";
bool rfidCheckedIn = false;
bool showSummaryDisplayed = false;
Task* currentTask = nullptr;
const char* currentTaskName = nullptr;

// Hàm in chuỗi với tự động xuống dòng
void printWrapped(String text, int x, int y, uint16_t color, uint8_t textSize) {
  tft.setTextColor(color);
  tft.setTextSize(textSize);
  int maxChars = (textSize == 1) ? 53 : 26; // 320 pixel / (6 hoặc 12 pixel/ký tự)
  int lineHeight = (textSize == 1) ? 12 : 20; // Khoảng cách dòng
  int currentY = y;

  while (text.length() > 0) {
    String line = text.substring(0, maxChars);
    if (text.length() > maxChars) {
      int lastSpace = line.lastIndexOf(' ');
      if (lastSpace > 0 && lastSpace < maxChars) {
        line = text.substring(0, lastSpace);
        text = text.substring(lastSpace + 1);
      } else {
        text = text.substring(maxChars);
      }
    } else {
      text = "";
    }
    tft.setCursor(x, currentY);
    tft.println(line);
    currentY += lineHeight;
    if (currentY >= 240) break; // Ngăn tràn màn hình
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  rfid.PCD_Init();
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1); // Thay từ 2 thành 1
  tft.setCursor(0, 0);
  tft.println("=== Check in ===");
  tft.setCursor(0, 12);
  tft.println("Nhan phim 1 de quet the");
  Serial.println("=== Check in ===");
  Serial.println("Nhấn phím 1 để quét thẻ");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println(" Connected!");
  timeClient.begin();
}

String getTime() {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&epochTime));
  return String(buf);
}

String byteArrayToString(byte *uid, byte uidLength) {
  String uidStr = "";
  for (byte i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }
  uidStr.toUpperCase();
  return uidStr;
}

bool readRFID(byte *uid, byte *uidLength) {
  unsigned long startTime = millis();
  while (millis() - startTime < 30000) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      *uidLength = rfid.uid.size;
      if (*uidLength > MAX_UID_LENGTH) *uidLength = MAX_UID_LENGTH;
      for (byte i = 0; i < *uidLength; i++) {
        uid[i] = rfid.uid.uidByte[i];
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return true;
    }
  }
  return false;
}

bool fetchWorkerData(String uidStr) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi khong ket noi!");
    return false;
  }
  HTTPClient http;
  String url = String(FIREBASE_URL) + "/workers/" + uidStr + ".json?auth=" + FIREBASE_API_KEY;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    if (!doc.isNull()) {
      workerName = doc["name"].as<String>();
      if (workerName.length() > 50) workerName = workerName.substring(0, 47) + "..."; // Cập nhật cho textSize(1)
      lapRap.available = doc["tasks"]["lapRap"]["available"] | false;
      suaChua.available = doc["tasks"]["suaChua"]["available"] | false;
      dongGoi.available = doc["tasks"]["dongGoi"]["available"] | false;
      http.end();
      return true;
    }
  }
  Serial.print("HTTP GET that bai, ma loi: "); Serial.println(httpCode);
  http.end();
  return false;
}

void sendWorkerStatus(String uidStr, String state) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi khong ket noi, khong gui trang thai!");
    return;
  }
  HTTPClient http;
  String url = String(FIREBASE_URL) + "/workers/" + uidStr + "/status.json?auth=" + FIREBASE_API_KEY;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  DynamicJsonDocument doc(256);
  doc["state"] = state;
  doc["timestamp"] = getTime();
  String payload;
  serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("HTTP POST trang thai that bai, ma loi: "); Serial.println(httpCode);
  }
  http.end();
}

void sendTaskData(String uidStr, String taskName, int completed, int failed) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi khong ket noi, khong gui du lieu!");
    return;
  }
  HTTPClient http;
  String url = String(FIREBASE_URL) + "/history/" + uidStr + ".json?auth=" + FIREBASE_API_KEY;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  DynamicJsonDocument doc(256);
  doc["timestamp"] = getTime();
  doc["task"] = taskName;
  doc["completed"] = completed;
  doc["failed"] = failed;
  String payload;
  serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("HTTP POST that bai, ma loi: "); Serial.println(httpCode);
  }
  http.end();
}

void sendCheckOutData(String uidStr) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi khong ket noi, khong gui du lieu!");
    return;
  }
  HTTPClient http;
  String url = String(FIREBASE_URL) + "/history/" + uidStr + ".json?auth=" + FIREBASE_API_KEY;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  DynamicJsonDocument doc(512);
  doc["timestamp"] = getTime();
  doc["checkOut"]["lapRap"]["completed"] = lapRap.completed;
  doc["checkOut"]["lapRap"]["failed"] = lapRap.failed;
  doc["checkOut"]["suaChua"]["completed"] = suaChua.completed;
  doc["checkOut"]["suaChua"]["failed"] = suaChua.failed;
  doc["checkOut"]["dongGoi"]["completed"] = dongGoi.completed;
  doc["checkOut"]["dongGoi"]["failed"] = dongGoi.failed;
  String payload;
  serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("HTTP POST that bai, ma loi: "); Serial.println(httpCode);
  }
  http.end();
}

void showMainMenu() {
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
  tft.setTextSize(1); // Thay từ 2 thành 1
  tft.setCursor(0, 0);
  printWrapped("Ten: " + workerName, 0, 0, ILI9341_WHITE, 1);
  tft.setCursor(0, 24);
  tft.println("1. Bat dau lam viec");
  tft.setCursor(0, 36);
  tft.println("2. Check out");
  Serial.print("Tên: "); Serial.println(workerName);
  Serial.println("1. Bắt đầu làm việc");
  Serial.println("2. Check out");
}

void showWorkMenu() {
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
  tft.setTextSize(2); // Thay từ 3 thành 2
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("=== Chon cong viec ===");
  tft.setTextSize(1); // Thay từ 2 thành 1
  tft.setCursor(0, 20);
  if (lapRap.available) tft.println("1. Lap rap");
  tft.setCursor(0, 32);
  if (suaChua.available) tft.println("2. Sua chua");
  tft.setCursor(0, 44);
  if (dongGoi.available) tft.println("3. Dong goi");
  tft.setCursor(0, 56);
  tft.println("*. Quay lai");
  Serial.println("=== Chọn công việc ===");
  if (lapRap.available) Serial.println("1. Lắp ráp");
  if (suaChua.available) Serial.println("2. Sửa chữa");
  if (dongGoi.available) Serial.println("3. Đóng gói");
  Serial.println("*. Quay lại");
}

void showTaskMenu(const char* taskName) {
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
  tft.setTextSize(2); // Thay từ 3 thành 2
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  String title = String("=== ") + taskName + " ===";
  printWrapped(title, 0, 0, ILI9341_WHITE, 2);
  tft.setTextSize(1); // Thay từ 2 thành 1
  tft.setCursor(0, 24);
  tft.println("1. Bat dau lam viec");
  tft.setCursor(0, 36);
  tft.println("2. Xem so cong viec hoan thanh");
  tft.setCursor(0, 48);
  tft.println("3. Xem so loi");
  tft.setCursor(0, 60);
  tft.println("*. Quay lai");
  Serial.print("=== "); Serial.print(taskName); Serial.println(" ===");
  Serial.println("1. Bắt đầu làm việc");
  Serial.println("2. Xem số công việc hoàn thành");
  Serial.println("3. Xem số lỗi");
  Serial.println("*. Quay lại");
}

void showWorkingMenu(const char* taskName) {
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
  tft.setTextSize(2); // Thay từ 3 thành 2
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  String title = String("=== ") + taskName + " - Lam viec ===";
  printWrapped(title, 0, 0, ILI9341_WHITE, 2);
  tft.setTextSize(1); // Thay từ 2 thành 1
  tft.setCursor(0, 24);
  tft.println("1. Hoan thanh");
  tft.setCursor(0, 36);
  tft.println("2. Loi");
  tft.setCursor(0, 48);
  tft.println("*. Quay lai");
  Serial.print("=== "); Serial.print(taskName); Serial.println(" - Làm việc ===");
  Serial.println("1. Hoàn thành");
  Serial.println("2. Lỗi");
  Serial.println("*. Quay lại");
}

void showSummary() {
  tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
  tft.setTextSize(2); // Thay từ 3 thành 2
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_CYAN);
  tft.println("=== Tong quan cong viec ===");
  tft.setTextSize(1); // Thay từ 2 thành 1
  tft.setCursor(0, 20);
  printWrapped("Lap rap - Hoan thanh: " + String(lapRap.completed) + ", Loi: " + String(lapRap.failed), 0, 20, ILI9341_CYAN, 1);
  tft.setCursor(0, 44);
  printWrapped("Sua chua - Hoan thanh: " + String(suaChua.completed) + ", Loi: " + String(suaChua.failed), 0, 44, ILI9341_CYAN, 1);
  tft.setCursor(0, 68);
  printWrapped("Dong goi - Hoan thanh: " + String(dongGoi.completed) + ", Loi: " + String(dongGoi.failed), 0, 68, ILI9341_CYAN, 1);
  tft.setCursor(0, 92);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Nhan 0 de tiep tuc");
  Serial.println("=== Tổng quan công việc ===");
  Serial.print("Lắp ráp - Hoàn thành: "); Serial.print(lapRap.completed); Serial.print(", Lỗi: "); Serial.println(lapRap.failed);
  Serial.print("Sửa chữa - Hoàn thành: "); Serial.print(suaChua.completed); Serial.print(", Lỗi: "); Serial.println(suaChua.failed);
  Serial.print("Đóng gói - Hoàn thành: "); Serial.print(dongGoi.completed); Serial.print(", Lỗi: "); Serial.println(dongGoi.failed);
  Serial.println("Nhấn 0 để tiếp tục");
}

void loop() {
  char key = keypad.getKey();
  if (!key && currentState != CHECK_IN && currentState != CHECK_OUT && currentState != WORK_MENU && currentState != TASK_MENU && currentState != WORKING) return;

  switch (currentState) {
    case CHECK_IN:
      if (key == '1') {
        byte uid[MAX_UID_LENGTH];
        byte length;
        tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(ILI9341_YELLOW);
        tft.setTextSize(2); // Thay từ 3 thành 2
        tft.println("Dang quet the...");
        Serial.println("Đang quét thẻ...");
        if (readRFID(uid, &length)) {
          String uidStr = byteArrayToString(uid, length);
          if (fetchWorkerData(uidStr)) {
            memcpy(currentUID, uid, length);
            uidLength = length;
            rfidCheckedIn = true;
            sendWorkerStatus(uidStr, "checkIn");
            currentState = MAIN_MENU;
            showMainMenu();
          } else {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Khong tim thay cong nhan!");
            tft.setCursor(0, 12);
            tft.println("Nhan phim 1 de quet lai");
            Serial.println("Không tìm thấy công nhân!");
            Serial.println("Nhấn phím 1 để quét lại");
          }
        } else {
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_MAGENTA);
          tft.setTextSize(1); // Thay từ 2 thành 1
          tft.println("Khong doc duoc the...");
          tft.setCursor(0, 12);
          tft.println("Nhan phim 1 de quet lai");
          Serial.println("Không đọc được thẻ...");
          Serial.println("Nhấn phím 1 để quét lại");
        }
      } else if (key) {
        tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(ILI9341_MAGENTA);
        tft.setTextSize(1); // Thay từ 2 thành 1
        tft.println("Phim khong hop le!");
        tft.setCursor(0, 12);
        tft.println("Nhan phim 1 de quet the");
        Serial.println("Phím không hợp lệ!");
        Serial.println("Nhấn phím 1 để quét thẻ");
      }
      break;

    case MAIN_MENU:
      switch (key) {
        case '1':
          currentState = WORK_MENU;
          showWorkMenu();
          break;
        case '2':
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_YELLOW);
          tft.setTextSize(1); // Thay từ 2 thành 1
          tft.println("Dang check out...");
          tft.setCursor(0, 12);
          tft.println("Nhan phim 1 de quet the");
          Serial.println("Đang check out...");
          Serial.println("Nhấn phím 1 để quét thẻ");
          currentState = CHECK_OUT;
          showSummaryDisplayed = false;
          break;
        default:
          if (key) {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Phim khong hop le!");
            showMainMenu();
            Serial.println("Phím không hợp lệ!");
            Serial.println("1. Bắt đầu làm việc");
            Serial.println("2. Check out");
          }
          break;
      }
      break;

    case WORK_MENU:
      switch (key) {
        case '1':
          if (lapRap.available) {
            currentTask = &lapRap;
            currentTaskName = "Lap rap";
            currentState = TASK_MENU;
            showTaskMenu(currentTaskName);
          } else {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Cong viec khong kha dung!");
            showWorkMenu();
            Serial.println("Công việc không khả dụng!");
            Serial.println("Chọn lại công việc.");
          }
          break;
        case '2':
          if (suaChua.available) {
            currentTask = &suaChua;
            currentTaskName = "Sua chua";
            currentState = TASK_MENU;
            showTaskMenu(currentTaskName);
          } else {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Cong viec khong kha dung!");
            showWorkMenu();
            Serial.println("Công việc không khả dụng!");
            Serial.println("Chọn lại công việc.");
          }
          break;
        case '3':
          if (dongGoi.available) {
            currentTask = &dongGoi;
            currentTaskName = "Dong goi";
            currentState = TASK_MENU;
            showTaskMenu(currentTaskName);
          } else {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Cong viec khong kha dung!");
            showWorkMenu();
            Serial.println("Công việc không khả dụng!");
            Serial.println("Chọn lại công việc.");
          }
          break;
        case '*':
          currentState = MAIN_MENU;
          showMainMenu();
          break;
        default:
          if (key) {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Phim khong hop le!");
            showWorkMenu();
            Serial.println("Phím không hợp lệ!");
            Serial.println("Chọn lại công việc.");
          }
          break;
      }
      break;

    case TASK_MENU:
      switch (key) {
        case '1':
          currentState = WORKING;
          showWorkingMenu(currentTaskName);
          break;
        case '2': {
          String completedStr = String(currentTaskName) + ": Hoan thanh: " + String(currentTask->completed);
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_CYAN);
          tft.setTextSize(1); // Thay từ 2 thành 1
          printWrapped(completedStr, 0, 0, ILI9341_CYAN, 1);
          tft.setCursor(0, 24);
          tft.println("Nhan bat ky phim de quay lai");
          Serial.print(currentTaskName); Serial.print(": Hoàn thành: "); Serial.println(currentTask->completed);
          Serial.println("Nhấn bất kỳ phím để quay lại");
          break;
        }
        case '3': {
          String failedStr = String(currentTaskName) + ": Loi: " + String(currentTask->failed);
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_MAGENTA);
          tft.setTextSize(1); // Thay từ 2 thành 1
          printWrapped(failedStr, 0, 0, ILI9341_MAGENTA, 1);
          tft.setCursor(0, 24);
          tft.println("Nhan bat ky phim de quay lai");
          Serial.print(currentTaskName); Serial.print(": Lỗi: "); Serial.println(currentTask->failed);
          Serial.println("Nhấn bất kỳ phím để quay lại");
          break;
        }
        case '*':
          currentState = MAIN_MENU;
          showMainMenu();
          break;
        default:
          if (key) {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Phim khong hop le!");
            showTaskMenu(currentTaskName);
            Serial.println("Phím không hợp lệ!");
            Serial.println("Chọn lại.");
          }
          break;
      }
      break;

    case WORKING:
      switch (key) {
        case '1':
          if (currentTask) {
            currentTask->completed++;
            String uidStr = byteArrayToString(currentUID, uidLength);
            String taskName = (currentTask == &lapRap) ? "lapRap" : (currentTask == &suaChua) ? "suaChua" : "dongGoi";
            sendTaskData(uidStr, taskName, currentTask->completed, currentTask->failed);
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_GREEN);
            tft.setTextSize(2); // Thay từ 3 thành 2
            String completedMsg = "Da hoan thanh 1 SP - " + String(currentTaskName);
            printWrapped(completedMsg, 0, 0, ILI9341_GREEN, 2);
            showWorkingMenu(currentTaskName);
            Serial.print("Đã hoàn thành 1 sản phẩm - "); Serial.println(currentTaskName);
          }
          break;
        case '2':
          if (currentTask) {
            currentTask->failed++;
            String uidStr = byteArrayToString(currentUID, uidLength);
            String taskName = (currentTask == &lapRap) ? "lapRap" : (currentTask == &suaChua) ? "suaChua" : "dongGoi";
            sendTaskData(uidStr, taskName, currentTask->completed, currentTask->failed);
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(2); // Thay từ 3 thành 2
            String failedMsg = "Da ghi nhan 1 loi - " + String(currentTaskName);
            printWrapped(failedMsg, 0, 0, ILI9341_MAGENTA, 2);
            showWorkingMenu(currentTaskName);
            Serial.print("Đã ghi nhận 1 lỗi - "); Serial.println(currentTaskName);
          }
          break;
        case '*':
          currentState = WORK_MENU;
          showWorkMenu();
          break;
        default:
          if (key) {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Phim khong hop le!");
            showWorkingMenu(currentTaskName);
            Serial.println("Phím không hợp lệ!");
            Serial.println("Chọn lại.");
          }
          break;
      }
      break;

    case CHECK_OUT:
      if (!showSummaryDisplayed) {
        if (key == '1') {
          byte uid[MAX_UID_LENGTH];
          byte length;
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_YELLOW);
          tft.setTextSize(1); // Thay từ 2 thành 1
          tft.println("Dang quet the de check out...");
          tft.setCursor(0, 12);
          tft.println("Nhan phim 1 de quet the");
          Serial.println("Đang quét thẻ để check out...");
          if (readRFID(uid, &length)) {
            if (length == uidLength && memcmp(uid, currentUID, length) == 0) {
              String uidStr = byteArrayToString(uid, length);
              sendCheckOutData(uidStr);
              sendWorkerStatus(uidStr, "checkOut");
              tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
              tft.setCursor(0, 0);
              tft.setTextColor(ILI9341_GREEN);
              tft.setTextSize(2); // Thay từ 3 thành 2
              tft.println("Check out thanh cong");
              showSummary();
              Serial.println("Check out thành công");
            } else {
              tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
              tft.setCursor(0, 0);
              tft.setTextColor(ILI9341_MAGENTA);
              tft.setTextSize(1); // Thay từ 2 thành 1
              tft.println("The khong khop voi the check-in!");
              tft.setCursor(0, 12);
              tft.println("Nhan phim 1 de quet the");
              Serial.println("Thẻ không khớp với thẻ check-in!");
              Serial.println("Nhấn phím 1 để quét thẻ");
            }
          } else {
            tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_MAGENTA);
            tft.setTextSize(1); // Thay từ 2 thành 1
            tft.println("Khong doc duoc the...");
            tft.setCursor(0, 12);
            tft.println("Nhan phim 1 de quet the");
            Serial.println("Không đọc được thẻ...");
            Serial.println("Nhấn phím 1 để quét thẻ");
          }
        } else if (key) {
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_MAGENTA);
          tft.setTextSize(1); // Thay từ 2 thành 1
          tft.println("Phim khong hop le!");
          tft.setCursor(0, 12);
          tft.println("Nhan phim 1 de quet the");
          Serial.println("Phím không hợp lệ!");
          Serial.println("Nhấn phím 1 để quét thẻ");
        }
      } else {
        if (key == '0') {
          rfidCheckedIn = false;
          currentTask = nullptr;
          currentTaskName = nullptr;
          lapRap.completed = 0; lapRap.failed = 0;
          suaChua.completed = 0; suaChua.failed = 0;
          dongGoi.completed = 0; dongGoi.failed = 0;
          workerName = "";
          currentState = CHECK_IN;
          showSummaryDisplayed = false;
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_WHITE);
          tft.setTextSize(1); // Thay từ 2 thành 1
          tft.println("=== Check in ===");
          tft.setCursor(0, 12);
          tft.println("Nhan phim 1 de quet the");
          Serial.println("=== Check in ===");
          Serial.println("Nhấn phím 1 để quét thẻ");
        } else if (key) {
          tft.fillRect(0, 0, 320, 240, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_MAGENTA);
          tft.setTextSize(1); // Thay từ 2 thành 1
          tft.println("Phim khong hop le!");
          tft.setCursor(0, 12);
          tft.println("Nhan '0' de tiep tuc.");
          Serial.println("Phím không hợp lệ!");
          Serial.println("Nhấn '0' để tiếp tục.");
          showSummary();
        }
      }
      break;
  }
}
