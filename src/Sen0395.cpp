#include <string.h>
#include <stdio.h>
#include "Sen0395.h"

#include "driver/uart.h"

#include <memory>
#include <vector>

#define SEN0395_PORT (CONFIG_SEN0395_UART_PORT_NUM)
#define SEN0395_TXD (CONFIG_SEN0395_UART_TXD_PIN)
#define SEN0395_RXD (CONFIG_SEN0395_UART_RXD_PIN)
#define SEN0395_RTS (UART_PIN_NO_CHANGE)
#define SEN0395_CTS (UART_PIN_NO_CHANGE)
#define SEN0395_BAUD (CONFIG_SEN0395_UART_BAUD)
#define SEN0395_UART_BUF_SIZE (CONFIG_SEN0395_UART_BUF_SIZE)
#define SEN0395_DETECT_DELAY (CONFIG_SEN0395_DETECT_DELAY)
#define SEN0395_LEAVE_DELAY (CONFIG_SEN0395_LEAVE_DELAY)
#define SEN0395_AUTOSTART (CONFIG_SEN0395_AUTOSTART)

#define RD_BUF_SIZE (1024)

Sen0395 *Sen0395::s_instance = nullptr;

Sen0395* Sen0395::GetInstance()
{
    if (s_instance == nullptr)
    {
        s_instance = new Sen0395();
    }
    return s_instance;
}

Sen0395::Sen0395()
{

}


void Sen0395::EventHandler(void*)
{
    uart_event_t event;
    char* buf = (char*) malloc(RD_BUF_SIZE);
    auto me = GetInstance();
    for (;;)
    {
        if (xQueueReceive(me->m_uartQueue, (void*)&event, (TickType_t)portMAX_DELAY))
        {
            bzero(buf, RD_BUF_SIZE);
            switch(event.type) {
                case UART_DATA:
                    if (event.size !=  17)
                    {
                         continue;
                    }
                    uart_read_bytes(SEN0395_PORT, buf, event.size, 100 / portTICK_RATE_MS);
                    uart_flush(SEN0395_PORT);
                    buf[event.size] = '\0';
                    if (! strncmp(buf, "$JYBSS,", 7))
                    {
                        bool detected = (buf[7] == '1');
                        if (detected != me->m_isDetected)
                        {
                            me->m_callback(detected);
                        }
                        me->m_isDetected = detected;
                    }
                    break;
                default:
                    printf("EVENT: %d\n", event.type);
                    break;
            }
        }
    }
    free(buf);
}

bool Sen0395::SendCommand(const std::string& cmd)
{
    char *buf = (char*) malloc(256);
    uart_flush(SEN0395_PORT);
    uart_write_bytes(SEN0395_PORT, (const char*) cmd.c_str(), cmd.size());
    int sz = uart_read_bytes(SEN0395_PORT, buf, 256, 1000 / portTICK_RATE_MS);
    buf[sz] = '\0';
    if (strstr(buf, "Done") != nullptr)
    {
        free(buf);
        return true;
    }
    else
    {
        printf("CMD FAILED: %d %s\n", sz, buf);
        free(buf);
        return false;
    }
}

void Sen0395::Start()
{
    auto me = GetInstance();
    me->Initialize();
    me->SendCommand("sensorStop");
    char *buf = (char*) malloc(64);
    snprintf(buf, 64, "outputLatency %d %d %d", -1, SEN0395_DETECT_DELAY, SEN0395_LEAVE_DELAY);
    me->SendCommand(buf);
    snprintf(buf, 64, "sensorCfgStart  %d", SEN0395_AUTOSTART);
    me->SendCommand(buf);
    me->SendCommand("saveCfg 0x45670123 0xCDEF89AB 0x956128C6 0xDF54AC89");
    me->SendCommand("sensorStart");
    free(buf);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    uart_flush(SEN0395_PORT);
    xTaskCreate(EventHandler, "EventHandler", 2048*4, nullptr, tskIDLE_PRIORITY, &(me->m_taskHandle));
}


void Sen0395::Initialize()
{
    uart_config_t uart_config = {
        .baud_rate = SEN0395_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    ESP_ERROR_CHECK(uart_driver_install(SEN0395_PORT, SEN0395_UART_BUF_SIZE, SEN0395_UART_BUF_SIZE, 10, &m_uartQueue, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(SEN0395_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(SEN0395_PORT, SEN0395_RXD, SEN0395_TXD, SEN0395_RTS, SEN0395_CTS));
}
