#include "uart_protocol.h"
#include <Arduino.h>

UartProtocol::UartProtocol(HardwareSerial* serial_port, uint32_t baud) :
	serial(serial_port), 
	baud_rate(baud), 
	rx_state(STATE_WAITING_START), 
	rx_index(0),
	last_byte_time(0)
{
	memset(rx_buffer, 0, sizeof(Frame));
}

//==============================================SEND FUNCTION=====================================

void UartProtocol::send_uart_data()
{
	static uint16_t test_counter = 0;
	static unsigned long last_throughput_check = 0;

	Frame frame;
	String message = "Test " + String(test_counter) + " - Time: " + String(millis());

	if (packet_frame.create_frame(TYPE_DATA, (uint8_t*)message.c_str(), message.length(), &frame))//Create message data
	{
		bool success = send_uart_master(&frame);//Check frames are sent or not

		if (success)
		{
			Serial.println("Reliable delivery comfirmed via ");
		}
		else
		{
			Serial.print("Delivery failed for packet ");
			Serial.println(frame.sequence_num);
		}
	}
	test_counter++;

	if (millis() - last_throughput_check > 5000)
	{
		float throughput = packet_frame.get_performance_monitor().get_throughput_kbps();
		Serial.print("Current throughput: ");
		Serial.print(throughput);
		Serial.println(" kbps");
		last_throughput_check = millis();
	}
}

void UartProtocol::send_uart_ack(uint16_t seq_num)//Sent ACK to Master
{
	Frame ack_frame;
	uint8_t empty_data[1] = { 0 };

	if (packet_frame.create_frame(TYPE_ACK, empty_data, 0, &ack_frame))
	{
		ack_frame.sequence_num = seq_num;

		//Use current communication interface
		send_uart_slave(&ack_frame);

		Serial.print("Sent ACK for frame ");
		Serial.print(seq_num);
	}
}

void UartProtocol::send_uart_nack(uint16_t seq_num)//Sent NACK to Master
{
	Frame nack_frame;
	uint8_t empty_data[1] = { 0 };

	if (packet_frame.create_frame(TYPE_NACK, empty_data, 0, &nack_frame))
	{
		nack_frame.sequence_num = seq_num;
		send_uart_slave(&nack_frame);

		Serial.print("Sent NACK for frame ");
		Serial.println(seq_num);
	}

}

bool UartProtocol::send_uart_master(Frame* frame)
{
	int retries = MAX_RETRIES;

	while (retries > 0)
	{
		//Start timing
		packet_frame.start_packet_timing(frame->sequence_num);

		//Send frame
		serial->write((uint8_t*)frame, sizeof(Frame));
		serial->flush();
		delay(2);

		Serial.print("\nSent frame ");
		Serial.print(frame->sequence_num);
		Serial.println(", waiting for ACK...");

		if (wait_for_ack(frame->sequence_num, *serial, ACK_TIMEOUT_MS))
		{
			Serial.println("ACK received - SUCCESS");
			return true;
		}

		//Timeout - retry
		retries--;
		packet_frame.record_retransmission();
		Serial.print("Timeout - Retransmitting frame ");
		Serial.print(frame->sequence_num);
		Serial.print(", Attemps left: ");
		Serial.println(retries);
	}

	packet_frame.record_timeout();
	packet_frame.record_packet_lost(frame->sequence_num);
	return false;
}

bool UartProtocol::send_uart_slave(Frame* frame)
{
	if (!serial) return false;

	size_t bytes_written = serial->write((uint8_t*)frame, sizeof(Frame));
	serial->flush();
	return bytes_written == sizeof(Frame);//Sending completed
}

bool UartProtocol::wait_for_ack(uint16_t seq_num, HardwareSerial& serial, uint32_t timeout_ms)
{
	uint32_t start_time = millis();

	while (millis() - start_time < timeout_ms)
	{
		if (serial.available() >= sizeof(Frame))
		{
			Frame response;
			serial.readBytes((uint8_t*)&response, sizeof(Frame));

			/*Serial.print("Received frame - Type: ");
			Serial.print(response.packet_type);
			Serial.print(", Seq: ");
			Serial.print(response.sequence_num);
			Serial.print(", Expected Seq: ");
			Serial.println(seq_num);*/

			if (packet_frame.validate_frame(&response))
			{
				if (response.packet_type == TYPE_ACK && response.sequence_num == seq_num)
				{
					packet_frame.end_packet_timing(seq_num);
					Serial.println("VALID ACK RECEIVED");
					return true;
				}
				else if (response.packet_type == TYPE_NACK && response.sequence_num == seq_num)
				{
					Serial.println("NACK RECEIVED");
					return false;
				}
				else
				{
					Serial.println("UNEXPECTED FRAME TYPE OR SEQUENCE");
				}
			}
			else
			{
				Serial.println("INVALID FRAME CRC");
			}
		}
		delay(1);
	}
	Serial.println("ACK TIMEOUT");
	return false;
}

//============================================ RECEIVE FUNCTION ========================================

void UartProtocol::print_frame_info(Frame* frame)
{
	Serial.print("Frame[");
	Serial.print(frame->sequence_num);
	Serial.print("] Type:");

	switch (frame->packet_type)
	{
	case TYPE_DATA: Serial.print("DATA"); break;
	case TYPE_ACK: Serial.print("ACK"); break;
	case TYPE_NACK: Serial.print("NACK"); break;
	default: Serial.print("UNKNOWN"); break;
	}

	Serial.print(" Len: "); Serial.print(frame->data_length);
	Serial.print(" CRC: 0x"); Serial.print(frame->crc16, HEX);
	Serial.print(" Valid: "); Serial.print(packet_frame.validate_frame(frame) ? "YES" : "NO");
}

void UartProtocol::receive_data_uart_master()
{
	Frame frame;

	if (receive_uart(&frame))
	{
		process_received_frame_uart_master(&frame);
	}
}

void UartProtocol::receive_data_uart_slave()
{
	Frame frame;
	if (receive_uart(&frame))
	{
		process_received_frame_uart_slave(&frame);
	}
}

void UartProtocol::process_received_frame_uart_master(Frame* frame)//process received frames
{
	Serial.print("\n<<< MASTER RECEIVED: ");
	print_frame_info(frame);//Print frame infos
	Serial.println();

	if (packet_frame.validate_frame(frame))//Check frames
	{
		switch (frame->packet_type)
		{
		case TYPE_DATA:
			Serial.print("Data: ");
			for (int i = 0; i < frame->data_length; i++)
			{
				Serial.print((char)frame->data[i]);
			}
			Serial.println();
			send_uart_ack(frame->sequence_num);
			break;
		case TYPE_ACK:
			Serial.println("ACK processed");
			break;
		case TYPE_NACK:
			Serial.println("NACK processed - will retry");
			break;

		}
	}
	else
	{
		Serial.println("INVALID FRAME");
		send_uart_nack(frame->sequence_num);
	}
}

void UartProtocol::process_received_frame_uart_slave(Frame* frame)
{
	Serial.print("\n<<< SLAVE RECEIVED: ");
	print_frame_info(frame);
	Serial.println();

	if (packet_frame.validate_frame(frame))
	{
		switch (frame->packet_type)
		{
		case TYPE_DATA:
		{
			String received_data = "";

			for (int i = 0; i < frame->data_length; i++)
			{
				received_data += (char)frame->data[i];
			}
			Serial.print("Data: ");
			Serial.println(received_data);

			Serial.print("Sending ACK for seq: ");
			Serial.println(frame->sequence_num);
			send_uart_ack(frame->sequence_num);
			break;
		}
		case TYPE_ACK:
			Serial.println("ACK processed - THIS SHOULD NOT HAPPEN ON SLAVE");
			break;
		case TYPE_NACK:
			Serial.println("NACK processed - will retry");
			break;
		}
	}
	else
	{
		Serial.println("INVALID FRAME - CRC ERROR");
		Serial.print("Sending NACK for seq: ");
		Serial.println(frame->sequence_num);
		send_uart_nack(frame->sequence_num);
	}
}

bool UartProtocol::receive_uart(Frame* frame)
{
	//Using Receiver State Machine for receiving

	if (!serial) return false;

	while (serial->available())
	{
		uint8_t byte = serial->read();
		last_byte_time = millis();

		switch (rx_state)
		{
		case STATE_WAITING_START:
			if (byte == START_MARKER)
			{
				rx_index = 0;
				rx_buffer[rx_index++] = byte;
				rx_state = STATE_RECEIVING_FRAME;
			}
			break;

		case STATE_RECEIVING_FRAME:
			rx_buffer[rx_index++] = byte;

			if (rx_index >= sizeof(Frame))
			{
				//Copy to output frame
				memcpy(frame, rx_buffer, sizeof(Frame));

				if (frame->end_marker == END_MARKER)
				{
					rx_state = STATE_WAITING_START;
					return true;//Valid frame
				}
				else
				{
					Serial.println("Invalid end marker");
					reset_receiver();
					return false;//framing error
				}
			}
			break;
		}
	}

	if (rx_state == STATE_RECEIVING_FRAME && check_timeout())
	{
		reset_receiver();
	}

	return false;//No complete frame available yet
}

void UartProtocol::reset_receiver()
{
	//Resetting for new UART transfer
	rx_state = STATE_WAITING_START;
	rx_index = 0;
	memset(rx_buffer, 0, sizeof(Frame));
	last_byte_time = 0;
}

bool UartProtocol::check_timeout()
{
	return (millis() - last_byte_time) > 500;
}