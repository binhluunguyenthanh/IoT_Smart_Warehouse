#include "tinyml.h"
#include "global.h"             

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

    // Biến để tránh gửi lệnh LCD liên tục gây nhấp nháy
    bool lastStateWasAnomaly = false;

    while (1) {
        float t = currentTemperature;
        float h = currentHumidity;

        if (t == 0 && h == 0) {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }

        // 1. Chạy AI
        input->data.f[0] = t; 
        input->data.f[1] = h;

        float ai_result = 0.0;
        if (interpreter->Invoke() != kTfLiteOk) {
            Serial.println("[AI] Invoke failed");
        } else {
            ai_result = output->data.f[0];
            anomalyScore = ai_result; // Cập nhật ra web
        }

        // 2. Logic Báo Động Kép (Hybrid)
        // Báo động nếu: AI thấy bất thường (>0.5) HOẶC Nhiệt độ quá cao (>40 độ)
        bool ai_detect = (ai_result > 0.5);
        bool temp_detect = (t > 40.0); // Ngưỡng an toàn cứng
        
        bool isDangerous = ai_detect || temp_detect;
        isAnomaly = isDangerous; // Cập nhật biến toàn cục cho Web

        // 3. Xử lý Hành động (Terminal & LCD)
        if (isDangerous) {
            // --- IN RA TERMINAL ---
            Serial.printf(">>> CANH BAO! AI:%.2f | Temp:%.1f -- NGUY HIEM! <<<\n", ai_result, t);

            // --- GỬI RA LCD ---
            if (g_queueManager_to_LCD != NULL) {
                SystemMessage msg;
                msg.type = EVENT_UPDATE_LCD;
                
                String line1 = "!!! WARNING !!!";
                String line2 = "";

                if (temp_detect) {
                    line2 = "QUA NHIET: " + String(t, 1) + "C";
                } else {
                    line2 = "AI: BAT THUONG";
                }

                // Gửi: "Dòng 1|Dòng 2"
                String fullMsg = line1 + "|" + line2;
                strncpy(msg.payload, fullMsg.c_str(), sizeof(msg.payload) - 1);
                
                // Gửi ngay lập tức
                xQueueSend(g_queueManager_to_LCD, &msg, 10);
                lastStateWasAnomaly = true;
            }
        } 
        else {
            // Nếu vừa hết báo động -> Trả lại màn hình bình thường 1 lần
            if (lastStateWasAnomaly) {
                if (g_queueManager_to_LCD != NULL) {
                    SystemMessage msg;
                    msg.type = EVENT_UPDATE_LCD;
                    String normalMsg = "SYSTEM NORMAL|Temp: " + String(t,1) + "C";
                    strncpy(msg.payload, normalMsg.c_str(), sizeof(msg.payload) - 1);
                    xQueueSend(g_queueManager_to_LCD, &msg, 10);
                }
                lastStateWasAnomaly = false;
            }
        }

        // Chạy mỗi 2 giây
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}