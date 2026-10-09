# myself_foc_g431

基于 **STM32G431** + **HAL 库** 的 FOC（磁场定向控制）学习工程：从 VF 开环到闭环的电机控制实践。

本项目用于记录 FOC 算法的手写实现过程，配套 ST7789 屏幕实时显示中间变量、
VOFA+ 上位机波形观察，以及 MT6701 磁编码器角度反馈。

---

## 硬件平台

| 项目 | 参数 |
| --- | --- |
| 主控 | STM32G431（Cortex-M4F，带硬件浮点） |
| 驱动板 | 三相驱动板（死区由硬件限制，软件无需配置） |
| 母线电压 | 24 V |
| 功率级 | TIM1 三相互补 PWM（CH1/CH2/CH3 + CH1N/CH2N/CH3N） |
| PWM 频率 | 20 kHz，中心对齐，CH4 触发 FOC 中断 |
| 角度反馈 | MT6701 磁编码器（14 位，16384 线，SPI 读取） |
| 电机 | 极对数 Pn = 10 |
| 显示 | ST7789 LCD（SPI，160×80） |
| 上位机 | USB CDC 虚拟串口 → VOFA+ 波形 |

> 原理图、电机参数表、LCD 屏参考资料体积较大，未纳入版本库。

---

## FOC 算法实现

代码位于 `myapp/`，算法按"一步一步手推公式"的思路拆分，便于对照教材阅读：

```
编码器电角度 Theta
      │
      ├─ 反 Park 变换   Rev_Park_Transf()      (Vd, Vq) → (Valpha, Vbeta)
      │
      ├─ 反 Clark 变换  Rev_Clark_Transf()     (Valpha, Vbeta) → (Vu, Vv, Vw)
      │
      └─ SVPWM         SVPWM_ZeroSqInject()    零序分量注入 → Tcmp1/2/3
                                                       │
                                                       └→ Motor_SetPWM() → TIM1 CCR
```

- **反 Park**：`Valpha = Vd·cosθ − Vq·sinθ`，`Vbeta = Vd·sinθ + Vq·cosθ`
- **反 Clark**：等幅值变换，`Vv`/`Vw` 由 `Valpha`/`Vbeta` 线性组合得到
- **SVPWM**：不用七段式扇区判断，改用**零序分量注入**（取三相最大/最小值求中点偏移），
  等效提升约 15% 直流电压利用率，实现更直观

---

## 目录结构

```
myself_foc_g431/
├── myself_foc_g431.ioc      # CubeMX 配置源文件（改外设先改这里再生成）
├── myapp/                   # ★ 自己写的应用层代码
│   ├── FocAlgorithm.c/.h    #   FOC 核心：反Park、反Clark、SVPWM
│   ├── Encoder.c/.h         #   MT6701 编码器读取、机械角/电角度换算
│   ├── Motor_Config.h       #   ★ 电机与系统参数集中配置（改参数看这里）
│   ├── Hardware_Driver.c/.h #   PWM 使能/关闭、比较值下发
│   ├── interrupt.c/.h       #   TIM1 CH4 中断回调 → 运行 FOC 状态机
│   ├── StateMachine.c/.h    #   时间片轮询调度器 + MotorSystem 总结构体
│   ├── Vofa.c/.h            #   VOFA+ 波形上报（USB CDC，非阻塞）
│   ├── lcd_app.c/.h         #   把电机中间变量刷到 LCD
│   ├── lcd_st7789.c/.h      #   ST7789 屏幕驱动
│   ├── key_app.c/.h         #   按键
│   └── led_app.c/.h         #   指示灯
├── Core/                    # CubeMX 生成：main.c、外设初始化、中断向量
├── Drivers/                 # STM32G4 HAL 库 + CMSIS
├── Middlewares/             # USB Device 协议栈
├── USB_Device/              # USB CDC 应用层（Vofa 走这个虚拟串口）
└── MDK-ARM/                 # Keil 工程文件（.uvprojx）
```

## 关键参数

集中定义在 `myapp/Motor_Config.h`：

```c
#define FOC_HZ        20000      /* FOC 中断频率 20kHz */
#define FOC_TS        (1/(float)FOC_HZ)
#define ANGLE_2PI     6.2831853f
#define UDC           24.0f      /* 母线电压 */
#define TPWM          4249.0f    /* PWM 周期计数值 */
#define ENCODER_LINE  16383      /* 编码器线数 */
#define Pn            10         /* 电机极对数 */
```

---

## 编译烧录

1. 用 **Keil MDK-ARM 5** 打开 `MDK-ARM/myself_foc_g431.uvprojx`
2. 编译（F7），下载器按你的实际硬件选择（ST-Link / J-Link）
3. 上电后 LCD 会显示编码器值、电角度、机械角、Vd/Vq、VF 速度等实时数据

## 调参 / 看波形

- 想改上报的变量，编辑 `myapp/Vofa.c` 里的 `Vofa_Send_Task()`，往
  `VofaSend.Send_Data_Array[]` 里塞即可
- VOFA+ 连上 USB 虚拟串口，用 **JustFloat** 协议解析，通道数要与数组长度一致

---

## 说明

- 仓库**只提交源码**：Keil 编译产物（`.o` / `.axf` / `.hex` 等）已通过 `.gitignore` 排除，
  这类文件每次编译都会重新生成，提交它们只会让仓库无意义地膨胀
- `.ioc` 文件是"源头"：外设配置改动请在 CubeMX 里做，生成的代码会覆盖 `Core/` 下的相应段落，
  自己的代码记得写在 `/* USER CODE BEGIN */ ... /* USER CODE END */` 之间，否则会被覆盖

## 进度

- [x] 三相 PWM 输出与死区
- [x] MT6701 编码器角度读取与电角度换算
- [x] 反 Park / 反 Clark / SVPWM 零序注入
- [x] LCD 实时显示 + VOFA+ 波形上报
- [ ] 电流采样与电流环（Clarke / Park 正变换）
- [ ] 速度环、位置环
- [ ] 电流环 PI 参数整定
