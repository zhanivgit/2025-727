#include "ti_msp_dl_config.h"
#include "main.h"
#include "stdio.h"

// -- OpenMV Packet Parsing & State Machine --
#define UART_RX_BUFFER_SIZE 5
uint8_t g_uartRxBuffer[UART_RX_BUFFER_SIZE];
uint8_t g_rxIndex = 0;

typedef enum {
    STATE_WAITING,
    STATE_GOTO_P0,
    STATE_GOTO_P1,
    STATE_GOTO_P2,
    STATE_GOTO_P3
} ControlState;

volatile ControlState g_control_state = STATE_WAITING;
volatile int g_rect_corners[4][2];
volatile uint8_t g_corners_received_mask = 0; // 用位掩码记录收到了哪些顶点
// 全局变量，存储当前的目标坐标和激光坐标 (为中心测试模式保留)
volatile int g_current_target[2] = {80, 60}; // 初始化为中心
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

    // 设置初始目标为画面中心 (中心测试模式)
    // g_current_target[0] = 110;
    // g_current_target[1] = 20;

    while (1)
    {
        switch (g_control_state) {
            case STATE_WAITING:
                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t *)"WAITING...", 16);
                if (g_corners_received_mask == 15) {
                    g_corners_received_mask = 0;
                    g_control_state = STATE_GOTO_P0;
                }
                break;

            case STATE_GOTO_P0:
            case STATE_GOTO_P1:
            case STATE_GOTO_P2:
            case STATE_GOTO_P3:
            {
                int target_index = g_control_state - STATE_GOTO_P0;
                int target_x = g_rect_corners[target_index][0];
                int target_y = g_rect_corners[target_index][1];

                Servo_TrackPID(target_x, target_y, g_laser_coord[0], g_laser_coord[1]);

                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t *)"GOTO P", 16);
                OLED_ShowNum(48, 0, target_index, 1, 16);
                OLED_ShowString(0, 2, (uint8_t *)"T:", 16);
                OLED_ShowNum(16, 2, target_x, 3, 16);
                OLED_ShowNum(48, 2, target_y, 3, 16);
                OLED_ShowString(0, 4, (uint8_t *)"L:", 16);
                OLED_ShowNum(16, 4, g_laser_coord[0], 3, 16);
                OLED_ShowNum(48, 4, g_laser_coord[1], 3, 16);

                int error_x = g_laser_coord[0] - target_x;
                int error_y = g_laser_coord[1] - target_y;
                if (error_x > -5 && error_x < 5 && error_y > -5 && error_y < 5) {
                    delay_ms(500);
                    if (g_control_state == STATE_GOTO_P3) {
                        g_control_state = STATE_WAITING;
                    } else {
                        g_control_state++;
                    }
                }
                break;
            }
        }
        // 持续使用PID让激光点追踪当前目标 (中心测试模式)
        // Servo_TrackPID(g_current_target[0], g_current_target[1], g_laser_coord[0], g_laser_coord[1]);

        // 在OLED上显示调试信息 (中心测试模式)
        // if(g_new_data_received) // 只在收到新数据时刷新OLED，节省资源
        // {
        //     g_new_data_received = false;
        //     OLED_Clear();
        //     OLED_ShowString(0, 0, (uint8_t *)"Target:", 16);
        //     OLED_ShowNum(56, 0, g_current_target[0], 3, 16);
        //     OLED_ShowNum(88, 0, g_current_target[1], 3, 16);
            
        //     OLED_ShowString(0, 2, (uint8_t *)"Laser:", 16);
        //     OLED_ShowNum(56, 2, g_laser_coord[0], 3, 16);
        //     OLED_ShowNum(88, 2, g_laser_coord[1], 3, 16);
        // }
        
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
                if (index < 4) { // 索引0-3是矩形顶点
                    g_rect_corners[index][0] = g_uartRxBuffer[2]; // X
                    g_rect_corners[index][1] = g_uartRxBuffer[3]; // Y
                    g_corners_received_mask |= (1 << index); // 设置对应位
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
