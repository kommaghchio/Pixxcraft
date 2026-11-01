#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"
#include "CST816S.h"

#define TOUCH_IRQ 3
CST816S touch(-1, -1, 4, TOUCH_IRQ);	// sda, scl, rst, irq

void setup()
{
    Config_Init();
    touch.begin();
    attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), touchISR, RISING);
    LCD_Init();

    LCD_SetBacklight(100);
//UNO has limited memory and demo can only be run individually  UNO内存有限，demo只能单个运行 
    // Demo1(); 
    // Demo2();
    Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
  Paint_Clear(WHITE);
}


void loop()
{
  Demo3();    
}

void touchISR(void)
{
  touch.handleISR();
}

void Demo1(void)
{
#if 1
    Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 90, WHITE);
    Paint_Clear(WHITE);

    Paint_DrawString_EN(30, 10, "123", &Font24, YELLOW, RED);  
    Paint_DrawString_EN(30, 34, "ABC", &Font24, BLUE, CYAN);
    
    Paint_DrawString_CN(235,10, "微", &Font24CN, WHITE, RED);
    Paint_DrawString_CN(235,40, "雪", &Font24CN, WHITE, RED);
    Paint_DrawString_CN(235,70, "电", &Font24CN, WHITE, RED);
    Paint_DrawString_CN(235,100, "子", &Font24CN, WHITE, RED);

    Paint_DrawRectangle(125, 10, 225, 58, RED,DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawLine(125, 10, 225, 58, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(225, 10, 125, 58, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawCircle(150, 100, 25, BLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(180, 100, 25, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(210, 100, 25, RED, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(165, 125, 25, YELLOW, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(195, 125, 25, GREEN, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    Paint_DrawImage(gImage_70X70, 20, 80, 70, 70); 
    delay(3000);
#endif
        
#if 1
    Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
    Paint_Clear(WHITE);
    
    Paint_DrawString_EN(30, 10, "123", &Font24, YELLOW, RED);  
    Paint_DrawString_EN(30, 34, "ABC", &Font24, BLUE, CYAN);
    
    Paint_DrawString_CN(10, 150, "微", &Font24CN,WHITE, RED);
    Paint_DrawString_CN(45, 150, "雪", &Font24CN,WHITE, RED);
    Paint_DrawString_CN(80, 150, "电", &Font24CN,WHITE, RED);
    Paint_DrawString_CN(115, 150, "子", &Font24CN,WHITE, RED);
        
    Paint_DrawImage(gImage_70X70, 10, 70, 70, 70);
    
    Paint_DrawRectangle(100, 20, 160, 120, RED, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawLine(100, 20, 160, 120, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    Paint_DrawLine(100, 120, 160, 20, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    
    Paint_DrawCircle(50, 220, 25, BLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(80, 220, 25, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(110, 220, 25, RED, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(65, 245, 25, YELLOW, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(95, 245, 25, GREEN, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    
    delay(3000);
#endif
}

void Demo2(void)
{
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
  Paint_Clear(WHITE);

  Paint_DrawString_EN(35, 90, "Gesture test", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(10, 120, "Complete as prompted", &Font16, BLACK, WHITE);
  DEV_Delay_ms(500);
  Paint_ClearWindows(10,90,230,137,WHITE);
  Paint_DrawString_EN(105, 120, "UP", &Font24, BLUE, CYAN);
  while(touch.data.gestureID != SWIPE_UP){
    touch.available();
    DEV_Delay_ms(10);
  } 
  Paint_ClearWindows(105,120,160,155,WHITE);

  Paint_DrawString_EN(85, 120, "DOWN", &Font24, BLUE, CYAN);
  while(touch.data.gestureID != SWIPE_DOWN){
    touch.available();
    DEV_Delay_ms(10);
  } 
  Paint_ClearWindows(85,120,160,155,WHITE);

  Paint_DrawString_EN(85, 120, "LEFT", &Font24, BLUE, CYAN);
  while(touch.data.gestureID != SWIPE_LEFT){
    touch.available();
    DEV_Delay_ms(10);
  } 
  Paint_ClearWindows(85,120,160,155,WHITE);

  Paint_DrawString_EN(80, 120, "RIGHT", &Font24, BLUE, CYAN);
  while(touch.data.gestureID != SWIPE_RIGHT){
    touch.available();
    DEV_Delay_ms(10); 
  } 
  Paint_ClearWindows(80,120,165,155,WHITE);

  Paint_DrawString_EN(40, 120, "Long Press", &Font24, BLUE, CYAN);
  while(touch.data.gestureID != LONG_PRESS){
    touch.available();
    DEV_Delay_ms(10);
  } 
  Paint_ClearWindows(40,120,220,155,WHITE);

  Paint_DrawString_EN(15, 120, "Double Click", &Font24, BLUE, CYAN);
  while(touch.data.gestureID != DOUBLE_CLICK){   
    touch.available();
    Serial.println( touch.data.gestureID );
    DEV_Delay_ms(10);
  } 
  Paint_ClearWindows(15,120,240,155,WHITE);
  Paint_DrawPoint(120,140, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
}

void Demo3(void)
{

  if( touch.available()){
    Serial.print( "Data x " );
    Serial.print( touch.data.x );

    Serial.print( " Data y " );
    Serial.println( touch.data.y );
    Paint_DrawPoint(touch.data.x,touch.data.y, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
  }
  
  
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
