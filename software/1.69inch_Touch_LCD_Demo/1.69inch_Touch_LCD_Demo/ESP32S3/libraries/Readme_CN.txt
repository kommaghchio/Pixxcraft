/*****************************************************************************
* | File      	:   Readme_CN.txt
* | Author      :   
* | Function    :   Help with use
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-03-25
* | Info        :   在这里提供一个中文版本的使用文档，以便你的快速使用
******************************************************************************/
1、基本信息：
本例程使用ESP32-S3-DEV-KIT-N8R8 + 1.69inch Touch LCD进行了验证，你可以在工程中查看对应的测试例程;

2、安装下面的库
*注意库的版本，不同版本编译可能会有错误
ESP32_Display_Panel v0.0.2
ESP32_IO_Expander	>= v0.0.1
arduino-esp32 >3.0.0
LVGL v8.4.x
3、打开ino文件，编译上传
4、管脚连接：
管脚连接你可以在libraries\ESP_Panel_Conf.h查看，这里也再重述一次：
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
