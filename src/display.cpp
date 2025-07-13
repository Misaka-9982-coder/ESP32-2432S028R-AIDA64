#include "display.h"
#include "config.h"

// 静态缓冲区大小
#define BUFFER_SIZE (MAX_X * MAX_Y / 4)

SCREEN_DISPLAY_ENHANCED::SCREEN_DISPLAY_ENHANCED() : tft() {
    screen_dir = SCREEN_DIR_HORIZONTAL;
    disp = nullptr;
    buf1 = nullptr;
    buf2 = nullptr;
    
    // 初始化UI对象指针
    main_screen = nullptr;
    title_label = nullptr;
    time_label = nullptr;
    cpu_bar = nullptr;
    cpu_label = nullptr;
    gpu_bar = nullptr;
    gpu_label = nullptr;
    mem_bar = nullptr;
    mem_label = nullptr;
    
    // 初始化额外信息标签指针
    temp_label = nullptr;
    gpu_temp_label = nullptr;
    mem_usage_label = nullptr;
    cpu_freq_label = nullptr;
    cpu_power_label = nullptr;
    gpu_power_label = nullptr;
    gpu_mem_label = nullptr;
    net_down_label = nullptr;
    net_up_label = nullptr;
    local_ip_label = nullptr;
    external_ip_label = nullptr;
}

SCREEN_DISPLAY_ENHANCED::~SCREEN_DISPLAY_ENHANCED() {
    if (buf1) free(buf1);
    if (buf2) free(buf2);
}

void SCREEN_DISPLAY_ENHANCED::begin(int dir) {
    // 初始化TFT
    tft.init();
    setScreenDir(dir);
    
    // 初始化LVGL
    initLVGL();
    
    // 创建UI
    createUI();
    
    displayPrintLog("Enhanced display initialized with LVGL");
}

void SCREEN_DISPLAY_ENHANCED::setScreenDir(int dir) {
    screen_dir = dir;
    
    if (screen_dir == SCREEN_DIR_VERTICAL) {
        tft.setRotation(0);  // Portrait
    } else if (screen_dir == SCREEN_DIR_HORIZONTAL) {
        tft.setRotation(1);  // Landscape
    }
}

void SCREEN_DISPLAY_ENHANCED::initLVGL() {
    // 初始化LVGL
    lv_init();
    
    // 分配显示缓冲区
    buf1 = (lv_color_t*)malloc(BUFFER_SIZE * sizeof(lv_color_t));
    buf2 = (lv_color_t*)malloc(BUFFER_SIZE * sizeof(lv_color_t));
    
    if (!buf1 || !buf2) {
        displayPrintLog("Failed to allocate LVGL buffers");
        return;
    }
    
    // 初始化显示缓冲区
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUFFER_SIZE);
    
    // 初始化显示驱动
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = MAX_X;
    disp_drv.ver_res = MAX_Y;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = this;
    
    // 注册显示驱动
    disp = lv_disp_drv_register(&disp_drv);
    
    displayPrintLog("LVGL initialized successfully");
}

void SCREEN_DISPLAY_ENHANCED::createUI() {
    // 创建主屏幕
    main_screen = lv_scr_act();
    lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);
    
    // 创建标题
    title_label = lv_label_create(main_screen);
    lv_label_set_text(title_label, "AIDA64 System Monitor");
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 2);
    
    // 统一布局：不使用标签页，所有信息显示在一个屏幕上
    setupSingleScreenLayout();
    
    displayPrintLog("UI created successfully");
}

void SCREEN_DISPLAY_ENHANCED::setupSingleScreenLayout() {
    int y_pos = 25; // 从标题下方开始
    int col1_x = 10;   // 左列X位置
    int col2_x = 170;  // 右列X位置（增加间距）
    int line_height = 28; // 增加行高，更大间距
    
    // === 第一行：时间（单独一行，居中） ===
    time_label = lv_label_create(main_screen);
    lv_label_set_text(time_label, "System Time: --:--:--");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, y_pos);
    y_pos += line_height;
    
    // === 第二行：CPU使用率 + CPU温度 ===
    // CPU使用率（左列）
    lv_obj_t* cpu_title = lv_label_create(main_screen);
    lv_label_set_text(cpu_title, "CPU:");
    lv_obj_set_style_text_color(cpu_title, lv_color_hex(0xFF6666), 0);
    lv_obj_set_pos(cpu_title, col1_x, y_pos);
    
    cpu_label = lv_label_create(main_screen);
    lv_label_set_text(cpu_label, "0%");
    lv_obj_set_style_text_color(cpu_label, lv_color_white(), 0);
    lv_obj_set_pos(cpu_label, col1_x + 40, y_pos);
    
    cpu_bar = lv_bar_create(main_screen);
    lv_obj_set_size(cpu_bar, 70, 12);
    lv_obj_set_pos(cpu_bar, col1_x + 85, y_pos + 2);
    lv_obj_set_style_bg_color(cpu_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(cpu_bar, lv_color_hex(0xFF4444), LV_PART_INDICATOR);
    lv_bar_set_range(cpu_bar, 0, 100);
    
    // CPU温度（右列）
    temp_label = lv_label_create(main_screen);
    lv_label_set_text(temp_label, "CPU: --°C");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0x66CCFF), 0);
    lv_obj_set_pos(temp_label, col2_x, y_pos);
    y_pos += line_height;
    
    // === 第三行：GPU使用率 + GPU温度 ===
    // GPU使用率（左列）
    lv_obj_t* gpu_title = lv_label_create(main_screen);
    lv_label_set_text(gpu_title, "GPU:");
    lv_obj_set_style_text_color(gpu_title, lv_color_hex(0x66FF66), 0);
    lv_obj_set_pos(gpu_title, col1_x, y_pos);
    
    gpu_label = lv_label_create(main_screen);
    lv_label_set_text(gpu_label, "0%");
    lv_obj_set_style_text_color(gpu_label, lv_color_white(), 0);
    lv_obj_set_pos(gpu_label, col1_x + 40, y_pos);
    
    gpu_bar = lv_bar_create(main_screen);
    lv_obj_set_size(gpu_bar, 70, 12);
    lv_obj_set_pos(gpu_bar, col1_x + 85, y_pos + 2);
    lv_obj_set_style_bg_color(gpu_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(gpu_bar, lv_color_hex(0x44FF44), LV_PART_INDICATOR);
    lv_bar_set_range(gpu_bar, 0, 100);
    
    // GPU温度（右列）
    gpu_temp_label = lv_label_create(main_screen);
    lv_label_set_text(gpu_temp_label, "GPU: --°C");
    lv_obj_set_style_text_color(gpu_temp_label, lv_color_hex(0x66CCFF), 0);
    lv_obj_set_pos(gpu_temp_label, col2_x, y_pos);
    y_pos += line_height;
    
    // === 第四行：内存使用率 + CPU频率 ===
    // 内存使用率（左列）
    lv_obj_t* mem_title = lv_label_create(main_screen);
    lv_label_set_text(mem_title, "MEM:");
    lv_obj_set_style_text_color(mem_title, lv_color_hex(0x6666FF), 0);
    lv_obj_set_pos(mem_title, col1_x, y_pos);
    
    mem_label = lv_label_create(main_screen);
    lv_label_set_text(mem_label, "0%");
    lv_obj_set_style_text_color(mem_label, lv_color_white(), 0);
    lv_obj_set_pos(mem_label, col1_x + 40, y_pos);
    
    mem_bar = lv_bar_create(main_screen);
    lv_obj_set_size(mem_bar, 70, 12);
    lv_obj_set_pos(mem_bar, col1_x + 85, y_pos + 2);
    lv_obj_set_style_bg_color(mem_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mem_bar, lv_color_hex(0x4444FF), LV_PART_INDICATOR);
    lv_bar_set_range(mem_bar, 0, 100);
    
    // CPU频率（右列）
    cpu_freq_label = lv_label_create(main_screen);
    lv_label_set_text(cpu_freq_label, "CPU: -- MHz");
    lv_obj_set_style_text_color(cpu_freq_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_pos(cpu_freq_label, col2_x, y_pos);
    y_pos += line_height;
    
    // === 第五行：内存使用量 + CPU功耗 ===
    // 内存使用量（左列）
    mem_usage_label = lv_label_create(main_screen);
    lv_label_set_text(mem_usage_label, "Used: -- MB");
    lv_obj_set_style_text_color(mem_usage_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_pos(mem_usage_label, col1_x, y_pos);
    
    // CPU功耗（右列）
    cpu_power_label = lv_label_create(main_screen);
    lv_label_set_text(cpu_power_label, "CPU: -- W");
    lv_obj_set_style_text_color(cpu_power_label, lv_color_hex(0xFFCC66), 0);
    lv_obj_set_pos(cpu_power_label, col2_x, y_pos);
    y_pos += line_height;
    
    // === 第六行：已用显存 + GPU功耗 ===
    // 已用显存（左列）
    gpu_mem_label = lv_label_create(main_screen);
    lv_label_set_text(gpu_mem_label, "VRAM: -- MB");
    lv_obj_set_style_text_color(gpu_mem_label, lv_color_hex(0x66FFCC), 0);
    lv_obj_set_pos(gpu_mem_label, col1_x, y_pos);
    
    // GPU功耗（右列）
    gpu_power_label = lv_label_create(main_screen);
    lv_label_set_text(gpu_power_label, "GPU: -- W");
    lv_obj_set_style_text_color(gpu_power_label, lv_color_hex(0xFFCC66), 0);
    lv_obj_set_pos(gpu_power_label, col2_x, y_pos);
    y_pos += line_height;
    
    // === 第七行：网络下载 + 上传 ===
    net_down_label = lv_label_create(main_screen);
    lv_label_set_text(net_down_label, "Down: -- KB/s");
    lv_obj_set_style_text_color(net_down_label, lv_color_hex(0x88FF88), 0);
    lv_obj_set_pos(net_down_label, col1_x, y_pos);
    
    net_up_label = lv_label_create(main_screen);
    lv_label_set_text(net_up_label, "Up: -- KB/s");
    lv_obj_set_style_text_color(net_up_label, lv_color_hex(0xFF8888), 0);
    lv_obj_set_pos(net_up_label, col2_x, y_pos);
    y_pos += line_height;
    
    // === 第八行：内部IP + 外部IP ===
    local_ip_label = lv_label_create(main_screen);
    lv_label_set_text(local_ip_label, "Local: ---.---.---.---");
    lv_obj_set_style_text_color(local_ip_label, lv_color_hex(0xCCFF66), 0);
    lv_obj_set_pos(local_ip_label, col1_x, y_pos);
    
    external_ip_label = lv_label_create(main_screen);
    lv_label_set_text(external_ip_label, "Ext: ---.---.---.---");
    lv_obj_set_style_text_color(external_ip_label, lv_color_hex(0xFF66CC), 0);
    lv_obj_set_pos(external_ip_label, col2_x, y_pos);
}

void SCREEN_DISPLAY_ENHANCED::displayAida64Data(std::vector<AIDA64_DATA> &dataList) {
    updateSystemInfo(dataList);
}

void SCREEN_DISPLAY_ENHANCED::updateTimeDisplay(const String& timeString) {
    if (time_label) {
        char timeBuffer[64];
        snprintf(timeBuffer, sizeof(timeBuffer), "System Time: %s", timeString.c_str());
        lv_label_set_text(time_label, timeBuffer);
        
        // 设置时间同步状态颜色
        if (timeString == "--:--:--") {
            // 未同步时显示红色
            lv_obj_set_style_text_color(time_label, lv_color_hex(0xFF4444), 0);
        } else {
            // 已同步时显示绿色
            lv_obj_set_style_text_color(time_label, lv_color_hex(0x00FF00), 0);
        }
        
        displayPrintLog("Time updated: %s\r\n", timeString.c_str());
    }
}

void SCREEN_DISPLAY_ENHANCED::updateSystemInfo(std::vector<AIDA64_DATA> &dataList) {
    char buffer[64];
    bool display_updated = false;
    
    displayPrintLog("Updating system info with %d items\r\n", dataList.size());
    
    for (const auto& data : dataList) {
        displayPrintLog("Processing: ID=%s, Value=%s\r\n", data.id, data.val);
        
        // 解析数值数据
        const char* value_str = data.val;
        
        // 根据AIDA64的配置处理特定的ID
        if (strcmp(data.id, "Simple1") == 0) {
            // CPU使用率
            float percentage = 0.0f;
            if (sscanf(value_str, ">CPU使用率%f%%", &percentage) == 1 || 
                sscanf(value_str, ">CPU 使用率 %f%%", &percentage) == 1) {
                if (cpu_bar && cpu_label) {
                    lv_bar_set_value(cpu_bar, (int)percentage, LV_ANIM_ON);
                    snprintf(buffer, sizeof(buffer), "%.1f%%", percentage);
                    lv_label_set_text(cpu_label, buffer);
                    displayPrintLog("Updated CPU: %.1f%%\r\n", percentage);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple7") == 0) {
            // 内存使用率
            float percentage = 0.0f;
            if (sscanf(value_str, ">内存使用率%f%%", &percentage) == 1 ||
                sscanf(value_str, ">内存 使用率 %f%%", &percentage) == 1) {
                if (mem_bar && mem_label) {
                    lv_bar_set_value(mem_bar, (int)percentage, LV_ANIM_ON);
                    snprintf(buffer, sizeof(buffer), "%.1f%%", percentage);
                    lv_label_set_text(mem_label, buffer);
                    displayPrintLog("Updated Memory: %.1f%%\r\n", percentage);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple2") == 0) {
            // CPU温度
            float temp = 0.0f;
            if (sscanf(value_str, ">中央处理器(CPU) %f", &temp) == 1) {
                if (temp_label) {
                    snprintf(buffer, sizeof(buffer), "CPU: %.0f°C", temp);
                    lv_label_set_text(temp_label, buffer);
                    displayPrintLog("Updated CPU Temp: %.0f°C\r\n", temp);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple5") == 0) {
            // GPU温度
            float temp = 0.0f;
            if (sscanf(value_str, ">GPU 1 %f", &temp) == 1) {
                if (gpu_temp_label) {
                    snprintf(buffer, sizeof(buffer), "GPU: %.0f°C", temp);
                    lv_label_set_text(gpu_temp_label, buffer);
                    displayPrintLog("Updated GPU Temp: %.0f°C\r\n", temp);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple8") == 0) {
            // 已用内存
            float memory_mb = 0.0f;
            if (sscanf(value_str, ">已用内存 %f MB", &memory_mb) == 1) {
                if (mem_usage_label) {
                    // 当内存使用超过1GB时，显示GB单位
                    if (memory_mb >= 1024.0f) {
                        float memory_gb = memory_mb / 1024.0f;
                        snprintf(buffer, sizeof(buffer), "Used: %.1f GB", memory_gb);
                        displayPrintLog("Updated Memory Usage: %.1f GB\r\n", memory_gb);
                    } else {
                        snprintf(buffer, sizeof(buffer), "Used: %.0f MB", memory_mb);
                        displayPrintLog("Updated Memory Usage: %.0f MB\r\n", memory_mb);
                    }
                    lv_label_set_text(mem_usage_label, buffer);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple3") == 0) {
            // CPU频率
            float freq = 0.0f;
            if (sscanf(value_str, ">CPU 核心频率 %f MHz", &freq) == 1 ||
                sscanf(value_str, ">CPU核心频率%fMHz", &freq) == 1) {
                if (cpu_freq_label) {
                    // 当频率超过1000MHz时，显示GHz单位
                    if (freq >= 1000.0f) {
                        float freq_ghz = freq / 1000.0f;
                        snprintf(buffer, sizeof(buffer), "CPU: %.2f GHz", freq_ghz);
                        displayPrintLog("Updated CPU Freq: %.2f GHz\r\n", freq_ghz);
                    } else {
                        snprintf(buffer, sizeof(buffer), "CPU: %.0f MHz", freq);
                        displayPrintLog("Updated CPU Freq: %.0f MHz\r\n", freq);
                    }
                    lv_label_set_text(cpu_freq_label, buffer);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple4") == 0) {
            // CPU功耗
            float power = 0.0f;
            if (sscanf(value_str, ">CPU Package %f W", &power) == 1 ||
                sscanf(value_str, ">CPUPackage%fW", &power) == 1) {
                if (cpu_power_label) {
                    snprintf(buffer, sizeof(buffer), "CPU: %.1f W", power);
                    lv_label_set_text(cpu_power_label, buffer);
                    displayPrintLog("Updated CPU Power: %.1f W\r\n", power);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple6") == 0) {
            // GPU功耗
            float power = 0.0f;
            if (sscanf(value_str, ">GPU 1 %f W", &power) == 1 ||
                sscanf(value_str, ">GPU1%fW", &power) == 1) {
                if (gpu_power_label) {
                    snprintf(buffer, sizeof(buffer), "GPU: %.1f W", power);
                    lv_label_set_text(gpu_power_label, buffer);
                    displayPrintLog("Updated GPU Power: %.1f W\r\n", power);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple11") == 0) {
            // GPU使用率
            float percentage = 0.0f;
            if (sscanf(value_str, ">GPU1 使用率 %f%%", &percentage) == 1 ||
                sscanf(value_str, ">GPU1使用率%f%%", &percentage) == 1) {
                if (gpu_bar && gpu_label) {
                    lv_bar_set_value(gpu_bar, (int)percentage, LV_ANIM_ON);
                    snprintf(buffer, sizeof(buffer), "%.1f%%", percentage);
                    lv_label_set_text(gpu_label, buffer);
                    displayPrintLog("Updated GPU: %.1f%%\r\n", percentage);
                    display_updated = true;
                }
            }
        }
        else if (strcmp(data.id, "Simple9") == 0) {
            // 下载速率
            if (net_down_label) {
                // 解析网络速率，格式: >NIC[1-7]下载速率数值单位
                float rate_value = 0.0f;
                char rate_unit[16] = "KB/s";
                
                // 使用正则表达式风格的解析，匹配数值和单位
                const char* ptr = value_str;
                while (*ptr && !isdigit(*ptr) && *ptr != '.') ptr++; // 跳过非数字字符
                
                if (*ptr) {
                    // 提取数值
                    if (sscanf(ptr, "%f", &rate_value) == 1) {
                        // 查找单位
                        while (*ptr && (isdigit(*ptr) || *ptr == '.')) ptr++;
                        
                        // 提取单位部分（跳过中文字符）
                        const char* unit_start = ptr;
                        int unit_idx = 0;
                        while (*unit_start && unit_idx < 15) {
                            if ((*unit_start >= 'A' && *unit_start <= 'Z') || 
                                (*unit_start >= 'a' && *unit_start <= 'z') ||
                                *unit_start == '/' || *unit_start == 's') {
                                rate_unit[unit_idx++] = *unit_start;
                            }
                            unit_start++;
                        }
                        rate_unit[unit_idx] = '\0';
                        
                        if (strlen(rate_unit) == 0) {
                            strcpy(rate_unit, "KB/s");
                        }
                        
                        // 单位转换：当速度超过1MB/s时显示MB/s
                        if (strcmp(rate_unit, "KB/s") == 0 && rate_value >= 1024.0f) {
                            rate_value = rate_value / 1024.0f;
                            strcpy(rate_unit, "MB/s");
                            snprintf(buffer, sizeof(buffer), "Down: %.1f %s", rate_value, rate_unit);
                        } else {
                            snprintf(buffer, sizeof(buffer), "Down: %.1f %s", rate_value, rate_unit);
                        }
                    }
                } else {
                    snprintf(buffer, sizeof(buffer), "Down: 0.0 KB/s");
                }
                
                lv_label_set_text(net_down_label, buffer);
                displayPrintLog("Updated Download: %s (raw: %s)\r\n", buffer, value_str);
                display_updated = true;
            }
        }
        else if (strcmp(data.id, "Simple10") == 0) {
            // 上传速率
            if (net_up_label) {
                // 解析网络速率，格式: >NIC[1-7]上传速率数值单位
                float rate_value = 0.0f;
                char rate_unit[16] = "KB/s";
                
                // 使用正则表达式风格的解析，匹配数值和单位
                const char* ptr = value_str;
                while (*ptr && !isdigit(*ptr) && *ptr != '.') ptr++; // 跳过非数字字符
                
                if (*ptr) {
                    // 提取数值
                    if (sscanf(ptr, "%f", &rate_value) == 1) {
                        // 查找单位
                        while (*ptr && (isdigit(*ptr) || *ptr == '.')) ptr++;
                        
                        // 提取单位部分（跳过中文字符）
                        const char* unit_start = ptr;
                        int unit_idx = 0;
                        while (*unit_start && unit_idx < 15) {
                            if ((*unit_start >= 'A' && *unit_start <= 'Z') || 
                                (*unit_start >= 'a' && *unit_start <= 'z') ||
                                *unit_start == '/' || *unit_start == 's') {
                                rate_unit[unit_idx++] = *unit_start;
                            }
                            unit_start++;
                        }
                        rate_unit[unit_idx] = '\0';
                        
                        if (strlen(rate_unit) == 0) {
                            strcpy(rate_unit, "KB/s");
                        }
                        
                        // 单位转换：当速度超过1MB/s时显示MB/s
                        if (strcmp(rate_unit, "KB/s") == 0 && rate_value >= 1024.0f) {
                            rate_value = rate_value / 1024.0f;
                            strcpy(rate_unit, "MB/s");
                            snprintf(buffer, sizeof(buffer), "Up: %.1f %s", rate_value, rate_unit);
                        } else {
                            snprintf(buffer, sizeof(buffer), "Up: %.1f %s", rate_value, rate_unit);
                        }
                    }
                } else {
                    snprintf(buffer, sizeof(buffer), "Up: 0.0 KB/s");
                }
                
                lv_label_set_text(net_up_label, buffer);
                displayPrintLog("Updated Upload: %s (raw: %s)\r\n", buffer, value_str);
                display_updated = true;
            }
        }
        else if (strcmp(data.id, "Simple12") == 0) {
            // 主IP地址
            if (local_ip_label) {
                // 提取IP地址部分
                const char* ip_start = value_str;
                while (*ip_start && !isdigit(*ip_start)) {
                    ip_start++;
                }
                
                if (ip_start && *ip_start) {
                    snprintf(buffer, sizeof(buffer), "Local: %s", ip_start);
                } else {
                    snprintf(buffer, sizeof(buffer), "Local: ---.---.---.---");
                }
                lv_label_set_text(local_ip_label, buffer);
                displayPrintLog("Updated Local IP: %s\r\n", value_str);
                display_updated = true;
            }
        }
        else if (strcmp(data.id, "Simple13") == 0) {
            // 外部IP地址
            if (external_ip_label) {
                // 提取IP地址部分
                const char* ip_start = value_str;
                while (*ip_start && !isdigit(*ip_start)) {
                    ip_start++;
                }
                
                if (ip_start && *ip_start) {
                    snprintf(buffer, sizeof(buffer), "Ext: %s", ip_start);
                } else {
                    snprintf(buffer, sizeof(buffer), "Ext: ---.---.---.---");
                }
                lv_label_set_text(external_ip_label, buffer);
                displayPrintLog("Updated External IP: %s\r\n", value_str);
                display_updated = true;
            }
        }
        else if (strcmp(data.id, "Simple14") == 0) {
            // 已用显存
            float vram_mb = 0.0f;
            if (sscanf(value_str, ">已用显存 %f MB", &vram_mb) == 1 ||
                sscanf(value_str, ">已用显存%fMB", &vram_mb) == 1) {
                if (gpu_mem_label) {
                    // 当显存使用超过1GB时，显示GB单位
                    if (vram_mb >= 1024.0f) {
                        float vram_gb = vram_mb / 1024.0f;
                        snprintf(buffer, sizeof(buffer), "VRAM: %.1f GB", vram_gb);
                        displayPrintLog("Updated GPU Memory: %.1f GB\r\n", vram_gb);
                    } else {
                        snprintf(buffer, sizeof(buffer), "VRAM: %.0f MB", vram_mb);
                        displayPrintLog("Updated GPU Memory: %.0f MB\r\n", vram_mb);
                    }
                    lv_label_set_text(gpu_mem_label, buffer);
                    display_updated = true;
                }
            }
        }
    }
    
    // 如果有数据更新，强制刷新显示
    if (display_updated) {
        lv_obj_invalidate(main_screen);
        lv_refr_now(disp);
        displayPrintLog("Display refreshed after data update\r\n");
    }
}

void SCREEN_DISPLAY_ENHANCED::clear() {
    if (main_screen) {
        lv_obj_clean(main_screen);
        createUI();
    }
}

void SCREEN_DISPLAY_ENHANCED::updateDisplay() {
    // LVGL 自动处理显示更新
}

void SCREEN_DISPLAY_ENHANCED::tick() {
    lv_timer_handler();
}

// 静态回调函数
void SCREEN_DISPLAY_ENHANCED::disp_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    SCREEN_DISPLAY_ENHANCED* display = (SCREEN_DISPLAY_ENHANCED*)disp_drv->user_data;
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    display->tft.startWrite();
    display->tft.setAddrWindow(area->x1, area->y1, w, h);
    display->tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    display->tft.endWrite();
    
    lv_disp_flush_ready(disp_drv);
}

void SCREEN_DISPLAY_ENHANCED::disp_flush_ready(lv_disp_drv_t* disp_drv) {
    // 刷新完成回调
}

// 全局实例
SCREEN_DISPLAY_ENHANCED display_enhanced;
