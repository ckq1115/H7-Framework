#include "Power_Ctrl.h"
#include <math.h>

void Power_Ctrl_Init(Power_Ctrl_t *ctrl) {
    ctrl->Kp = 2.0f;
    ctrl->target_buffer = 40.0f;
    ctrl->total_pred_power = 0.0f;

    ctrl->m3508.k1 = 1.5756e-02f; ctrl->m3508.k2 = 1.94e-01f;
    ctrl->m3508.k3 = 1.9202e-05f;  ctrl->m3508.k4 = 1.15f;
    ctrl->m3508.current_convert = 20.0f / 16384.0f;

    ctrl->m6020.k1 = 0.751f;      ctrl->m6020.k2 = 2.5f;
    ctrl->m6020.k3 = 2.1e-5f;     ctrl->m6020.k4 = 1.15f;
    ctrl->m6020.current_convert = 3.0f / 16384.0f;
}

static inline float predict_motor_power(float speed_rpm, Power_Motor_Model_t *m, float I_cmd) {
    float w = speed_rpm * POWER_RPM_TO_RAD;
    float I = I_cmd * m->current_convert;
    return (m->k1 * w * I) + (m->k2 * I * I) + (m->k3 * w * w) + m->k4;
}

static void solve_motor_group(Motor_Power_State_t group[4], float P_limit, Power_Motor_Model_t *m) {
    float A = 0.0f, B = 0.0f, C_base = 4.0f * m->k4;

    for (int i = 0; i < 4; i++) {
        float w = group[i].speed_rpm * POWER_RPM_TO_RAD;
        float I = group[i].original_cmd * m->current_convert;
        A += m->k2 * I * I;
        B += m->k1 * w * I;
        C_base += m->k3 * w * w;
    }

    if (A + B + C_base <= P_limit) {
        for(int i=0; i<4; i++) group[i].limited_cmd = group[i].original_cmd;
        return;
    }

    float C = C_base - P_limit;
    float scale = 1.0f;

    if (A > 1e-6f) {
        float delta = B * B - 4.0f * A * C;
        if (delta >= 0) scale = (-B + sqrtf(delta)) / (2.0f * A);
        else scale = 0.0f;
    } else if (B > 1e-6f) {
        scale = -C / B;
    }

    scale = (scale > 1.0f) ? 1.0f : ((scale < 0.0f) ? 0.0f : scale);
    for (int i = 0; i < 4; i++) {
        group[i].limited_cmd = group[i].original_cmd * scale;
    }
}

void Power_Ctrl_Calculate(Power_Ctrl_t *ctrl,
                           float allowed_limit,
                           float cur_buffer,
                           Motor_Power_State_t m3508_group[4],
                           Motor_Power_State_t m6020_group[4])
{
    float power_comp = -ctrl->Kp * (ctrl->target_buffer - cur_buffer);
    float actual_limit = allowed_limit + power_comp;

    float p3508_pred = 0.0f, p6020_pred = 0.0f;
    for (int i = 0; i < 4; i++) {
        p3508_pred += predict_motor_power(m3508_group[i].speed_rpm, &ctrl->m3508, m3508_group[i].original_cmd);
        p6020_pred += predict_motor_power(m6020_group[i].speed_rpm, &ctrl->m6020, m6020_group[i].original_cmd);
    }
    if (p3508_pred < 0) p3508_pred = 0;
    if (p6020_pred < 0) p6020_pred = 0;

    if ((p3508_pred + p6020_pred) > actual_limit) {
        float limit_3508 = actual_limit - p6020_pred;
        if (limit_3508 < 0) limit_3508 = 0;

        solve_motor_group(m3508_group, limit_3508, &ctrl->m3508);
        if (p6020_pred > actual_limit) {
            solve_motor_group(m6020_group, actual_limit, &ctrl->m6020);
        }
    } else {
        for(int i=0; i<4; i++) {
            m3508_group[i].limited_cmd = m3508_group[i].original_cmd;
            m6020_group[i].limited_cmd = m6020_group[i].original_cmd;
        }
    }

    ctrl->total_pred_power = 0.0f;
    for (int i = 0; i < 4; i++) {
        ctrl->total_pred_power += predict_motor_power(m3508_group[i].speed_rpm, &ctrl->m3508, m3508_group[i].limited_cmd) +
                                  predict_motor_power(m6020_group[i].speed_rpm, &ctrl->m6020, m6020_group[i].limited_cmd);
    }
}