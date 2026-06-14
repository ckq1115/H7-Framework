//
// Created by CaoKangqi on 2026/2/13.
//

#ifndef H7_FRAMEWORK_ALL_INIT_H
#define H7_FRAMEWORK_ALL_INIT_H

#include <string.h>
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "WS2812.h"
#include "Vofa.h"
#include "All_define.h"
#include "All_Motor.h"
#include "cmsis_os2.h"
#include "BSP-FDCAN.h"
#include "BSP_DWT.h"
#include "DBUS.h"
#include "DJI_Motor.h"
#include "DM_Motor.h"
#include "VT13.h"
#include "Referee.h"
#include "BSP_UART.h"
#include "BMI088driver.h"
#include "mahony_filter.h"
#include "IMU_Task.h"
#include "Power_CAP.h"

extern uint8_t DBUS_RX_DATA[18];
extern DBUS_Typedef DBUS;

extern uint8_t VT13_RX_DATA[21];
extern VT13_Typedef VT13;
extern VT13_UNION_Typdef VT13_UNION;

extern MOTOR_Typdef All_Motor;
void All_Init(void);
#endif //H7_FRAMEWORK_ALL_INIT_H