#include "crc16.h"

const uint16_t CRC16::CRC16_POLYNOMIAL = 0x1021;
const uint16_t CRC16::CRC16_INITIAL = 0xFFFF;

uint16_t CRC16::calculate(const uint8_t* data, uint16_t length)
{
	uint16_t crc = CRC16_INITIAL;

	for (uint16_t i = 0; i < length; i++)
	{
		crc ^= (uint16_t)data[i] << 8;

		for (uint8_t j = 0; j < 8; j++)
		{
			if (crc & 0x8000)
			{
				crc = (crc << 1) ^ CRC16_POLYNOMIAL;
			}
			else
			{
				crc <<= 1;
			}
		}
	}

	return crc;
}

bool CRC16::verify(const uint8_t* data, uint16_t length, uint16_t received_crc)
{
	return calculate(data, length) == received_crc;
}