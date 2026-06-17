#include "Power_Ctrl.h"
#include <math.h>

void Power_Ctrl_Init(Power_Ctrl_Instance_t *ctrl)
{
    ctrl->Kp = 2.0f;
    ctrl->target_buffer = 40.0f;
    ctrl->cap_mode = 0;
    ctrl->cap_is_open = 0;
    ctrl->basic_limit = 0;
    ctrl->actual_limit = 0;
    ctrl->total_pred_power = 0;
    ctrl->meter.buffer_energy = MAX_BUFFER_ENERGY;

    ctrl->m3508.k1 = 1.5756e-02f;
    ctrl->m3508.k2 = 1.94e-01f;
    ctrl->m3508.k3 = 1.9202e-05f;
    ctrl->m3508.k4 = 1.15f;
    ctrl->m3508.current_convert = 20.0f / 16384.0f;

    ctrl->m6020.k1 = 0.751f;
    ctrl->m6020.k2 = 2.5f;
    ctrl->m6020.k3 = 2.1e-5f;
    ctrl->m6020.k4 = 1.15f;
    ctrl->m6020.current_convert = 3.0f / 16384.0f;
}

void CAN_Power_Rx(Power_Ctrl_Instance_t *ctrl, uint8_t *rx_data)
{
    int16_t raw_shunt = (int16_t)(rx_data[0] << 8 | rx_data[1]);
    int16_t raw_bus   = (int16_t)(rx_data[2] << 8 | rx_data[3]);
    int16_t raw_curr  = (int16_t)(rx_data[4] << 8 | rx_data[5]);

    ctrl->meter.shunt_volt = (float)raw_shunt / 1000.0f;
    ctrl->meter.bus_volt   = (float)raw_bus   / 1000.0f;
    ctrl->meter.current    = (float)raw_curr  / 1000.0f;
    ctrl->meter.power      = ctrl->meter.bus_volt * ctrl->meter.current;
}

void Buffer_Calc(Power_Ctrl_Instance_t *ctrl, float dt, float ref_power_limit)
{
    float limit = (ref_power_limit > 0) ? ref_power_limit : DEFAULT_POWER_LIMIT;
    ctrl->meter.buffer_energy += (limit - ctrl->meter.power) * dt;

    if (ctrl->meter.buffer_energy > MAX_BUFFER_ENERGY) {
        ctrl->meter.buffer_energy = MAX_BUFFER_ENERGY;
    } else if (ctrl->meter.buffer_energy < 0.0f) {
        ctrl->meter.buffer_energy = 0.0f;
    }
}

static inline float predict_motor_power(DJI_MOTOR_Typedef *motor, Power_Motor_Model_t *m, float I_cmd)
{
    float w = motor->DATA.Speed_now * POWER_RPM_TO_RAD;
    float I = I_cmd * m->current_convert;
    return (m->k1 * w * I) + (m->k2 * I * I) + (m->k3 * w * w) + m->k4;
}

static void solve_motor_group(DJI_MOTOR_Typedef *motors[4], float I_cmd[4], float P_limit, Power_Motor_Model_t *m)
{
    float A = 0.0f, B = 0.0f, C_base = 4.0f * m->k4;

    for (int i = 0; i < 4; i++) {
        float w = motors[i]->DATA.Speed_now * POWER_RPM_TO_RAD;
        float I = I_cmd[i] * m->current_convert;
        A += m->k2 * I * I;
        B += m->k1 * w * I;
        C_base += m->k3 * w * w;
    }

    if (A + B + C_base <= P_limit) return;

    float C = C_base - P_limit;
    float scale = 1.0f;

    if (A > 1e-6f) {
        float delta = B * B - 4.0f * A * C;
        if (delta >= 0) {
            scale = (-B + sqrtf(delta)) / (2.0f * A);
        } else {
            scale = 0.0f;
        }
    } else if (B > 1e-6f) {
        scale = -C / B;
    }

    scale = (scale > 1.0f) ? 1.0f : ((scale < 0.0f) ? 0.0f : scale);
    for (int i = 0; i < 4; i++) {
        I_cmd[i] *= scale;
    }
}

uint8_t Power_Ctrl_Execute(Power_Ctrl_Instance_t *ctrl, User_Data_T *referee, Cap_t *cap, MOTOR_Typdef *motors)
{
    float ref_buffer = referee->power_heat_data.buffer_energy;
    float ref_limit  = referee->robot_status.chassis_power_limit;

    float power_comp = -ctrl->Kp * (ctrl->target_buffer - ref_buffer);
    ctrl->basic_limit = (ref_limit != 0) ? (ref_limit + power_comp) : DEFAULT_POWER_LIMIT;

    float available_super_power = CAP_SUPER_POWER_MAX;
    float current_cap_vol = cap->get.Cap_Capacity;

    if (current_cap_vol < CAP_THRESHOLD_CAPACITY && current_cap_vol > CAP_MIN_CAPACITY) {
        available_super_power *= (current_cap_vol - CAP_MIN_CAPACITY) / (CAP_THRESHOLD_CAPACITY - CAP_MIN_CAPACITY);
    }

    if (ctrl->cap_mode == 1 && cap->get.cap_state == 0 && current_cap_vol > CAP_MIN_CAPACITY) {
        ctrl->cap_is_open = 1;
        ctrl->actual_limit = ctrl->basic_limit + available_super_power;
    } else if (ctrl->cap_mode == 2) {
        ctrl->cap_is_open = 0;
        ctrl->actual_limit = ctrl->basic_limit;
    } else {
        ctrl->cap_is_open = 0;
        ctrl->actual_limit = ctrl->basic_limit - 5.0f;
    }

    float I_cmd_3508[4], I_cmd_6020[4];
    float p3508_pred = 0.0f, p6020_pred = 0.0f;
    DJI_MOTOR_Typedef *ptr_3508[4], *ptr_6020[4];

    for (int i = 0; i < 4; i++) {
        ptr_3508[i] = &motors->DJI_3508_Chassis[i];
        ptr_6020[i] = &motors->DJI_6020_Steer[i];

        I_cmd_3508[i] = ptr_3508[i]->PID_S.Output;
        I_cmd_6020[i] = ptr_6020[i]->PID_S.Output;

        p3508_pred += predict_motor_power(ptr_3508[i], &ctrl->m3508, I_cmd_3508[i]);
        p6020_pred += predict_motor_power(ptr_6020[i], &ctrl->m6020, I_cmd_6020[i]);
    }

    if (p3508_pred < 0) p3508_pred = 0;
    if (p6020_pred < 0) p6020_pred = 0;

    float total_pred = p3508_pred + p6020_pred;

    if (total_pred > ctrl->actual_limit) {
        float limit_3508 = ctrl->actual_limit - p6020_pred;
        if (limit_3508 < 0) limit_3508 = 0;

        solve_motor_group(ptr_3508, I_cmd_3508, limit_3508, &ctrl->m3508);

        if (p6020_pred > ctrl->actual_limit) {
            solve_motor_group(ptr_6020, I_cmd_6020, ctrl->actual_limit, &ctrl->m6020);
        }
    }

    ctrl->total_pred_power = 0.0f;
    for (int i = 0; i < 4; i++) {
        motors->DJI_3508_Chassis[i].PID_S.Output = I_cmd_3508[i];
        motors->DJI_6020_Steer[i].PID_S.Output   = I_cmd_6020[i];

        ctrl->total_pred_power += predict_motor_power(ptr_3508[i], &ctrl->m3508, I_cmd_3508[i]) +
                                  predict_motor_power(ptr_6020[i], &ctrl->m6020, I_cmd_6020[i]);
    }

    return 0;
}