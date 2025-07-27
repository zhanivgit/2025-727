#ifndef __SERVO_H
#define __SERVO_H

#include <stdint.h>

// 全局变量，保存舵机当前角度
extern float g_servo_angle_x;
extern float g_servo_angle_y;

void servo_start(void);
void Servo_ySetAngle(int16_t angle);
void Servo_xSetAngle(int16_t angle);

// 新增的舵机PID追踪函数
void Servo_PID_Init(void);
void Servo_TrackPID(int target_x, int target_y, int current_x, int current_y);

#endif
