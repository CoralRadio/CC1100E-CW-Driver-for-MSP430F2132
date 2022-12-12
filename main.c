
#include "msp430x21x2.h"
#include "CCxxx0.h"

//#define MSP430_CC2500

#define COUNT               10
#define TIME_COUNT          500

#define CCxxx0_WRITE_BURST  0x40
#define CCxxx0_READ_SINGLE  0x80
#define CCxxx0_READ_BURST   0xC0

void Init_Uart(void);
void Uart_SendString(char string[],unsigned int len);
void Setup_SPI(void);
void SPI_write(unsigned char value);
unsigned char SPI_read(void);
char halSpiReadReg(unsigned char addr);
char halSpiReadStatus(char addr);

void halSpiWriteReg(unsigned char addr,unsigned char value);
void halSpiStrobe(unsigned char strobe);
void halRfWirteRfSettings_CC2500(void);
void TI_CC_Wait(unsigned int cycles);

unsigned char temp;
unsigned char data_buf[40];
unsigned char data_len = 0;
unsigned char data_flag = 0;

void Init_Uart(void)
{
  P3SEL |= 0x30;
  P3DIR |= 0x30;
  UCA0CTL1 |= UCSSEL_1;
  UCA0BR0 = 0x03;           //9600   8N1;
  UCA0BR1 = 0x00;
  UCA0MCTL = UCBRS1 + UCBRS0;
  UCA0CTL1 &= ~UCSWRST;
  //IFG2 &= ~UCA0RXIFG;
  IE2 |= UCA0RXIE;
}

void Uart_SendString(char string[],unsigned int len)
{
  unsigned int i;
  
  for(i = 0;i < len;i++)
  {
    while(!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = string[i];
  }
}

void Setup_SPI(void)
{
  P2DIR |= 0x01;
  P3OUT |= 0x01;                           // P3.3,2,1 USCI_B0 option select.P3SEL |= 0x11;//P3.0,4 USCI_A0 option select
  P3DIR |= 0x01;
  UCB0CTL0 |= UCMSB + UCMST + UCSYNC + UCCKPL;       // 3-pin, 8-bit SPI mstr, MSB 1st
  UCB0CTL1 |= UCSSEL_2;                     // MCLK
  UCB0BR0 = 0x02;
  UCB0BR1 = 0;
  P3SEL |= 0x0E; 
  P3DIR |= 0x0A;
  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}

void SPI_write(unsigned char value)
{
  IFG2 &= ~UCB0RXIFG;
  UCB0TXBUF = value;
  while (!(IFG2 & UCB0RXIFG));
}

unsigned char SPI_read(void)
{
  unsigned char Value;
  UCB0TXBUF = 0x00;
  while (!(IFG2 & UCB0RXIFG));
  Value = UCB0RXBUF;
  return Value;
}

char halSpiReadReg(unsigned char addr)
{
  char temp;
  P3OUT &= ~0x01;
  while (!(IFG2&UCB0TXIFG));                // Wait for TX to finish
  UCB0TXBUF = (addr | CCxxx0_READ_SINGLE);// Send address
  while (!(IFG2&UCB0TXIFG));                // Wait for TX to finish
  UCB0TXBUF = 0;                            // Dummy write so we can read data
  // Address is now being TX'ed, with dummy byte waiting in TXBUF...
  while (!(IFG2&UCB0RXIFG));                // Wait for RX to finish
  // Dummy byte RX'ed during addr TX now in RXBUF
  IFG2 &= ~UCB0RXIFG;                       // Clear flag set during addr write
  while (!(IFG2&UCB0RXIFG));                // Wait for end of dummy byte TX
  // Data byte RX'ed during dummy byte write is now in RXBUF
  temp = UCB0RXBUF; 
  P3OUT |= 0x01;
  return temp;
}

char halSpiReadStatus(char addr)
{
  char temp;
  P3OUT &= ~0x01;
  while(P3IN & 0x04);
  IFG2 &= ~UCB0RXIFG;
  UCB0TXBUF = (addr | CCxxx0_READ_BURST);
  while (!(IFG2&UCB0RXIFG));                // Wait for TX to finish
  IFG2 &= ~UCB0RXIFG;                       // Clear flag set during last write
  UCB0TXBUF = 0;                            // Dummy write so we can read data
  while (!(IFG2&UCB0RXIFG));                // Wait for RX to finish
  temp = UCB0RXBUF;                            // Read data
  P3OUT |= 0x01;
  return temp; 
}

void halSpiWriteReg(unsigned char addr,unsigned char value)
{
  P3OUT &= ~0x01;                //Enable CC2500;
  while(P3IN & 0x04);
  SPI_write(addr);
  SPI_write(value);
  P3OUT |= 0x01;                 //Disable CC2500;
}

void halSpiStrobe(unsigned char strobe)
{
  IFG2 &= ~UCB0RXIFG;
  P3OUT &= ~0x01;                //Enable CC2500;
  while(P3IN & 0x04);
  UCB0TXBUF = strobe;
  while(!(IFG2&UCB0RXIFG));
  P3OUT |= 0x01;                 //Disable CC2500;
}
#ifdef CC2500
void halRfWirteRfSettings_CC2500(void)
{
   halSpiWriteReg(CCxxx0_FREQ2,    0x5D);	
   //---------------------------------------------------------------------------
   //halSpiWriteReg(CCxxx0_FREQ1,    0xa7);    //2434.999847Mhz	//
   //halSpiWriteReg(CCxxx0_FREQ0,    0x62);
   //---------------------------------------------------------------------------
   halSpiWriteReg(CCxxx0_FREQ1,    0x89);    //2431.999847Mhz	//
   halSpiWriteReg(CCxxx0_FREQ0,    0xd8);
   //---------------------------------------------------------------------------
   halSpiWriteReg(CCxxx0_PKTLEN,   0xFF);			//1
   halSpiWriteReg(CCxxx0_FIFOTHR,  0x0f);
   halSpiWriteReg(CCxxx0_PKTCTRL1, 0x04);			//2     
   halSpiWriteReg(CCxxx0_PKTCTRL0, 0x05);			//3
	halSpiWriteReg(CCxxx0_FSCTRL1,  0x0B);			//4
	halSpiWriteReg(CCxxx0_FSCTRL0,  0x00);			//5
	halSpiWriteReg(CCxxx0_MDMCFG4,  0x78);   		//
	halSpiWriteReg(CCxxx0_MDMCFG3,  0x93);			//8
	halSpiWriteReg(CCxxx0_MDMCFG2,  0x93);			//9
  	halSpiWriteReg(CCxxx0_MDMCFG1,  0x22);			//10
  	halSpiWriteReg(CCxxx0_MDMCFG0,  0xF8);			//11	
  	halSpiWriteReg(CCxxx0_CHANNR,   0x0A);			//6
	halSpiWriteReg(CCxxx0_DEVIATN,  0x44);			//12
	halSpiWriteReg(CCxxx0_FREND0,   0x10);
	halSpiWriteReg(CCxxx0_FREND1,   0x56);

    halSpiWriteReg(CCxxx0_MCSM2 ,   0x07); //CCA enbale
    halSpiWriteReg(CCxxx0_MCSM1 ,   0x30); //0x30 CCA enbale

    halSpiWriteReg(CCxxx0_MCSM0 ,   0x18); //
    halSpiWriteReg(CCxxx0_FOCCFG,   0x16);			//16
    halSpiWriteReg(CCxxx0_BSCFG,    0x6C);			//17
	halSpiWriteReg(CCxxx0_AGCCTRL2, 0x43);			//18
    halSpiWriteReg(CCxxx0_AGCCTRL1, 0x40);			//19
    halSpiWriteReg(CCxxx0_AGCCTRL0, 0x91);			//20
    halSpiWriteReg(CCxxx0_FSCAL3,   0xA9);			//23
    halSpiWriteReg(CCxxx0_FSCAL2,   0x0A);			//24
    halSpiWriteReg(CCxxx0_FSCAL1,   0x00);			//25
    halSpiWriteReg(CCxxx0_FSCAL0,   0x11);			//26
    halSpiWriteReg(CCxxx0_FSTEST,   0x59);			//27
    halSpiWriteReg(CCxxx0_TEST2,    0x88);			//30
    halSpiWriteReg(CCxxx0_TEST1,    0x31);			//31
    halSpiWriteReg(CCxxx0_TEST0,    0x0B);			//32

    halSpiWriteReg(CCxxx0_IOCFG2,  0x0B);
    halSpiWriteReg(CCxxx0_IOCFG0,  0x06);
}
#elif defined CC1101
void halRfWirteRfSettings_CC1101(void) //433.9MHZ
{
   //     halSpiWriteReg(CCxxx0_FREQ2,    0x5D);	
   //---------------------------------------------------------------------------
   //halSpiWriteReg(CCxxx0_FREQ1,    0xa7);    //2434.999847Mhz	//
   //halSpiWriteReg(CCxxx0_FREQ0,    0x62);
   //---------------------------------------------------------------------------
  // halSpiWriteReg(CCxxx0_FREQ1,    0x89);    //2431.999847Mhz	//
   //halSpiWriteReg(CCxxx0_FREQ0,    0xd8);
   //---------------------------------------------------------------------------
     halSpiWriteReg(CCxxx0_FREQ2,    0x10);
     halSpiWriteReg(CCxxx0_FREQ1,    0xB0);
     halSpiWriteReg(CCxxx0_FREQ0,    0x71);
   //===========================================================================
       halSpiWriteReg(CCxxx0_PKTLEN,   0xFF);			//1
       halSpiWriteReg(CCxxx0_FIFOTHR,  0x0f);
       halSpiWriteReg(CCxxx0_PKTCTRL1, 0x04);			//2     
        halSpiWriteReg(CCxxx0_PKTCTRL0, 0x05);			//3
	halSpiWriteReg(CCxxx0_FSCTRL1,  0x0B);			//4
	halSpiWriteReg(CCxxx0_FSCTRL0,  0x00);			//5
	halSpiWriteReg(CCxxx0_MDMCFG4,  0x7c);   		//
	halSpiWriteReg(CCxxx0_MDMCFG3,  0x93);			//8
	halSpiWriteReg(CCxxx0_MDMCFG2,  0x93);			//9
  	halSpiWriteReg(CCxxx0_MDMCFG1,  0x22);			//10
  	halSpiWriteReg(CCxxx0_MDMCFG0,  0xF8);			//11	
  	halSpiWriteReg(CCxxx0_CHANNR,   0x00);			//6
	halSpiWriteReg(CCxxx0_DEVIATN,  0x44);			//12
	halSpiWriteReg(CCxxx0_FREND0,   0x10);
	halSpiWriteReg(CCxxx0_FREND1,   0x56);

    halSpiWriteReg(CCxxx0_MCSM2 ,   0x07); //CCA enbale
    halSpiWriteReg(CCxxx0_MCSM1 ,   0x30); //0x30 CCA enbale

    halSpiWriteReg(CCxxx0_MCSM0 ,   0x18); //
    halSpiWriteReg(CCxxx0_FOCCFG,   0x16);			//16
    halSpiWriteReg(CCxxx0_BSCFG,    0x6C);			//17
	halSpiWriteReg(CCxxx0_AGCCTRL2, 0x43);			//18
    halSpiWriteReg(CCxxx0_AGCCTRL1, 0x40);			//19
    halSpiWriteReg(CCxxx0_AGCCTRL0, 0x91);			//20
    halSpiWriteReg(CCxxx0_FSCAL3,   0xA9);			//23
    halSpiWriteReg(CCxxx0_FSCAL2,   0x0A);			//24
    halSpiWriteReg(CCxxx0_FSCAL1,   0x00);			//25
    halSpiWriteReg(CCxxx0_FSCAL0,   0x11);			//26
    halSpiWriteReg(CCxxx0_FSTEST,   0x59);			//27
    halSpiWriteReg(CCxxx0_TEST2,    0x88);			//30
    halSpiWriteReg(CCxxx0_TEST1,    0x31);			//31
    halSpiWriteReg(CCxxx0_TEST0,    0x0B);			//32

    halSpiWriteReg(CCxxx0_IOCFG2,  0x0B);
    halSpiWriteReg(CCxxx0_IOCFG0,  0x06);
}
#elif defined CC1100E
void halRfWirteRfSettings_CC1100E(void)  
{
   //     halSpiWriteReg(CCxxx0_FREQ2,    0x5D);	
   //---------------------------------------------------------------------------
   //halSpiWriteReg(CCxxx0_FREQ1,    0xa7);    //2434.999847Mhz	//
   //halSpiWriteReg(CCxxx0_FREQ0,    0x62);
   //---------------------------------------------------------------------------
  // halSpiWriteReg(CCxxx0_FREQ1,    0x89);    //2431.999847Mhz	//
   //halSpiWriteReg(CCxxx0_FREQ0,    0xd8);
   //---------------------------------------------------------------------------
     halSpiWriteReg(CCxxx0_FREQ2,    0x12);  //490MHZ
     halSpiWriteReg(CCxxx0_FREQ1,    0xD8);
     halSpiWriteReg(CCxxx0_FREQ0,    0x9D);
   //===========================================================================
       halSpiWriteReg(CCxxx0_PKTLEN,   0xFF);			//1
       halSpiWriteReg(CCxxx0_FIFOTHR,  0x0f);
       halSpiWriteReg(CCxxx0_PKTCTRL1, 0x04);			//2     
        halSpiWriteReg(CCxxx0_PKTCTRL0, 0x05);			//3
	halSpiWriteReg(CCxxx0_FSCTRL1,  0x0B);			//4
	halSpiWriteReg(CCxxx0_FSCTRL0,  0x00);			//5
	halSpiWriteReg(CCxxx0_MDMCFG4,  0xFA);   		//
	halSpiWriteReg(CCxxx0_MDMCFG3,  0x93);			//8
	halSpiWriteReg(CCxxx0_MDMCFG2,  0x93);			//9
  	halSpiWriteReg(CCxxx0_MDMCFG1,  0x22);			//10
  	halSpiWriteReg(CCxxx0_MDMCFG0,  0xF8);			//11	
  	halSpiWriteReg(CCxxx0_CHANNR,   0x00);			//6
	halSpiWriteReg(CCxxx0_DEVIATN,  0x35);			//12
	halSpiWriteReg(CCxxx0_FREND0,   0x10);
	halSpiWriteReg(CCxxx0_FREND1,   0x56);

    halSpiWriteReg(CCxxx0_MCSM2 ,   0x07); //CCA enbale
    halSpiWriteReg(CCxxx0_MCSM1 ,   0x30); //0x30 CCA enbale

    halSpiWriteReg(CCxxx0_MCSM0 ,   0x18); //
    halSpiWriteReg(CCxxx0_FOCCFG,   0x16);			//16
    halSpiWriteReg(CCxxx0_BSCFG,    0x6C);			//17
	halSpiWriteReg(CCxxx0_AGCCTRL2, 0x43);			//18
    halSpiWriteReg(CCxxx0_AGCCTRL1, 0x40);			//19
    halSpiWriteReg(CCxxx0_AGCCTRL0, 0x91);			//20
    halSpiWriteReg(CCxxx0_FSCAL3,   0xA9);			//23
    halSpiWriteReg(CCxxx0_FSCAL2,   0x0A);			//24
    halSpiWriteReg(CCxxx0_FSCAL1,   0x00);			//25
    halSpiWriteReg(CCxxx0_FSCAL0,   0x11);			//26
    halSpiWriteReg(CCxxx0_FSTEST,   0x59);			//27
    halSpiWriteReg(CCxxx0_TEST2,    0x88);			//30
    halSpiWriteReg(CCxxx0_TEST1,    0x31);			//31
    halSpiWriteReg(CCxxx0_TEST0,    0x0B);			//32

    halSpiWriteReg(CCxxx0_IOCFG2,  0x0B);
    halSpiWriteReg(CCxxx0_IOCFG0,  0x06);
}
#endif
void POWER_UP_RESET(void)
{
  P3OUT |= 0x01;
  TI_CC_Wait(60);
  P3OUT &= ~0x01;
  TI_CC_Wait(60);
  P3OUT |= 0x01;
  TI_CC_Wait(90);
  
  P3OUT &= ~0x01;
  while(P3IN & 0x04);
  UCB0TXBUF = CCxxx0_SRES;
  IFG2 &= ~UCB0RXIFG;
  while(!(IFG2 & UCB0RXIFG));
  while(P3IN & 0x04);
  P3OUT |= 0x01;
}
void TI_CC_Wait(unsigned int cycles)
{
  while(cycles>15)                          // 15 cycles consumed by overhead
    cycles = cycles - 6;                    // 6 cycles consumed each iteration
}
void SPIWriteBurstReg(char addr, char *buffer, char count)
{
  char i;
  P3OUT &= ~0x01;
  while(P3IN & 0x04);
  IFG2 &= ~UCB0RXIFG;
  UCB0TXBUF = addr | CCxxx0_WRITE_BURST;
  while(!(IFG2 & UCB0RXIFG));
  for(i = 0;i < count; i++)
  {
    IFG2 &= ~UCB0RXIFG;
    UCB0TXBUF = buffer[i];
    while(!(IFG2 & UCB0RXIFG));
  }
  P3OUT |= 0x01;
}

main( void )
{
  char paTable[] = {0xFF};
  char paTableLen = 1;
  unsigned int i,j,k;
  char txBuffer[40] = {0x27,0x55,0x7A,0x20,0x60,0xff,0xff,0xff,0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x01,0x02,0x03,0x04,0x05,0xff,0xff,0xff,0xff,0xb0};
  char txBuffer1[4] = {0x03,0x01,0x01,0x00};
  WDTCTL = WDTPW + WDTHOLD;
  //-------------ADD外部32K晶振----------
  
  do
  {
     IFG1&=~OFIFG;
     for(i=250;i>0;i--);
  }
   while((IFG1&OFIFG)==OFIFG);   //当OSCFault＝1 即晶振不起振则等待
  BCSCTL2 = SELM1 +SELM0 + SELS;
  
  //--------------------------
  Init_Uart();
  Setup_SPI();
  for(i = 100; i > 0;i--);
  POWER_UP_RESET();
#ifdef CC2500
  halRfWirteRfSettings_CC2500();
  paTable[0] = 0xFF;  //CC2500 MAX OUTPUT POWER
  SPIWriteBurstReg(CCxxx0_PATABLE,paTable,paTableLen);
#elif defined CC1101
  halRfWirteRfSettings_CC1101();
  paTable[0] = 0xC0; //CC1101 MAX OUTPUT POWER
  SPIWriteBurstReg(CCxxx0_PATABLE,paTable,paTableLen);
#elif defined CC1100E
  halRfWirteRfSettings_CC1100E();
  paTable[0] = 0x60;  //CC1100E MAX OUTPUT POWER  0xC2为最大功率10dbm,0x60为0dbm
  SPIWriteBurstReg(CCxxx0_PATABLE,paTable,paTableLen);
#endif  
  _EINT();
  P2OUT |= 0x01;  //亮
  for(k = COUNT;k > 0;k--)
    {
      halSpiStrobe(CCxxx0_SIDLE);
      SPIWriteBurstReg(CCxxx0_TXFIFO,txBuffer,40);
      halSpiStrobe(CCxxx0_STX);
      while(!(P2IN & 0x02));
      while(P2IN & 0x02);
      for(j = 1; j > 0;j--)
      {
        for(i = TIME_COUNT; i > 0;i--);
      }
      //txBuffer[2]++;
      //txBuffer[9]++;
      //txBuffer[39]++;
    }
  for(k = COUNT;k > 0;k--)
    {
      halSpiStrobe(CCxxx0_SIDLE);
      SPIWriteBurstReg(CCxxx0_TXFIFO,txBuffer1,4);
      halSpiStrobe(CCxxx0_STX);
      while(!(P2IN & 0x02));
      while(P2IN & 0x02);
      for(j = 1; j > 0;j--)
      {
        for(i = TIME_COUNT; i > 0;i--);
      }
    }
  
  Uart_SendString(txBuffer,40);
  for(i = 0;i < 1000;i++);
  while(data_flag)
  {
    P2OUT &= ~0x01;  //灭
    IE2 &= ~UCA0RXIE;
    data_flag = 0;
    //halSpiStrobe(CCxxx0_SPWD);
    halSpiStrobe(CCxxx0_SXOFF);
    halSpiStrobe(CCxxx0_SPWD);
    P3OUT |= 0x01;
    for(i = 0;i < 10;i++);
    LPM4;
  }
  
}


#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIAB0_ISR(void)
{
  if(IFG2 & UCA0RXIFG)
  {
    data_buf[data_len] = UCA0RXBUF;
    data_len++;
    if(data_len == 40)
    {
      //Uart_SendString(data_buf,data_len);
      data_flag = 1;
      data_len = 0;
    }
    //IFG2 &= ~UCA0RXIFG;
  }
}
