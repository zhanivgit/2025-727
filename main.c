#include "ti_msp_dl_config.h"
#include "main.h"
#include "stdio.h"

// -- OpenMV Packet Parsing --
#define UART_RX_BUFFER_SIZE 5
uint8_t g_uartRxBuffer[UART_RX_BUFFER_SIZE];
uint8_t g_rxIndex = 0;

volatile int g_rect_corners[4][2];
volatile uint8_t g_corners_received_mask = 0; // 用位掩码记录收到了哪些顶点
// 全局变量，存储当前的目标坐标和激光坐标
volatile int g_current_target[2] = {80, 60}; // 初始化为中心，将被矩形中心覆盖
volatile int g_laser_coord[2] = {80, 60};    // 初始化为中心
volatile bool g_new_data_received = false; // 用于OLED刷新

// -- End OpenMV --

uint8_t oled_buffer[32];

int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();
    NVIC_ClearPendingIRQ(UART_OPENMV_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_OPENMV_INST_INT_IRQN);
    OLED_Init();
    Interrupt_Init();
    Servo_PID_Init();

    while (1)
    {
        // 持续使用PID让激光点追踪当前目标
        Servo_TrackPID(g_current_target[0], g_current_target[1], g_laser_coord[0], g_laser_coord[1]);

        // 在OLED上显示调试信息
        if(g_new_data_received) // 只在收到新数据时刷新OLED，节省资源
        {
            g_new_data_received = false;
            OLED_Clear();
            OLED_ShowString(0, 0, (uint8_t *)"Target:", 16);
            OLED_ShowNum(56, 0, g_current_target[0], 3, 16);
            OLED_ShowNum(88, 0, g_current_target[1], 3, 16);
            
            OLED_ShowString(0, 2, (uint8_t *)"Laser:", 16);
            OLED_ShowNum(56, 2, g_laser_coord[0], 3, 16);
            OLED_ShowNum(88, 2, g_laser_coord[1], 3, 16);
        }
        
        delay_ms(20); // 循环延时
    }
}

void UART_OPENMV_INST_IRQHandler(void)
{
    uint8_t gData;
    switch (DL_UART_Main_getPendingInterrupt(UART_OPENMV_INST)) {
        case DL_UART_MAIN_IIDX_RX:
            gData = DL_UART_Main_receiveData(UART_OPENMV_INST);

            if (g_rxIndex == 0 && gData == 0xFF) { // 帧头
                g_uartRxBuffer[g_rxIndex++] = gData;
            } else if (g_rxIndex > 0 && g_rxIndex < 4) { // 数据体
                g_uartRxBuffer[g_rxIndex++] = gData;
            } else if (g_rxIndex == 4 && gData == 0xFE) { // 帧尾
                uint8_t index = g_uartRxBuffer[1];
                if (index == 5) { // 索引5是矩形中心
                    g_current_target[0] = g_uartRxBuffer[2]; // X
                    g_current_target[1] = g_uartRxBuffer[3]; // Y
                } else if (index == 4) { // 索引4是激光点，更新当前位置
                    g_laser_coord[0] = g_uartRxBuffer[2]; // X
                    g_laser_coord[1] = g_uartRxBuffer[3]; // Y
                }
                g_new_data_received = true;
                g_rxIndex = 0;
            } else {
                g_rxIndex = 0;
            }
            break;
        default:
            break;
    }
}
