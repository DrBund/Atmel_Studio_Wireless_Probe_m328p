/* Bio SPI Slave Arduino 
 *
 * Controls print head for biological materials for RepRap printer.
 *
 * SPI pin numbers:
 * SCK   13  // Serial Clock.
 * MISO  12  // Master In Slave Out.
 * MOSI  11  // Master Out Slave In.
 * SS    10  // Slave Select
*/

#include <avr.io>

// Pin Definitions
#define LED_PIN 9 

// Useful functions
                                 /* Which pin selects BIOSPI as slave? */
//#define SLAVE_SELECT    {digitalWrite(SS, LOW); digitalWrite(MISO,OUTPUT)}
//#define SLAVE_DESELECT  digitalWrite(SS, HIGH)

// Instruction Set -- interface with Arduino  Not currently used
#define SPI_LED_ON    0b00000011                     /* turn on LED */
#define SPI_LED_OFF   0b00000010                    /* turn off LED */

#define SPI_RDSR      0b00000101            /* read status register */
#define SPI_WRSR      0b00000001           /* write status register */

/* Define Pins */

void slaveInitSPI(void) {
  // Initialize SPI pins
  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(SCK, INPUT);
  pinMode(SS, INPUT);
  // Enable SPI as slave
  //SPCR |= (1 << SPR1);      /* div 16, safer for breadboards */
  SPCR |= (1 << SPE);
}

/*
void slaveSelect() {
  //digitalWrite(SS, LOW); 
  digitalWrite(MISO, OUTPUT);
}

void slaveDeselect() {
  //digitalWrite(SS, HIGH); 
  digitalWrite(MISO, INPUT);
}
*/

void SPI_tradeByte(uint8_t byte) {
  SPDR = byte;                       /* SPI starts sending immediately */
  //Serial.println("Begin bit trade, slave");
  loop_until_bit_is_set(SPSR, SPIF);                /* wait until done */
                                /* SPDR now contains the received byte */
  //Serial.println("End bit trade, slave");
}
  
uint8_t SPI_readByte() {
  //SLAVE_SELECT;
  SPI_tradeByte(0);
  //Serial.println("Begin bit trade, slave");
  //while(!(SPSR & (1<<SPIF)));
  //Serial.println("End bit trade, slave");
  //delay(10);
  return (SPDR);
}

void SPI_writeByte(uint8_t byte) {
  //SLAVE_SELECT;
  SPI_tradeByte(byte);
  //SLAVE_DESELECT;
}




void setup() {
  // Initialize serial for troubleshooting.
  Serial.begin(9600);
  // Initialize slave LED pin.
  pinMode(LED_PIN, OUTPUT);
  // Initialize SPI Slave.
  slaveInitSPI();
  Serial.println("Slave Initialized");
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
}


void loop() {
  
  // Poll SS and if it goes Low there is an incomming message to deal with

  // If SS goes low, there is an incomming transmission
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


} // loop()
