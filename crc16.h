#pragma once
#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

class CRC16
{
public:
	static uint16_t calculate(const uint8_t* data, uint16_t length);//tinh crc16
	static bool verify(const uint8_t* data, uint16_t length, uint16_t received_crc);//ham kiem tra

private:
	static const uint16_t CRC16_POLYNOMIAL;//Constant da thuc crc16
	static const uint16_t CRC16_INITIAL;//Constant kiem tra crc16
};

#endif