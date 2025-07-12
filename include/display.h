#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <TFT_eSPI.h>
#include <lvgl.h>
#include "public.h"
#include <vector>

#define displayPrintLog(format, arg...) UARTPrintf("\r\n[DISPLAY] " format, ##arg)

// 屏幕方向枚举
enum SCREEN_DIRECTION {
    SCREEN_DIR_HORIZONTAL,
    SCREEN_DIR_VERTICAL,
    SCREEN_DIR_MAX,
};

// AIDA64数据类别
enum AIDA64_CATEGORY {
    AIDA64_TIME,
    AIDA64_CPU,
    AIDA64_GPU,
    AIDA64_MEMORY,
    AIDA64_NETWORK,
    AIDA64_OTHER
};

// 数据项结构
struct AIDA64_ITEM {
    AIDA64_CATEGORY category;
    const char* label;
    const char* value;
    const char* unit;
    lv_color_t color;
};

class SCREEN_DISPLAY_ENHANCED {
public:
    SCREEN_DISPLAY_ENHANCED();
    ~SCREEN_DISPLAY_ENHANCED();

    void begin(int dir);
    void setScreenDir(int dir);
    void displayAida64Data(std::vector<AIDA64_DATA> &dataList);
    void updateTimeDisplay(const String& timeString);
    void clear();
    void updateDisplay();
    void tick();

    void setPowerSave(uint8_t is_enable) {
        // TFT displays don't have power save mode like VFD
    }

private:
    TFT_eSPI tft;
    int screen_dir;
    
    // LVGL 相关
    lv_disp_t* disp;
    lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t* buf1;
    lv_color_t* buf2;
    
    // UI 对象
    lv_obj_t* main_screen;
    lv_obj_t* title_label;
    
    // 系统信息对象
    lv_obj_t* time_label;
    lv_obj_t* cpu_bar;
    lv_obj_t* cpu_label;
    lv_obj_t* gpu_bar;
    lv_obj_t* gpu_label;
    lv_obj_t* mem_bar;
    lv_obj_t* mem_label;
    
    // 额外的信息标签
    lv_obj_t* temp_label;
    lv_obj_t* gpu_temp_label;
    lv_obj_t* mem_usage_label;
    lv_obj_t* cpu_freq_label;
    lv_obj_t* cpu_power_label;
    lv_obj_t* gpu_power_label;
    lv_obj_t* gpu_mem_label;
    lv_obj_t* net_down_label;
    lv_obj_t* net_up_label;
    lv_obj_t* local_ip_label;
    lv_obj_t* external_ip_label;
    
    // 私有方法
    void initLVGL();
    void createUI();
    void setupSingleScreenLayout();
    void updateSystemInfo(std::vector<AIDA64_DATA> &dataList);
    
    // LVGL 回调函数
    static void disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
    static void disp_flush_ready(lv_disp_drv_t* disp_drv);
};

extern SCREEN_DISPLAY_ENHANCED display_enhanced;

#endif // _DISPLAY_H_
