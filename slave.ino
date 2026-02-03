#include "uart_protocol.h"
#include <ESP32SPISlave.h>

HardwareSerial SerialPort(2);
#define RX2 16
#define TX2 17

#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define SPI_CS 5

#define FRAME_LEN 64

PacketFrame packet_frame;
UartProtocol uart_protocol(&SerialPort, 115200);
ESP32SPISlave slave;

Frame rx_frame;
Frame tx_frame;

uint8_t rx_buffer[FRAME_LEN];
uint8_t tx_buffer[FRAME_LEN];
//========================================== DEBUG FUNCTION =====================================
void display_frame_proper(Frame frame)
{
  Serial.print("Start Marker: "); Serial.println(frame.start_marker);
  Serial.print("Packet Type: "); Serial.println(frame.packet_type);
  Serial.print("Sequence Num: "); Serial.println(frame.sequence_num);
  Serial.print("Data: ");
  for(size_t i = 0; i < frame.data_length; i++)
  {
    Serial.print((char)frame.data[i]);
  }
  Serial.println();
  Serial.print("Data length: "); Serial.println(frame.data_length);
  Serial.print("CRC: "); Serial.println(frame.crc16);
  Serial.print("End Marker: "); Serial.println(frame.end_marker);
  Serial.println("------------------------------------------------");
}

void print_frame_info_spi(Frame* frame)
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

void process_received_frame_spi_slave(Frame* frame)
{
	Serial.print("\n<<< SLAVE RECEIVED: ");
	print_frame_info_spi(frame);
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
	}
}


//======================================================= MAIN FUNCTION ===========================================

void setup() 
{
  Serial.begin(115200);

  //UART CONFIG
  SerialPort.begin(115200, SERIAL_8N1, RX2, TX2);

  //SPI SLAVE CONFIG
  slave.setDataMode(SPI_MODE0);
  slave.begin(VSPI);

  //Preload buffer
  memset(tx_buffer, 0 , FRAME_LEN);

}

void loop() 
{
  static bool mode = true;

  if(mode == false)//UART Mode
  {
    //Nhận và xử lí frame
    uart_protocol.receive_data_uart_slave();

    delay(10);//Small delay to prevent watchdog timer
  }
  else//SPI Mode
  {
    slave.queue(nullptr, rx_buffer, FRAME_LEN);
    slave.wait();

    memcpy(&rx_frame, rx_buffer, FRAME_LEN);

    // Process frame
    //
    if (rx_frame.data_length > MAX_DATA_LEN)
    {
      Serial.println("FRAME OVERFLOW");
      packet_frame.create_frame(TYPE_NACK, nullptr, 0, &tx_frame);
    }
    else if (!packet_frame.validate_frame(&rx_frame))
    {
      Serial.println("CRC ERROR");
      packet_frame.create_frame(TYPE_NACK, nullptr, 0, &tx_frame);
    }
    else
    {
      
      process_received_frame_spi_slave(&rx_frame);
      packet_frame.create_frame(TYPE_ACK, nullptr, 0, &tx_frame);
    }
    tx_frame.sequence_num = rx_frame.sequence_num;

    uint16_t crc_len = 1 + 2 + 2; // type + seq + len
    tx_frame.crc16 = CRC16::calculate((uint8_t*)&tx_frame.packet_type, crc_len);

    memcpy(tx_buffer, &tx_frame, FRAME_LEN);

    // send ACK/NACK in next transaction
    slave.queue(tx_buffer, nullptr, FRAME_LEN);
    slave.wait();
  }
}
