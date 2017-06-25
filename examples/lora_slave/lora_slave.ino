// lora_slave.pde

// Caution:lora1276/1278 can only work under 3.3V
// please make sure the supply of you board is under 3.3V
// 5v supply will destroy lora module!!

// This code runs in slave mode and  works with lora_master.pde 
// Flow:receive packet from master->print to serial->reply
// data of packet is "swwxABCDEFGHIm"

#include<LORA.h>
#include <SPI.h>
#include <SoftwareSerial.h>
LORA lora;
unsigned char tx_buf[]={"swwxABCDEFGHIm"};
unsigned char val;
unsigned char flag=1;    //  flag of rx mode
unsigned char rx_len;
unsigned char rx_buf[20];

void setup() {
  Serial.begin(9600);
  if(!lora.init())
  {
     Serial.println("Init fail!");
  }
   lora.rxInit();    // wait for packet from master
}
void loop() 
{
    if(flag==0)    // tx a packet if receive a packet
    {
       lora.txPacket(tx_buf,sizeof(tx_buf));
       lora.rxInit();    // turn to rx mode to wait next packet
       flag=1;             // enable rx flag
       Serial.println("tx");
    }
    else            // if(flag==1)
    {
        if(lora.waitIrq(LR_RXDONE_MASK))    // wait for RXDONE interrupt
        {
            flag=0;                          // clear rx flag
            lora.clrInterrupt();
            rx_len=lora.rxPacket(rx_buf);  // read rx data
            Serial.write(rx_buf,rx_len);    // print out by serial
            Serial.println();
        }
    }
}
