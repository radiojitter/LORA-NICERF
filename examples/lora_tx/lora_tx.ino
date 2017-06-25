// lora_tx.pde

// Caution:lora module can only work under 3.3V
// please make sure the supply of you board is under 3.3V
// 5v supply will destroy lora module!!

// This code runs in tx mode and  works with lora_rx.pde 
// Flow:receive "T" from serial->send a packet
// data of packet is "swwxABCDEFGHIm"

#include <LORA.h>
#include <SPI.h>
#include <SoftwareSerial.h>
LORA lora;
char tx_buf[]={"ABCDEFGHIJKLMN"};
unsigned char val;

void setup() {
  Serial.begin(9600);
  if(!lora.init())
  {
     Serial.println("Init fail!");
  }
}
void loop() 
{
    //val=Serial.read();  // please make sure serial is OK befor runing this code
    //if(val=='T')    // tx a packet if receive "T"
    while(1)
    {
       lora.txPacket(tx_buf,sizeof(tx_buf));
       Serial.print("Transmitting:");
       Serial.println(tx_buf);    // print out by serial
       
       delay(2000);
       lora.enterStbyMode();    // turn to standby mode 
    }
}
