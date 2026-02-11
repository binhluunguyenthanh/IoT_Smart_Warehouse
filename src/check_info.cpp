#include "check_info.h"

// Hàm đọc thông tin Wifi từ file info.dat
void Load_info_File()
{
  if(!LittleFS.begin(true)) return; // Đảm bảo File System đã bật, nếu lỗi thì format lại
  
  File file = LittleFS.open("/info.dat", "r"); // Mở file chế độ đọc
  if (!file) return; // Nếu file không tồn tại thì thôi

  // Tạo bộ nhớ đệm JSON
  DynamicJsonDocument doc(1024); 
  DeserializationError error = deserializeJson(doc, file); // Giải mã JSON từ file
  
  if (!error) {
    // Nếu giải mã thành công, nạp vào biến toàn cục
    if(doc.containsKey("WIFI_SSID")) WIFI_SSID = strdup(doc["WIFI_SSID"]);
    if(doc.containsKey("WIFI_PASS")) WIFI_PASS = strdup(doc["WIFI_PASS"]);
  }
  file.close();
}

// Hàm xóa file cấu hình (Reset Wifi)
void Delete_info_File()
{
  LittleFS.begin(true);
  if (LittleFS.exists("/info.dat")) LittleFS.remove("/info.dat");
  // Sau khi xóa, khởi động lại ESP để vào chế độ AP nhập lại từ đầu
  ESP.restart();
}

// Hàm lưu thông tin Wifi mới vào Flash
void Save_info_File(String wifi_ssid, String wifi_pass)
{
  Serial.println("Dang luu cau hinh...");

  // 1. ÉP BUỘC KHỞI ĐỘNG FILE SYSTEM NẾU CHƯA CÓ
  if(!LittleFS.begin(true)){
      Serial.println("LittleFS chua bat -> Dang bat lai...");
      if(!LittleFS.begin(true)) {
          Serial.println("❌ LOI: KHONG THE MO FILE SYSTEM!");
          return;
      }
  }

  // Đóng gói dữ liệu vào JSON
  DynamicJsonDocument doc(1024);
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;

  // 2. GHI FILE (Mode 'w' sẽ ghi đè nội dung cũ)
  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile) {
    serializeJson(doc, configFile); // Ghi JSON vào file
    configFile.close();
    Serial.println("✅ Da luu file info.dat thanh cong!");
  } else {
    Serial.println("❌ LOI: Khong the tao file info.dat");
  }

  // 3. TUYỆT ĐỐI KHÔNG RESET Ở ĐÂY
  // Lý do: Để Task Wifi tự động chuyển từ AP sang STA mượt mà không cần boot lại
};

// Hàm kiểm tra xem đã có thông tin Wifi chưa
bool check_info_File(bool check)
{
  if (!LittleFS.begin(true)) return false;
  Load_info_File(); // Thử load ra
  // Nếu SSID rỗng tức là chưa có cấu hình
  if (WIFI_SSID.isEmpty()) return false;
  return true;
}