#include "DEV_Config.h"
#include "LCD_2inch.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <math.h>
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <time.h>


#define SRC_WRD_X_START 5
#define SRC_TITLE_Y_START 5
#define DEST_WRD_X_START 194
#define DEST_TITLE_Y_START 5


#define FONT_X 11
#define FONT_Y 16


void LCD_2IN_test(void)
{
    // Exception handling:ctrl + c
    signal(SIGINT, Handler_2IN_LCD);

    srand(time(NULL));  // Seed RNG
    
    /* Module Init */
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    /* LCD Init */
	printf("2 inch LCD demo...\r\n");
	LCD_2IN_Init();
	LCD_2IN_Clear(WHITE);
	LCD_SetBacklight(1023);
	
    UDOUBLE Imagesize = LCD_2IN_HEIGHT*LCD_2IN_WIDTH*2;
    UWORD *BlackImage;
    if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }
	
    /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage(BlackImage, LCD_2IN_WIDTH, LCD_2IN_HEIGHT, 90, WHITE, 16);
    Paint_Clear(WHITE);
	Paint_SetRotate(ROTATE_270);

    /* GUI */

    // Table Setup

    // Paint_Clear(WHITE); // Clear the Screen
    // Paint_DrawString_EN(SRC_WRD_X_START, SRC_TITLE_Y_START, "Source", &Font16, WHITE, BLACK);
    // Paint_DrawString_EN(DEST_WRD_X_START, DEST_TITLE_Y_START, "Destination", &Font16, WHITE, BLACK);

    // Paint_DrawLine(160, 0, 160, 240, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // Vertical Line
    // Paint_DrawLine(0, 21, 320, 21, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // Horizontal Line

    char * languages[] = {
        "English",
        "Spanish",
        "German",
        "Dutch",
        "Chinese"
    };

    int num_languages = sizeof(languages) / sizeof(languages[0]);
    int starting_y = 26;

    for (int iter = 0; iter < 5; iter++) { // You can make this `while(1)` if you want it to run forever
        int highlight_index = rand() % num_languages;

        Paint_Clear(WHITE);

        Paint_DrawString_EN(47, 5, "Source", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(194, 5, "Destination", &Font16, WHITE, BLACK);
        Paint_DrawLine(160, 0, 160, 240, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(0, 21, 320, 21, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        for (int i = 0; i < num_languages; i++) {
            UWORD bg_color = (i == highlight_index) ? WHITE : BLACK;
            UWORD fg_color = (i == highlight_index) ? BLUE : WHITE;

            Paint_DrawString_EN(5, starting_y + 15 * i, languages[i], &Font16, fg_color, bg_color);
            Paint_DrawString_EN(194, starting_y + 15 * i, languages[i], &Font16, fg_color, bg_color);
        }

        LCD_2IN_Display((UBYTE *)BlackImage);
        DEV_Delay_ms(1000);  // Delay 1 second between highlights
    }
	
    /* Module Exit */
    free(BlackImage);
    BlackImage = NULL;
	DEV_ModuleExit();
}



int main(int argc, char *argv[])
{
    if (argc != 2){
        printf("please input LCD type!\r\n");
        printf("example: sudo ./main -1.3\r\n");
        exit(1);
    }
    
    double size;
    sscanf(argv[1],"%lf",&size);
    size = (fabs(size));
    
    if(size<0.1||size>10){
        printf("error: bad size\r\n");
        exit(1);
    }
    else {
        printf("%.2lf inch LCD Module\r\n",size);
    }

    LCD_2IN_test();
    return 0;
}
