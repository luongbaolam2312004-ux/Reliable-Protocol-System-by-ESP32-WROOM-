#include "spi_master_protocol.h"

#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

SpiMasterProtocol::SpiMasterProtocol(SPIClass* s, int cs) :
	spi(s), cs_pin(cs)
{
	memset(rx_buffer, 0, sizeof(Frame));
	memset(tx_buffer, 0, sizeof(Frame));
}

void SpiMasterProtocol::begin()
{
	pinMode(cs_pin, OUTPUT);
	digitalWrite(cs_pin, HIGH);

	spi->begin(SPI_SCK, SPI_MISO, SPI_MOSI, cs_pin);
	spi->setDataMode(SPI_MODE0);
	spi->setBitOrder(MSBFIRST);

	Serial.println("SPI MASTER READY");
}

void SpiMasterProtocol::send_spi_data()
{
	static uint16_t test_counter = 0;
	static unsigned long last_throughput_check = 0;

	Frame frame;
	String message = "Test " + String(test_counter) + " - Time: " + String(millis());
	//String message = "HELLO SLAVE";

	if (packet_frame.create_frame(TYPE_DATA, (uint8_t*)message.c_str(), message.length(), &frame))//Create message data
	{
		bool success = send_spi_master(&frame);//Check frames are sent or not

		if (success)
		{
			Serial.println("Reliable delivery comfirmed via SPI");
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

bool SpiMasterProtocol::send_spi_master(Frame* frame)
{
	Frame tx_frame;
	Frame rx_frame;
	

	memcpy(&tx_frame, frame, sizeof(Frame));
	memcpy(tx_buffer, &tx_frame, sizeof(Frame));

	//------ SEND DATA ------
	packet_frame.start_packet_timing(frame->sequence_num);
	digitalWrite(cs_pin, LOW);
	for (size_t i = 0; i < sizeof(Frame); i++)
	{
		spi->transfer(tx_buffer[i]);
	}

	digitalWrite(cs_pin, HIGH);
	delay(2);
	//delayMicroseconds(50);

	//------ READ ACK/NACK ------

	digitalWrite(cs_pin, LOW);
	for (size_t i = 0; i < sizeof(Frame); i++)
	{
		rx_buffer[i] = spi->transfer(0x00);
	}
	digitalWrite(cs_pin, HIGH);
	memcpy(&rx_frame, rx_buffer, sizeof(Frame));
	
	/*Serial.println("----------------------------------");
	Serial.println("\nSend:");
	display_frame_proper(tx_frame);
	Serial.println("\nReceive:");
	display_frame_proper(rx_frame);*/

	if (!packet_frame.validate_frame(&rx_frame))
	{
		Serial.println("\nRX CRC ERROR");
		packet_frame.record_crc_error();
		packet_frame.record_packet_lost(tx_frame.sequence_num);
		return false;
	}
	else if (rx_frame.packet_type == TYPE_ACK)
	{
		Serial.println("\nACK RECEIVED");
		packet_frame.end_packet_timing(rx_frame.sequence_num);
		return true;
	}
	else if (rx_frame.packet_type == TYPE_NACK)
	{
		Serial.println("\nNACK RECEIVED");
		packet_frame.record_packet_lost(tx_frame.sequence_num);
		return false;
	}
	else
	{
		Serial.println("\nUNKNOWN FRAME");
		packet_frame.record_packet_lost(tx_frame.sequence_num);
		return false;
	}
	return false;
}

void SpiMasterProtocol::display_frame_proper(Frame frame)
{
	Serial.print("Start Marker: "); Serial.println(frame.start_marker);
	Serial.print("Packet Type: "); Serial.println(frame.packet_type);
	Serial.print("Sequence Num: "); Serial.println(frame.sequence_num);
	Serial.print("Data: ");
	for (size_t i = 0; i < frame.data_length; i++)
	{
		Serial.print((char)frame.data[i]);
	}
	Serial.println();
	Serial.print("Data length: "); Serial.println(frame.data_length);
	Serial.print("CRC: "); Serial.println(frame.crc16);
	Serial.print("End Marker: "); Serial.println(frame.end_marker);
	Serial.println("------------------------------------------------");
}