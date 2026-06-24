#include "System_State.h"
#include "Buzzer.h"
#include "WS2812.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include "Message_Center.h"
#include "Referee.h"

System_State_t sys_state;
static Subscriber_t *referee_sub = NULL;
static Publisher_t  *sys_state_pub = NULL;

#define MAX_FRAGMENTS 32

#define RGB_OFF       0,   0,   0
#define RGB_RED       255, 0,   0
#define RGB_GREEN     0,   255, 0
#define RGB_BLUE      0,   0,   255
#define RGB_YELLOW    255, 200, 0
#define RGB_CYAN      0,   200, 200

typedef struct {
    uint16_t freq;
    uint16_t duration;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Step_t;

typedef struct {
    Step_t steps[MAX_FRAGMENTS];
    uint8_t total;
} Flow_t;

static const Flow_t Flow_Init = {.steps = {
    {880, 170,  RGB_RED},  // 880Hz (A5)
    {0,   50,  RGB_CYAN},
    {1100, 170, RGB_BLUE},  // 1100Hz (C#6)
    {0,    50, RGB_BLUE},
    {1320, 400, RGB_GREEN} // 1320Hz (E6)
    },
    .total = 5
};
static const Flow_t Flow_Hint = {.steps = {
    {2500, 120, RGB_GREEN},
    {0, 20, RGB_OFF},
    {3000, 180, RGB_GREEN}},
    .total = 3
};
static const Flow_t Flow_Lost = {.steps = {
    {1500, 200, RGB_BLUE},
    {0, 50, RGB_OFF},
    {1500, 200, RGB_BLUE}},
    .total = 3
};

static const Flow_t Flow_Remote_Recover = {.steps = {
    {2500, 150, RGB_BLUE},
    {0, 40, RGB_OFF},
    {3000, 150, RGB_GREEN}},
    .total = 3
};

// 动态错误报警流
static Flow_t Flow_Dynamic_Error;

// 控制器与状态机变量
static struct {
    bool     is_running;
    uint32_t timer_ms;
    Step_t   steps[MAX_FRAGMENTS];
    uint8_t  total;
    uint8_t  idx;
} ctrl;

static struct {
    bool ref_online;
    bool any_ref_pwr;
    bool chassis_pwr, chassis_grace;
    bool gimbal_pwr,  gimbal_grace;
    bool shoot_pwr,   shoot_grace;
} pwr_info;

static uint32_t state_timer = 0;
static System_Error_Code_u last_error = {0};
static const Flow_t *playing_flow = NULL;
static bool g_remote_is_online = false;

static void Action_Push(const Flow_t *flow);
static void Update_Power_Status(uint32_t now, Referee_Data_t *ref);
static bool Check_Boot_Sequence(uint32_t now);
static void Update_Error_Flags(bool remote_is_online, bool in_boot_grace_period);
static void Arbitrate_Global_Mode(uint32_t now);
static void Build_And_Play_Module_Error(void);


static bool Is_All_Tasks_Running(void) {
    return (sys_state.task_health.Chassis == STATUS_RUN &&
            sys_state.task_health.Gimbal  == STATUS_RUN &&
            sys_state.task_health.Shoot   == STATUS_RUN);
}

void System_State_Set_Remote_Status(bool is_online) {
    g_remote_is_online = is_online;
}

void System_State_Init(void) {
    memset(&sys_state, 0, sizeof(sys_state));
    sys_state.global_mode = GLOBAL_INIT_STAGE;
    memset(&ctrl, 0, sizeof(ctrl));

    referee_sub = SubRegister("referee_data", sizeof(Referee_Data_t));
    sys_state_pub = PubRegister("system_state", &sys_state, sizeof(System_State_t));

    System_Trigger_Notify(GLOBAL_INIT_STAGE);
}

void System_State_Report(Module_ID_e id, App_Status_e status) {
    if (id < MODULE_COUNT) {
        App_Status_e *health_array = (App_Status_e *)&sys_state.task_health;
        health_array[id] = status;
    }
}

void System_State_Update(void) {
    uint32_t now = HAL_GetTick();
    Referee_Data_t local_ref = {0};

    if (referee_sub) SubGetMessage(referee_sub, &local_ref);

    if (local_ref.offline.is_online)
    {
        sys_state.power_limit   = (float)local_ref.robot_status.chassis_power_limit;
        sys_state.buffer_energy = (float)local_ref.power_heat_data.buffer_energy;
        sys_state.robot_level   = local_ref.robot_status.robot_level;
        sys_state.remain_HP     = local_ref.robot_status.current_HP;
        sys_state.max_HP        = local_ref.robot_status.maximum_HP;

        uint8_t id = local_ref.robot_status.robot_id;
        if (id == 1 || id == 101) {
            sys_state.shooter_heat = (float)local_ref.power_heat_data.shooter_42mm_barrel_heat;
        } else {
            sys_state.shooter_heat = (float)local_ref.power_heat_data.shooter_17mm_barrel_heat;
        }
        sys_state.shooter_limit = (float)local_ref.robot_status.shooter_barrel_heat_limit;
        // 获取弹速
        if (local_ref.shoot_data.initial_speed > 10.0f) {
            sys_state.bullet_speed = local_ref.shoot_data.initial_speed;
        }
    }
    else
    {
        sys_state.power_limit   = 45.0f;
        sys_state.buffer_energy = 40.0f;
        sys_state.robot_level   = 1;
        sys_state.remain_HP     = 100;
        sys_state.max_HP        = 100;
        sys_state.shooter_heat  = 0.0f;
        sys_state.shooter_limit = 100.0f;
        sys_state.bullet_speed  = 15.0f;
    }

    Update_Power_Status(now, &local_ref);
    bool in_boot_grace_period = !Check_Boot_Sequence(now);

    Update_Error_Flags(g_remote_is_online, in_boot_grace_period);
    Arbitrate_Global_Mode(now);

    if (sys_state_pub) {
        PubPushMessage(sys_state_pub, &sys_state);
    }
}

// 电源状态与边沿检测提取
static void Update_Power_Status(uint32_t now, Referee_Data_t *ref) {
    pwr_info.ref_online = ref->offline.is_online;

    pwr_info.chassis_pwr = pwr_info.ref_online ? ref->robot_status.power_management_chassis_output : true;
    pwr_info.gimbal_pwr  = pwr_info.ref_online ? ref->robot_status.power_management_gimbal_output  : true;
    pwr_info.shoot_pwr   = pwr_info.ref_online ? ref->robot_status.power_management_shooter_output : true;

    static bool last_c = false, last_g = false, last_s = false;
    static uint32_t tick_c = 0, tick_g = 0, tick_s = 0;

    if (pwr_info.chassis_pwr && !last_c) tick_c = now;
    if (pwr_info.gimbal_pwr  && !last_g) tick_g = now;
    if (pwr_info.shoot_pwr   && !last_s) tick_s = now;

    last_c = pwr_info.chassis_pwr;
    last_g = pwr_info.gimbal_pwr;
    last_s = pwr_info.shoot_pwr;

    pwr_info.chassis_grace = pwr_info.chassis_pwr && ((now - tick_c) < 1800);
    pwr_info.gimbal_grace  = pwr_info.gimbal_pwr  && ((now - tick_g) < 1800);
    pwr_info.shoot_grace   = pwr_info.shoot_pwr   && ((now - tick_s) < 1800);

    pwr_info.any_ref_pwr = pwr_info.ref_online &&
                           (ref->robot_status.power_management_chassis_output ||
                            ref->robot_status.power_management_gimbal_output ||
                            ref->robot_status.power_management_shooter_output);
}

// 开机自检等待
static bool Check_Boot_Sequence(uint32_t now) {
    static bool init_done = false;
    if (init_done) return true;

    if (now < 1800 || (sys_state.global_mode == GLOBAL_INIT_STAGE && ctrl.is_running)) {
        return false;
    }

    if (now < 21800 && !Is_All_Tasks_Running() && !pwr_info.any_ref_pwr) {
        if (!ctrl.is_running && sys_state.global_mode == GLOBAL_INIT_STAGE) {
            WS2812_SetMode_Breathing(0, RGB_CYAN, 3.0f);
        }
        return false;
    }

    init_done = true;
    return true;
}

// 更新错误标志位
static void Update_Error_Flags(bool remote_is_online, bool in_boot_grace_period) {
    sys_state.error.bit.remote_lost  = !remote_is_online; // 直接赋值
    sys_state.error.bit.referee_lost = !pwr_info.ref_online;
    sys_state.error.bit.imu_fault    = (sys_state.task_health.IMU == STATUS_ERROR || sys_state.task_health.IMU == STATUS_INIT);

    if (Is_All_Tasks_Running() || in_boot_grace_period) {
        sys_state.error.bit.chassis_offline = 0;
        sys_state.error.bit.gimbal_offline  = 0;
        sys_state.error.bit.shoot_offline   = 0;
    } else {
        bool chassis_fault = (sys_state.task_health.Chassis == STATUS_LOST || sys_state.task_health.Chassis == STATUS_ERROR);
        bool gimbal_fault  = (sys_state.task_health.Gimbal  == STATUS_LOST || sys_state.task_health.Gimbal  == STATUS_ERROR);
        bool shoot_fault   = (sys_state.task_health.Shoot   == STATUS_LOST || sys_state.task_health.Shoot   == STATUS_ERROR);

        sys_state.error.bit.chassis_offline = chassis_fault && pwr_info.chassis_pwr && !pwr_info.chassis_grace;
        sys_state.error.bit.gimbal_offline  = gimbal_fault  && pwr_info.gimbal_pwr  && !pwr_info.gimbal_grace;
        sys_state.error.bit.shoot_offline   = shoot_fault   && pwr_info.shoot_pwr   && !pwr_info.shoot_grace;
    }
}

// 状态仲裁
static void Arbitrate_Global_Mode(uint32_t now) {
    if (now < 1800 || (sys_state.global_mode == GLOBAL_INIT_STAGE && ctrl.is_running)) {
        return;
    }

    Global_Mode_e next_mode = GLOBAL_NORMAL_MATCH;

    if (sys_state.error.bit.chassis_offline || sys_state.error.bit.gimbal_offline || sys_state.error.bit.shoot_offline) {
        next_mode = GLOBAL_MODULE_ERROR;
    } else if (sys_state.error.bit.remote_lost) {
        next_mode = GLOBAL_STANDBY;
    } else if (sys_state.error.bit.imu_fault) {
        next_mode = GLOBAL_SAFE_LOCK;
    }

    if (next_mode != sys_state.global_mode) {
        if (sys_state.global_mode != GLOBAL_NORMAL_MATCH && next_mode == GLOBAL_NORMAL_MATCH) {
            System_Trigger_Hint();
            WS2812_SetMode_Breathing(0, RGB_GREEN, 3.0f);
        } else {
            System_Trigger_Notify(next_mode);
        }
        sys_state.global_mode = next_mode;
        state_timer = now;
    } else {
        bool remote_recovered = (last_error.bit.remote_lost == 1) && (sys_state.error.bit.remote_lost == 0);
        if (remote_recovered && sys_state.global_mode != GLOBAL_NORMAL_MATCH) {
            Action_Push(&Flow_Remote_Recover);
            state_timer = now;
        }
    }
    last_error.all = sys_state.error.all;

    uint32_t interval_ms = 0;
    switch (sys_state.global_mode) {
        case GLOBAL_MODULE_ERROR: interval_ms = 1000; break;
        case GLOBAL_SAFE_LOCK:    interval_ms = 1200; break;
        case GLOBAL_STANDBY:      interval_ms = 1000; break;
        default: break;
    }

    if (interval_ms > 0 && (now - state_timer >= interval_ms) && !ctrl.is_running) {
        System_Trigger_Notify(sys_state.global_mode);
        state_timer = now;
    }
}

static void Safe_Buzzer_Set(uint16_t freq) {
    if (sys_state.task_health.IMU == STATUS_PREPARING && sys_state.global_mode != GLOBAL_INIT_STAGE) {
        Buzzer_Off();
    } else {
        Buzzer_Set_Freq(freq);
    }
}

static void Action_Push(const Flow_t *flow) {
    if (!flow || flow->total == 0) return;
    if (ctrl.is_running && playing_flow == flow) return;

    playing_flow = flow;
    memcpy(ctrl.steps, flow->steps, flow->total * sizeof(Step_t));
    ctrl.total = flow->total;
    ctrl.idx = 0;
    ctrl.is_running = true;
    ctrl.timer_ms = ctrl.steps[0].duration;

    Safe_Buzzer_Set(ctrl.steps[0].freq);
    WS2812_SetMode_Static(0, ctrl.steps[0].r, ctrl.steps[0].g, ctrl.steps[0].b);
}

// 动态报警序列生成器
static void Build_And_Play_Module_Error(void) {
    Flow_Dynamic_Error.total = 0;

    uint8_t offline_counts[MODULE_COUNT] = {0};

    if (sys_state.error.bit.chassis_offline) offline_counts[ID_CHASSIS] = ID_CHASSIS + 1;
    if (sys_state.error.bit.gimbal_offline)  offline_counts[ID_GIMBAL]  = ID_GIMBAL  + 1;
    if (sys_state.error.bit.shoot_offline)   offline_counts[ID_SHOOT]   = ID_SHOOT   + 1;
    if (sys_state.error.bit.vision_lost)     offline_counts[ID_VISION]  = ID_VISION  + 1;
    if (sys_state.error.bit.imu_fault)       offline_counts[ID_IMU]     = ID_IMU     + 1;

    for (int i = 0; i < MODULE_COUNT; i++) {
        if (offline_counts[i] > 0) {
            for (int beep = 0; beep < offline_counts[i]; beep++) {
                if (Flow_Dynamic_Error.total < MAX_FRAGMENTS - 2) {
                    Flow_Dynamic_Error.steps[Flow_Dynamic_Error.total++] = (Step_t){3000, 80, RGB_RED};
                    Flow_Dynamic_Error.steps[Flow_Dynamic_Error.total++] = (Step_t){0, 60, RGB_OFF};
                }
            }
            // 模块间间隔
            if (Flow_Dynamic_Error.total > 0) {
                Flow_Dynamic_Error.steps[Flow_Dynamic_Error.total - 1].duration = 800;
            }
        }
    }

    if (Flow_Dynamic_Error.total == 0) {
        Flow_Dynamic_Error.steps[0] = (Step_t){3000, 500, RGB_YELLOW};
        Flow_Dynamic_Error.steps[1] = (Step_t){0, 500, RGB_OFF};
        Flow_Dynamic_Error.total = 2;
    }

    Action_Push(&Flow_Dynamic_Error);
}

void System_Trigger_Notify(Global_Mode_e mode) {
    switch(mode) {
        case GLOBAL_INIT_STAGE:
            Action_Push(&Flow_Init);
            break;
        case GLOBAL_MODULE_ERROR:
        case GLOBAL_SAFE_LOCK:
            Build_And_Play_Module_Error();
            break;
        case GLOBAL_STANDBY:
            Action_Push(&Flow_Lost);
            break;
        default: break;
    }
}

void System_Trigger_Hint(void) {
    if (!ctrl.is_running) Action_Push(&Flow_Hint);
}

void System_State_Ticks(void) {
    if (sys_state.task_health.IMU == STATUS_PREPARING && sys_state.global_mode != GLOBAL_INIT_STAGE) {
        Buzzer_Off();
    }

    if (!ctrl.is_running) return;

    if (ctrl.timer_ms > 0) {
        ctrl.timer_ms--;
        return;
    }

    if (++ctrl.idx < ctrl.total) {
        ctrl.timer_ms = ctrl.steps[ctrl.idx].duration;

        Safe_Buzzer_Set(ctrl.steps[ctrl.idx].freq);

        WS2812_SetMode_Static(0, ctrl.steps[ctrl.idx].r, ctrl.steps[ctrl.idx].g, ctrl.steps[ctrl.idx].b);
    } else {
        Buzzer_Off();
        if (sys_state.global_mode == GLOBAL_NORMAL_MATCH) {
            WS2812_SetMode_Breathing(0, RGB_GREEN, 3.0f);
        } else {
            WS2812_SetMode_Static(0, RGB_OFF);
        }

        ctrl.is_running = false;
        playing_flow = NULL;
    }
}