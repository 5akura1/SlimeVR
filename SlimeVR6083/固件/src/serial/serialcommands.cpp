/*
    SlimeVR Code is placed under the MIT license
    Copyright (c) 2021 Eiren Rain & SlimeVR contributors

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "serialcommands.h"
#include "network/network.h"
#include "logging/Logger.h"
#include <CmdCallback.hpp>
#include "GlobalVars.h"
#include "batterymonitor.h"

#if ESP32
    #include "nvs_flash.h"
#endif

namespace SerialCommands {
    SlimeVR::Logging::Logger logger("串口指令");

    CmdCallback<5> cmdCallbacks;
    CmdParser cmdParser;
    CmdBuffer<64> cmdBuffer;

    void cmdSet(CmdParser * parser) {
        if(parser->getParamCount() != 1 && parser->equalCmdParam(1, "WIFI")  ) {
            if(parser->getParamCount() < 3) {
                logger.error("通过串口设置WiFi出现错误：缺少参数");
                logger.info("设置WiFi名：\"<SSID>\" 密码：\"<PASSWORD>\"");
            } else {
                WiFiNetwork::setWiFiCredentials(parser->getCmdParam(2), parser->getCmdParam(3));
                logger.info("通过串口设置WiFi成功：已存储新的WiFi凭据，正在重新连接");
            }
        } else {
            logger.error("错误：该变量无法识别");
        }
    }

    void cmdGet(CmdParser * parser) {
        if (parser->getParamCount() < 2) {
            return;
        }
        
        if (parser->equalCmdParam(1, "INFO")) {
            logger.info(
                "SlimeVR追踪器，主板：%d，惯性传感器：%d，构建版本：%d，固件版本：%s，IPv4地址：%s",
                BOARD,
                HARDWARE_MCU,
                FIRMWARE_BUILD_NUMBER,
                FIRMWARE_VERSION,
                WiFiNetwork::getAddress().toString().c_str()
            );
            // TODO Print sensors number and types
        }

        if (parser->equalCmdParam(1, "CONFIG")) {
            String str =
                "BOARD=%d\n" 
                "IMU=%d\n"
                "SECOND_IMU=%d\n"
                "IMU_ROTATION=%f\n"
                "SECOND_IMU_ROTATION=%f\n"
                "BATTERY_MONITOR=%d\n"
                "BATTERY_SHIELD_RESISTANCE=%d\n"
                "BATTERY_SHIELD_R1=%d\n"
                "BATTERY_SHIELD_R2=%d\n"
                "PIN_IMU_SDA=%d\n"
                "PIN_IMU_SCL=%d\n"
                "PIN_IMU_INT=%d\n"
                "PIN_IMU_INT_2=%d\n"
                "PIN_BATTERY_LEVEL=%d\n"
                "LED_PIN=%d\n"
                "LED_INVERTED=%d\n";

            Serial.printf(
                str.c_str(),
                BOARD,
                IMU,
                SECOND_IMU,
                IMU_ROTATION,
                SECOND_IMU_ROTATION,
                BATTERY_MONITOR,
                BATTERY_SHIELD_RESISTANCE,
                BATTERY_SHIELD_R1,
                BATTERY_SHIELD_R2,
                PIN_IMU_SDA,
                PIN_IMU_SCL,
                PIN_IMU_INT,
                PIN_IMU_INT_2,
                PIN_BATTERY_LEVEL,
                LED_PIN,
                LED_INVERTED
            );
        }
    }

    void cmdReport(CmdParser * parser) {
        // TODO Health and status report
    }

    void cmdReboot(CmdParser * parser) {
        logger.info("REBOOT");
        ESP.restart();
    }

    void cmdFactoryReset(CmdParser * parser) {
        logger.info("FACTORY RESET");

        configuration.reset();

        WiFi.disconnect(true); // Clear WiFi credentials
        #if ESP8266
            ESP.eraseConfig(); // Clear ESP config
        #elif ESP32
                nvs_flash_erase();
        #else
            #warning SERIAL COMMAND FACTORY RESET NOT SUPPORTED
            logger.info("目前还不支持使用串口指令恢复出厂设置");
            return;
        #endif

        #if defined(WIFI_CREDS_SSID) && defined(WIFI_CREDS_PASSWD)
            #warning FACTORY RESET does not clear your hardcoded WiFi credentials!
            logger.warn("请注意：恢复出厂设置并不会清空你的WiFi凭据！");
        #endif

        delay(3000);
        ESP.restart();
    }

    void setUp() {
        cmdCallbacks.addCmd("SET", &cmdSet);
        cmdCallbacks.addCmd("GET", &cmdGet);
        cmdCallbacks.addCmd("FRST", &cmdFactoryReset);
        cmdCallbacks.addCmd("REP", &cmdReport);
        cmdCallbacks.addCmd("REBOOT", &cmdReboot);
    }

    void update() {
        cmdCallbacks.updateCmdProcessing(&cmdParser, &cmdBuffer, &Serial);
    }
}
