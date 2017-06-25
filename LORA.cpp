#include <LORA.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// default header mode is explicit header mode
uint8_t _headerMode;
// for implicit header mode
uint8_t _payloadLength;

LORA::LORA(uint8_t NSSPin, uint8_t NRESETPin)
{
	_NSSPin=NSSPin;
	_NRESETPin=NRESETPin;
//	_txEnPin=txEnPin;
//	_rxEnPin=rxEnPin;
}
void LORA::spiInit()
{
	SPI.begin();
	//init slave select pin
	pinMode(_NSSPin, OUTPUT);
	digitalWrite(_NSSPin, HIGH);

	// depends on LORA spi timing
	SPI.setBitOrder(MSBFIRST);
	// too fast may cause error
	SPI.setClockDivider(SPI_CLOCK_DIV16);
	SPI.setDataMode(SPI_MODE0);
}
void LORA::pinInit()
{
	pinMode(_NRESETPin, OUTPUT);
	digitalWrite(_NRESETPin, LOW);
//	pinMode(_txEnPin, OUTPUT);
//	digitalWrite(_txEnPin, LOW);
//	pinMode(_rxEnPin, OUTPUT);
//	digitalWrite(_rxEnPin, LOW);
}
bool LORA::init()
{
	pinInit();
	spiInit();

	// reset lora
	powerOnReset();
	// Set RF parameter,like frequency,data rate etc
	config();

	return true;
}
void LORA::powerOnReset()
{
	// close antenna switch
//	digitalWrite(_txEnPin, LOW);
//	digitalWrite(_rxEnPin, LOW);

	digitalWrite(_NRESETPin, LOW); 
	delay(10);
	digitalWrite(_NRESETPin, HIGH);
	delay(20);	
}
bool LORA::config()
{
	// In setting mode, RF module should turn to sleep mode
	// low frequency mode£¬sleep mode
	SPIWriteReg(LR_RegOpMode,LR_Mode_SLEEP|LORA_FREQUENCY_BAND);	
	// wait for steady
	delay(5);								

	// external Crystal
	SPIWriteReg(LR_RegTCXO,LR_EXT_CRYSTAL|LR_REGTCXO_RESERVED);	
	// set lora mode
	SPIWriteReg(LR_RegOpMode,LR_LongRangeMode_LORA|LORA_FREQUENCY_BAND);
	// RF frequency 868.5M
	setFrequency(868500000);	
	// max power ,20dB
	setTxPower(0x0f);
	// close ocp
	SPIWriteReg(LR_RegOcp,LR_OCPON_OFF|0x0B);	
	// enable LNA
	SPIWriteReg(LR_RegLna,LR_LNAGAIN_G1|LR_LNABOOSTHF_1);		
	
	_headerMode=LR_EXPLICIT_HEADER_MODE;
	
	// bandwidth = 500Hz, spreading factor=7,
	// coding rate = 4/5,implict header mode 
	setHeaderMode(_headerMode);
	setRFpara(LR_BW_125k,LR_CODINGRATE_1p25,LR_SPREADING_FACTOR_7,LR_PAYLOAD_CRC_ON);
	// LNA
	SPIWriteReg(LR_RegModemConfig3,LR_MOBILE_MODE);	
	// max rx time out
	setRxTimeOut(0x3ff);
	// preamble 12+4.25 bytes  	
	setPreambleLen(12);

	// 20dBm on PA_BOOST pin
	SPIWriteReg(LR_RegPADAC,LR_REGPADAC_RESERVED|LR_20DB_OUTPUT_ON);  
	 // no hopping
	SPIWriteReg(LR_RegHopPeriod,0x00);     

	// DIO5=ModeReady,DIO4=CadDetected
	SPIWriteReg(LR_RegDIOMAPPING2,LR_DIO4_CADDETECTED|LR_DIO5_MODEREADY);   
	 // standby mode
	SPIWriteReg(LR_RegOpMode,LR_Mode_STBY|LORA_FREQUENCY_BAND); 
	
	// default payload length is 10 bytes in implicit mode
	setPayloadLength(28);

}
bool LORA::setFrequency(uint32_t freq)
{
	uint32_t frf;
	uint32_t temp1;
	uint32_t temp2;
	uint8_t reg[3];

	// Frf(23:0)=frequency/(XOSC/2^19)
	
	temp1=freq/1000000;
	temp2=LORA_XOSC/1000000;
	frf=temp1*524288/temp2;

	temp1=freq%1000000/1000;
	temp2=LORA_XOSC/1000;
	frf=frf+temp1*524288/temp2;

	temp1=freq%1000;
	temp2=LORA_XOSC;
	frf=frf+temp1*524288/temp2;
	
	reg[0]=frf>>16&0xff;
	reg[1]=frf>>8&0xff;
	reg[2]=frf&0xff;
	
	SPIWriteReg(LR_RegFrMsb,reg[0]);
	SPIWriteReg(LR_RegFrMid,reg[1]);
	SPIWriteReg(LR_RegFrLsb,reg[2]);	
	
	// read if the value has been in register
	if((reg[0]!=SPIReadReg(LR_RegFrMsb))||(reg[1]!=SPIReadReg(LR_RegFrMid))||(reg[2]!=SPIReadReg(LR_RegFrLsb)))
		return false;
}
bool LORA::setRFpara(uint8_t BW,uint8_t CR,uint8_t SF,uint8_t payloadCRC)
{
	// check if the data is correct
	if(((BW&0x0f)!=0)||((BW>>8)>0x09))
		return false;
	if(((CR&0xf1)!=0)||((CR>>1)>0x04)||((CR>>1)<0x00))
		return false;
	if(((SF&0x0f)!=0)||((SF>>4)>12)||((SF>>4)<6))
		return false;
	if((payloadCRC&0xfb)!=0)
		return false;
	
	uint8_t temp;
	//SF=6 must be use in implicit header mode,and have some special setting
	if(SF==LR_SPREADING_FACTOR_6)		
	{
		_headerMode=LR_IMPLICIT_HEADER_MODE;
		SPIWriteReg(LR_RegModemConfig1,BW|CR|LR_IMPLICIT_HEADER_MODE);
		temp=SPIReadReg(LR_RegModemConfig2);
		temp=temp&0x03;
		SPIWriteReg(LR_RegModemConfig2,SF|payloadCRC|temp);	

		// according to datasheet
		temp = SPIReadReg(0x31);
		temp &= 0xF8;
		temp |= 0x05;
		SPIWriteReg(0x31,temp);
		SPIWriteReg(0x37,0x0C);	
	}
	else
	{
		temp=SPIReadReg(LR_RegModemConfig2);
		temp=temp&0x03;
		SPIWriteReg(LR_RegModemConfig1,BW|CR|_headerMode);
		SPIWriteReg(LR_RegModemConfig2,SF|payloadCRC|temp);
	}
	return true;
}
bool LORA::setPreambleLen(uint16_t length)
{
	// preamble length is 6~65535
	if(length<6)
		return false;
	SPIWriteReg(LR_RegPreambleMsb,length>>8);
	 // the actual preamble len is length+4.25
	SPIWriteReg(LR_RegPreambleLsb,length&0xff);     
}
bool LORA::setHeaderMode(uint8_t mode)
{
	if(_headerMode>0x01)
		return false;
	_headerMode=mode;

	uint8_t temp;
	// avoid overload the other setting
	temp=SPIReadReg(LR_RegModemConfig1);
	temp=temp&0xfe;
	SPIWriteReg(LR_RegModemConfig1,temp|mode);
}
// in implict header mode, the payload length is fix len
// need to set payload length first in this mode
bool LORA::setPayloadLength(uint8_t len)
{
	_payloadLength=len;
	SPIWriteReg(LR_RegPayloadLength,len);
}
bool LORA::setTxPower(uint8_t power)
{
	if(power>0x0f)
		return false;
	SPIWriteReg(LR_RegPaConfig,LR_PASELECT_PA_POOST|0x70|power);
}
// only valid in rx single mode
bool LORA::setRxTimeOut(uint16_t symbTimeOut)
{
	//rxtimeout=symbTimeOut*(2^SF*BW)
	if((symbTimeOut==0)||(symbTimeOut>0x3ff))
		return false;

	uint8_t temp;
	temp=SPIReadReg(LR_RegModemConfig2);
	temp=temp&0xfc;
	SPIWriteReg(LR_RegModemConfig2,temp|(symbTimeOut>>8&0x03));
	SPIWriteReg(LR_RegSymbTimeoutLsb,symbTimeOut&0xff); 
}
// RSSI[dBm]=-137+rssi value
uint8_t LORA::readRSSI(uint8_t mode)
{
	if(!mode)	//read current rssi
	{
		return SPIReadReg(LR_RegRssiValue);
	}
	else			// read rssi of last packet received
		return SPIReadReg(LR_RegPktRssiValue);
}
bool LORA::rxInit()
{
	if(_headerMode==LR_IMPLICIT_HEADER_MODE)
		setPayloadLength(_payloadLength);
	setRxInterrupt();	// enable RxDoneIrq
	clrInterrupt();		// clear irq flag
	setFifoAddrPtr(LR_RegFifoRxBaseAddr);	// set FIFO addr
	enterRxMode();		// start rx
}
bool LORA::txPacket(uint8_t* sendbuf,uint8_t sendLen)
{
	uint8_t temp;
	
	setTxInterrupt();	// enable TxDoneIrq
	clrInterrupt();		// clear irq flag
	writeFifo(sendbuf,sendLen);
	enterTxMode();
	
	uint16_t txTimer;

	// you should make sure the tx timeout is greater than the max time on air
	txTimer=LORA_TX_TIMEOUT;
	while(txTimer--)
	{
		// wait for txdone
		if(waitIrq(LR_TXDONE_MASK))					
		{
			enterStbyMode();
			clrInterrupt();
			return true;	
		}
		delay(1);
	}
	// if tx time out , reset lora module
	init();
	return false;
}
uint8_t LORA::rxPacket(uint8_t* recvbuf)
{
	// read data from fifo
	return readFifo(recvbuf);	
}
bool LORA::waitIrq(uint8_t irqMask)
{
	uint8_t flag;
	// read irq flag
	flag=SPIReadReg(LR_RegIrqFlags);
	// if irq flag was set
	if(flag&irqMask)					
		return true;
	return false;
}
void LORA::setAntSwitch(uint8_t mode)
{
	// tx
/*	if(mode==LORA_MODE_TX)					
	{
		digitalWrite(_txEnPin, HIGH);
		digitalWrite(_rxEnPin, LOW);
	}
	else if(mode==LORA_MODE_RX)	 //rx
	{
		digitalWrite(_txEnPin, LOW);
		digitalWrite(_rxEnPin, HIGH);
	}
	else						// other
	{
		digitalWrite(_txEnPin, LOW);
		digitalWrite(_rxEnPin, LOW);
	}*/
}
void LORA::setFifoAddrPtr(uint8_t addrReg)
{
	uint8_t addr;
	// read BaseAddr     
	addr = SPIReadReg(addrReg);		
	// BaseAddr->FifoAddrPtr     
	SPIWriteReg(LR_RegFifoAddrPtr,addr);	
}
void LORA::enterRxMode()
{
	// set rx antenna switch
	setAntSwitch(LORA_MODE_RX);
	// enter rx continuous mode
	SPIWriteReg(LR_RegOpMode,LR_Mode_RXCONTINUOUS|LORA_FREQUENCY_BAND);				
}
void LORA::enterTxMode()
{
	// set tx antenna switch
	setAntSwitch(LORA_MODE_TX);
	// enter tx mode
	SPIWriteReg(LR_RegOpMode,LR_Mode_TX|LORA_FREQUENCY_BAND);					
}
void LORA::enterStbyMode()
{
	// close antenna switch
	setAntSwitch(LORA_MODE_STBY);
	// enter Standby mode
	SPIWriteReg(LR_RegOpMode,LR_Mode_STBY|LORA_FREQUENCY_BAND);  	
}
void LORA::enterSleepMode()
{
	setAntSwitch(LORA_MODE_STBY);
	// enter sleep mode
	SPIWriteReg(LR_RegOpMode,LR_Mode_SLEEP|LORA_FREQUENCY_BAND);  	
}
void LORA::writeFifo(uint8_t* databuf,uint8_t length)
{
	// set packet length
	if(_headerMode==LR_EXPLICIT_HEADER_MODE)
		SPIWriteReg(LR_RegPayloadLength,length);	
	// set Fifo addr
	setFifoAddrPtr(LR_RegFifoTxBaseAddr);  
	// fill data into fifo
	SPIBurstWrite(0x00,databuf,length);		
}
uint8_t LORA::readFifo(uint8_t* databuf)
{
	uint8_t readLen;

	// set Fifo addr
	setFifoAddrPtr(LR_RegFifoRxCurrentaddr);
	// read length of packet  
	if(_headerMode==LR_IMPLICIT_HEADER_MODE)
		readLen=_payloadLength;
	else
		readLen = SPIReadReg(LR_RegRxNbBytes);	
	// read from fifo
	SPIBurstRead(0x00, databuf, readLen);		

	return readLen;
}
void LORA::setTxInterrupt()
{
	// DIO0=TxDone,DIO1=RxTimeout,DIO3=ValidHeader
	SPIWriteReg(LR_RegDIOMAPPING1,LR_DIO0_TXDONE); 	
	// enable txdone irq
	SPIWriteReg(LR_RegIrqFlagsMask,0xff^LR_TXDONE_MASK);			
}
void LORA::setRxInterrupt()
{
	//DIO0=00, DIO1=00, DIO2=00, DIO3=01  DIO0=00--RXDONE
	SPIWriteReg(LR_RegDIOMAPPING1,LR_DIO0_RXDONE);	
	// enable rxdone irq
	SPIWriteReg(LR_RegIrqFlagsMask,0xff^LR_RXDONE_MASK);			
}
void LORA::clrInterrupt()
{
	SPIWriteReg(LR_RegIrqFlags,0xff);
}
// SPI read register
uint8_t LORA::SPIReadReg(uint8_t addr)
{
	uint8_t data; 
	
	digitalWrite(_NSSPin, LOW);
	// write register address
	SPI.transfer(addr);	
	// read register value
	data = SPI.transfer(0);			
	digitalWrite(_NSSPin, HIGH);

	return(data);
}

// SPI write register
void LORA::SPIWriteReg(uint8_t addr, uint8_t value)                
{                                                       
	digitalWrite(_NSSPin, LOW);
	// write register address
	SPI.transfer(addr|LORA_SPI_WNR);	
	// write register value
	SPI.transfer(value);			
	digitalWrite(_NSSPin, HIGH);
	
}
void LORA::SPIBurstRead(uint8_t addr, uint8_t *ptr, uint8_t len)
{
	uint8_t i;
	// length>1,use burst mode
	if(len<=1)			
		return;
	else
	{
		digitalWrite(_NSSPin, LOW);
		SPI.transfer(addr);
		for(i=0;i<len;i++)
			ptr[i] = SPI.transfer(0);
		digitalWrite(_NSSPin, HIGH);
	}
}
void LORA::SPIBurstWrite(uint8_t addr, uint8_t *ptr, uint8_t len)
{ 
	uint8_t i;	
	// length>1,use burst mode
	if(len<=1)			
		return;
	else  
	{   
		digitalWrite(_NSSPin, LOW);      
		SPI.transfer(addr|LORA_SPI_WNR);
		for(i=0;i<len;i++)
			SPI.transfer(ptr[i]);
		digitalWrite(_NSSPin, HIGH); 
	}
}
