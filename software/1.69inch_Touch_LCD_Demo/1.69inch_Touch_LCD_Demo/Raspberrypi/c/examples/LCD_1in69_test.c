#include "DEV_Config.h"
#include "LCD_1in69.h"
#include "Touch_1in69.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include "test.h"
#include "image.h"
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()

UBYTE flag = 0,flgh = 0;
UWORD *BlackImage,x,y,l;
PI_THREAD (myThread) //Used to determine whether it is continuous touch or single touch
{
    while(1)
    {
        l++;
        if(l>30)
            l=25;
        DEV_Delay_ms(1);
    }
    
    return NULL;
}

void LCD_1IN69_test(void)
{
    // Exception handling:ctrl + c
    signal(SIGINT, Handler_1IN69_LCD);
    
    /* Module Init */
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    /* LCD Init */
	printf("1.69inch LCD demo...\r\n");
	LCD_1IN69_Init(VERTICAL);
	LCD_1IN69_Clear(BLACK);
	LCD_SetBacklight(1023);


    UDOUBLE Imagesize = LCD_1IN69_HEIGHT*LCD_1IN69_WIDTH*2;
    printf("Imagesize = %d\r\n", Imagesize);
    if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage(BlackImage, LCD_1IN69_WIDTH, LCD_1IN69_HEIGHT, 0, BLACK, 16);
    Paint_Clear(WHITE);
	Paint_SetRotate(ROTATE_0);
    
    char *BmpPath[3] = {"./pic/LCD_1inch69_1.bmp", "./pic/LCD_1inch69_2.bmp", "./pic/LCD_1inch69_3.bmp"};
    for(UBYTE i=0; i<3; i++) {
        GUI_ReadBmp(BmpPath[i]);
        LCD_1IN69_Display(BlackImage);
        DEV_Delay_ms(1000);
    }

    wiringPiISR(TP_INT,INT_EDGE_FALLING,&Touch_INT_callback);

    Touch_Gesture();
    Touch_HandWriting();

    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;
	DEV_ModuleExit();
}

void Touch_HandWriting()
{
    XY.mode = 1;
    XY.color = BLACK;
    flgh = TOUCH_INIT; //Initializes the flag bit   
    flag = TOUCH_INIT;
    /* TP Init */
    Touch_1IN69_init(XY.mode);

    piThreadCreate(myThread);

    DEV_Delay_ms(10);
    Paint_Clear(WHITE);

    Paint_DrawRectangle(0, 0, 35, 280,RED,DOT_PIXEL_1X1,DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 0, 240, 35,GREEN,DOT_PIXEL_1X1,DRAW_FILL_FULL);
    Paint_DrawRectangle(205, 0, 240, 280,BLUE,DOT_PIXEL_1X1,DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 245, 240, 280,GRAY,DOT_PIXEL_1X1,DRAW_FILL_FULL); 
    LCD_1IN69_Display(BlackImage);
    while(1)
    {
        if(flgh == TOUCH_INIT && XY.x_point != TOUCH_INIT)
        {
            flgh = true; 
            //Get the coordinates of the first point    
            x = XY.x_point;
            y = XY.y_point;
        }   
        
        if (flag == TOUCH_IRQ)
        {
            XY = Touch_1IN69_Get_Point();
            if ((XY.x_point > 35 && XY.x_point < 205) && (XY.y_point > 35 && XY.y_point < 245))
            {
                flgh = TOUCH_DRAW;//Permit painting     
            }
            else
            {
                if ((XY.x_point > 0 && XY.x_point < 33) && (XY.y_point > 0 && XY.y_point < 245))
                    {
                        XY.color = RED;
                    }
                if ((XY.x_point > 0 && XY.x_point < 205) && (XY.y_point > 0 && XY.y_point < 33))
                    {
                        XY.color = GREEN;
                    }
                if ((XY.x_point > 208 && XY.x_point < 240) && (XY.y_point > 0 && XY.y_point < 245))
                    {
                        XY.color = BLUE;
                    }
                if ((XY.x_point > 0 && XY.x_point < 240) && (XY.y_point > 245 && XY.y_point < 280))
                    {
                        Paint_ClearWindows(35,35,205,245,WHITE);
                        Paint_DrawRectangle(0, 245, 240, 280,GRAY,DOT_PIXEL_1X1,DRAW_FILL_FULL); 
                        LCD_1IN69_DisplayWindows(35,35,205,245,BlackImage);
                    }
                flgh = TOUCH_NO_DRAW; //No painting     
                flag = TOUCH_DRAW;
            }
            

            if (flgh == TOUCH_DRAW)
            {
                XY.x_point = (XY.x_point > 37)? XY.x_point : 37;
                XY.y_point = (XY.y_point > 37)? XY.y_point : 37;

                XY.x_point = (XY.x_point < 205)? XY.x_point : 205;
                XY.y_point = (XY.y_point < 245)? XY.y_point : 245;    
                
                if (l<16) //Continuous drawing  
                {
                    flag = TOUCH_DRAW;
                    flgh = TOUCH_NO_DRAW;
                    Paint_DrawLine(x,y,XY.x_point, XY.y_point, XY.color, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
                    LCD_1IN69_DisplayWindows(x,y,XY.x_point, XY.y_point,BlackImage);
                    l = 0;
                }
                else//POINT     
                {
                    flag = TOUCH_DRAW; 
                    flgh = TOUCH_NO_DRAW;
                    Paint_DrawPoint(XY.x_point, XY.y_point, XY.color, DOT_PIXEL_2X2, DOT_FILL_AROUND);
                    LCD_1IN69_DisplayWindows(x,y,XY.x_point, XY.y_point,BlackImage);
                    l = 0;
                }
                x = XY.x_point;
                y = XY.y_point;
            }
        }
    }
}

void Touch_Gesture()
{
    XY.mode = 0;
    /* TP Init */
    if(Touch_1IN69_init(XY.mode) == true)
        printf("OK!\r\n");
    else
        printf("NO!\r\n");
    Paint_Clear(WHITE);
    
    Paint_DrawString_EN(35, 90, "Gesture test", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(10, 120, "Complete as prompted", &Font16, BLACK, WHITE);
    LCD_1IN69_Display(BlackImage);
    Paint_Clear(WHITE);
    DEV_Delay_ms(1500);
    Paint_DrawImage(gImage_up, 45, 30, 150, 150);
    LCD_1IN69_Display(BlackImage);
    while(XY.Gesture != UP)
    {
        
        Paint_DrawString_EN(105, 190, "Up", &Font24, 0X647C, WHITE);
        LCD_1IN69_DisplayWindows(105,190,145,215,BlackImage);
    }
    Paint_ClearWindows(105,190,145,215,WHITE);
    Paint_DrawImage(gImage_down, 45, 30, 150, 150);
    LCD_1IN69_Display(BlackImage);
    while(XY.Gesture != Down)
    {
        
        Paint_DrawString_EN(85, 190, "Down", &Font24, 0X647C, WHITE);
        LCD_1IN69_DisplayWindows(85,190,155,215,BlackImage);
    }
    Paint_ClearWindows(85,190,155,215,WHITE);
    Paint_DrawImage(gImage_left, 45, 30, 150, 150);
    LCD_1IN69_Display(BlackImage);
    while(XY.Gesture != LEFT)
    {
        Paint_DrawString_EN(85, 190, "Left", &Font24, 0X647C, WHITE);
        LCD_1IN69_DisplayWindows(85,190,155,215,BlackImage);
    } 
    Paint_ClearWindows(85,190,155,215,WHITE);
    Paint_DrawImage(gImage_right, 45, 30, 150, 150);
    LCD_1IN69_Display(BlackImage);
    while(XY.Gesture != RIGHT)
    {
        Paint_DrawString_EN(80, 190, "Right", &Font24, 0X647C, WHITE);
        LCD_1IN69_DisplayWindows(80,190,160,215,BlackImage);
    }
    Paint_ClearWindows(80,190,170,215,WHITE);
    Paint_DrawImage(gImage_long_press, 45, 30, 150, 150);
    LCD_1IN69_Display(BlackImage);
    while(XY.Gesture != LONG_PRESS)
    {
        Paint_DrawString_EN(47, 190, "Long Press", &Font20, 0X647C, WHITE);
        LCD_1IN69_DisplayWindows(47,190,180,215,BlackImage);
    }
    Paint_ClearWindows(47,190,200,215,WHITE);
    Paint_DrawImage(gImage_double_click, 45, 30, 150, 150);
    LCD_1IN69_Display(BlackImage);
    while(XY.Gesture != DOUBLE_CLICK)
    {
        Paint_DrawString_EN(35, 185, "Double Click", &Font20, 0X647C, WHITE);
        LCD_1IN69_DisplayWindows(35,185,210,215,BlackImage);
    }
    Paint_ClearWindows(35,185,210,215,WHITE);
    flag = TOUCH_OUT_GESTURE; //Exit gesture mode 
}

void Touch_INT_callback(void)
{
    flag = TOUCH_IRQ;
    if (XY.mode == 0)
        XY.Gesture = I2C_Read_Byte(0x01);
}




