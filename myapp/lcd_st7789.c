#include "lcd_st7789.h"

/* ============================================================================
 * lcd_st7789.c —— 0.96 寸 IPS 液晶屏驱动（ST7735S）
 * ----------------------------------------------------------------------------
 * 硬件连接（对应 CubeMX 标签）：
 *   LCD_RES  -> PC14      LCD_DC   -> PC15
 *   LCD_CS   -> PB6       LCD_BLK  -> PB7   （低电平点亮）
 *   SCL/SCK  -> PC10      SDA/MOSI -> PC12  （SPI3，Mode 0，10.6MHz）
 *
 * 屏幕参数（已实测确认）：
 *   型号    ：0.96" IPS (BOE) + ST7735S 控制器
 *   分辨率  ：80 x 160（竖屏）
 *   MADCTL  ：0x08
 *   显存偏移：列 +24，行 +0
 *   像素格式：0x3A = 0x05（16bit RGB565）
 *   背光    ：PB7 低电平点亮（active-low）
 *
 * 典型用法：
 *   LCD_Init();                                   // 初始化 + 开背光
 *   LCD_Clear(LCD_BLACK);                         // 清屏
 *   LCD_ShowStringScale(4, 10, "RPM", LCD_WHITE, LCD_BLACK, 2);  // 写大字
 *   LCD_ShowNum(4, 30, speed, LCD_GREEN, LCD_BLACK);             // 写数字
 *
 * 注意：本驱动为逐字节阻塞发送，整屏刷新约需数百毫秒。
 *       在电机控制等实时场合，请只刷新变化的小区域。
 * ============================================================================ */

#define LCD_CS_L()      HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_RESET)
#define LCD_CS_H()      HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_SET)
#define LCD_DC_CMD()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_RESET)
#define LCD_DC_DATA()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_SET)
#define LCD_RES_L()     HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_RESET)
#define LCD_RES_H()     HAL_GPIO_WritePin(LCD_RES_GPIO_Port, LCD_RES_Pin, GPIO_PIN_SET)
/* 背光：低电平点亮 */
#define LCD_BLK_ON()    HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_RESET)
#define LCD_BLK_OFF()   HAL_GPIO_WritePin(LCD_BLK_GPIO_Port, LCD_BLK_Pin, GPIO_PIN_SET)

#define LCD_SPI_TIMEOUT 100

/* 6x8 ASCII 字库（0x20 ~ 0x7E），每字符 6 列，bit0 在上 */
static const uint8_t LCD_Font6x8[][6] =
{
    {0x00,0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00,0x00}, {0x14,0x7F,0x14,0x7F,0x14,0x00},
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, {0x23,0x13,0x08,0x64,0x62,0x00},
    {0x36,0x49,0x55,0x22,0x50,0x00}, {0x00,0x05,0x03,0x00,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00,0x00}, {0x00,0x41,0x22,0x1C,0x00,0x00},
    {0x14,0x08,0x3E,0x08,0x14,0x00}, {0x08,0x08,0x3E,0x08,0x08,0x00},
    {0x00,0x50,0x30,0x00,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08,0x00},
    {0x00,0x60,0x60,0x00,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02,0x00},
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, {0x00,0x42,0x7F,0x40,0x00,0x00},
    {0x42,0x61,0x51,0x49,0x46,0x00}, {0x21,0x41,0x45,0x4B,0x31,0x00},
    {0x18,0x14,0x12,0x7F,0x10,0x00}, {0x27,0x45,0x45,0x45,0x39,0x00},
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, {0x01,0x71,0x09,0x05,0x03,0x00},
    {0x36,0x49,0x49,0x49,0x36,0x00}, {0x06,0x49,0x49,0x29,0x1E,0x00},
    {0x00,0x36,0x36,0x00,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00,0x00}, {0x14,0x14,0x14,0x14,0x14,0x00},
    {0x00,0x41,0x22,0x14,0x08,0x00}, {0x02,0x01,0x51,0x09,0x06,0x00},
    {0x32,0x49,0x79,0x41,0x3E,0x00}, {0x7E,0x11,0x11,0x11,0x7E,0x00},
    {0x7F,0x49,0x49,0x49,0x36,0x00}, {0x3E,0x41,0x41,0x41,0x22,0x00},
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, {0x7F,0x49,0x49,0x49,0x41,0x00},
    {0x7F,0x09,0x09,0x09,0x01,0x00}, {0x3E,0x41,0x49,0x49,0x7A,0x00},
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, {0x00,0x41,0x7F,0x41,0x00,0x00},
    {0x20,0x40,0x41,0x3F,0x01,0x00}, {0x7F,0x08,0x14,0x22,0x41,0x00},
    {0x7F,0x40,0x40,0x40,0x40,0x00}, {0x7F,0x02,0x04,0x02,0x7F,0x00},
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, {0x3E,0x41,0x41,0x41,0x3E,0x00},
    {0x7F,0x09,0x09,0x09,0x06,0x00}, {0x3E,0x41,0x51,0x21,0x5E,0x00},
    {0x7F,0x09,0x19,0x29,0x46,0x00}, {0x46,0x49,0x49,0x49,0x31,0x00},
    {0x01,0x01,0x7F,0x01,0x01,0x00}, {0x3F,0x40,0x40,0x40,0x3F,0x00},
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, {0x3F,0x40,0x38,0x40,0x3F,0x00},
    {0x63,0x14,0x08,0x14,0x63,0x00}, {0x07,0x08,0x70,0x08,0x07,0x00},
    {0x61,0x51,0x49,0x45,0x43,0x00}, {0x00,0x7F,0x41,0x41,0x00,0x00},
    {0x02,0x04,0x08,0x10,0x20,0x00}, {0x00,0x41,0x41,0x7F,0x00,0x00},
    {0x04,0x02,0x01,0x02,0x04,0x00}, {0x40,0x40,0x40,0x40,0x40,0x00},
    {0x00,0x01,0x02,0x04,0x00,0x00}, {0x20,0x54,0x54,0x54,0x78,0x00},
    {0x7F,0x48,0x44,0x44,0x38,0x00}, {0x38,0x44,0x44,0x44,0x20,0x00},
    {0x38,0x44,0x44,0x48,0x7F,0x00}, {0x38,0x54,0x54,0x54,0x18,0x00},
    {0x08,0x7E,0x09,0x01,0x02,0x00}, {0x08,0x14,0x54,0x54,0x3C,0x00},
    {0x7F,0x08,0x04,0x04,0x78,0x00}, {0x00,0x44,0x7D,0x40,0x00,0x00},
    {0x20,0x40,0x44,0x3D,0x00,0x00}, {0x00,0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00,0x00}, {0x7C,0x04,0x18,0x04,0x78,0x00},
    {0x7C,0x08,0x04,0x04,0x78,0x00}, {0x38,0x44,0x44,0x44,0x38,0x00},
    {0x7C,0x14,0x14,0x14,0x08,0x00}, {0x08,0x14,0x14,0x18,0x7C,0x00},
    {0x7C,0x08,0x04,0x04,0x08,0x00}, {0x48,0x54,0x54,0x54,0x20,0x00},
    {0x04,0x3F,0x44,0x40,0x20,0x00}, {0x3C,0x40,0x40,0x20,0x7C,0x00},
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, {0x3C,0x40,0x30,0x40,0x3C,0x00},
    {0x44,0x28,0x10,0x28,0x44,0x00}, {0x0C,0x50,0x50,0x50,0x3C,0x00},
    {0x44,0x64,0x54,0x4C,0x44,0x00}, {0x00,0x08,0x36,0x41,0x00,0x00},
    {0x00,0x00,0x7F,0x00,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00,0x00},
    {0x08,0x08,0x2A,0x1C,0x08,0x00},
};

/* ============================================================
 * 底层收发
 * ============================================================ */
/**
 * @brief  向屏写一条命令（DC = 低）
 * @param  cmd : 命令字节
 */
static void LCD_WriteCmd(uint8_t cmd)
{
    LCD_DC_CMD();
    LCD_CS_L();
    HAL_SPI_Transmit(LCD_SPI, &cmd, 1, LCD_SPI_TIMEOUT);
    LCD_CS_H();
}

/**
 * @brief  向屏写一个数据字节（DC = 高）
 * @param  data : 数据字节
 */
static void LCD_WriteData8(uint8_t data)
{
    LCD_DC_DATA();
    LCD_CS_L();
    HAL_SPI_Transmit(LCD_SPI, &data, 1, LCD_SPI_TIMEOUT);
    LCD_CS_H();
}

/**
 * @brief  连续写入一段数据（DC = 高），比逐字节调用快
 * @param  buf : 数据缓冲区
 * @param  len : 字节数
 */
static void LCD_WriteDataBuf(const uint8_t *buf, uint16_t len)
{
    LCD_DC_DATA();
    LCD_CS_L();
    HAL_SPI_Transmit(LCD_SPI, (uint8_t *)buf, len, LCD_SPI_TIMEOUT);
    LCD_CS_H();
}

/* ============================================================
 * 设置绘图窗口
 * ============================================================ */
/**
 * @brief  设置绘图窗口（矩形区域），并进入写显存模式
 * @param  x0,y0 : 左上角坐标（屏坐标，0 起）
 * @param  x1,y1 : 右下角坐标（含）
 * @note   内部会自动加显存偏移（列 +24，行 +0）
 */
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 += LCD_X_OFFSET;  x1 += LCD_X_OFFSET;
    y0 += LCD_Y_OFFSET;  y1 += LCD_Y_OFFSET;

    LCD_WriteCmd(0x2A);
    LCD_WriteData8(x0 >> 8); LCD_WriteData8(x0 & 0xFF);
    LCD_WriteData8(x1 >> 8); LCD_WriteData8(x1 & 0xFF);

    LCD_WriteCmd(0x2B);
    LCD_WriteData8(y0 >> 8); LCD_WriteData8(y0 & 0xFF);
    LCD_WriteData8(y1 >> 8); LCD_WriteData8(y1 & 0xFF);

    LCD_WriteCmd(0x2C);
}

/* ============================================================
 * 区域填充
 * ============================================================ */
/**
 * @brief  用单色填充矩形区域
 * @param  x,y   : 左上角坐标
 * @param  w,h   : 宽、高（像素）
 * @param  color : RGB565 颜色（如 LCD_RED）
 * @note   超出屏幕会自动裁剪
 */
void LCD_FillArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t i, total;
    uint8_t  hi = color >> 8;
    uint8_t  lo = color & 0xFF;

    if (x >= LCD_W || y >= LCD_H) return;
    if (x + w > LCD_W) w = LCD_W - x;
    if (y + h > LCD_H) h = LCD_H - y;

    total = (uint32_t)w * h;

    LCD_SetWindow(x, y, x + w - 1, y + h - 1);

    LCD_CS_L();
    LCD_DC_DATA();
    for (i = 0; i < total; i++)
    {
        HAL_SPI_Transmit(LCD_SPI, &hi, 1, LCD_SPI_TIMEOUT);
        HAL_SPI_Transmit(LCD_SPI, &lo, 1, LCD_SPI_TIMEOUT);
    }
    LCD_CS_H();
}

/**
 * @brief  整屏填充单色
 * @param  color : RGB565 颜色
 */
void LCD_Clear(uint16_t color)
{
    LCD_FillArea(0, 0, LCD_W, LCD_H, color);
}

/* ============================================================
 * 画点
 * ============================================================ */
/**
 * @brief  画一个像素点
 * @param  x,y   : 坐标
 * @param  color : RGB565 颜色
 */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_W || y >= LCD_H) return;

    LCD_SetWindow(x, y, x, y);
    LCD_CS_L(); LCD_DC_DATA();
    LCD_WriteData8(color >> 8);
    LCD_WriteData8(color & 0xFF);
    LCD_CS_H();
}

/* ============================================================
 * 字符 / 字符串 / 数字
 * ============================================================ */
/**
 * @brief  显示一个 ASCII 字符（6x8 点阵）
 * @param  x,y   : 字符左上角坐标
 * @param  ch    : 字符（可打印 ASCII 32~126）
 * @param  fc,bc : 前景色（字色）、背景色
 */
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc)
{
    uint8_t  i, j, idx;
    uint16_t c;

    if (ch < ' ' || ch > '~') ch = '?';
    idx = (uint8_t)(ch - ' ');

    LCD_SetWindow(x, y, x + 5, y + 7);      /* 6 列 x 8 行 */

    LCD_CS_L(); LCD_DC_DATA();
    /* 行优先写入：与显存窗口的自然递增顺序一致，不会错位 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 6; i++)
        {
            c = (LCD_Font6x8[idx][i] & (1 << j)) ? fc : bc;
            LCD_WriteData8(c >> 8);
            LCD_WriteData8(c & 0xFF);
        }
    }
    LCD_CS_H();
}

/* ============================================================
 * 放大字符：把 6x8 字库按 scale 倍放大（scale=1 就是原尺寸）
 * 这样在大屏上也能看清
 * ============================================================ */
/**
 * @brief  显示一个放大后的 ASCII 字符
 * @param  x,y   : 字符左上角坐标
 * @param  ch    : 字符
 * @param  fc,bc : 前景色、背景色
 * @param  scale : 放大倍数（1=6x8；2=12x16；3=18x24）
 */
void LCD_ShowCharScale(uint16_t x, uint16_t y, char ch,
                       uint16_t fc, uint16_t bc, uint8_t scale)
{
    uint8_t  i, j, sx, sy, idx;
    uint16_t c;

    if (scale == 0) scale = 1;
    if (ch < ' ' || ch > '~') ch = '?';
    idx = (uint8_t)(ch - ' ');

    LCD_SetWindow(x, y,
                  x + 6 * scale - 1,
                  y + 8 * scale - 1);

    LCD_CS_L(); LCD_DC_DATA();
    for (j = 0; j < 8; j++)                 /* 字模的 8 行 */
    {
        for (sy = 0; sy < scale; sy++)      /* 每行重复 scale 次 */
        {
            for (i = 0; i < 6; i++)         /* 每行 6 个像素 */
            {
                c = (LCD_Font6x8[idx][i] & (1 << j)) ? fc : bc;
                for (sx = 0; sx < scale; sx++)   /* 每像素横向重复 scale 次 */
                {
                    LCD_WriteData8(c >> 8);
                    LCD_WriteData8(c & 0xFF);
                }
            }
        }
    }
    LCD_CS_H();
}

/* 放大字符串（行末自动换行，x 回到起点） */
/**
 * @brief  显示一个放大后的 ASCII 字符串
 * @param  x,y   : 起始坐标
 * @param  str   : 字符串（'\\n' 强制换行，超宽自动换行）
 * @param  fc,bc : 前景色、背景色
 * @param  scale : 放大倍数
 */
void LCD_ShowStringScale(uint16_t x, uint16_t y, const char *str,
                         uint16_t fc, uint16_t bc, uint8_t scale)
{
    uint16_t cx = x, cy = y;
    uint16_t cw = 6 * scale;
    uint16_t chh = 8 * scale;

    while (*str)
    {
        if (*str == '\n')
        {
            cx = x;
            cy += chh;
        }
        else
        {
            if (cx + cw > LCD_W) { cx = x; cy += chh; }
            if (cy + chh > LCD_H) break;
            LCD_ShowCharScale(cx, cy, *str, fc, bc, scale);
            cx += cw;
        }
        str++;
    }
}

/**
 * @brief  显示一个 ASCII 字符串（6x8 点阵）
 * @param  x,y   : 起始坐标
 * @param  str   : 字符串（'\\n' 强制换行，超宽自动换行）
 * @param  fc,bc : 前景色、背景色
 */
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc)
{
    uint16_t cx = x, cy = y;

    while (*str)
    {
        if (*str == '\n')
        {
            cx = x;
            cy += 8;
        }
        else
        {
            if (cx + 6 > LCD_W) { cx = x; cy += 8; }
            if (cy + 8 > LCD_H) break;
            LCD_ShowChar(cx, cy, *str, fc, bc);
            cx += 6;
        }
        str++;
    }
}

/**
 * @brief  显示一个有符号整数（6x8 点阵）
 * @param  x,y   : 起始坐标
 * @param  num   : 要显示的数（支持负数）
 * @param  fc,bc : 前景色、背景色
 */
void LCD_ShowNum(uint16_t x, uint16_t y, int32_t num, uint16_t fc, uint16_t bc)
{
    char buf[12];
    uint8_t i = 0, j;
    uint32_t u;
    uint8_t neg = 0;

    if (num < 0) { neg = 1; u = (uint32_t)(-num); }
    else         { u = (uint32_t)num; }

    if (u == 0) buf[i++] = '0';
    while (u > 0) { buf[i++] = (char)('0' + (u % 10)); u /= 10; }
    if (neg) buf[i++] = '-';

    for (j = 0; j < i / 2; j++)
    {
        char t = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = t;
    }
    buf[i] = '\0';

    LCD_ShowString(x, y, buf, fc, bc);
}

/* ============================================================
 * 背光
 * ============================================================ */
/**
 * @brief  显示定点小数（浮点转整数后手工插小数点）
 * @param  x,y   : 起始坐标
 * @param  val   : 数值
 * @param  dp    : 小数位数（0~3）
 * @param  fc,bc : 前景色、背景色
 * @note   不依赖 sprintf / printf，省好几 KB Flash。例如：
 *             LCD_ShowDecimal(30, 0, 1.5708f, 3, ...) -> "1.570"
 *             LCD_ShowDecimal(30, 20, -12.34f, 2, ...) -> "-12.34"
 * @note   按 dp 固定宽度输出，配合"先擦固定宽度再写"不会留残影
 */
void LCD_ShowDecimal(uint16_t x, uint16_t y, float val,
                     uint8_t dp, uint16_t fc, uint16_t bc)
{
    char     buf[16];
    uint8_t  n = 0;
    int32_t  scaled;
    uint32_t mag;
    int32_t  div;
    uint32_t ip, fp;
    char     tmp[12];
    uint8_t  k = 0;
    uint8_t  i;

    if (dp > 3) dp = 3;

    /* 放大成整数，四舍五入 */
    div = 1;
    for (i = 0; i < dp; i++) div *= 10;
    scaled = (int32_t)(val * (float)div + (val >= 0.0f ? 0.5f : -0.5f));

    /* 符号 */
    if (scaled < 0) { buf[n++] = '-'; mag = (uint32_t)(-scaled); }
    else            { mag = (uint32_t)scaled; }

    /* 拆整数部分和小数部分 */
    ip = mag / (uint32_t)div;
    fp = mag % (uint32_t)div;

    /* 整数部分（低位先出，再反转） */
    if (ip == 0) tmp[k++] = '0';
    while (ip > 0) { tmp[k++] = (char)('0' + (ip % 10)); ip /= 10; }
    while (k > 0) { buf[n++] = tmp[--k]; }

    /* 小数部分 */
    if (dp > 0)
    {
        buf[n++] = '.';
        for (i = 0; i < dp; i++)
        {
            div /= 10;
            buf[n++] = (char)('0' + (fp / (uint32_t)div) % 10);
        }
    }

    buf[n] = 0;

    LCD_ShowString(x, y, buf, fc, bc);
}

/**
 * @brief  背光开关
 * @param  on : 0 = 关，非 0 = 开
 * @note   背光为低电平点亮（PB7 拉低）
 */
void LCD_Backlight(uint8_t on)
{
    if (on) LCD_BLK_ON();
    else    LCD_BLK_OFF();
}

/* ============================================================
 * 初始化
 * ============================================================ */
/**
 * @brief  初始化 LCD：硬件复位 + 寄存器配置 + 开背光 + 清屏为白色
 * @note   上电后调用一次即可
 */
void LCD_Init(void)
{
    /* 硬件复位 */
    LCD_RES_L();
    HAL_Delay(100);
    LCD_RES_H();
    HAL_Delay(100);

    LCD_BLK_ON();
    HAL_Delay(100);

    /* 帧率 */
    LCD_WriteCmd(0xB1);
    LCD_WriteData8(0x05); LCD_WriteData8(0x3C); LCD_WriteData8(0x3C);
    LCD_WriteCmd(0xB2);
    LCD_WriteData8(0x05); LCD_WriteData8(0x3C); LCD_WriteData8(0x3C);
    LCD_WriteCmd(0xB3);
    LCD_WriteData8(0x05); LCD_WriteData8(0x3C); LCD_WriteData8(0x3C);
    LCD_WriteData8(0x05); LCD_WriteData8(0x3C); LCD_WriteData8(0x3C);

    LCD_WriteCmd(0xB4);                             /* 点反相 */
    LCD_WriteData8(0x03);

    /* 电源 */
    LCD_WriteCmd(0xC0);                             /* AVDD / GVDD */
    LCD_WriteData8(0xAB); LCD_WriteData8(0x0B); LCD_WriteData8(0x04);
    LCD_WriteCmd(0xC1);                             /* VGH / VGL */
    LCD_WriteData8(0xC5);
    LCD_WriteCmd(0xC2);
    LCD_WriteData8(0x0D); LCD_WriteData8(0x00);
    LCD_WriteCmd(0xC3);
    LCD_WriteData8(0x8D); LCD_WriteData8(0x6A);
    LCD_WriteCmd(0xC4);
    LCD_WriteData8(0x8D); LCD_WriteData8(0xEE);
    LCD_WriteCmd(0xC5);                             /* VCOM */
    LCD_WriteData8(0x0F);

    /* gamma */
    LCD_WriteCmd(0xE0);
    {
        static const uint8_t g[] = {0x07,0x0E,0x08,0x07,0x10,0x07,0x02,0x07,
                                    0x09,0x0F,0x25,0x36,0x00,0x08,0x04,0x10};
        LCD_WriteDataBuf(g, sizeof(g));
    }
    LCD_WriteCmd(0xE1);
    {
        static const uint8_t g[] = {0x0A,0x0D,0x08,0x07,0x0F,0x07,0x02,0x07,
                                    0x09,0x0F,0x25,0x35,0x00,0x09,0x04,0x10};
        LCD_WriteDataBuf(g, sizeof(g));
    }

    LCD_WriteCmd(0xFC);                             /* ST7735S 专有 */
    LCD_WriteData8(0x80);

    LCD_WriteCmd(0x3A);                             /* 16bit RGB565 */
    LCD_WriteData8(0x05);

    LCD_WriteCmd(0x36);                             /* 竖屏 */
    LCD_WriteData8(0x08);

    LCD_WriteCmd(0x11);                             /* 退出睡眠 */
    HAL_Delay(120);

    LCD_FillArea(0, 0, LCD_W, LCD_H, LCD_WHITE); /* 先清成白色 */

    LCD_WriteCmd(0x29);                             /* 开显示 */
}

/* ============================================================
 * 自检画面
 * ============================================================ */
/**
 * @brief  自检画面：三色轮播 -> 白底 + 四角红块 + 文字
 * @note   用于确认屏是否正常，正式使用时可删掉调用
 */
void LCD_Test(void)
{
    LCD_FillArea(0, 0, LCD_W, LCD_H, LCD_RED);   HAL_Delay(800);
    LCD_FillArea(0, 0, LCD_W, LCD_H, LCD_GREEN); HAL_Delay(800);
    LCD_FillArea(0, 0, LCD_W, LCD_H, LCD_BLUE);  HAL_Delay(800);

    LCD_FillArea(0, 0, LCD_W, LCD_H, LCD_WHITE);

    /* 四角红块（验证偏移是否正确） */
    LCD_FillArea(0, 0, 8, 8, LCD_RED);
    LCD_FillArea(LCD_W - 8, 0, 8, 8, LCD_RED);
    LCD_FillArea(0, LCD_H - 8, 8, 8, LCD_RED);
    LCD_FillArea(LCD_W - 8, LCD_H - 8, 8, 8, LCD_RED);

    /* 中间：2 倍大字，黄底黑字，分两行 */
    LCD_FillArea(0, 64, LCD_W, 36, LCD_YELLOW);

    LCD_ShowStringScale(4, 66, "LCD", LCD_BLACK, LCD_YELLOW, 2);   /* 12x16 字 */
    LCD_ShowStringScale(4, 84, "OK",  LCD_BLACK, LCD_YELLOW, 2);

    /* 底部：一行小字（原尺寸 6x8），分开写避免挤在一起 */
    LCD_ShowString(2, 150, "80x160", LCD_BLACK, LCD_WHITE);
}