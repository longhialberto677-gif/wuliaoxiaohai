#ifndef __LCD_ST7789_H__
#define __LCD_ST7789_H__

/* ============================================================================
 * lcd_st7789.h —— 0.96 寸 IPS 液晶屏驱动（ST7735S，80x160 竖屏）
 * ----------------------------------------------------------------------------
 * 硬件连接：
 *   LCD_RES  -> PC14      LCD_DC   -> PC15
 *   LCD_CS   -> PB6       LCD_BLK  -> PB7   （低电平点亮）
 *   SCL/SCK  -> PC10      SDA/MOSI -> PC12  （SPI3）
 *
 * 使用步骤：
 *   1) main() 里 MX_SPI3_Init() 之后调用  LCD_Init();
 *   2) 用 LCD_Clear / LCD_FillArea / LCD_Show* 绘图
 *   3) 需要刷屏时只刷新变化的小区域，避免长时间阻塞主循环
 * ============================================================================ */

#include "bsp_system.h"

/* ---------- 引脚与 SPI ---------- */
#define LCD_SPI         (&hspi3)

/* ---------- 屏幕分辨率（竖屏 80x160） ---------- */
#define LCD_W           80
#define LCD_H           160

/* ---------- 显存偏移（列 +24，行 +0，已实测确认） ---------- */
#define LCD_X_OFFSET    24
#define LCD_Y_OFFSET    0

/* ---------- 常用颜色（RGB565） ---------- */
#define LCD_BLACK       0x0000
#define LCD_WHITE       0xFFFF
#define LCD_RED         0xF800
#define LCD_GREEN       0x07E0
#define LCD_BLUE        0x001F
#define LCD_YELLOW      0xFFE0
#define LCD_CYAN        0x07FF
#define LCD_MAGENTA     0xF81F
#define LCD_GRAY        0x8410

/* ============================================================================
 * API
 * ============================================================================ */

/* ---- 初始化 / 背光 ---- */
void LCD_Init(void);                                  /* 初始化（含开背光、清屏白） */
void LCD_Backlight(uint8_t on);                       /* 背光开关：0=关，非0=开 */

/* ---- 绘图 ---- */
void LCD_SetWindow(uint16_t x0, uint16_t y0,
                   uint16_t x1, uint16_t y1);         /* 设置绘图窗口 */
void LCD_Clear(uint16_t color);                       /* 整屏填单色 */
void LCD_FillArea(uint16_t x, uint16_t y,
                  uint16_t w, uint16_t h, uint16_t color);  /* 填矩形区域 */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color); /* 画点 */

/* ---- 文字（6x8 点阵） ---- */
void LCD_ShowChar(uint16_t x, uint16_t y,
                  char ch, uint16_t fc, uint16_t bc);
void LCD_ShowString(uint16_t x, uint16_t y,
                    const char *str, uint16_t fc, uint16_t bc);
void LCD_ShowNum(uint16_t x, uint16_t y,
                 int32_t num, uint16_t fc, uint16_t bc);
void LCD_ShowDecimal(uint16_t x, uint16_t y, float val,
                     uint8_t dp, uint16_t fc, uint16_t bc);

/* ---- 放大文字（scale: 1=6x8, 2=12x16, 3=18x24 ...） ---- */
void LCD_ShowCharScale(uint16_t x, uint16_t y, char ch,
                       uint16_t fc, uint16_t bc, uint8_t scale);
void LCD_ShowStringScale(uint16_t x, uint16_t y, const char *str,
                         uint16_t fc, uint16_t bc, uint8_t scale);

/* ---- 自检 ---- */
void LCD_Test(void);                                  /* 三色轮播 + 四角块 + 文字 */

#endif