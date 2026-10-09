#include "interrupt.h"



void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	/* 是否是定时器一通道四触发的中断 */
	if(htim->Instance == TIM1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
	{
		/* 运行FOC */
		Motor_StateMachine_Run(&MotorSystem);
	}
}

