// lora_rx.pde

// Caution:lora can only work under 3.3V
// please make sure the supply of you board is under 3.3V
// 5v supply will destroy lora module!!

// This code runs in rx mode and  works with lora_tx.pde 
// Flow:receive packet from tx->print to serial
// data of packet is "ABCDEFGHIJKLMN"

#include<LORA.h>
#include <SPI.h>
#include <SoftwareSerial.h>
LORA lora;

unsigned int rx_len;
static char rx_buf[400];
unsigned int payloadsize=30;
void setup() {
  Serial.begin(115200);
  if(!lora.init())
  {
     Serial.println("Init fail!");
  }
   lora.rxInit();    // wait for packet from master
}
void loop() 
{
    for(;;)
    {
        memset(rx_buf, 0, sizeof(rx_buf));
        if(lora.waitIrq(LR_RXDONE_MASK))    // wait for RXDONE interrupt
        {
            
            rx_len=lora.rxPacket((uint8_t*)rx_buf);  // read rx data
            Serial.print(rx_len);
            if(rx_len<=payloadsize)
            {
            Serial.print("Receiving:");
            //Serial.println(rx_buf);    // print out by serial
            for (int i = 0; i < payloadsize; i++)
            {
              Serial.print(rx_buf[i]);
              
            }
            Serial.println();
            delay(2000);
            Serial.flush(); 
            }
            lora.clrInterrupt();
            lora.rxInit();    // wait for packet from master
            
            
        }
       
        
    }
}
