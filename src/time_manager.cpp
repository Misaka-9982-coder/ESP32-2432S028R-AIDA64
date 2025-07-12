#include "time_manager.h"
#include "public.h"

TimeManager::TimeManager() {
    last_ntp_sync = 0;
    last_time_update = 0;
    time_synced = false;
}

void TimeManager::initNTP() {
    UARTPrintf("[TIME] Initializing NTP time synchronization...\r\n");
    
    // 配置时区
    configTime(TIMEZONE_OFFSET_SECONDS, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    // 等待时间同步
    UARTPrintf("[TIME] Waiting for NTP time sync...\r\n");
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    
    while (!getLocalTime(&timeinfo) && retry < retry_count) {
        delay(1000);
        retry++;
        UARTPrintf("[TIME] Waiting for NTP sync... (%d/%d)\r\n", retry, retry_count);
    }
    
    if (retry < retry_count) {
        time_synced = true;
        last_ntp_sync = millis();
        UARTPrintf("[TIME] NTP time synchronized successfully!\r\n");
        UARTPrintf("[TIME] Current time: %s\r\n", getCurrentDateTimeString().c_str());
    } else {
        time_synced = false;
        UARTPrintf("[TIME] Failed to synchronize NTP time\r\n");
    }
}

String TimeManager::getCurrentTimeString() {
    if (!time_synced) {
        return "--:--:--";
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "--:--:--";
    }
    
    char timeString[16];
    strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
    return String(timeString);
}

String TimeManager::getCurrentDateTimeString() {
    if (!time_synced) {
        return "----/--/-- --:--:--";
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "----/--/-- --:--:--";
    }
    
    char dateTimeString[32];
    strftime(dateTimeString, sizeof(dateTimeString), "%Y/%m/%d %H:%M:%S", &timeinfo);
    return String(dateTimeString);
}

void TimeManager::checkAndSyncTime() {
    unsigned long current_millis = millis();
    
    // 检查是否需要重新同步时间（每小时一次）
    if (time_synced && (current_millis - last_ntp_sync >= NTP_UPDATE_INTERVAL)) {
        UARTPrintf("[TIME] Performing periodic NTP resync...\r\n");
        forceSyncTime();
    }
    
    // 处理millis()溢出情况（约49天后会发生）
    if (current_millis < last_ntp_sync) {
        last_ntp_sync = current_millis;
    }
}

void TimeManager::forceSyncTime() {
    UARTPrintf("[TIME] Force syncing time with NTP servers...\r\n");
    
    // 重新配置时间
    configTime(TIMEZONE_OFFSET_SECONDS, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    // 等待同步完成
    struct tm timeinfo;
    int retry = 0;
    const int retry_count = 10;
    
    while (!getLocalTime(&timeinfo) && retry < retry_count) {
        delay(500);
        retry++;
    }
    
    if (retry < retry_count) {
        time_synced = true;
        last_ntp_sync = millis();
        UARTPrintf("[TIME] Time resync successful: %s\r\n", getCurrentDateTimeString().c_str());
    } else {
        UARTPrintf("[TIME] Time resync failed\r\n");
    }
}

// 全局实例
TimeManager timeManager;
