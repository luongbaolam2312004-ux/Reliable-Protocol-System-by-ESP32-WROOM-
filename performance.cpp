#include "performance.h"
#include <Arduino.h>
#include <climits>

PerformanceMonitor::PerformanceMonitor()
{
	reset_statistics();
}

void PerformanceMonitor::reset_statistics()
{
	total_bytes_sent = 0;
	total_bytes_received = 0;
	total_packets_sent = 0;
	total_packets_received = 0;

	lost_packets = 0;
	sequence_errors = 0;
	crc_errors = 0;
	timeouts = 0;
	retransmissions = 0;

	//Initialize latency tracking
	for (int i = 0; i < LATENCY_BUFFER_SIZE; i++)
	{
		latency_samples[i] = 0;
	}
	latency_index = 0;
	total_latency = 0;
	min_latency = UINT32_MAX;
	max_latency = 0;
	last_latency = 0;

	//Create Packet Timing
	memset(packet_start_time, 0, sizeof(packet_start_time));

	measurement_start_time = millis();
}

void PerformanceMonitor::packet_sent(uint16_t packet_size)
{
	total_packets_sent++;
	total_bytes_sent += packet_size;
}

void PerformanceMonitor::packet_received(uint16_t packet_size)
{
	total_packets_received++;
	total_bytes_received += packet_size;
}

float PerformanceMonitor::get_throughput_kbps() const
{
	unsigned long current_time = millis();
	unsigned long elapsed_time = current_time - measurement_start_time;

	if (elapsed_time == 0) return 0.0;

	float total_bits = total_bytes_sent * 8.0;
	float time_seconds = elapsed_time / 1000.0;

	return total_bits / time_seconds / 1024.0;
}

float PerformanceMonitor::get_packet_rate() const
{
	unsigned long current_time = millis();
	unsigned long elapsed_time = current_time - measurement_start_time;

	if (elapsed_time == 0) return 0.0;

	float time_seconds = elapsed_time / 1000.0;
	return total_packets_sent / time_seconds;
}

void PerformanceMonitor::start_latency_measurement(uint16_t sequence_num)
{
	packet_start_time[sequence_num % MAX_SEQUENCE_NUMS] = millis();
}

void PerformanceMonitor::end_latency_measurement(uint16_t sequence_num)
{
	unsigned long end_time = millis();
	unsigned long start_time = packet_start_time[sequence_num % MAX_SEQUENCE_NUMS];

	//Micro() overflow handled automatically
	uint32_t latency;
	if (end_time < start_time)
	{
		latency = (ULONG_MAX - start_time) + end_time;
		Serial.print(" [OVERFLOW_HANDLED]");
	}
	else
	{
		latency = end_time - start_time;
	}

	Serial.print("Raw Latency: ");
	Serial.print(latency);
	Serial.println(" ms");

	if (latency < 1 || latency > 5000)
	{
		Serial.print("[WARN] Ignoring abnormal latency: ");
		Serial.print(latency);
		Serial.print("ms for seq: ");
		Serial.println(sequence_num);
		packet_start_time[sequence_num % MAX_SEQUENCE_NUMS] = 0;
		return;
	}

	if (min_latency == UINT32_MAX)
	{
		min_latency = latency;
		Serial.print("[INIT] First min_latency set to: ");
		Serial.println(min_latency);
	}

	//Reset for not measure many time
	packet_start_time[sequence_num % MAX_SEQUENCE_NUMS] = 0;

	//Update circular buffer
	total_latency -= latency_samples[latency_index];
	latency_samples[latency_index] = latency;
	total_latency += latency;

	//Update min/max
	if (latency < min_latency)
	{
		Serial.print("[UPDATE] New min_latency: ");
		Serial.print(latency); Serial.print("ms (was: ");
		Serial.print(min_latency); Serial.println("ms)");
		min_latency = latency;
	}

	if (latency > max_latency)
	{
		Serial.print("[UPDATE] New max_latency: ");
		Serial.print(latency); Serial.print("ms (was: ");
		Serial.print(max_latency); Serial.println("ms)");
		max_latency = latency;
	}

	last_latency = latency;
	latency_index = (latency_index + 1) % LATENCY_BUFFER_SIZE;
	Serial.print("[BUFFER] Index: ");
	Serial.print(latency_index);
	Serial.print("; Current Sample: ");
	Serial.println(latency);
}

float PerformanceMonitor::get_average_latency() const
{
	uint32_t valid_samples = 0;
	uint32_t sum = 0;

	for (int i = 0; i < LATENCY_BUFFER_SIZE; i++)
	{
		if (latency_samples[i] > 0)
		{
			sum += latency_samples[i];
			valid_samples++;
		}
	}

	if (valid_samples == 0) return 0.0;
	return sum / (float)valid_samples;
}

int PerformanceMonitor::get_max_latency() const
{
	if (max_latency == 0 || max_latency == UINT32_MAX)
	{
		return 0;
	}
	else return max_latency;
}

int PerformanceMonitor::get_min_latency() const
{
	if (min_latency == UINT32_MAX)
	{
		return 0;
	}
	else return min_latency;
}

float PerformanceMonitor::get_average_jitter() const
{
	if (min_latency == UINT32_MAX || max_latency == 0) return 0.0;

	uint32_t valid_samples = 0;
	for (int i = 0; i < LATENCY_BUFFER_SIZE; i++)
	{
		if (latency_samples[i] > 0) valid_samples++;
	}

	if (valid_samples < 2) return 0.0;

	return (max_latency - min_latency) / 2.0;
}

void PerformanceMonitor::packet_lost(uint16_t sequence_num)
{
	lost_packets++;
}

void PerformanceMonitor::sequence_error(uint16_t expected, uint16_t received)
{
	sequence_errors++;
}

void PerformanceMonitor::crc_error()
{
	crc_errors++;
}

void PerformanceMonitor::timeout_occurred()
{
	timeouts++;
}

void PerformanceMonitor::retransmission_occurred()
{
	retransmissions++;
}

float PerformanceMonitor::get_packet_loss_rate() const
{
	if (total_packets_sent == 0) return 0.0;
	return (float)lost_packets / total_packets_sent * 100.0;
}

float PerformanceMonitor::get_error_rate() const
{
	if (total_packets_received == 0) return 0.0;
	uint32_t total_errors = crc_errors + sequence_errors;
	return (float)total_errors / total_packets_received * 100.0;
}

float PerformanceMonitor::get_success_rate() const
{
	if (total_packets_sent == 0) return 0.0;
	uint32_t successful = total_packets_sent - retransmissions - lost_packets;

	return (float)successful / total_packets_sent * 100.0;
}

void PerformanceMonitor::print_statistics()
{
	unsigned long current_time = millis();
	unsigned long elapsed_time = current_time - measurement_start_time;

	Serial.println("\n ==== PERFORMANCE STATISTICS ====");
	Serial.println("THROUGHPUT: ");
	Serial.print(" Packets Sent: "); Serial.println(total_packets_sent);
	Serial.print(" Packets Received: "); Serial.println(total_packets_received);
	Serial.print(" Data Sent: "); Serial.print(total_bytes_sent / 1024.0, 2); Serial.println(" KB");
	Serial.print(" Throughput: "); Serial.print(get_throughput_kbps(), 2); Serial.println(" kbps");
	Serial.print(" Packet Rate "); Serial.print(get_packet_rate(), 2); Serial.println(" packets/s");

	Serial.println("LATENCY: ");
	Serial.print(" Average: "); Serial.print(get_average_latency(), 2); Serial.println(" ms");
	Serial.print(" Min: "); Serial.print(get_min_latency()); Serial.println(" ms");
	Serial.print(" Max: "); Serial.print(get_max_latency()); Serial.println(" ms");
	Serial.print(" Jitter: "); Serial.print(get_average_jitter(), 2); Serial.println(" ms");

	Serial.println("ERROR ANALYSIS:");
	Serial.print(" Packet Loss: "); Serial.print(get_packet_loss_rate(), 2); Serial.println("%");
	Serial.print(" CRC Errors: "); Serial.println(crc_errors);
	Serial.print(" Sequence Errors: "); Serial.println(sequence_errors);
	Serial.print(" Timeouts: "); Serial.println(timeouts);
	Serial.print(" Retransmissions: "); Serial.println(retransmissions);
	Serial.print(" Success Rate: "); Serial.print(get_success_rate(), 2); Serial.println("%");

	Serial.print("Measurement Duration: ");
	Serial.print(elapsed_time / 1000.0, 1);
	Serial.println(" seconds");
	Serial.println("====================================================\n");

}