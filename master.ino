#include "uart_protocol.h"
#include "spi_master_protocol.h"
#include <SPI.h>

HardwareSerial SerialPort(2);
#define RX2 16
#define TX2 17

#define SPI_CS 5

PacketFrame packet_frame;
UartProtocol uart_protocol(&SerialPort, 115200);
SpiMasterProtocol spi_master(&SPI, SPI_CS);

//======================================================= MAIN FUNCTION ===========================================

void setup() 
{
  Serial.begin(115200);

  //UART CONFIG
  SerialPort.begin(115200, SERIAL_8N1, RX2, TX2);

  //SPI CONFIG
  spi_master.begin();

  Serial.println("RELIABLE COMMUNICATION SYSTEM - TRANSMITER");
  Serial.println("MODE: SPI");
  Serial.println("START FOR SENDING...");
  Serial.println("=============================================");

}

void loop() 
{
  static bool mode = true;

  if(mode == false)//UART Mode
  {
    static unsigned long last_send = 0;
    static unsigned long last_stats = 0;

    if(millis() - last_send > 2000)
    {
      uart_protocol.send_uart_data();
      last_send = millis();
    }

    uart_protocol.receive_data_uart_master();

    if(millis() - last_stats > 15000)
    {
      uart_protocol.get_perf_protocol().print_statistics();
      last_stats = millis();
    }

    delay(10);
  }
  else//SPI Mode
  {
    static unsigned long last_send = 0;
    static unsigned long last_stats = 0;

    
    if(millis() - last_send > 2000)
    {
      spi_master.send_spi_data();
      last_send = millis();
    }

    if(millis() - last_stats > 15000)
    {
      spi_master.get_perf_protocol().print_statistics();
      last_stats = millis();
    }

    delay(10);
  }
}
