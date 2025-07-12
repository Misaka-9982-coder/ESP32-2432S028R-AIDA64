#ifndef _TIME_MANAGER_H_
#define _TIME_MANAGER_H_

#include <WiFi.h>
#include <time.h>
#include "config.h"

class TimeManager {
private:
    unsigned long last_ntp_sync;
    unsigned long last_time_update;
    bool time_synced;
    
public:
    TimeManager();
    
    // 初始化NTP时间同步
    void initNTP();
    
    // 获取当前时间字符串 (HH:MM:SS格式)
    String getCurrentTimeString();
    
    // 获取当前日期时间字符串 (YYYY-MM-DD HH:MM:SS格式)
    String getCurrentDateTimeString();
    
    // 检查是否需要重新同步时间
    void checkAndSyncTime();
    
    // 获取时间同步状态
    bool isTimeSynced() const { return time_synced; }
    
    // 强制同步时间
    void forceSyncTime();
};

// 全局时间管理器实例
extern TimeManager timeManager;

#endif
