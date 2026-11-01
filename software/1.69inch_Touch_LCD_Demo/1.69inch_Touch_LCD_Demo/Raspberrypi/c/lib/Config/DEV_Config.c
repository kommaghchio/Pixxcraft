/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V2.0
* | Date        :   2019-07-08
* | Info        :   Basic version
*
******************************************************************************/
#include "DEV_Config.h"

uint32_t fd;
int INT_PIN;

void DEV_SetBacklight(UWORD Value)
{
	
#ifdef USE_BCM2835_LIB
    bcm2835_pwm_set_data(0,Value);
    
#elif USE_WIRINGPI_LIB
    pwmWrite(LCD_BL,Value);
    
#endif
	
}

/*****************************************
                GPIO
*****************************************/
void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
#ifdef USE_BCM2835_LIB
    bcm2835_gpio_write(Pin, Value);
    
#elif USE_WIRINGPI_LIB
    digitalWrite(Pin, Value);
    
#endif
}

UBYTE DEV_Digital_Read(UWORD Pin)
{
    UBYTE Read_value = 0;
#ifdef USE_BCM2835_LIB
    Read_value = bcm2835_gpio_lev(Pin);
    
#elif USE_WIRINGPI_LIB
    Read_value = digitalRead(Pin);

#endif
    return Read_value;
}

void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
#ifdef USE_BCM2835_LIB  
    if(Mode == 0 || Mode == BCM2835_GPIO_FSEL_INPT){
        bcm2835_gpio_fsel(Pin, BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(Pin,BCM2835_GPIO_PUD_UP);
    }else {
        bcm2835_gpio_fsel(Pin, BCM2835_GPIO_FSEL_OUTP);
    }
#elif USE_WIRINGPI_LIB
    if(Mode == 0 || Mode == INPUT){
        pinMode(Pin, INPUT);
        pullUpDnControl(Pin, PUD_UP);
    }else{ 
        pinMode(Pin, OUTPUT);
        // printf (" %d OUT \r\n",Pin);
    }

#endif   
}

/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
#ifdef USE_BCM2835_LIB
    bcm2835_delay(xms);
#elif USE_WIRINGPI_LIB
    delay(xms);

#endif
}

static void DEV_GPIO_Init(void)
{
    DEV_GPIO_Mode(LCD_CS, 1);
    DEV_GPIO_Mode(LCD_RST, 1);
    DEV_GPIO_Mode(LCD_DC, 1);
    DEV_GPIO_Mode(LCD_BL, 1);

    DEV_GPIO_Mode(TP_INT, 0);
    DEV_GPIO_Mode(TP_RST, 1);
    
    LCD_CS_1;
	LCD_BL_1;
    
}
/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE DEV_ModuleInit(void)
{

 #ifdef USE_BCM2835_LIB
    if(!bcm2835_init()) {
        printf("bcm2835 init failed  !!! \r\n");
        return 1;
    } else {
        printf("bcm2835 init success !!! \r\n");
    }
    DEV_GPIO_Init();
    bcm2835_spi_begin();                                         //Start spi interface, set spi pin for the reuse function
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);     //High first transmission
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                  //spi mode 0
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);  //Frequency
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                     //set CE0
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);     //enable cs0
	
	bcm2835_gpio_fsel(LCD_BL, BCM2835_GPIO_FSEL_ALT5);
    bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
    
	bcm2835_pwm_set_mode(0, 1, 1);
    bcm2835_pwm_set_range(0,1024);
	bcm2835_pwm_set_data(0,512);
	
#elif USE_WIRINGPI_LIB  
    //if(wiringPiSetup() < 0)//use wiringpi Pin number table  
    if(wiringPiSetupGpio() < 0) { //use BCM2835 Pin number table
        DEBUG("set wiringPi lib failed	!!! \r\n");
        return 1;
    } else {
        DEBUG("set wiringPi lib success  !!! \r\n");
    }
    DEV_GPIO_Init();
    wiringPiSPISetup(0,40000000);
	pinMode (LCD_BL, PWM_OUTPUT);
    pwmWrite(LCD_BL,512);
	
#endif
    DEV_I2C_Init(0x15);
    return 0;
}

void DEV_SPI_WriteByte(uint8_t Value)
{
#ifdef USE_BCM2835_LIB
    bcm2835_spi_transfer(Value);
    
#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0,&Value,1);
     
#endif
}

void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
    uint8_t Data[Len];
    memcpy(Data, pData,  Len);
#ifdef USE_BCM2835_LIB
    uint8_t rData[Len];
    bcm2835_spi_transfernb((char *)Data,(char *)rData,Len);
    
#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, (unsigned char *)Data, Len);

#endif
}

/******************************************************************************
function:	I2C Function initialization and transfer
parameter:
Info:
******************************************************************************/
void DEV_I2C_Init(uint8_t Add)
{

#ifdef USE_BCM2835_LIB
    printf("BCM2835 I2C Device\r\n");  
    bcm2835_i2c_begin();
    bcm2835_i2c_setSlaveAddress(Add);
    
#elif USE_WIRINGPI_LIB
    printf("WIRINGPI I2C Device\r\n");       
    fd = wiringPiI2CSetup(Add);

#endif

}

void I2C_Write_Byte(uint8_t Cmd, uint8_t value)
{
	int ref;
#ifdef USE_BCM2835_LIB
    char wbuf[2]={Cmd, value};
    bcm2835_i2c_write(wbuf, 2);
#elif USE_WIRINGPI_LIB
    //wiringPiI2CWrite(fd,Cmd);
    ref = wiringPiI2CWriteReg8(fd, (int)Cmd, (int)value);
    while(ref != 0) {
        ref = wiringPiI2CWriteReg8 (fd, (int)Cmd, (int)value);
        if(ref == 0)
            break;
    }

#endif

}

int I2C_Read_Byte(uint8_t Cmd)
{
	int ref;

#ifdef USE_BCM2835_LIB
    char rbuf[2]={0};
    bcm2835_i2c_read_register_rs(&Cmd, rbuf, 1);
    ref = rbuf[0];
    
#elif USE_WIRINGPI_LIB
    ref = wiringPiI2CReadReg8 (fd, (int)Cmd);

#endif
    return ref;
}

int I2C_Read_Word(uint8_t Cmd)
{
	int ref;
#ifdef USE_BCM2835_LIB
    char rbuf[2] = {0};
    bcm2835_i2c_read_register_rs(&Cmd, rbuf, 2);
    ref = rbuf[1]<<8 | rbuf[0];
    
#elif USE_WIRINGPI_LIB
    ref = wiringPiI2CReadReg16 (fd, (int)Cmd);
    
#endif
    return ref;
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_ModuleExit(void)
{
#ifdef USE_BCM2835_LIB
    bcm2835_spi_end();
    bcm2835_close();
#elif USE_WIRINGPI_LIB

#endif
}