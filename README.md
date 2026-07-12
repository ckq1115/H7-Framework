# H7_Framework

基于 STM32H723xx 的机器人控制框架，当前默认应用为 `User/App/1_Catapult_Hero`。
工程集成了底盘、云台、发射、IMU、超级电容、遥控器、裁判系统和双板通信等模块。

## 项目简介

- MCU：STM32H723xx
- 构建方式：CMake
- 当前应用：`1_Catapult_Hero`
- 输出文件：`.elf`、`.hex`、`.bin`

## 目录说明

- `Core/`：HAL / CMSIS 核心代码
- `Drivers/`：芯片与外设驱动
- `Middlewares/`：第三方中间件
- `USB_DEVICE/`：USB 设备相关代码
- `User/Utils`：通用工具、数学、离线检测
- `User/BSP`：板级驱动（UART、FDCAN、SPI、TIM、DWT）
- `User/Device`：具体外设驱动（IMU、电机、遥控器、裁判系统、功率模块等）
- `User/System`：消息中心、系统状态、系统反馈
- `User/Algorithm`：控制、滤波、运动学、功率控制
- `User/App/1_Catapult_Hero`：当前机器人应用

## 当前应用结构

- `System_Init`：系统初始化入口
- `Command`：指令中心，汇总遥控器 / 键鼠 / 上位机输入
- `IMU_Task`：IMU 初始化、温控、校准、姿态融合
- `Chassis_Task`：底盘运动学与功率控制
- `Catapult_Task`：发射机构与云台控制
- `All_Task`：任务调度与定时器回调
- `Robot_Config`：电机组与 PWM 等硬件配置

## 主要机制

- 使用消息中心（Pub/Sub）在任务间传递数据
- UART 接收遥控器、VT13、裁判系统数据
- FDCAN 连接电机、超级电容与双板通信
- TIM4 周期中断驱动系统节拍、离线检测和状态刷新
- 系统状态模块统一管理任务健康、全局模式与错误码

## 构建方法

1. 使用 CMake 配置工程。
2. 在根目录 `CMakeLists.txt` 中确认当前应用：
   - `set(ACTIVE_APP "1_Catapult_Hero")`
3. 编译后会生成 `.elf`、`.hex`、`.bin` 文件。

## 切换应用

如果需要切换到其他机器人应用，只需修改根目录 `CMakeLists.txt` 中的 `ACTIVE_APP` 变量，然后重新配置并编译即可。

## 运行说明

- 上电后先完成系统初始化，再进入各任务调度。
- IMU 支持 VQF / Mahony / EKF 姿态解算。
- 安全模式下会自动清零底盘、云台和发射输出，避免误动作。
