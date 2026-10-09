#ifndef __FOCALGORITHM_H__
#define __FOCALGORITHM_H__

#include "stdint.h"
/**
 * @brief FOC参数结构体
 *
 */
typedef struct
{
	float Udc;       /* 母线电压 */
	float Tpwm;      /* PWM周期 */
    float Vd;        /* d轴电压 */
    float Vq;        /* q轴电压 */
    float Theta;     /* 电机角度 */

    float Valpha;    /* alpha电压 */
    float Vbeta;     /* beta电压 */

    float Vu;        /* U相电压 */
    float Vv;        /* V相电压 */
    float Vw;        /* W相电压 */

    float Vu_Mod;    /* U相调制电压 */
    float Vv_Mod;    /* V相调制电压 */
    float Vw_Mod;    /* W相调制电压 */
	
	int Tcmp1;       /* U相比较值 */
	int Tcmp2;       /* V相比较值 */
	int Tcmp3;       /* W相比较值 */
}Foc_TypeDef;

/**
 * @brief Vf角度生成器参数结构体
 *
 */
typedef struct
{
	/* 输入参数 */
	float Acc;               /* 加速度 */
	float Target_Speed;      /* 目标速度 */
	float Dead_Zone;         /* 死区阈值 */
	float Ts;                /* 步长 */
	
	/* 中间参数 */
	float Speed_Error;       /* 速度误差 */
	float Speed_Error_Dead;  /* 带死区的速度误差 */
	float Acc_Dir;           /* 加速度方向 */
	float Speed_Integrator;  /* 速度积分器 */
	float Theta_Integrator;  /* 角度积分器 */
	
	/* 输出参数 */
	float Speed;             /* 实际速度 */
	float Theta;             /* 实际角度 */
	
	
}Vf_SpeedControl_TypeDef;

#include "bsp_system.h"







void Rev_Park_Transf(Foc_TypeDef *state);
void Rev_Clark_Transf(Foc_TypeDef *state);
void SVPWM_ZeroSqInject(Foc_TypeDef *state);
void Set_Udc_Tpwm_Param(Foc_TypeDef*state,float udc_temp,float Tpwm_temp);
void Foc_VoltageUpdate(Foc_TypeDef*state);
void Vf_SpeedControl_Param_Init(Vf_SpeedControl_TypeDef*state,float acc,float target_speed,float dead_zone,float ts);
void Vf_SpeedControl_Update(Vf_SpeedControl_TypeDef*state,float *Target_Theta_Acc);
//extern Foc_TypeDef Foc; 

#endif
