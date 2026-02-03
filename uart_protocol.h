#pragma once
#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <HardwareSerial.h>
#include "packet_frame.h"

enum ReceiverState
{
	STATE_WAITING_START,
	STATE_RECEIVING_FRAME
};

class UartProtocol
{
private:
	PacketFrame packet_frame;

	HardwareSerial* serial;
	uint32_t baud_rate;

	ReceiverState rx_state;
	uint8_t rx_buffer[sizeof(Frame)];
	uint8_t rx_index;
	unsigned long last_byte_time;
public:
	UartProtocol(HardwareSerial* serial_port, uint32_t baud = 115200);


	//Send data
	void send_uart_data();
	void send_uart_ack(uint16_t seq_num);
	void send_uart_nack(uint16_t seq_num);
	bool send_uart_master(Frame* frame);
	bool send_uart_slave(Frame* frame);
	bool wait_for_ack(uint16_t seq_num, HardwareSerial& serial, uint32_t timeout_ms);

	//Received data
	void receive_data_uart_master();
	void receive_data_uart_slave();
	bool receive_uart(Frame* frame);
	void process_received_frame_uart_master(Frame* frame);
	void process_received_frame_uart_slave(Frame* frame);
	void print_frame_info(Frame* frame);
	
	void reset_receiver();
	bool check_timeout();

	PerformanceMonitor& get_perf_protocol() { return packet_frame.get_performance_monitor(); }

};

#endif // !UART_PROTOCOL_H
