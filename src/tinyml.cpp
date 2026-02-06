#include "tinyml.h"
#include "global.h"             
#include "dht_anomaly_model.h"  

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// --- CẤU HÌNH NGƯỠNG CỐ ĐỊNH ---
// Vì model đã train chuẩn, ta set cứng ngưỡng này.
// Ví dụ: Nếu bình thường Score khoảng 0.3 -> Set ngưỡng 0.5 hoặc 0.6
#define AI_THRESHOLD  0.1  

// Globals cho TFLite
namespace {
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 32 * 1024; 
    uint8_t tensor_arena[kTensorArenaSize];
} 

void setupTinyML() {
    Serial.println("[AI] Init TensorFlow Lite...");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[AI] Error: Model Schema Version Mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[AI] Error: AllocateTensors failed!");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("[AI] Model Loaded Successfully!");
}

void tiny_ml_task(void *pvParameters) {
    setupTinyML();
    
    // Báo lên LCD là AI đã sẵn sàng ngay lập tức (không cần chờ 10s)
    if (g_queueManager_to_LCD != NULL) {
        SystemMessage msg;
        msg.type = EVENT_UPDATE_LCD;
        String fullMsg = "AI SYSTEM:|READY (FIXED)";
        strncpy(msg.payload, fullMsg.c_str(), sizeof(msg.payload) - 1);
        xQueueSend(g_queueManager_to_LCD, &msg, 10);
    }
    
    // --- VÒNG LẶP GIÁM SÁT CHÍNH ---
    bool lastStateWasAnomaly = false;

    while (1) {
        float t = currentTemperature;
        float h = currentHumidity;

        // Bỏ qua nếu lỗi đọc cảm biến
        if (t == 0 && h == 0) {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        // 1. CHUẨN HÓA DỮ LIỆU (QUAN TRỌNG)
        // Vì bên Python bạn đã train với dữ liệu chia 100, nên ở đây cũng phải chia.
        // Nếu nạp số 30.0 vào model mong đợi 0.3 thì AI sẽ bị sai.
        // 1. Chuẩn hóa đầu vào
        float input_t = t / 100.0;
        float input_h = h / 100.0;
        
        input->data.f[0] = input_t; 
        input->data.f[1] = input_h;

        float ai_score = 0.0;
        if (interpreter->Invoke() == kTfLiteOk) {
            // Lấy giá trị AI "tái tạo" (Dự đoán)
            float predicted_t = output->data.f[0];
            float predicted_h = output->data.f[1];
            
            // --- CÔNG THỨC TÍNH ĐIỂM BẤT THƯỜNG CHUẨN (SỬA Ở ĐÂY) ---
            // Điểm = |Thực tế - Dự đoán|
            // Nếu AI đoán đúng (bình thường) -> Điểm gần bằng 0
            float error_t = abs(input_t - predicted_t);
            float error_h = abs(input_h - predicted_h);
            
            // Lấy trung bình cộng sai số làm điểm chốt
            ai_score = (error_t + error_h) / 2.0;
            
            anomalyScore = ai_score; 
        }

        // 3. SO SÁNH VỚI NGƯỠNG CỐ ĐỊNH
        // Logic đơn giản: Vượt ngưỡng là Báo động
        bool ai_detect = (ai_score > AI_THRESHOLD);
        isAnomaly = ai_detect; 

        // 4. XỬ LÝ BÁO ĐỘNG (LOGIC & LCD)
        if (ai_detect) {
            // Chỉ in ra Terminal khi CÓ BIẾN
            Serial.printf(">>> [AI ALERT] Score: %.4f > %.2f | T:%.1f H:%.1f\n", 
                          ai_score, AI_THRESHOLD, t, h);

            if (g_queueManager_to_LCD != NULL) {
                SystemMessage msg;
                msg.type = EVENT_UPDATE_LCD;
                String line1 = "!!! AI ALERT !!!";
                String line2 = "Score:" + String(ai_score,2); 
                
                String fullMsg = line1 + "|" + line2;
                strncpy(msg.payload, fullMsg.c_str(), sizeof(msg.payload) - 1);
                xQueueSend(g_queueManager_to_LCD, &msg, 10);
                lastStateWasAnomaly = true;
            }
        } else {
            // Khi hết báo động -> Trả lại màn hình bình thường
            if (lastStateWasAnomaly) {
                Serial.println(">>> [AI INFO] Moi truong on dinh tro lai.");
                
                if (g_queueManager_to_LCD != NULL) {
                    SystemMessage msg;
                    msg.type = EVENT_UPDATE_LCD;
                    String normalMsg = "SYSTEM NORMAL|AI Active";
                    strncpy(msg.payload, normalMsg.c_str(), sizeof(msg.payload) - 1);
                    xQueueSend(g_queueManager_to_LCD, &msg, 10);
                }
                lastStateWasAnomaly = false;
            }
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}