#include "Encoder.h"

/**
 * @brief 编码器参数初始化
 * @param line：编码器线数
 * @param dir：编码器方向 （0同向，1反向）
 * @param MotorPn：电机极对数
 * @param Phase：编码器误差
 * @return 无
 */
void Encoder_Param_Init(Encoder_Typedef *state,uint32_t line,uint8_t dir,uint8_t MotorPn,uint32_t Phase)
{
	state->Line = line;
	state->Dir = dir;
	state->MotorPn = MotorPn;
	state->Phase = Phase;
	
}

/**
 * @brief 编码器角度更新函数
 * @param Data：读取到的MT6701数据
 * @return 无
 */

void Encoder_UpData(Encoder_Typedef *state,uint32_t Data)
{
	/* 判断编码器方向 */
	if(state->Dir == 0)
	{
		state->Pulse_Data = Data;
	}
	else
	{
		state->Pulse_Data = state->Line - Data;
	}
	uint32_t Temp = (state->Pulse_Data - state->Phase + state->Line) % state->Line;
	/* 计算电机 机械角度 */
	state->Theta = (float)Temp / (float)state->Line *ANGLE_2PI;
	/* 计算电机 电角度 */
	state->Theta_e = fmod(state->Theta * state->MotorPn,ANGLE_2PI);
	
}










