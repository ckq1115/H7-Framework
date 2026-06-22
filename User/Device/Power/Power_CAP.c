#include "Power_CAP.h"
#include <string.h>

Cap_t cap;
int open_cap_flag = 0;

/**
 * @brief 电容接收数据解算
 */
void Power_Cap_Rx(void *instance, uint8_t *rx_buf)
{
    if (instance == NULL || rx_buf == NULL) return;

    CapRxData_t *cap_ptr = instance;

    if (rx_buf[7] == 0xAA)
    {
        cap_ptr->offline.last_feed_tick = HAL_GetTick();
        cap_ptr->cap_key   = rx_buf[0];
        cap_ptr->cap_state = rx_buf[1];

        uint16_t raw_voltage = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
        uint16_t raw_power   = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];

        cap_ptr->bat_voltage = (float)raw_voltage / 100.0f;
        cap_ptr->nowPower    = (float)raw_power / 100.0f;

        cap_ptr->Cap_Capacity = rx_buf[6];
        cap_ptr->check_code = rx_buf[7];
    }
}

/**
 * @brief 电容控制数据发送
 */
void Power_Cap_Tx(hcan_t *hcan, uint16_t can_id, Cap_t *cap, float power_limit, Referee_Data_t *referee)
{
    if (cap == NULL || referee == NULL) return;

    cap->set.Control.power_key      = (uint8_t)open_cap_flag;
    cap->set.Control.capPowerLimit  = (uint8_t)power_limit;
    cap->set.Control.robot_state    = (referee->robot_status.current_HP > 0) ? 1 : 0;
    cap->set.Control.check_code     = 0xAA;

    FDCAN_Send_Msg(hcan, can_id, cap->set.raw_data, 8);
}