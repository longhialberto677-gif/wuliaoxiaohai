#include "lcd_app.h"

/* ============================================================
 * lcd_app.c —— 把电机状态显示到 LCD
 * ------------------------------------------------------------
 * 上电调用一次 LCD_App_InitStatic()，
 * 把 lcd_proc 挂到调度器里周期调用（建议 200ms）。
 *
 * 每行只刷"数值那一小块"，不刷全屏，对主循环影响很小。
 * 浮点用 LCD_ShowDecimal 显示，不依赖 sprintf。
 * ============================================================ */

/* ---------- 布局参数 ---------- */
#define ROW_Y0      4       /* 第一行的 y 坐标 */
#define ROW_H       19      /* 行间距（8 行 × 19 = 152，屏高 160） */
#define COL_LABEL   0       /* 标签 x */
#define COL_VALUE   30      /* 数值 x */
#define VAL_W       50      /* 数值区宽度（先擦这么宽，旧值才擦得干净） */
#define ROWS        8       /* 行数 */

/* ---------- 行表：标签 / 小数位数 / 颜色 ---------- */
static const char *const row_label[ROWS] =
{
    "enc :",    /* 编码器原始值 */
    "angE:",    /* 电角度 rad */
    "angM:",    /* 机械角 rad */
    "theta",    /* Foc.Theta rad */
    "Vd  :",    /* d 轴电压 V */
    "Vq  :",    /* q 轴电压 V */
    "vfs :",    /* VF 速度 */
    "mode:",    /* 角度模式 */
};

static const uint8_t row_dp[ROWS] =
{
    0,          /* enc   整数 */
    3,          /* angE  3 位小数 */
    3,          /* angM  3 位小数 */
    3,          /* theta 3 位小数 */
    2,          /* Vd    2 位小数 */
    2,          /* Vq    2 位小数 */
    1,          /* vfs   1 位小数 */
    0,          /* mode  整数 */
};

static const uint16_t row_color[ROWS] =
{
    LCD_GREEN, LCD_GREEN, LCD_GREEN, LCD_CYAN,
    LCD_RED,   LCD_RED,   LCD_MAGENTA, LCD_YELLOW,
};

/**
 * @brief  按行号取当前值（浮点）
 * @param  n : 行号 0~ROWS-1
 * @return 该行要显示的数值
 * @note   返回 float，整数行在显示时 dp=0 会自动取整

 */
static float row_value(uint8_t n)
{
    switch (n)
    {
        case 0:  return (float)MotorSystem.Encoder.Pulse_Data;
        case 1:  return MotorSystem.Encoder.Theta_e;
        case 2:  return MotorSystem.Encoder.Theta;
        case 3:  return MotorSystem.Foc.Theta;
        case 4:  return MotorSystem.Foc.Vd;
        case 5:  return MotorSystem.Foc.Vq;
        case 6:  return MotorSystem.Vf.Speed;
        case 7:  return (float)MotorSystem.Theta_Mode;
        default: return 0.0f;
    }
}

/**
 * @brief  画静态内容：各行标签。上电调用一次即可
 */
void LCD_App_InitStatic(void)
{
    uint8_t n;

    LCD_Clear(LCD_BLACK);

    for (n = 0; n < ROWS; n++)
    {
        LCD_ShowString(COL_LABEL, ROW_Y0 + n * ROW_H,
                       row_label[n], LCD_WHITE, LCD_BLACK);
    }
}

/**
 * @brief  周期刷新：只重画每行的数值区
 * @note   挂到调度器，建议 200ms 一次
 */
void lcd_proc(void)
{
    uint8_t n;
    uint16_t y;

    for (n = 0; n < ROWS; n++)
    {
        y = ROW_Y0 + n * ROW_H;

        /* 先擦固定宽度的数值区（保证旧的长数字也被擦掉） */
        LCD_FillArea(COL_VALUE, y, VAL_W, 8, LCD_BLACK);

        /* 再写新值：大于 0 位小数的用浮点显示，否则用整数显示 */
        if (row_dp[n] > 0)
            LCD_ShowDecimal(COL_VALUE, y, row_value(n), row_dp[n],
                            row_color[n], LCD_BLACK);
        else
            LCD_ShowNum(COL_VALUE, y, (int32_t)row_value(n),
                        row_color[n], LCD_BLACK);
    }
}