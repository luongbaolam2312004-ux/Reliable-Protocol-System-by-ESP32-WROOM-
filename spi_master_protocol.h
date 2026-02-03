#pragma once
#ifndef SPI_MASTER_PROTOCOL_H
#define SPI_MASTER_PROTOCOL_H

#include <Arduino.h>
#include <SPI.h>
#include "packet_frame.h"

class SpiMasterProtocol
{
private:
	SPIClass* spi;
	int cs_pin;
	PacketFrame packet_frame;

	uint8_t rx_buffer[sizeof(Frame)];
	uint8_t tx_buffer[sizeof(Frame)];

	unsigned long last_byte_time;
public:
	SpiMasterProtocol(SPIClass* spi, int cs_pin);

	void begin();

	//Send DATA frame + wait ACK/NACK
	void send_spi_data();
	bool send_spi_master(Frame* frame);

	void display_frame_proper(Frame frame);

	PerformanceMonitor& get_perf_protocol() { return packet_frame.get_performance_monitor(); }

};

#endif // !SPI_MASTER_PROTOCOL_H