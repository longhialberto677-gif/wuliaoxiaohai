#include "FocAlgorithm.h"

//Foc_TypeDef Foc;           //Foc结构体变量

/* ============================================================================
 * 反 Park 变换
 *
 * 
 * 
 * ==========================================================================*/
void Rev_Park_Transf(Foc_TypeDef *state)
{

    state->Valpha = state->Vd * cosf(state->Theta) - state->Vq * sinf(state->Theta);
    state->Vbeta  = state->Vd * sinf(state->Theta) + state->Vq * cosf(state->Theta);
}

/* ============================================================================
 * 反 Clark 变换
 * ==========================================================================*/
void Rev_Clark_Transf(Foc_TypeDef *state)
{
    state->Vu = state->Valpha;
    state->Vv = -0.5f * state->Valpha + 0.8660254f * state->Vbeta;
    state->Vw = -0.5f * state->Valpha - 0.8660254f * state->Vbeta;
}

/* ============================================================================
 * 零序分量注入 + 转三相 PWM 比较值
 * ==========================================================================*/
void SVPWM_ZeroSqInject(Foc_TypeDef *state)
{
    float Max;
    float Min;
    float V0;

    /* 寻找三相最大值 */
    Max = (state->Vu > state->Vv) ? ((state->Vu > state->Vw) ? state->Vu : state->Vw)
                                  : ((state->Vv > state->Vw) ? state->Vv : state->Vw);

    /* 寻找三相最小值 */
    Min = (state->Vu < state->Vv) ? ((state->Vu < state->Vw) ? state->Vu : state->Vw)
                                  : ((state->Vv < state->Vw) ? state->Vv : state->Vw);

    V0 = -0.5f * (Max + Min);

    /* 计算三相马鞍波电压 */
    state->Vu_Mod = V0 + state->Vu;
    state->Vv_Mod = V0 + state->Vv;
    state->Vw_Mod = V0 + state->Vw;

    /* 将三相马鞍电压转化成三相PWM比较值 */
    state->Tcmp1 = (int)(((state->Vu_Mod) / state->Udc + 0.5f) * state->Tpwm);
    state->Tcmp2 = (int)(((state->Vv_Mod) / state->Udc + 0.5f) * state->Tpwm);
    state->Tcmp3 = (int)(((state->Vw_Mod) / state->Udc + 0.5f) * state->Tpwm);

}
/* ============================================================================
 * 设置母线电压和PWM周期函数
 * ==========================================================================*/
void Set_Udc_Tpwm_Param(Foc_TypeDef*state,float udc_temp,float Tpwm_temp)
{
	state->Udc = udc_temp;
	state->Tpwm = Tpwm_temp;
}
/* ============================================================================
 * 坐标变换电压更新函数
 * ==========================================================================*/
void Foc_VoltageUpdate(Foc_TypeDef*state)
{
	/* 反Park变换 */
	Rev_Park_Transf(state);
	/* 反Clark变换 */
    Rev_Clark_Transf(state);
	/* SVPWM */
    SVPWM_ZeroSqInject(state);
}
/* ============================================================================
 * VF速度控制参数初始化函数
 * ==========================================================================*/
void Vf_SpeedControl_Param_Init(Vf_SpeedControl_TypeDef*state,float acc,float target_speed,float dead_zone,float ts)
{
	state->Acc = acc;
	state->Target_Speed = target_speed;
	state->Dead_Zone = dead_zone;
	state->Ts = ts;
	
}
/* ============================================================================
 * VF速度控制周期函数
 * ==========================================================================*/
void Vf_SpeedControl_Update(Vf_SpeedControl_TypeDef*state,float *Target_Theta_Acc)
{
	/* 计算速度误差 */
	state->Speed_Error = state->Target_Speed - state->Speed_Integrator;
	/* 死区处理 */
	if(fabs(state->Speed_Error) < state->Dead_Zone)
	{
		state->Speed_Error_Dead = 0;
	}
	else
	{
		state->Speed_Error_Dead = state->Speed_Error;
	}
	/* 判断加速度方向 */
	if(state->Speed_Error_Dead > 0)
	{
		state->Acc_Dir = 1;
	}
	else if(state->Speed_Error_Dead < 0)
	{
		state->Acc_Dir = -1;
	}
	else
	{
		state->Acc_Dir = 0;
	}
	/* 加速度积分得到速度 */
	state->Speed_Integrator += state->Acc_Dir * state->Acc * state->Ts;
	
	/* 速度经过速度积分得到角度 */
	state->Theta_Integrator += state->Speed_Integrator * state->Ts;
	
	/* 计算输出 */
	state->Theta = fmod(state->Theta_Integrator,ANGLE_2PI);
	if(state->Theta < 0)
	{
		state->Theta += ANGLE_2PI;
	}
	state->Speed = state->Speed_Integrator;
	/* 函数输出 */
	*Target_Theta_Acc = state->Theta;
}




