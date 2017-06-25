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

unsigned char rx_len;
static char rx_buf[300];
unsigned payloadsize=28;
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
            
            rx_len=lora.rxPacket(rx_buf);  // read rx data
            if(rx_len<=payloadsize)
            {
            Serial.print("Receiving:");
            Serial.println(rx_buf);    // print out by serial
            }
            lora.clrInterrupt();
            lora.rxInit();    // wait for packet from master
            Serial.flush();
            
        }
       
     delay(2000);   
    }
}
