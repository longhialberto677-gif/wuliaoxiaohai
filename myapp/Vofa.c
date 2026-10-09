#include "Vofa.h"

/* Vofa发送结构体实例 */
VofaSend_TypeDef VofaSend;

/**
 * @brief Vofa+发送函数（非阻塞：只填数据 + 启动DMA发送，不等待发送完成）
 * @param 无
 * @return 无
 */
void Vofa_Send_Task(void)
{
    /* 填入需要发送的数据 */
//    VofaSend.Send_Data_Array[0] = MotorSystem.Foc.Theta;
//    VofaSend.Send_Data_Array[1] = MotorSystem.Foc.Valpha;
//    VofaSend.Send_Data_Array[2] = MotorSystem.Foc.Vbeta;
//    VofaSend.Send_Data_Array[3] = MotorSystem.Foc.Vu;
	VofaSend.Send_Data_Array[0] = MotorSystem.Foc.Theta;
    VofaSend.Send_Data_Array[1] = MotorSystem.Vf.Target_Speed;
    VofaSend.Send_Data_Array[2] = MotorSystem.Vf.Speed;
    VofaSend.Send_Data_Array[3] = MotorSystem.Vf.Theta;
	
    VofaSend.Send_Data_Array[4] = MotorSystem.Foc.Vv;
	VofaSend.Send_Data_Array[5] = MotorSystem.Foc.Vw;
	VofaSend.Send_Data_Array[6] = MotorSystem.Encoder.Pulse_Data;
    VofaSend.Send_Data_Array[7] = MotorSystem.Encoder.Theta;
	VofaSend.Send_Data_Array[8] = MotorSystem.Encoder.Theta_e;

    Vofa_Data_Process();
}

/**
 * @brief Vofa+数据处理函数（组装一帧并启动DMA发送，立即返回）
 * @param 无
 * @return 无
 */
void Vofa_Data_Process(void)
{

    /* 通过拷贝将浮点数数据转换成单字节数据（小端 IEEE-754 原样搬运） */
    memcpy(VofaSend.Byte_Data_Array, VofaSend.Send_Data_Array, data_num * 4);

    /* 添加通信协议的帧尾：0x7F800000，即 +INF，小端字节序为 00 00 80 7F */
    VofaSend.Byte_Data_Array[data_len - 4] = 0x00;
    VofaSend.Byte_Data_Array[data_len - 3] = 0x00;
    VofaSend.Byte_Data_Array[data_len - 2] = 0x80;
    VofaSend.Byte_Data_Array[data_len - 1] = 0x7f;

	CDC_Transmit_FS((uint8_t*)VofaSend.Byte_Data_Array,data_len);
}