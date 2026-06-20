//
// Created by CaoKangqi on 2026/2/14.
//

#ifndef G4_FRAMEWORK_ALL_MOTOR_H
#define G4_FRAMEWORK_ALL_MOTOR_H

#include "DJI_Motor.h"
#include "DM_Motor.h"
#include "LK_Motor.h"

typedef struct
{
    DJI_MOTOR_Typedef DJI_6020_Pitch;
    DJI_MOTOR_Typedef DJI_6020_Yaw;

    DJI_MOTOR_Typedef DJI_3508_Shoot_L;
    DJI_MOTOR_Typedef DJI_3508_Shoot_R;
    DJI_MOTOR_Typedef DJI_3508_Shoot_M;

    DJI_MOTOR_Typedef DJI_3508_Chassis[4];
    DJI_MOTOR_Typedef DJI_6020_Steer[4];

    DJI_MOTOR_Typedef DJI_3508_Pull;
    DJI_MOTOR_Typedef DJI_3508_Travel;
    DJI_MOTOR_Typedef DJI_3508_Yaw;
    DJI_MOTOR_Typedef DJI_2006_bo;

    DM_MOTOR_Typdef DM4310_Feed;
    DM_MOTOR_Typdef DM4310_Pitch;
    DM_MOTOR_Typdef DM4310_Yaw;
    LK_MOTOR_Typedef LK9025_Yaw;
}MOTOR_Typdef;

extern MOTOR_Typdef All_Motor;

#endif //G4_FRAMEWORK_ALL_MOTOR_H