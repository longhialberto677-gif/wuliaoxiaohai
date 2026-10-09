#ifndef __LCD_APP_H__
#define __LCD_APP_H__

#include "bsp_system.h"

/* 画标题和各行标签，上电调用一次 */
void LCD_App_InitStatic(void);

/* 周期刷新电机状态，挂到调度器（建议 200ms） */
void lcd_proc(void);

#endif