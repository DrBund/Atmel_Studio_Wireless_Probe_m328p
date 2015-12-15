#include "SPI.h"


// Variables
bool LED_latch_FLAG = false; // 0-not latched, 1-latched


void initSPImaster(void) {
  // set pin modes
  SPI_SS_DDR  |= (1 << SPI_SS);                       /* set SS Output */
  SPI_SS_PORT |= (1 << SPI_SS);       /* Start off not selected (high) */

  SPI_MOSI_DDR   |= (1 << SPI_MOSI);                 /* output on MOSI */
  //SPI_MISO_DDR   &= ~ (1 << SPI_MISO);                /* input on MISO */
  SPI_MISO_PORT  |= (1 << SPI_MISO);                 /* pullup on MISO */
  SPI_SCK_DDR |= (1 << SPI_SCK);                      /* output on SCK */

  /* Don't have to set phase, polarity b/c
   * default works with 25LCxxx chips */
  SPCR |= (1 << SPR1);                /* div 16, safer for breadboards */
  SPCR |= (1 << MSTR);                                  /* clockmaster */
  SPCR |= (1 << SPE);                                        /* enable */
}

void initSPIslave(void) {
  // set pin modes
  SPI_SS_DDR  &= ~(1 << SPI_SS);                        /* set SS Input */
          
  SPI_MISO_DDR  &= ~(1 << SPI_MISO);                   /* input on MOSI */
  SPI_MOSI_DDR |= (1 << SPI_MOSI);                    /* output on MISO */
  SPI_SCK_DDR   &= ~(1 << SPI_SCK);                     /* input on SCK */

  /* Don't have to set phase, polarity b/c
   * default works with 25LCxxx chips */
  SPCR |= (1 << SPR1);                /* div 16, safer for breadboards */
  SPCR &= ~(1 << MSTR);                                
    /* set to slave */
  SPCR |= (1 << SPE);                                        /* enable */
}


void SPI_tradeByte(uint8_t tData) {
//uint8_t SPI_tradeByte(uint8_t tData) {
  SPDR = tData;                        /* set SPI Data Register to tData */
  loop_until_bit_is_set(SPSR, SPIF);                /* wait until done */
                                /* SPDR now contains the received byte */
  //return SPDR;
}

uint8_t SPI_readByte(void) {
  SLAVE_SELECT;         /* Every new command must be started by a high to low 
                                   transition on CSN pin on the NRF24L01p */
  //uint8_t tmp_rx = SPI_tradeByte(0);
  SLAVE_DESELECT;
  return (SPDR);
}

uint8_t SPI_writeByte(uint8_t wData) {
  SLAVE_SELECT;         /* Every new command must be started by a high to low 
                                   transition on CSN pin on the NRF24L01p */
  SPI_tradeByte(wData);
  SLAVE_DESELECT; 
  return (SPDR);
}



void SPI_turnOnLED() {
    SLAVE_SELECT;
    _delay_ms(30);

    uint8_t tmp_ack = 0;
    do {
      // Send serial byte to SPI slave
      tmp_ack = SPI_writeByte(SPI_LED_ON);
      //SERIAL_ECHO("ACK Recieved: ");
      //SERIAL_ECHOLN((int)tmp_ack);
    } while (!(tmp_ack == ACK_SPI));

    _delay_ms(30);
    SLAVE_DESELECT;

}

void SPI_turnOffLED() {
    SLAVE_SELECT;
    _delay_ms(30); // 50 works well Note: play with this delay to see how low it can be

    uint8_t tmp_ack = 0;
    do {
      // Send serial byte to SPI slave
      tmp_ack = SPI_writeByte(SPI_LED_OFF);
      //SERIAL_ECHO("ACK Recieved: ");
      //SERIAL_ECHOLN((int)tmp_ack);
    } while (!(tmp_ack == ACK_SPI));

    _delay_ms(30); // Delay to allow the transmissions to complete before deselecting the slave device 
    SLAVE_DESELECT;

    //SERIAL_ECHO("** SPI LED OFF ** ");

    //Unlatch LED_latch_FLAG
    LED_latch_FLAG = false;
    
}


void SPI_latchOnLED() {
    // If latch flag is not set, latch on
    if (!LED_latch_FLAG) {
      LED_latch_FLAG = true;

      SLAVE_SELECT;
      _delay_ms(30);

      uint8_t tmp_ack = 0;
      do {
        // Send serial byte to SPI slave
        tmp_ack = SPI_writeByte(SPI_LED_ON);
        //SERIAL_ECHO("ACK Recieved: ");
        //SERIAL_ECHOLN((int)tmp_ack);
      } while (!(tmp_ack == ACK_SPI));

      _delay_ms(30);
      SLAVE_DESELECT;

    }
    // else just leave it latched, unlatch with SPI_turnOffLED()

}
