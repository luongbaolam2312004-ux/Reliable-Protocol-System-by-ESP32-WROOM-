#pragma once
#ifndef PACKET_FRAME_H
#define PACKET_FRAME_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "crc16.h"
#include "performance.h"

#define START_MARKER 0xAA
#define END_MARKER 0x55
#define MAX_DATA_LEN 53
#define MAX_RETRIES 3
#define ACK_TIMEOUT_MS 1000

typedef enum
{
	TYPE_DATA = 0x01,
	TYPE_ACK = 0x02,
	TYPE_NACK = 0x03,
}PacketType;

typedef struct
{
	uint8_t start_marker;//1 byte
	uint8_t packet_type;//1 byte
	uint16_t sequence_num;//2 byte
	uint16_t data_length;//2 byte
	uint8_t data[MAX_DATA_LEN];//53 byte
	uint16_t crc16 ;//2 byte
	uint8_t end_marker;//1 byte
}Frame;

class PacketFrame
{
private:
	uint16_t sequence_counter;
	PerformanceMonitor perf_monitor;
public:
	PacketFrame();
	virtual ~PacketFrame() = default;

	//Frame creation & validation
	bool create_frame(PacketType type, const uint8_t* data, uint16_t data_len, Frame* frame);
	bool validate_frame(Frame* frame);
	uint16_t get_next_sequence();
	PerformanceMonitor& get_performance_monitor() { return perf_monitor; }

	//Error tracking
	void record_crc_error() { perf_monitor.crc_error(); }//crc_errors++
	void record_timeout() { perf_monitor.timeout_occurred(); }//timeouts++
	void record_retransmission() { perf_monitor.retransmission_occurred(); }//retransmissions++
	void record_packet_lost(uint16_t seq) { perf_monitor.packet_lost(seq); }//lost_packets++

	void start_packet_timing(uint16_t seq_num) { perf_monitor.start_latency_measurement(seq_num); }
	void end_packet_timing(uint16_t seq_num) { perf_monitor.end_latency_measurement(seq_num); }

};

#endif// 