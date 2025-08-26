1. Giới thiệu

Keypad 4x4 là loại bàn phím ma trận gồm 16 phím nhấn, thường được sử dụng trong các hệ thống nhúng (Arduino, ESP32, STM32, PIC,…) để nhập số liệu hoặc điều khiển menu. Keypad được kết nối với vi điều khiển thông qua các chân hàng (ROW) và cột (COL), theo nguyên tắc quét ma trận.

Ứng dụng điển hình: máy rút tiền ATM, hệ thống khóa cửa điện tử, hệ thống chấm công, menu điều khiển nhúng.

2. Cấu tạo và nguyên lý hoạt động
a. Cấu tạo

Số phím: 16 phím (0–9, A–D, *, #).

Cách bố trí: 4 hàng × 4 cột.

Số chân kết nối: 8 chân (R1–R4, C1–C4).

Công nghệ: Mỗi phím là một công tắc dạng momentary switch (nhấn giữ → đóng mạch, thả ra → mở mạch).

Mặt bàn phím:

   C1   C2   C3   C4
R1  1    2    3    A
R2  4    5    6    B
R3  7    8    9    C
R4  *    0    #    D

b. Nguyên lý hoạt động

Mỗi phím nhấn nối một hàng (ROW) với một cột (COL).

Vi điều khiển sẽ quét hàng – đọc cột:

Đặt mức logic LOW cho 1 hàng, giữ các hàng khác HIGH.

Đọc các chân cột. Nếu cột nào ở mức LOW → phím tại giao điểm hàng–cột đang được nhấn.

Lặp lại lần lượt cho tất cả hàng để phát hiện phím bấm.

Ví dụ:

Nhấn phím 6 → nối R2 với C3.

MCU đang kéo R2 xuống LOW, nếu C3 đọc được LOW → suy ra phím 6 được nhấn.

3. Thông số kỹ thuật (theo datasheet Keypad 4x4 phổ biến)
Thông số	Giá trị
Số phím	16 (4×4)
Số chân tín hiệu	8 (R1–R4, C1–C4)
Điện áp hoạt động	3.3V – 5V
Dòng tiêu thụ	< 1 mA (chỉ khi quét phím)
Tuổi thọ phím bấm	~ 1 triệu lần nhấn
Kích thước	7 × 7 cm (thông thường)
Chất liệu	Màng nhựa (plastic film) hoặc dạng module cứng
Giao tiếp	Song song (ma trận hàng–cột)
4. Sơ đồ chân (Pinout)

Keypad 4x4 có 8 chân, thường theo thứ tự:

[1] R1
[2] R2
[3] R3
[4] R4
[5] C1
[6] C2
[7] C3
[8] C4


⚠️ Lưu ý: Thứ tự chân có thể thay đổi tùy loại keypad, nên cần tham khảo datasheet đi kèm hoặc kiểm tra bằng đồng hồ đo thông mạch.

5. Kết nối với vi điều khiển

Ví dụ kết nối với ESP32 (trong code bạn đưa):

byte rowPins[ROWS] = {13, 14, 12, 27}; // R1–R4
byte colPins[COLS] = {26, 25, 33, 32}; // C1–C4


Sơ đồ kết nối:

R1 → GPIO13

R2 → GPIO14

R3 → GPIO12

R4 → GPIO27

C1 → GPIO26

C2 → GPIO25

C3 → GPIO33

C4 → GPIO32

6. Ưu điểm và hạn chế
Ưu điểm

Rẻ tiền, dễ mua.

Số lượng phím đủ cho nhiều ứng dụng (16 phím).

Dễ kết nối (chỉ cần 8 chân GPIO).

Có thư viện hỗ trợ (Keypad.h) → dễ lập trình.

Hạn chế

Không có hiển thị trực quan (phải kết hợp với LCD/OLED).

Nếu nhấn nhiều phím cùng lúc có thể gây lỗi quét (ghosting effect).

Dạng màng mỏng dễ hỏng nếu sử dụng nhiều.

7. Ứng dụng thực tế

Hệ thống khóa cửa bằng mật mã.

Máy bán hàng tự động, máy ATM.

Thiết bị nhập lệnh trong công nghiệp.

Hệ thống menu điều khiển (như trong đề tài RFID + Keypad).

8. Kết luận

Keypad 4x4 là thiết bị nhập liệu đơn giản, giá rẻ và dễ sử dụng. Với nguyên lý quét ma trận, nó chỉ cần 8 chân GPIO để đọc 16 phím, phù hợp cho các ứng dụng yêu cầu nhập liệu cơ bản. Trong hệ thống quản lý công việc với RFID, Keypad được dùng để chọn menu, ghi nhận thao tác làm việc, từ đó giúp người dùng tương tác với hệ thống hiệu quả hơn
