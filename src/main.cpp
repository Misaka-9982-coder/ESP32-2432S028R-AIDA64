#include <Arduino.h>
#include "http_client.h"
#include "display.h"
#include "config.h"
#include "wifi_client.h"
#include "time_manager.h"

/* default config */
int screen_dir = SCREEN_DIR_HORIZONTAL;
static unsigned long last_data_update = 0;
static unsigned long last_time_update = 0;

void setup()
{
    // put your setup code here, to run once:
    //Serial
    Serial.begin(115200);
    UARTPrintf("[SYSTEM] Initial start...\r\n");

    //Display
    UARTPrintf("[DISPLAY] Initializing display...\r\n");
    
    display_enhanced.begin(screen_dir);
    UARTPrintf("[ENHANCED DISPLAY] Init finish\r\n");
    display_enhanced.clear();

    // thread
    BaseType_t wifiTaskResult = xTaskCreate(taskWifiClient, "taskWifiClient", 4096, NULL, 2, NULL);
    BaseType_t httpTaskResult = xTaskCreate(taskHttpClient, "taskHttpClient", 8192, NULL, 2, NULL);
    
    UARTPrintf("[SYSTEM] WiFi task creation: %s\r\n", wifiTaskResult == pdPASS ? "SUCCESS" : "FAILED");
    UARTPrintf("[SYSTEM] HTTP task creation: %s\r\n", httpTaskResult == pdPASS ? "SUCCESS" : "FAILED");
    
    // 等待WiFi连接后初始化时间同步
    UARTPrintf("[SYSTEM] Waiting for WiFi connection before NTP sync...\r\n");
    
    // 打印可用内存
    UARTPrintf("[SYSTEM] Free heap: %d bytes\r\n", ESP.getFreeHeap());
}

void loop()
{
    unsigned long current_time = millis();
    
    // 检查WiFi连接状态并初始化NTP时间同步
    static bool ntp_initialized = false;
    if (WiFi.status() == WL_CONNECTED && !ntp_initialized) {
        timeManager.initNTP();
        ntp_initialized = true;
    }
    
    // 定期检查时间同步
    timeManager.checkAndSyncTime();
    
    // 增强显示模式
    // Handle LVGL tasks (更频繁的刷新)
    display_enhanced.tick();
    
    // Update AIDA64 data display
    if (!aida64DataList.empty() && 
        (current_time - last_data_update >= 1000)) { // 1秒更新间隔
        display_enhanced.displayAida64Data(aida64DataList);
        last_data_update = current_time;
    }
    
    // Update time display
    if (current_time - last_time_update >= TIME_UPDATE_INTERVAL) {
        display_enhanced.updateTimeDisplay(timeManager.getCurrentTimeString());
        last_time_update = current_time;
    }
    
    delay(5); // 减少延迟，更频繁的LVGL处理
}

