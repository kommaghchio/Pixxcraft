/*****************************************************************************
* | File      	:   Readme_EN.txt
* | Author      :   
* | Function    :   Help with use
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-03-25
* | Info        :   Here is an English version of the documentation for your quick use.
******************************************************************************/
This file is to help you use this demo.
Here is a brief description of the use of this project:

1. Basic information:
This demo is verified using ESP32-S3-DEV-KIT-N8R8 + 1.69inch Touch LCD. 
You can view the corresponding test demo in the project;

2. Dependency version:
*Pay attention to the version of the library. There may be errors when compiling different versions.
ESP32_Display_Panel v0.0.2
ESP32_IO_Expander	>= v0.0.1
arduino-esp32 >=3.0.0
LVGL v8.4.x

3. Open the ino file, compile and upload it

4. Pin connection:
You can check the pin connection at libraries\ESP_Panel_Conf.h, and repeat it here:
LCD  	=>	ESP32S3
VCC    	->    	3.3V
GND    	->    	GND
LCD_DIN  ->    	10
LCD_CLK  ->    	11
LCD_CS    ->    	9
LCD_DC   ->    	8
LCD_RST  ->    	14
LCD_BL    ->    	2

Touch      =>	ESP32S3
TP_SDA    ->            6
TP_SCL     ->           7
TP_RST     ->           16
TP_IRQ     ->           17
