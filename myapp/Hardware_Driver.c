#include "Hardware_Driver.h"



void Motor_EnablePWM()
{
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);
  HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_1);
  HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_2);
  HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_3);
}

void Motor_DisablePWM()
{
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_3);
	HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_3);
}

void Motor_Hardware_Init()
{
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4);
	/* 使能定时器一通道四捕获中断 */
	__HAL_TIM_ENABLE_IT(&htim1,TIM_IT_CC4);
	Motor_EnablePWM();
}

void Motor_SetPWM(uint16_t Tcmp1,uint16_t Tcmp2,uint16_t Tcmp3)
{
	__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_1,Tcmp1);
	__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_2,Tcmp2);
	__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_3,Tcmp3);
}

/**
 * @brief 读取MT6701编码器数据
 * @param 无
 * @return 编码器原始值
 */
uint16_t MT6701_ReadData(void)
{
	uint8_t rx[3] = {0};
	uint8_t tx[3] = {0};
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1,tx,rx,3,0xffffff);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	return ((rx[0]<<6)|(rx[1]>>2))&0x3FFF;
}







