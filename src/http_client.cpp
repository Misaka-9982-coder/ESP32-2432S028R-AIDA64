#include "http_client.h"
#include <lwip/sockets.h>
#include "config.h"
#include "display.h"
#include <regex>

#define DISPLAY_AIDA64_DATA(data) display_enhanced.displayAida64Data(data)

char httpDataBuffer[4096];
std::vector<AIDA64_DATA> aida64DataList;

void taskHttpClient(void *param)
{
    httpPrintLog("taskHttpClient starting...\r\n");
    
    // 等待系统稳定
    delay(5000);
    httpPrintLog("Starting HTTP client after delay\r\n");
    
    // 首先做一个简单的连接测试
    bool connectionTested = false;
    while(!connectionTested)
    {
        httpPrintLog("Checking WiFi status: %d\r\n", WiFi.status());
        
        // 等待WiFi连接
        while(WiFi.status() != WL_CONNECTED)
        {
            httpPrintLog("WiFi not connected, waiting...\r\n");
            delay(2000);
        }
        
        httpPrintLog("WiFi connected, testing basic HTTP connection...\r\n");
        
        // 简单的HTTP GET测试
        HTTPClient httpClient;
        String testUrl = "http://" + String(HTTP_HOST) + ":" + String(HTTP_PORT) + "/";
        
        httpPrintLog("Testing connection to: %s\r\n", testUrl.c_str());
        
        httpClient.begin(testUrl);
        int httpCode = httpClient.GET();
        
        httpPrintLog("HTTP GET result: %d\r\n", httpCode);
        
        if(httpCode > 0) {
            String payload = httpClient.getString();
            httpPrintLog("Received %d bytes\r\n", payload.length());
            
            if(payload.length() > 0) {
                httpPrintLog("Basic connection test successful!\r\n");
                connectionTested = true;
            }
        } else {
            httpPrintLog("HTTP GET failed: %s\r\n", httpClient.errorToString(httpCode).c_str());
        }
        
        httpClient.end();
        
        if(!connectionTested) {
            httpPrintLog("Retrying basic connection test in 10 seconds...\r\n");
            delay(10000);
        }
    }
    
    httpPrintLog("Basic connection test completed, starting SSE...\r\n");
    
    // 现在开始正常的SSE连接循环
    while(1)
    {
        HTTPClient httpClient;
        WiFiClient *tcpStream = NULL;
        int httpCode = 0;
        int fd = 0;
        int recv_len = 0;
        
        // 检查WiFi状态
        while(WiFi.status() != WL_CONNECTED)
        {
            httpPrintLog("WiFi disconnected during SSE, waiting...\r\n");
            delay(2000);
        }
        
        // 测试TCP连接
        WiFiClient testClient;
        if (testClient.connect(HTTP_HOST, HTTP_PORT)) {
            httpPrintLog("TCP connection to %s:%d successful\r\n", HTTP_HOST, HTTP_PORT);
            testClient.stop();
        } else {
            httpPrintLog("TCP connection to %s:%d failed\r\n", HTTP_HOST, HTTP_PORT);
            delay(5000);
            continue;
        }

        // SSE连接
        String sseUrl = "http://" + String(HTTP_HOST) + ":" + String(HTTP_PORT) + "/sse";
        httpClient.begin(sseUrl);
        httpClient.addHeader("Accept", "text/event-stream");
        httpClient.addHeader("Cache-Control", "no-cache");
        httpCode = httpClient.GET();
        
        httpPrintLog("SSE GET %s result: %d\r\n", sseUrl.c_str(), httpCode);

        if(httpCode == HTTP_CODE_OK)
        {
            httpPrintLog("SSE connection established successfully\n");

            while (1)
            {
                tcpStream = httpClient.getStreamPtr();
                if (tcpStream == NULL) {
                    httpPrintLog("TCP stream is NULL!\n");
                    break;
                }
                
                fd = tcpStream->fd();
                if (fd < 0) {
                    httpPrintLog("Invalid socket fd: %d\n", fd);
                    break;
                }
                
                recv_len = recv(fd, httpDataBuffer, 1024, 0);

                if(recv_len <= 0)
                {
                    httpPrintLog("Connect error! recv_len: %d\n", recv_len);
                    break;
                }

                httpDataBuffer[recv_len] = '\0';
                httpPrintLog("Received %d bytes: %s\n", recv_len, httpDataBuffer);

                //parse data
                parseAida64Data(httpDataBuffer, aida64DataList);
                
                //display data if we have any
                if (!aida64DataList.empty()) {
                    DISPLAY_AIDA64_DATA(aida64DataList);
                }
            }
        }
        else
        {
            httpPrintLog("GET /sse failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
            httpPrintLog("SSE Failed! Code: %d\r\n", httpCode);
        }

        httpClient.end();
        httpPrintLog("SSE connection ended, retrying in 10 seconds\r\n");
        delay(10000);
    }
}

void parseAida64HTML(char *htmlData, std::vector<AIDA64_DATA> &dataList)
{
    /*
     * 接收到的HTML有如下结构
     * ...
     * <body onload="MyOnLoad()">
     * <div id="page0">
     * <span id="xxx1" ...>XXX</span>
     * <span id="xxx2" ...>XXX</span>
     * ...
     * <span id="xxxn" ...>XXX</span>
     * </div>
     * </body>
     * ...
     * 其中span标签的内容即是在AIDA64中设置的LCD项目，需要将id和内容提取出来，保存在adia64DataList中
     * 之后会发送请求获取刷新数据，通过对比id，修改adia64DataList中对应的值
     */
    
    AIDA64_DATA data = {0};
    std::string input(htmlData);

    aida64DataList.clear();
    httpPrintLog("htmlData:\r\n%s\r\n", htmlData);

    // 定义正则表达式
    std::regex pattern("<span id=\"(.*?)\".*?>(.*?)<\\/span>");

    // 搜索匹配项
    std::sregex_iterator it(input.begin(), input.end(), pattern);
    std::sregex_iterator end;
    
    while (it != end) {
        // 提取匹配的子串
        std::smatch match = *it;
        httpPrintLog("match: %s, %s\r\n", match[1].str().c_str(), match[2].str().c_str());

        // 加入aida64DataList
        strcpy(data.id, match[1].str().c_str());
        strcpy(data.val, match[2].str().c_str());
        aida64DataList.push_back(data);

        ++it;
    }

    return;
}

void parseAida64Data(char *src, std::vector<AIDA64_DATA> &dataList)
{
    /* 
     * AIDA64会回复以下格式的响应体:
     * data: Page0|{|}Simple2|2:55:48{|}Simple4|3%{|}Simple5|1097MHz{|}Simple6|40°C{|}...
     * 需要将数据从字符串中提取出来
     */
    
    AIDA64_DATA data = {0};
    std::string input(src);

    httpPrintLog("SSE Data received:\r\n%s\r\n", src);

    // 首先检查是否包含 "data:" 开头的SSE数据
    size_t dataPos = input.find("data:");
    if (dataPos == std::string::npos) {
        return; // 不是有效的SSE数据
    }

    // 提取 data: 后面的内容
    std::string dataLine = input.substr(dataPos + 5); // 跳过 "data:"
    
    // 清空现有数据列表以准备新数据
    dataList.clear();

    // 解析数据格式：Page0|{|}Simple2|2:55:48{|}Simple4|3%{|}...
    size_t pos = 0;
    bool skipFirst = true; // 跳过第一个 Page0|{|}
    
    while (pos < dataLine.length()) {
        // 查找下一个分隔符 {|}
        size_t delimPos = dataLine.find("{|}", pos);
        if (delimPos == std::string::npos) {
            break; // 没有更多数据
        }
        
        // 提取这一段：例如 "Simple2|2:55:48"
        std::string segment = dataLine.substr(pos, delimPos - pos);
        
        if (!skipFirst && !segment.empty()) {
            // 在段中查找 | 分隔符
            size_t pipePos = segment.find('|');
            if (pipePos != std::string::npos) {
                std::string id = segment.substr(0, pipePos);
                std::string value = segment.substr(pipePos + 1);
                
                // 移除value中的多余字符
                strremove(const_cast<char*>(value.c_str()), ' ');
                
                // 将解析的数据添加到列表
                memset(&data, 0, sizeof(data));
                strncpy(data.id, id.c_str(), sizeof(data.id) - 1);
                strncpy(data.val, (">" + value).c_str(), sizeof(data.val) - 1);
                
                dataList.push_back(data);
                
                httpPrintLog("Parsed: ID=%s, Value=%s\n", data.id, data.val);
            }
        }
        
        skipFirst = false;
        pos = delimPos + 3; // 跳过 {|}
    }

    httpPrintLog("Total parsed items: %d\n", dataList.size());
    return;
}

void strremove(char* src, char remove)
{
    char* c = src;

    do
    {
        if(*c == remove)
        {
            c++;
        }

        *src = *c;
        src++;
        c++;
    } while (*c != '\0');
    
    *src = '\0';
}
