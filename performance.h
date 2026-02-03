#pragma once
#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <Arduino.h>
#include <stdint.h>

class PerformanceMonitor
{
private:
	static const uint8_t LATENCY_BUFFER_SIZE = 50;
	static const uint16_t MAX_SEQUENCE_NUMS = 256;

	//Throughtput metrics
	uint32_t total_bytes_sent;
	uint32_t total_bytes_received;
	uint32_t total_packets_sent;
	uint32_t total_packets_received;
	unsigned long measurement_start_time;

	//Latency metrics
	uint32_t latency_samples[LATENCY_BUFFER_SIZE];
	uint16_t latency_index;
	uint32_t total_latency;
	uint32_t min_latency;
	uint32_t max_latency;
	uint32_t last_latency;

	//Error tracking
	uint32_t lost_packets;
	uint32_t sequence_errors;
	uint32_t crc_errors;
	uint32_t timeouts;
	uint32_t retransmissions;

	//Packet timing
	unsigned long packet_start_time[MAX_SEQUENCE_NUMS];

public:
	PerformanceMonitor();

	//Throughtput measurement
	void packet_sent(uint16_t packet_size);
	void packet_received(uint16_t packet_size);
	float get_throughput_kbps() const;
	float get_packet_rate() const;

	//Latency measurement
	void start_latency_measurement(uint16_t sequence_num);
	void end_latency_measurement(uint16_t sequence_num);
	float get_average_latency() const;
	int get_min_latency() const;
	int get_max_latency() const;
	float get_average_jitter() const;

	//Error tracking
	void packet_lost(uint16_t sequence_num);
	void sequence_error(uint16_t expected, uint16_t received);
	void crc_error();
	void timeout_occurred();
	void retransmission_occurred();
	float get_packet_loss_rate() const;
	float get_error_rate() const;
	float get_success_rate() const;

	//Reporting
	void print_statistics();
	void reset_statistics();

	//Getter for external use
	uint32_t get_packet_sent() const { return total_packets_sent; }
	uint32_t get_packet_received() const { return total_packets_received; }
	uint32_t get_crc_errors() const { return crc_errors; }
	uint32_t get_retransmissions() const { return retransmissions; }
};

#endif
