#ifndef _CONFIG_H_
#define _CONFIG_H_

/* 
 * ESP32-2432S028R TFT Display Configuration
 * 2.8 inch 240*320 LCD TFT Module with Touch
 * Uses TFT_eSPI library instead of U8g2
 */

// Display dimensions
#define MAX_Y 240
#define MAX_X 320

//WIFI
#define WIFI_SSID "10086"
#define WIFI_PASS "aaaaa123456"

//HTTP
#define HTTP_HOST "192.168.1.1"
#define HTTP_PORT 80

//NTP时间同步配置
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov" 
#define NTP_SERVER_3 "cn.pool.ntp.org"
#define TIMEZONE_OFFSET_SECONDS (8 * 3600)  // 中国时区 UTC+8
#define NTP_UPDATE_INTERVAL (60 * 60 * 1000) // 每小时同步一次 (毫秒)

//显示更新间隔
#define DATA_UPDATE_INTERVAL 1000  // AIDA64数据更新间隔 (毫秒)
#define TIME_UPDATE_INTERVAL 1000  // 时间显示更新间隔 (毫秒)

#endif