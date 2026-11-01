/*****************************************************************************
* | File      	:   Readme_EN.txt
* | Author      :   Waveshare team
* | Function    :   Help with use
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-03-25
* | Info        :   Here is an English version of the documentation for your quick use.
******************************************************************************/
This file is to help you use this demo.


1. Basic information:
This demo has been verified using the Separate LCD + TP module. 
You can view the corresponding test demos in the \Examples\ of the project.
This Demo has been verified on the Raspberry Pi 4B;

2. Pin connection:
Pin connection You can view it in \lib\lcdconfig.py , and repeat it here:
LCD  	=>   RPI(BCM)
VCC    	->    3.3V
GND    	->    GND
LCD_DIN  ->    10(SPI0_MOSI)
LCD_CLK  ->    11(SPI0_SCK)
LCD_CS    ->    8(CE0)
LCD_DC   ->    25
LCD_RST  ->    27
LCD_BL    ->    18

Touch      =>   RPI(BCM)
TP_SDA    ->    SDA
TP_SCL     ->    SCL
TP_RST     ->    17
TP_IRQ     ->     4

3.Installation library
    sudo apt-get update
    sudo apt-get install python-pip
    sudo apt-get install python-pil
    sudo apt-get install python-numpy
    sudo pip install RPi.GPIO
    sudo pip install smbus
    sudo pip install spidev
or

    sudo apt-get update
    sudo apt-get install python3-pip
    sudo apt-get install python3-pil
    sudo apt-get install python3-numpy
    sudo pip3 install RPi.GPIO
    sudo pip3 install smbus
    sudo pip3 install spidev

4. Basic use:
For usage,you may need to read the following for use:
You can view the test program in the examples\ directory.
example:
     If you need to run the example, then you should execute the command:
     sudo python 1inch69_LCD_test.py