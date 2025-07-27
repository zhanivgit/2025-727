#include "Servo.h"
#include "ti_msp_dl_config.h" // SysConfig 生成的头文件

// PID 控制参数 (需要根据实际效果进行调整)
#define KP_X 0.2f
#define KI_X 0.0f // 暂时归零，以便调优Kp
#define KD_X 0.0f // 暂时归零，以便调优Kp

#define KP_Y 0.2f
#define KI_Y 0.0f // 暂时归零，以便调优Kp
#define KD_Y 0.0f // 暂时归零，以便调优Kp

// 图像中心坐标
#define CENTER_X 80
#define CENTER_Y 60

// 舵机角度限制
#define SERVO_ANGLE_MIN -135
#define SERVO_ANGLE_MAX 135

// PID 状态变量
float error_x = 0, last_error_x = 0, integral_x = 0;
float error_y = 0, last_error_y = 0, integral_y = 0;

// 全局变量，保存舵机当前角度, 初始化为中间位置90度
float g_servo_angle_x;
float g_servo_angle_y;


#define SERVO_MIN_PULSE 7800   // 0.5ms, 对应0度
#define SERVO_MAX_PULSE 7000  // 2.5ms, 对应180度
#define SERVO_PERIOD    8000 // 20ms, 50Hz


void servo_start(void)
{
    DL_TimerA_startCounter(PWM_SERVO_INST);
}

void Servo_ySetAngle(int16_t angle)
{
    // 用户确认此计算公式正确
    uint32_t pulse = SERVO_MIN_PULSE- (800 * (angle+45) / 270);//+45
   
    DL_TimerA_setCaptureCompareValue( PWM_SERVO_INST, pulse, GPIO_PWM_SERVO_C1_IDX );
}
void Servo_xSetAngle(int16_t angle)
{
    // 用户确认此计算公式正确
    uint32_t pulse = SERVO_MIN_PULSE- (800 * (angle+150) / 270);    //+150
   
    DL_TimerA_setCaptureCompareValue( PWM_SERVO_INST, pulse, DL_TIMER_CC_0_INDEX);
}

// 初始化PID控制器和舵机位置
void Servo_PID_Init(void)
{
    // 初始化PID变量
    error_x = 0;
    last_error_x = 0;
    integral_x = 0;
    error_y = 0;
    last_error_y = 0;
    integral_y = 0;
    g_servo_angle_x = 0.0f;
    g_servo_angle_y = 0.0f;
    Servo_xSetAngle(g_servo_angle_x);
    Servo_ySetAngle(g_servo_angle_y);
}

// PID追踪函数
void Servo_TrackPID(int target_x, int target_y, int current_x, int current_y)
{
    // 1. 计算误差 (目标位置 - 当前激光位置)
    error_x = (float)(target_x - current_x);
    error_y = (float)(target_y - current_y);

    // 2. 计算X轴PID输出
    // 当误差很小时，重置积分项，防止积分饱和导致漂移
    if (error_x > -2 && error_x < 2) {
        integral_x = 0;
    }
    integral_x += error_x;
    // 积分限幅
    if (integral_x > 300) integral_x = 300;
    if (integral_x < -300) integral_x = -300;
    
    float output_x = KP_X * error_x + KI_X * integral_x + KD_X * (error_x - last_error_x);
    last_error_x = error_x;

    // 3. 计算Y轴PID输出
    // 当误差很小时，重置积分项，防止积分饱和导致漂移
    if (error_y > -2 && error_y < 2) {
        integral_y = 0;
    }
    integral_y += error_y;
    // 积分限幅
    if (integral_y > 300) integral_y = 300;
    if (integral_y < -300) integral_y = -300;

    float output_y = KP_Y * error_y + KI_Y * integral_y + KD_Y * (error_y - last_error_y);
    last_error_y = error_y;

    // 4. 更新舵机角度
    // 注意：这里的 +/- 方向取决于舵机安装方向和运动方向定义，可能需要调整
    g_servo_angle_x -= output_x;
    g_servo_angle_y += output_y;

    // 5. 角度限幅
    if (g_servo_angle_x < SERVO_ANGLE_MIN) g_servo_angle_x = SERVO_ANGLE_MIN;
    if (g_servo_angle_x > SERVO_ANGLE_MAX) g_servo_angle_x = SERVO_ANGLE_MAX;
    if (g_servo_angle_y < SERVO_ANGLE_MIN) g_servo_angle_y = SERVO_ANGLE_MIN;
    if (g_servo_angle_y > SERVO_ANGLE_MAX) g_servo_angle_y = SERVO_ANGLE_MAX;

    // 6. 设置舵机
    Servo_xSetAngle(g_servo_angle_x);
    Servo_ySetAngle(g_servo_angle_y);
}
