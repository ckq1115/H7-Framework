#include "SBUS.h"
#include "Horizon_MATH.h"

/**
 * 解码SBUS数据函数。
 * 接收包含SBUS原始字节数据的数组，并将其解码填充到指定的SBUS_DATA结构体中。
 *
 * @param raw 指向包含原始SBUS数据的字节数组的指针。
 * @param data 指向用于存储解码后数据的SBUS_DATA结构体的指针。
 * @param len 原始数据数组的长度。
 * @return SBUS_ERROR_CODE_TypeDef 错误代码。0表示成功，其他值代表不同错误：
 *         1：无效参数，
 *         2：数据长度不足，
 *         3：起始字节不正确，
 *         4：结束字节不正确。
 */
SBUS_ERROR_CODE_TypeDef SBUS_decode(uint8_t* raw, SBUS_DATA_typedef* data, uint8_t len)
{
  if (!raw || !data) {
    return 1; // 返回-1为无效参数错误码
  }

  // SBUS数据包所需的最小字节数
  const uint8_t MIN_DATA_LENGTH = 25;
  if (len < MIN_DATA_LENGTH) {
    // 返回错误码表示数据长度不足
    return 2;  // -2为数据长度不足错误码
  }

  data->startbyte = raw[0];   // 读取并存储起始字节

  // 检查起始字节是否正确
  if (data->startbyte != 0x0F)
  {
    return 3; // 起始字节不正确，表示数据不完整
  }

  // 下面的代码用于解码SBUS数据中的各个通道值
  // 通过位操作将通道数据从原始字节序列中提取出来，并转换为11位的有符号整数
  data->CH[0] = (raw[1] | (int16_t)raw[2] << 8) & 0x7FF;
  data->CH[1] = (raw[2] >> 3 | (int16_t)raw[3] << 5) & 0x7FF;
  data->CH[2] = (raw[3] >> 6 | (int16_t)raw[4] << 2 | (int16_t)raw[5] << 10) & 0x7FF;
  data->CH[3] = (raw[5] >> 1 | (int16_t)raw[6] << 7) & 0x7FF;
  data->CH[4] = (raw[6] >> 4 | (int16_t)raw[7] << 4) & 0x7FF;
  data->CH[5] = (raw[7] >> 7 | (int16_t)raw[8] << 1 | (int16_t)raw[9] << 9) & 0x7FF;
  data->CH[6] = (raw[9] >> 2 | (int16_t)raw[10] << 6) & 0x7FF;
  data->CH[7] = (raw[10] >> 5 | (int16_t)raw[11] << 3) & 0x7FF;

  data->CH[8] = (raw[12] | (int16_t)raw[13] << 8) & 0x7FF;
  data->CH[9] = (raw[13] >> 3 | (int16_t)raw[14] << 5) & 0x7FF;
  data->CH[10] = (raw[14] >> 6 | (int16_t)raw[15] << 2 | (int16_t)raw[16] << 10) & 0x7FF;
  data->CH[11] = (raw[16] >> 1 | (int16_t)raw[17] << 7) & 0x7FF;
  data->CH[12] = (raw[17] >> 4 | (int16_t)raw[18] << 4) & 0x7FF;
  data->CH[13] = (raw[18] >> 7 | (int16_t)raw[19] << 1 | (int16_t)raw[20] << 9) & 0x7FF;
  data->CH[14] = (raw[20] >> 2 | (int16_t)raw[21] << 6) & 0x7FF;
  data->CH[15] = (raw[21] >> 5 | (int16_t)raw[22] << 3) & 0x7FF;

  for (uint8_t i = 0; i < 16; i++)
  {
    data->CH[i] -= 1024;
  }

  for (uint8_t i = 0; i < 16; i++)
  {
    if (data->CH[i]<=10 && data->CH[i]>=-10)
    {
      data->CH[i]=0;
    }
  }


  //data->CH[2]=averageFilter(data->CH[2]);原硬件问题添加滤波

  // 解码并存储标志位
  data->flags = raw[23];
  data->endbyte = raw[24];  // 存储结束字节

  if((data->flags)!=0){
    return 5;//设备丢失
  }

  // 检查结束字节是否正确
  if (data->endbyte != 0x00)
  {
    return 4;  // 结束字节不正确，表示数据不完整
  }

  return 0; // 默认返回成功
}

int16_t SBUS_GetChannelValue(SBUS_DATA_typedef* data, SBUS_Channel_t Channel)
{
  // 检查输入参数的有效性
  if (data == NULL || Channel > 16 || Channel < 0)
  {
    return 0;
  }
  return data->CH[Channel];
}

/**
 * 获取SBUS通道的状态
 *
 * @param data 指向SBUS_DATA结构体的指针，包含SBUS的通道数据
 * @param Channel 要查询的通道编号，从1到16
 * @return 返回通道的状态，包括错误状态、开关向上状态、开关向下状态或中心状态
 */
SBUS_ChannelState SBUS_GetChannelState(SBUS_DATA_typedef* data, SBUS_Channel_t Channel)
{
  // 检查输入参数的有效性
  if (data == NULL || Channel > 16)
  {
    return SBUS_SW_Error;
  }

  // 判断通道值是否大于阈值（向上状态）
  if (data->CH[Channel] > 500)
  {
    return SBUS_SW_Down;
  }
  // 判断通道值是否小于阈值（向下状态）
  else if (data->CH[Channel] < -500)
  {
    return SBUS_SW_UP;
  }
  // 通道值在阈值范围内（中心状态）
  else
  {
    return SBUS_SW_Cen;
  }

}