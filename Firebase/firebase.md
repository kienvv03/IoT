Báo cáo tích hợp Firebase trong hệ thống quản lý công việc dựa trên RFID
Giới thiệu
Báo cáo này trình bày việc tích hợp Google Firebase vào một hệ thống quản lý công việc sử dụng thẻ RFID, bàn phím và màn hình TFT, được phát triển trên nền tảng ESP32. Hệ thống cho phép công nhân đăng nhập, thực hiện các công việc (lắp ráp, sửa chữa, đóng gói), ghi nhận kết quả công việc (hoàn thành hoặc lỗi), và lưu trữ dữ liệu vào cơ sở dữ liệu thời gian thực của Firebase. Báo cáo tập trung vào các chức năng liên quan đến Firebase, cách dữ liệu được lưu trữ và truy xuất, cũng như cấu trúc của hệ thống.
Mục tiêu
Hệ thống nhằm:

Quản lý thông tin công nhân và trạng thái công việc thông qua thẻ RFID.
Ghi nhận và lưu trữ dữ liệu công việc (hoàn thành, lỗi) vào Firebase.
Cung cấp giao diện người dùng trên màn hình TFT và bàn phím để tương tác.
Đồng bộ thời gian thực với cơ sở dữ liệu Firebase để theo dõi và quản lý dữ liệu.

Tích hợp Firebase
Firebase Realtime Database được sử dụng để lưu trữ và truy xuất dữ liệu công nhân, trạng thái, và lịch sử công việc. Các thư viện liên quan bao gồm HTTPClient và ArduinoJson được sử dụng để giao tiếp với Firebase thông qua giao thức HTTPS.
Cấu hình Firebase

URL cơ sở dữ liệu: https://esp32-166c4-default-rtdb.firebaseio.com
Khóa API: Được sử dụng để xác thực các yêu cầu HTTP.
Giao thức: HTTPS với các yêu cầu GET và POST để truy xuất và cập nhật dữ liệu.

Các hàm liên quan đến Firebase

fetchWorkerData(String uidStr): Truy xuất thông tin công nhân từ Firebase dựa trên UID của thẻ RFID. Hàm gửi yêu cầu GET đến /workers/{uidStr}.json và phân tích JSON trả về để lấy tên công nhân và trạng thái công việc khả dụng.
sendWorkerStatus(String uidStr, String state): Gửi trạng thái của công nhân (check-in hoặc check-out) cùng với thời gian hiện tại đến /workers/{uidStr}/status.json.
sendTaskData(String uidStr, String taskName, int completed, int failed): Ghi nhận số lượng công việc hoàn thành và lỗi cho một công việc cụ thể vào /history/{uidStr}.json.
sendCheckOutData(String uidStr): Gửi tóm tắt công việc (số lượng hoàn thành và lỗi cho từng công việc) khi công nhân check-out vào /history/{uidStr}.json.

Cấu trúc dữ liệu trên Firebase
Dữ liệu được tổ chức trong Firebase Realtime Database như sau:

/workers/{uidStr}:
name: Tên công nhân (chuỗi).
tasks: Đối tượng chứa thông tin công việc khả dụng (lapRap, suaChua, dongGoi).
status: Trạng thái công nhân (checkIn hoặc checkOut) cùng thời gian.


/history/{uidStr}:
Lịch sử công việc với các thuộc tính: timestamp, task, completed, failed.
Dữ liệu check-out: Tóm tắt công việc (lapRap, suaChua, dongGoi) với số lượng hoàn thành và lỗi.



Cơ chế hoạt động
Hệ thống sử dụng giao thức HTTP để giao tiếp với Firebase:

Kết nối WiFi: ESP32 kết nối với mạng WiFi để truy cập Firebase.
Đồng bộ thời gian: Sử dụng NTPClient để lấy thời gian từ pool.ntp.org, đảm bảo dấu thời gian chính xác.
Xác thực RFID: Công nhân quét thẻ RFID để đăng nhập. UID được gửi đến Firebase để xác minh và lấy thông tin.
Ghi nhận công việc: Công nhân chọn công việc qua bàn phím, ghi nhận số lượng hoàn thành/lỗi, và dữ liệu được gửi lên Firebase.
Check-out: Tóm tắt công việc được gửi lên Firebase, sau đó hệ thống trở về trạng thái ban đầu.

Ưu điểm và hạn chế
Ưu điểm

Dữ liệu được lưu trữ thời gian thực, dễ dàng truy cập và quản lý từ xa.
Giao diện người dùng trên màn hình TFT thân thiện, hỗ trợ tiếng Việt.
Xác thực RFID đảm bảo tính bảo mật và chính xác trong việc theo dõi công nhân.

Hạn chế

Phụ thuộc vào kết nối WiFi; nếu mất kết nối, dữ liệu không thể được gửi hoặc truy xuất.
Không có cơ chế lưu trữ cục bộ khi mất kết nối mạng.
Xử lý lỗi HTTP cơ bản, cần cải thiện để quản lý các trường hợp lỗi phức tạp hơn.

Kết luận
Tích hợp Firebase vào hệ thống quản lý công việc dựa trên RFID cung cấp một giải pháp hiệu quả để theo dõi và lưu trữ dữ liệu công việc trong thời gian thực. Hệ thống tận dụng các tính năng của Firebase Realtime Database để lưu trữ thông tin công nhân và lịch sử công việc một cách có tổ chức. Tuy nhiên, cần cải thiện khả năng xử lý lỗi và hỗ trợ lưu trữ cục bộ để tăng độ tin cậy. Trong tương lai, có thể tích hợp thêm các tính năng như thông báo thời gian thực hoặc phân tích dữ liệu công việc.
