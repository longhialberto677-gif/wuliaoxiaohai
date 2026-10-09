#include "StateMachine.h"

typedef struct
{
	void (*pointer)(void);
	uint32_t time_work;
	uint32_t time_last;
}task_t;

uint8_t task_num = 0;

task_t task[]=
{
	{led_proc,100,0},
	{key_proc,10,0},
	{lcd_proc,200,0}      /* 每 200ms 刷新一次 LCD */
};


void scheduler_init()
{
	task_num = sizeof(task)/sizeof(task_t);
}

void scheduler_run()
{
	uint32_t time_now = HAL_GetTick();
	for(uint8_t i=0;i<task_num;i++)
	{
		if(time_now >= task[i].time_last+task[i].time_work)
		{
			task[i].pointer();
			task[i].time_last = time_now;
		}
	}
}



Motor_TypeDef MotorSystem;
/**
 * @brief 电机状态机初始化
 * @param state : 电机状态机结构体指针
 * @return 无
 */
void Motor_StateMachine_Init(Motor_TypeDef *state)
{
	/* 硬件初始化 */
	Motor_Hardware_Init();
	/* 设置电机电压和母线电压PWM周期 */
	Set_Udc_Tpwm_Param(&state->Foc,UDC,TPWM);
	/* 初始化VF参数 */
	Vf_SpeedControl_Param_Init(&state->Vf,50,0,0.001,FOC_TS);
	/* 初始化编码器参数 */
	Encoder_Param_Init(&state->Encoder,ENCODER_LINE,0,Pn,7990);
}
/**
 * @brief 电机状态机运行函数
 * @param state : 电机状态机结构体指针
 * @return 无
 */
void Motor_StateMachine_Run(Motor_TypeDef *state)
{
	uint16_t data = 0;
	data = MT6701_ReadData();
	Encoder_UpData(&state->Encoder,data);
	/* 角度模式选择 */
	switch(state->Theta_Mode)
	{
		/* 定位状态 */
		case THETA_MODE_ZERO:
			state->Foc.Theta = 0;
		break;
		/* VF角度自增状态 */
		case THETA_MODE_VF:
			Vf_SpeedControl_Update(&state->Vf,&state->Foc.Theta);
		break;
		/* 编码器 */
		case THETA_MODE_ENCODER:
			state->Foc.Theta = state->Encoder.Theta_e;
		break;
		default:
		break;
		
	}

	/* 设置Vq,Vd */
//	state->Foc.Vq = 0.0f;
//    state->Foc.Vd = 0.0f;

	/* 坐标变换更新 */
	Foc_VoltageUpdate(&state->Foc);
	/* 将计算出的三相CCR比较值赋值给三路CCR比较值寄存器 */
	Motor_SetPWM(state->Foc.Tcmp1,state->Foc.Tcmp2,state->Foc.Tcmp3);
}









