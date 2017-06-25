// lora_sleep.pde

// Caution:lora module can only work under 3.3V
// please make sure the supply of you board is under 3.3V
// 5v supply will destroy lora module!!

// This code runs in sleep mode

#include <LORA.h>
#include <SPI.h>
#include <SoftwareSerial.h>
LORA lora;
void setup() {
  Serial.begin(9600);
  if(!lora.init())
  {
     Serial.println("Init fail!");
  }
  // enter sleep mode
   lora.enterSleepMode();
}

void loop() 
{

}
