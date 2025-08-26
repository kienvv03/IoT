#include <Keypad.h>

// Khai báo số hàng và số cột
const byte ROWS = 4; 
const byte COLS = 4; 

// Bảng ánh xạ các phím (ma trận phím)
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Khai báo các chân Arduino nối với keypad
byte rowPins[ROWS] = {9, 8, 7, 6};   // R1, R2, R3, R4
byte colPins[COLS] = {5, 4, 3, 2};   // C1, C2, C3, C4

// Tạo đối tượng keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Biến lưu số nhập từ keypad
String input = "";

void setup() {
  Serial.begin(9600);
  Serial.println("=== DEMO KEYPAD 4x4 ===");
  Serial.println("Nhấn phím số (0-9), phím A-B-C-D để chọn chức năng.");
  Serial.println("Phím * để xóa, phím # để xác nhận.");
}

void loop() {
  char key = keypad.getKey();  // Đọc phím nhấn

  if (key) {  // Nếu có phím được nhấn
    Serial.print("Phím nhấn: ");
    Serial.println(key);

    if (key >= '0' && key <= '9') {
      // Nếu là số thì thêm vào chuỗi input
      input += key;
      Serial.print("Đã nhập: ");
      Serial.println(input);
    } 
    else if (key == '*') {
      // Xóa dữ liệu
      input = "";
      Serial.println("Đã xóa dữ liệu nhập!");
    } 
    else if (key == '#') {
      // Xác nhận dữ liệu
      Serial.print("Dữ liệu nhập hoàn tất: ");
      Serial.println(input);
      input = ""; // Reset sau khi xác nhận
    } 
    else if (key == 'A') {
      Serial.println("Chức năng A: Mở đèn");
    } 
    else if (key == 'B') {
      Serial.println("Chức năng B: Tắt đèn");
    } 
    else if (key == 'C') {
      Serial.println("Chức năng C: Bật còi");
    } 
    else if (key == 'D') {
      Serial.println("Chức năng D: Tắt còi");
    }
  }
}
