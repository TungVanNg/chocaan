# 🐠 DỰ ÁN CHO CÁ ĂN TỰ ĐỘNG BẰNG ESP32

Hệ thống giúp bạn cho cá ăn bằng 3 cách:
- ✋ Cảm biến chạm (TTP223)
- 📲 Điều khiển từ xa bằng app Blynk
- ⏰ Hẹn giờ tự động mỗi ngày

> Tự làm tại nhà – chi phí thấp – dễ nạp code – dễ đấu nối

---

## 🔧 PHẦN CỨNG CẦN CHUẨN BỊ

| Tên linh kiện        | Số lượng | Ghi chú                         |
|----------------------|----------|---------------------------------|
| ESP32 DevKit V1      | 1        | Board điều khiển chính          |
| Servo 360 độ (SG90)  | 1        | Dùng để xoay khay thức ăn       |
| Cảm biến chạm TTP223 | 1        | Kích hoạt bằng tay              |
| Dây nối, breadboard  | Tùy      | Kết nối linh kiện               |
| Nguồn 5V qua USB     | 1        | Cấp nguồn cho ESP32 & Servo     |

---

## 📱 CÀI ĐẶT APP BLYNK (Phiên bản 2.0 – Blynk IoT)

### Bước 1: Tạo tài khoản  
- Vào: [https://blynk.cloud](https://blynk.cloud)  
- Hoặc tải app "Blynk IoT" trên điện thoại

### Bước 2: Tạo Template
- **Template Name:** `ESP32_FishFeeder`
- **Hardware:** `ESP32`
- **Connection Type:** `WiFi`
- Lưu lại `BLYNK_TEMPLATE_ID`, `BLYNK_DEVICE_NAME`

### Bước 3: Tạo thiết bị (Device)
- Từ Template vừa tạo → Add Device
- Sau khi tạo xong, lấy `Auth Token`

### Bước 4: Thêm widget trong app Blynk
| Widget     | Virtual Pin | Mục đích                     |
|------------|-------------|------------------------------|
| Slider 1   | `V8`        | Điều chỉnh lượng thức ăn (200–200ms) |
| Slider 2   | `V9`        | Điều chỉnh tốc độ servo (90–180 độ)  |
| Nút Manual | `V2`        | Bấm để cho ăn từ xa (nếu thích)      |

---

## 🧠 KHAI BÁO TRONG CODE

Thêm vào code Arduino:

```cpp

#define BLYNK_TEMPLATE_ID "YourTemplateID"
#define BLYNK_DEVICE_NAME "ESP32_FishFeeder"
#define BLYNK_AUTH_TOKEN "YourAuthToken"

char ssid[] = "Tên WiFi";
char pass[] = "Mật khẩu WiFi";
