#ifndef __ENCODER_H__
#define __ENCODER_H__


#include "stdint.h"
/**
 * @brief 编码器结构体
 *
 */
typedef struct
{
	/* 编码器参数 */
	uint32_t Line;         /* 编码器线数 */  
	uint8_t MotorPn;       /* 极对数 */  
	uint8_t Dir;           /* 编码器方向 */
	
	/* 编码器数据与误差 */
	uint32_t Pulse_Data;   /* 编码器原始数据 */
	uint32_t Phase;        /* 编码器误差 */
	
	/* 角度计算 */
	float Theta;           /* 机械角度 */
	float Theta_e;         /* 电角度 */
	
	
	
}Encoder_Typedef;
	 
#include "bsp_system.h"

void Encoder_Param_Init(Encoder_Typedef *state,uint32_t line,uint8_t dir,uint8_t MotorPn,uint32_t Phase);
void Encoder_UpData(Encoder_Typedef *state,uint32_t Data);


#endif
