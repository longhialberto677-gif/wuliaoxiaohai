#ifndef __MOTOR_CONFIG_H__
#define __MOTOR_CONFIG_H__

#include "bsp_system.h"

/* =========== 系统时序常量 ============ */
#define FOC_HZ            20000                      /* FOC中断频率20KHZ */  
#define FOC_TS            (1/(float) FOC_HZ)
/* =========== 数学常量 ============ */
#define ANGLE_2PI         6.2831853f                



/* =========== SVPWM常量 ============ */
#define UDC               24.0f
#define TPWM              4249.0f

/* =========== 编码器常量 ============ */
#define ENCODER_LINE      16383

/* =========== 电机参数 ============ */
#define Pn                10                         /* 电机极对数 */  

#endif
