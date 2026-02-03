#include "packet_frame.h"

PacketFrame::PacketFrame() : sequence_counter(0)

{
	sequence_counter = 0;
}

uint16_t PacketFrame::get_next_sequence()
{
	uint16_t seq = sequence_counter;
	sequence_counter = (sequence_counter + 1) % 65535;
	return seq;
}

bool PacketFrame::create_frame(PacketType type, const uint8_t* data, uint16_t data_len, Frame* frame)
{
	if (data_len > MAX_DATA_LEN || !frame) return false;

	frame->start_marker = START_MARKER;
	frame->packet_type = type;
	frame->sequence_num = get_next_sequence();
	frame->data_length = data_len;
	frame->end_marker = END_MARKER;

	//Clear and copy data
	memset(frame->data, 0, MAX_DATA_LEN);
	if (data_len > 0)
	{
		memcpy(frame->data, data, data_len);
	}

	//Calculate CRC
	uint16_t data_part_size = 1 + 2 + 2 + data_len;//type + seq + len + data
	frame->crc16 = CRC16::calculate((uint8_t*)&frame->packet_type, data_part_size);

	perf_monitor.packet_sent(sizeof(Frame));
	return true;
}

bool PacketFrame::validate_frame(Frame* frame)
{
	if (!frame) return false;

	//Check marker
	if (frame->start_marker != START_MARKER || frame->end_marker != END_MARKER)
	{
		return false;
	}

	//Verify CRC
	uint16_t data_part_size = 1 + 2 + 2 + frame->data_length;
	uint16_t calculated_crc = CRC16::calculate((uint8_t*)&frame->packet_type, data_part_size);

	if (calculated_crc != frame->crc16)
	{
		record_crc_error();
		return false;
	}


	return true;
}