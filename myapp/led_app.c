#include "led_app.h"

void led_proc()
{
	HAL_GPIO_TogglePin(R_EN_GPIO_Port,R_EN_Pin);
}