1. Giới thiệu chung

RFID (Radio Frequency Identification) là công nghệ nhận dạng đối tượng bằng sóng vô tuyến.

RC522 là một module RFID phổ biến, giá rẻ, hoạt động ở tần số 13.56 MHz, tương thích với chuẩn ISO/IEC 14443A (các thẻ MIFARE).

Thường được sử dụng trong: chấm công, kiểm soát ra vào, hệ thống thanh toán, khóa cửa thông minh, quản lý sản phẩm,...

2. Thông số kỹ thuật

Chip điều khiển: MFRC522 (NXP).

Nguồn cấp: 3.3V (có thể dùng 5V qua mạch chuyển mức logic).

Giao tiếp: SPI (chính), I²C, UART.

Tần số hoạt động: 13.56 MHz.

Khoảng cách đọc: 0–6 cm (tùy loại thẻ, anten, nguồn cấp).

Dòng tiêu thụ: 13–26 mA khi hoạt động, <80 µA ở chế độ standby.

Kích thước module: ~40mm x 60mm.

Chuẩn thẻ hỗ trợ: MIFARE 1K, 4K, Ultralight, DESFire, và các thẻ tương thích ISO14443A.

3. Cấu tạo phần cứng

Anten PCB: cuộn dây in sẵn trên module, dùng để phát và thu sóng RF.

Chip MFRC522: xử lý tín hiệu RF, mã hóa/giải mã dữ liệu, giao tiếp với vi điều khiển.

Header 8 chân: thường dùng với giao tiếp SPI:

SDA (SS) – Chip select

SCK – Clock

MOSI – Master Out Slave In

MISO – Master In Slave Out

IRQ – Ngắt (ít dùng)

GND – Mass

RST – Reset

3.3V – Nguồn

4. Nguyên lý hoạt động

Nguồn phát: RC522 phát sóng RF 13.56 MHz từ anten.

Thẻ RFID (không nguồn hoặc có nguồn) nhận năng lượng từ sóng này để hoạt động.

Trao đổi dữ liệu: thẻ phản hồi lại thông tin (UID, dữ liệu bộ nhớ) qua cảm ứng điện từ.

RC522 giải mã tín hiệu và truyền dữ liệu UID về vi điều khiển (Arduino, ESP32, STM32,...).

Ứng dụng xử lý: so sánh UID, lưu log, mở khóa, tính công,...

5. Ứng dụng thực tế

Chấm công nhân viên, sinh viên.

Khóa cửa điện tử (quẹt thẻ để mở).

Hệ thống bãi giữ xe thông minh.

Thanh toán điện tử không tiếp xúc.

Quản lý sản phẩm trong kho (gắn thẻ RFID).

6. Ví dụ ứng dụng với Arduino
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Quet the RFID...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
  
  Serial.print("UID the: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  rfid.PICC_HaltA();  
  rfid.PCD_StopCrypto1();
}


👉 Chương trình trên đọc UID của thẻ RFID và in ra Serial Monitor.

7. Ưu điểm và hạn chế
Ưu điểm:

Giá thành rẻ, dễ mua.

Khoảng cách đọc đủ dùng (≤6cm).

Hỗ trợ nhiều chuẩn thẻ RFID.

Giao tiếp SPI/I²C/UART linh hoạt.

Hạn chế:

Khoảng cách đọc ngắn.

Bảo mật chưa cao (nếu chỉ dùng UID thẻ).

Dễ bị nhiễu nếu đặt gần kim loại hoặc thiết bị phát sóng khác.

8. Kết luận

Module RFID RC522 là lựa chọn phù hợp cho các ứng dụng giá rẻ – phổ biến – dễ tích hợp. Với khả năng đọc/ghi thẻ MIFARE, giao tiếp linh hoạt, RC522 được sử dụng rộng rãi trong học tập, nghiên cứu, và các ứng dụng IoT như kiểm soát ra vào, chấm công, quản lý kho bãi.
