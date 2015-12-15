/* SPI Slave ATMEGA-328 
 *
 * SPI pin numbers:
 * SCK   13  // Serial Clock.
 * MISO  12  // Master In Slave Out.
 * MOSI  11  // Master Out Slave In.
 * SS    10  // Slave Select
*/

#include <avr/io.h>
#include <util/delay.h>

#include <pinDefines.h>
#include <macros.h>
#include <USART.h>
#include <SPI.h>


int main(void) {
  
  // Poll SS and if it goes Low there is an incomming message to deal with

  initPins();
  initSPI();


  // ------Event Loop ------//
  // If SS goes low, there is an incomming transmission
  while(1)  {
    // START HERE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if (!digitalRead(SS)) {

      //Serial.println("Slave Enabled");
      
      // Get byte from SPI 
      uint8_t msg_byte = SPI_readByte();  
      
      // Write acknoledge byte
      //SPI_writeByte(100);
      
      Serial.print("SPI recieved: ");
      Serial.println(msg_byte);

      // Acknowledge byte was recieved
      //SPI_writeByte(msg_byte);
      
      // Light LED if msg_byte valid command
      if (msg_byte == SPI_LED_ON){
        digitalWrite(LED_PIN, HIGH);
        SPI_writeByte(msg_byte);
      }
      else if (msg_byte == SPI_LED_OFF) {
        digitalWrite(LED_PIN, LOW);
        SPI_writeByte(msg_byte);
      }
      else {
        SPI_writeByte(255);
      }
    } // if(!digitalRead(SS))
  }

} // loop()
