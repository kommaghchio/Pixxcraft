#include "image.h"
#include "LCD_Test.h"
#include "LCD_1in69.h"
#include "Touch_Driver.h"
#include "DEV_Config.h"

Touch_1IN69_XY XY;
UBYTE flag = 0,flgh = 0;
UWORD x,y,l = 0;

void LCD_1in69_test()
{
	printf("LCD_1IN28_test Demo\r\n");
	DEV_Module_Init();
  
  printf("LCD_1IN28_ Init and Clear...\r\n");
	LCD_1IN69_SetBackLight(1000);
	LCD_1IN69_Init(VERTICAL);
	LCD_1IN69_Clear(WHITE);

	XY.mode = 0;
	if(Touch_1IN69_init(XY.mode) == true)
        printf("OK!\r\n");
    else
        printf("NO!\r\n");

	printf("Paint_NewImage\r\n");
	Paint_NewImage(LCD_1IN69_WIDTH, LCD_1IN69_HEIGHT, 0, WHITE);

	printf("Set Clear and Display Funtion\r\n");
	Paint_SetClearFuntion(LCD_1IN69_Clear);
	Paint_SetDisplayFuntion(LCD_1IN69_DrawPoint);

  printf("Paint_Clear\r\n");
	Paint_Clear(WHITE);
		
	Paint_DrawString_EN(35, 90, "Gesture test", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(10, 120, "Complete as prompted", &Font16, BLACK, WHITE);
  DEV_Delay_ms(500);
	
	Paint_ClearWindows(10,90,230,137,WHITE);  
  Paint_DrawString_EN(105, 100, "Up", &Font24, 0X647C, WHITE);
  while(XY.Gesture != UP)
  {    
		 if (flag == TOUCH_IRQ)
		 {
				XY.Gesture = I2C_Read_Byte(0x01);
				flag = TOUCH_DRAW;
		 }      
		DEV_Delay_ms(10);   
  }

	Paint_ClearWindows(105,100,160,155,WHITE);  
	Paint_DrawString_EN(85, 100, "Down", &Font24, 0X647C, WHITE);
	while(XY.Gesture != Down)
	{
			if (flag == TOUCH_IRQ)
			{
					XY.Gesture = I2C_Read_Byte(0x01);
					flag = TOUCH_DRAW;
			}    
			DEV_Delay_ms(10);     
	}

	Paint_ClearWindows(85, 100,160,155,WHITE);
	Paint_DrawString_EN(85, 100, "Left", &Font24, 0X647C, WHITE);
	while(XY.Gesture != LEFT)
	{
			if (flag == TOUCH_IRQ)
			{
					XY.Gesture = I2C_Read_Byte(0x01);
					flag = TOUCH_DRAW;
			}    
			DEV_Delay_ms(10);     
	}

	Paint_ClearWindows(85, 100,160,155,WHITE);
	Paint_DrawString_EN(80, 100, "Right", &Font24, 0X647C, WHITE);
	while(XY.Gesture != RIGHT)
	{
			if (flag == TOUCH_IRQ)
			{
					XY.Gesture = I2C_Read_Byte(0x01);
					flag = TOUCH_DRAW;
			}     
			DEV_Delay_ms(10);    
	}

	Paint_ClearWindows(80, 100,165,160,WHITE);
	Paint_DrawString_EN(47, 100, "Long Press", &Font20, 0X647C, WHITE);
	while(XY.Gesture != LONG_PRESS)
	{
			if (flag == TOUCH_IRQ)
			{
					XY.Gesture = I2C_Read_Byte(0x01);
					flag = TOUCH_DRAW;
			}     
			DEV_Delay_ms(10);    
	}

	Paint_ClearWindows(47, 100,200,155,WHITE);
	Paint_DrawString_EN(35, 100, "Double Click", &Font20, 0X647C, WHITE);
	while(XY.Gesture != DOUBLE_CLICK)
	{
			if (flag == TOUCH_IRQ)
			{
					XY.Gesture = I2C_Read_Byte(0x01);
					flag = TOUCH_DRAW;
			} 
			DEV_Delay_ms(10);        
	}

	XY.mode = 1;
	Touch_1IN69_init(XY.mode);
	Paint_ClearWindows(10,90,230,137,WHITE); 
	Paint_DrawPoint(120, 140, BLACK, DOT_PIXEL_2X2, DOT_FILL_AROUND);
	while (1)
	{
			if (flag == TOUCH_IRQ)
			{				
					Paint_DrawPoint(XY.x_point, XY.y_point, XY.color, DOT_PIXEL_2X2, DOT_FILL_AROUND);
			}
	}
  
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin & TP_INT_Pin)
	{
		if(XY.mode == 1)
		{
			flag = TOUCH_IRQ;
			XY = Touch_1IN69_Get_Point();
		}
		else
		{
			flag = TOUCH_IRQ;
		}
	}
}


