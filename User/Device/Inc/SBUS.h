#ifndef _SBUS_H_
#define _SBUS_H_

#include "main.h"

/**
 * SBUS_ERROR_CODE_TypeDef ö�ٶ�����SBUSͨ�Ź����п��������Ĵ�����롣
 * ��Щ����������ڱ�ʶͨ�Ź����������ľ������⣬������д������͵��ԡ�
 */
typedef enum
{
  SBUS_SUCCESS = 0,  // �����ɹ�
  SBUS_INVALID_PARAM = 1,  // �����������紫��Ĳ���ֵ������Ҫ��
  SBUS_DATA_LENGTH_ERROR = 2,  // ���ݳ��Ȳ��㣬��������ɲ���
  SBUS_STARTBYTE_ERROR = 3,  // ��ʼ�ֽڲ���ȷ����������Ϊ����֡����ʼ��ʶ������
  SBUS_ENDBYTE_ERROR = 4  // �����ֽڲ���ȷ����������Ϊ����֡�Ľ�����ʶ������
}SBUS_ERROR_CODE_TypeDef;

/**
 * @struct SBUS_DATA
 * ������SBUS���ݵĽṹ�壬���ڴ洢ͨ��SBUSЭ����յ�ң�����ݡ�
 */
typedef struct
{
  int8_t ONLINE_JUDGE_TIME;
  uint8_t startbyte; /**< ��ʼ�ֽڣ����ڱ�ʶSBUS���ݵĿ�ʼ�� */
  int16_t CH[16]; /**< ͨ���������飬�洢����ң������16��ͨ����ֵ�� */
  uint8_t flags; /**< ��־λ���ṩ���ڽ������ݵ�״̬��Ϣ���緭ת�ȡ� */
  uint8_t endbyte; /**< �����ֽڣ����ڱ�ʶSBUS���ݵĽ����� */
}SBUS_DATA_typedef;


typedef enum
{
  SBUS_SW_UP=1,
  SBUS_SW_Down,
  SBUS_SW_Cen,
  SBUS_SW_Error,
}SBUS_ChannelState;

typedef enum
{
  SBUS_Channel_1=0,
  SBUS_Channel_2,
  SBUS_Channel_3,
  SBUS_Channel_4,
  SBUS_Channel_5,
  SBUS_Channel_6,
  SBUS_Channel_7,
  SBUS_Channel_8,
  SBUS_Channel_9,
  SBUS_Channel_10,
  SBUS_Channel_11,
  SBUS_Channel_12,
  SBUS_Channel_13,
  SBUS_Channel_14,
  SBUS_Channel_15,
  SBUS_Channel_16,
}SBUS_Channel_t;

SBUS_ERROR_CODE_TypeDef SBUS_decode(uint8_t *raw,  SBUS_DATA_typedef* data,uint8_t len);
int16_t SBUS_GetChannelValue( SBUS_DATA_typedef* data, SBUS_Channel_t Channel);
SBUS_ChannelState SBUS_GetChannelState( SBUS_DATA_typedef* data, SBUS_Channel_t Channel);

#endif