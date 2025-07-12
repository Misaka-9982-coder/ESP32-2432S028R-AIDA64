#include "config.h"
#include "display.h"
#include "wifi_client.h"

typedef enum
{
    WIFI_NOT_INIT,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_STATUS_MAX,
}WIFI_STATUS;

void taskWifiClient(void *param)
{
    unsigned long reconnectTick = 0;
    unsigned long connectBeginTick = 0;
    unsigned long connectingTick = 0;
    int wifiStatus = WIFI_NOT_INIT;

    wifiPrintLog("taskWifiClient run!\r\n");

    while(1)
    {
        if(WiFi.status() != WL_CONNECTED)
        {
            if(wifiStatus == WIFI_NOT_INIT)
            {
                wifiStatus = WIFI_CONNECTING;

                WiFi.begin(WIFI_SSID, WIFI_PASS);
                wifiPrintLog("Connect to " WIFI_SSID "\r\n");
                connectBeginTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTED)
            {
                wifiStatus = WIFI_CONNECTING;

                wifiPrintLog("Disconnect!\r\n");
                wifiPrintLog("Reconnect to " WIFI_SSID "\r\n");

                WiFi.reconnect();
                connectBeginTick = millis();
            }

            if(wifiStatus == WIFI_CONNECTING)
            {
                if(getElapsedTick(connectingTick) >= 1000)
                {
                    wifiPrintLog("Connecting...\r\n");
                    connectingTick = millis();
                }

                if(getElapsedTick(reconnectTick) >= 15000)
                {
                    wifiPrintLog("Connect timeout!\r\n");
                    wifiPrintLog("Reconnect\r\n");

                    WiFi.reconnect();
                    reconnectTick = millis();
                }

                if(getElapsedTick(connectBeginTick) >= 60000)
                {
                    display_enhanced.setPowerSave(1);//60s未连接关闭屏幕
                    wifiPrintLog("Screen enter PowerSave Mode\r\n");
                }
            }
        }
        else
        {
            if(wifiStatus == WIFI_CONNECTING)
            {
                wifiStatus = WIFI_CONNECTED;
                wifiPrintLog("Wifi Connect succeed!\r\n");

                display_enhanced.setPowerSave(0);
            }
        }

        delay(100);
    }
}

unsigned long getElapsedTick(unsigned long lastTick)
{
    return (millis() - lastTick);
}