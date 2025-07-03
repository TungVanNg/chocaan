/*
 * HỆ THỐNG CHO CÁ ĂN THÔNG MINH - PHIÊN BẢN TỐI ƯU SERVO + TIẾT KIỆM BLYNK QUOTA
 * Sử dụng: ESP32 + Servo SG90 360° + HC-SR04 + Blynk + Hẹn giờ tự động
 * Tác giả: TUNG VAN
 * Ngày: 2025
 * 
 * ✨ TÍNH NĂNG MỚI:
 * - Servo quay nhanh và chính xác vị trí
 * - Điều khiển lượng thức ăn tối ưu
 * - Có thể điều chỉnh góc quay và thời gian
 * - TIẾT KIỆM TỐI ĐA BLYNK QUOTA MESSAGES
 */

// ======= THÔNG TIN BLYNK (PHẢI KHAI BÁO TRƯỚC KHI INCLUDE) =======
#define BLYNK_TEMPLATE_ID "TMPL6nmuVb1ro"
#define BLYNK_TEMPLATE_NAME "chocaan"
#define BLYNK_AUTH_TOKEN "B6kvGj0s7VeVwEoLMr04InI3voHwJu1v"

// ======= THƯ VIỆN CẦN THIẾT =======
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <time.h>

// ======= THÔNG TIN WIFI =======
char ssid[] = "Homie 10/5";     // Tên WiFi
char pass[] = "motdentam"; // Mật khẩu WiFi

// ======= CẤU HÌNH THỜI GIAN =======
#define GMT_OFFSET_SEC 7*3600    // UTC+7 (Việt Nam)
#define DAYLIGHT_OFFSET_SEC 0    // Không có giờ mùa hè
const char* ntpServer = "pool.ntp.org";

// ======= ĐỊNH NGHĨA CHÂN KẾT NỐI =======
#define SERVO_PIN 23    // Servo 360° nối với GPIO 23
#define TRIG_PIN 5      // HC-SR04 TRIG nối với GPIO 5
#define ECHO_PIN 18     // HC-SR04 ECHO nối với GPIO 18

// ======= ĐỊNH NGHĨA VIRTUAL PIN BLYNK =======
#define V_AUTO_MODE 0    // V0: Switch bật/tắt chế độ cảm biến tự động
#define V_DISTANCE 1     // V1: Hiển thị khoảng cách
#define V_MANUAL_FEED 2  // V2: Button cho ăn thủ công
#define V_SCHEDULE_MODE 3 // V3: Switch bật/tắt chế độ hẹn giờ
#define V_MORNING_TIME 4  // V4: Time Input cho buổi sáng
#define V_EVENING_TIME 5  // V5: Time Input cho buổi chiều
#define V_CURRENT_TIME 6  // V6: Hiển thị thời gian hiện tại
#define V_LAST_FEED 7     // V7: Hiển thị lần cho ăn cuối
#define V_FEED_AMOUNT 8   // V8: Slider điều chỉnh lượng thức ăn (200-1000ms)
#define V_SERVO_SPEED 9   // V9: Slider điều chỉnh tốc độ servo (90-180)

// ======= CẤU HÌNH SERVO TỐI ƯU =======
#define SERVO_STOP_SPEED 90      // Tốc độ dừng servo
#define SERVO_MIN_SPEED 95       // Tốc độ quay chậm nhất
#define SERVO_MAX_SPEED 180      // Tốc độ quay nhanh nhất
#define SERVO_DEFAULT_SPEED 150  // Tốc độ mặc định (nhanh)

// Cấu hình thời gian cho ăn
#define FEED_TIME_MIN 200        // Thời gian cho ăn tối thiểu (ms)
#define FEED_TIME_MAX 2000       // Thời gian cho ăn tối đa (ms)
#define FEED_TIME_DEFAULT 600    // Thời gian cho ăn mặc định (ms)

// ======= CẤU HÌNH TIẾT KIỆM BLYNK QUOTA =======
#define DISTANCE_UPDATE_INTERVAL 5000    // Gửi khoảng cách mỗi 5 giây (thay vì mỗi 100ms)
#define TIME_UPDATE_INTERVAL 300000      // Gửi thời gian mỗi 5 phút (thay vì mỗi phút)
#define DISTANCE_CHANGE_THRESHOLD 2.0    // Chỉ gửi khi khoảng cách thay đổi >2cm
#define MAX_DISTANCE_FOR_DISPLAY 50.0    // Không gửi nếu khoảng cách >50cm (tránh nhiễu)

// ======= KHỞI TẠO ĐỐI TƯỢNG =======
Servo feedServo;

// ======= BIẾN TOÀN CỤC =======
bool autoMode = false;           // Chế độ cảm biến tự động (mặc định tắt)
bool scheduleMode = false;       // Chế độ hẹn giờ (mặc định tắt)
unsigned long lastAutoFeedTime = 0;  // Thời gian lần cho ăn tự động cuối (chỉ áp dụng cho cảm biến)
const unsigned long SENSOR_FEED_DELAY = 10000; // 10 giây delay cho chế độ cảm biến
const float TRIGGER_DISTANCE = 5.0;     // Khoảng cách kích hoạt (5cm)

// Biến hẹn giờ
int morningHour = 8, morningMinute = 0;    // Mặc định 8:00 sáng
int eveningHour = 17, eveningMinute = 0;   // Mặc định 17:00 chiều
bool morningFed = false, eveningFed = false; // Cờ đã cho ăn trong ngày
int lastFeedDay = -1;                      // Ngày cho ăn cuối để reset cờ

// Biến điều khiển servo tối ưu
int servoSpeed = SERVO_DEFAULT_SPEED;      // Tốc độ servo có thể điều chỉnh
int feedDuration = FEED_TIME_DEFAULT;      // Thời gian cho ăn có thể điều chỉnh

// ======= BIẾN TIẾT KIỆM BLYNK QUOTA =======
float lastSentDistance = -1;              // Khoảng cách gửi lần cuối
unsigned long lastDistanceUpdate = 0;     // Thời gian cập nhật khoảng cách cuối
unsigned long lastTimeUpdate = 0;         // Thời gian cập nhật thời gian cuối
bool blynkDataSent = false;               // Cờ đã gửi dữ liệu khởi tạo

// ======= HÀM ĐO KHOẢNG CÁCH HC-SR04 =======
float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = (duration * 0.034) / 2;
  
  if (duration == 0) {
    distance = 999;
  }
  
  return distance;
}

// ======= HÀM GỬI KHOẢNG CÁCH THÔNG MINH (TIẾT KIỆM QUOTA) =======
void smartSendDistance(float distance) {
  unsigned long currentTime = millis();
  
  // Chỉ gửi nếu:
  // 1. Đã đủ thời gian delay (5 giây)
  // 2. Khoảng cách thay đổi đáng kể (>2cm)
  // 3. Khoảng cách trong phạm vi hợp lý (<50cm để tránh nhiễu)
  
  if (currentTime - lastDistanceUpdate >= DISTANCE_UPDATE_INTERVAL) {
    if (distance <= MAX_DISTANCE_FOR_DISPLAY && 
        (lastSentDistance == -1 || abs(distance - lastSentDistance) >= DISTANCE_CHANGE_THRESHOLD)) {
      
      Blynk.virtualWrite(V_DISTANCE, distance);
      lastSentDistance = distance;
      lastDistanceUpdate = currentTime;
      
      Serial.printf("📡 Gửi khoảng cách: %.1fcm\n", distance);
    } else if (distance > MAX_DISTANCE_FOR_DISPLAY && lastSentDistance <= MAX_DISTANCE_FOR_DISPLAY) {
      // Gửi một lần khi vật ra khỏi phạm vi
      Blynk.virtualWrite(V_DISTANCE, 999);
      lastSentDistance = 999;
      lastDistanceUpdate = currentTime;
      Serial.println("📡 Gửi: Không phát hiện vật thể");
    }
  }
}

// ======= HÀM CHO CÁ ĂN TỐI ƯU (NÂNG CẤP HOÀN TOÀN) =======
void feedFish(String feedType = "MANUAL") {
  Serial.println("🐟 BẮT ĐẦU CHO CÁ ĂN - " + feedType);
  Serial.printf("⚙️ Cấu hình: Tốc độ=%d, Thời gian=%dms\n", servoSpeed, feedDuration);
  
  // Bước 1: Đảm bảo servo dừng hoàn toàn trước khi bắt đầu
  feedServo.write(SERVO_STOP_SPEED);
  delay(200); // Chờ servo ổn định
  
  // Bước 2: Quay với tốc độ cao để mở lỗ cho ăn
  Serial.println("🔄 Đang mở lỗ cho ăn...");
  feedServo.write(servoSpeed); // Quay nhanh
  delay(feedDuration);         // Thời gian cho ăn có thể điều chỉnh
  
  // Bước 3: Dừng servo ngay lập tức
  feedServo.write(SERVO_STOP_SPEED);
  delay(100); // Chờ servo dừng hoàn toàn
  
  // Bước 4: Tinh chỉnh vị trí lỗ cho ăn (quay ngược lại một chút)
  // Điều này đảm bảo lỗ quay về đúng vị trí ban đầu (hướng lên)
  Serial.println("🎯 Điều chỉnh vị trí lỗ cho ăn...");
  feedServo.write(SERVO_MIN_SPEED); // Quay chậm ngược lại
  delay(100);                       // Điều chỉnh nhẹ
  
  // Bước 5: Dừng hoàn toàn
  feedServo.write(SERVO_STOP_SPEED);
  delay(200);
  
  // Cập nhật thời gian cho ăn cuối chỉ khi là AUTO (cảm biến)
  if (feedType == "AUTO") {
    lastAutoFeedTime = millis();
  }
  
  // Gửi thông tin lên Blynk (CHỈ KHI CHO ĂN - TIẾT KIỆM QUOTA)
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    char timeString[64];
    strftime(timeString, sizeof(timeString), "%H:%M:%S %d/%m", &timeinfo);
    Blynk.virtualWrite(V_LAST_FEED, String(timeString) + " - " + feedType);
    Serial.println("📡 Gửi thông tin cho ăn lên Blynk");
  }
  
  Serial.println("✅ HOÀN THÀNH CHO ĂN!");
  Serial.println("🔄 Lỗ cho ăn đã quay về vị trí tối ưu");
  Serial.println("==========================================");
}

// ======= HÀM CHO ĂN VỚI LƯỢNG TÙY CHỈNH =======
void feedFishCustom(int customDuration, int customSpeed, String feedType = "CUSTOM") {
  Serial.println("🐟 CHO ĂN TÙY CHỈNH - " + feedType);
  Serial.printf("⚙️ Tốc độ: %d, Thời gian: %dms\n", customSpeed, customDuration);
  
  feedServo.write(SERVO_STOP_SPEED);
  delay(200);
  
  feedServo.write(customSpeed);
  delay(customDuration);
  
  feedServo.write(SERVO_STOP_SPEED);
  delay(100);
  
  // Điều chỉnh vị trí
  feedServo.write(SERVO_MIN_SPEED);
  delay(50);
  feedServo.write(SERVO_STOP_SPEED);
  
  Serial.println("✅ Hoàn thành cho ăn tùy chỉnh!");
}

// ======= HÀM KIỂM TRA CÓ THỂ CHO ĂN TỰ ĐỘNG KHÔNG (CHỈ CHO CẢM BIẾN) =======
bool canAutoFeed() {
  return (millis() - lastAutoFeedTime) >= SENSOR_FEED_DELAY;
}

// ======= HÀM KIỂM TRA VÀ THỰC HIỆN HẸN GIỜ =======
void checkScheduledFeeding() {
  if (!scheduleMode) return;
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;
  
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  int currentDay = timeinfo.tm_mday;
  
  // Reset cờ cho ăn khi sang ngày mới
  if (currentDay != lastFeedDay) {
    morningFed = false;
    eveningFed = false;
    lastFeedDay = currentDay;
    Serial.println("🌅 Ngày mới - Reset trạng thái cho ăn");
  }
  
  // Kiểm tra giờ cho ăn sáng
  if (!morningFed && currentHour == morningHour && currentMinute == morningMinute) {
    Serial.println("⏰ ĐẾN GIỜ CHO ĂN SÁNG!");
    feedFish("MORNING");
    morningFed = true;
  }
  
  // Kiểm tra giờ cho ăn chiều
  if (!eveningFed && currentHour == eveningHour && currentMinute == eveningMinute) {
    Serial.println("⏰ ĐẾN GIỜ CHO ĂN CHIỀU!");
    feedFish("EVENING");
    eveningFed = true;
  }
}

// ======= HÀM CẬP NHẬT THỜI GIAN LÊN BLYNK (TIẾT KIỆM QUOTA) =======
void smartUpdateTimeDisplay() {
  unsigned long currentTime = millis();
  
  // Chỉ cập nhật thời gian mỗi 5 phút thay vì mỗi phút
  if (currentTime - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
      char timeString[64];
      strftime(timeString, sizeof(timeString), "%H:%M:%S\n%d/%m/%Y", &timeinfo);
      Blynk.virtualWrite(V_CURRENT_TIME, timeString);
      lastTimeUpdate = currentTime;
      Serial.println("📡 Cập nhật thời gian lên Blynk");
    }
  }
}

// ======= HÀM GỬI DỮ LIỆU KHỞI TẠO CHỈ MỘT LẦN =======
void sendInitialData() {
  if (!blynkDataSent && Blynk.connected()) {
    // Chỉ gửi một lần khi kết nối thành công
    Blynk.virtualWrite(V_AUTO_MODE, autoMode);
    Blynk.virtualWrite(V_SCHEDULE_MODE, scheduleMode);
    Blynk.virtualWrite(V_FEED_AMOUNT, feedDuration);
    Blynk.virtualWrite(V_SERVO_SPEED, servoSpeed);
    
    blynkDataSent = true;
    Serial.println("📡 Đã gửi dữ liệu khởi tạo lên Blynk");
  }
}

// ======= BLYNK: CHẾ ĐỘ CẢNH BÁO TỰ ĐỘNG =======
BLYNK_WRITE(V_AUTO_MODE) {
  autoMode = param.asInt();
  Serial.print("🔄 Chế độ cảm biến tự động: ");
  Serial.println(autoMode ? "BẬT" : "TẮT");
}

// ======= BLYNK: CHẾ ĐỘ HẸN GIỜ =======
BLYNK_WRITE(V_SCHEDULE_MODE) {
  scheduleMode = param.asInt();
  Serial.print("⏰ Chế độ hẹn giờ: ");
  Serial.println(scheduleMode ? "BẬT" : "TẮT");
}

// ======= BLYNK: CHO ĂN THỦ CÔNG (KHÔNG DELAY) =======
BLYNK_WRITE(V_MANUAL_FEED) {
  int buttonState = param.asInt();
  
  if (buttonState == 1) {
    Serial.println("📱 NHẬN LỆNH CHO ĂN THỦ CÔNG TỪ BLYNK");
    feedFish("MANUAL"); // Cho ăn ngay lập tức, không cần delay
  }
}

// ======= BLYNK: ĐIỀU CHỈNH LƯỢNG THỨC ĂN =======
BLYNK_WRITE(V_FEED_AMOUNT) {
  feedDuration = param.asInt();
  feedDuration = constrain(feedDuration, FEED_TIME_MIN, FEED_TIME_MAX);
  Serial.printf("🍽️ Điều chỉnh thời gian cho ăn: %dms\n", feedDuration);
}

// ======= BLYNK: ĐIỀU CHỈNH TỐC ĐỘ SERVO =======
BLYNK_WRITE(V_SERVO_SPEED) {
  servoSpeed = param.asInt();
  servoSpeed = constrain(servoSpeed, SERVO_MIN_SPEED, SERVO_MAX_SPEED);
  Serial.printf("⚡ Điều chỉnh tốc độ servo: %d\n", servoSpeed);
}

// ======= BLYNK: CẬP NHẬT GIỜ CHO ĂN SÁNG =======
BLYNK_WRITE(V_MORNING_TIME) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    morningHour = t.getStartHour();
    morningMinute = t.getStartMinute();
    Serial.printf("🌅 Đã cập nhật giờ cho ăn sáng: %02d:%02d\n", morningHour, morningMinute);
  }
}

// ======= BLYNK: CẬP NHẬT GIỜ CHO ĂN CHIỀU =======
BLYNK_WRITE(V_EVENING_TIME) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    eveningHour = t.getStartHour();
    eveningMinute = t.getStartMinute();
    Serial.printf("🌆 Đã cập nhật giờ cho ăn chiều: %02d:%02d\n", eveningHour, eveningMinute);
  }
}

// ======= THIẾT LẬP BAN ĐẦU =======
void setup() {
  Serial.begin(115200);
  Serial.println("🚀 KHỞI ĐỘNG HỆ THỐNG CHO CÁ ĂN THÔNG MINH V3.1 - TIẾT KIỆM BLYNK QUOTA");
  Serial.println("=======================================================================");
  
  // Cấu hình chân HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Khởi tạo servo và đặt về vị trí dừng
  feedServo.attach(SERVO_PIN);
  feedServo.write(SERVO_STOP_SPEED);
  delay(1000); // Chờ servo ổn định
  Serial.println("✅ Servo đã sẵn sàng (dừng ở vị trí 90°)");
  
  // Kết nối WiFi và Blynk
  Serial.print("📶 Đang kết nối WiFi: ");
  Serial.println(ssid);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  Serial.println("✅ Kết nối WiFi thành công!");
  
  // Cấu hình thời gian NTP
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServer);
  Serial.println("⏰ Đang đồng bộ thời gian...");
  
  // Chờ đồng bộ thời gian
  struct tm timeinfo;
  int retryCount = 0;
  while(!getLocalTime(&timeinfo) && retryCount < 10) {
    delay(1000);
    retryCount++;
    Serial.print(".");
  }
  
  if (retryCount < 10) {
    Serial.println("\n✅ Đồng bộ thời gian thành công!");
    char timeString[64];
    strftime(timeString, sizeof(timeString), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    Serial.println("🕐 Thời gian hiện tại: " + String(timeString));
  } else {
    Serial.println("\n⚠️ Không thể đồng bộ thời gian!");
  }
  
  Serial.println("📱 Blynk đã sẵn sàng");
  Serial.println("=======================================================================");
  
  Serial.println("🎯 HỆ THỐNG ĐÃ SẴN SÀNG HOẠT ĐỘNG!");
  Serial.println("📏 Chế độ cảm biến: Khoảng cách kích hoạt < 5cm");
  Serial.println("⏰ Chế độ hẹn giờ: 8:00 sáng & 17:00 chiều");
  Serial.println("📱 Cho ăn thủ công: Không có delay");
  Serial.printf("⚙️ Servo tối ưu: Tốc độ %d, Thời gian %dms\n", servoSpeed, feedDuration);
  Serial.println("🎛️ Có thể điều chỉnh tốc độ và lượng thức ăn qua app");
  Serial.println("💡 TỐI ƯU BLYNK QUOTA:");
  Serial.println("   - Khoảng cách: Gửi mỗi 5s, chỉ khi thay đổi >2cm");
  Serial.println("   - Thời gian: Cập nhật mỗi 5 phút");
  Serial.println("   - Dữ liệu khởi tạo: Chỉ gửi 1 lần");
  Serial.println("=======================================================================");
  
  // Test servo hoạt động
  Serial.println("🧪 Test servo...");
  feedServo.write(SERVO_DEFAULT_SPEED);
  delay(500);
  feedServo.write(SERVO_STOP_SPEED);
  Serial.println("✅ Test servo hoàn tất!");
}

// ======= VÒNG LẶP CHÍNH =======
void loop() {
  Blynk.run();
  
  // Gửi dữ liệu khởi tạo chỉ một lần
  sendInitialData();
  
  // Cập nhật thời gian lên Blynk (mỗi 5 phút)
  smartUpdateTimeDisplay();
  
  // Kiểm tra hẹn giờ cho ăn
  checkScheduledFeeding();
  
  // Đo khoảng cách
  float distance = measureDistance();
  
  // Gửi khoảng cách thông minh (tiết kiệm quota)
  smartSendDistance(distance);
  
  // In khoảng cách ra Serial (mỗi 2 giây)
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime >= 2000) {
    Serial.print("📏 Khoảng cách: ");
    if (distance < 999) {
      Serial.print(distance, 1);
      Serial.println(" cm");
    } else {
      Serial.println("Không phát hiện");
    }
    lastPrintTime = millis();
  }
  
  // LOGIC CHO ĂN TỰ ĐỘNG THEO CẢM BIẾN (chỉ khi bật chế độ cảm biến)
  if (autoMode && distance < TRIGGER_DISTANCE && distance > 0) {
    if (canAutoFeed()) {
      Serial.println("🎯 PHÁT HIỆN CÁ TRONG PHẠM VI!");
      Serial.print("📏 Khoảng cách: ");
      Serial.print(distance, 1);
      Serial.println(" cm");
      feedFish("AUTO");
    } else {
      unsigned long remainingTime = (SENSOR_FEED_DELAY - (millis() - lastAutoFeedTime)) / 1000;
      static unsigned long lastWarningTime = 0;
      
      if (millis() - lastWarningTime >= 3000) {
        Serial.print("⚠️ Cá đã tới nhưng chưa thể cho ăn! Còn lại: ");
        Serial.print(remainingTime);
        Serial.println(" giây");
        lastWarningTime = millis();
      }
    }
  }
  
  delay(100);
}