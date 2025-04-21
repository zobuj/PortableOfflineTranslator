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

    char * languages[] = {
        "English",
        "Spanish",
        "German",
        "Dutch",
        "Chinese"
    };

    char *screen_table[][2] = {
        {"Source", "Destination"},
        {"English", "English"},
        {"Spanish", "Spanish"},
        {"German",  "German"},
        {"Dutch",   "Dutch"},
        {"Chinese", "Chinese"}
    };
    
    int num_rows = sizeof(screen_table) / sizeof(screen_table[0]);
    int y_start = 0;
    int row_height = 26;
    int cell_width = 160;
    int padding = 5;
    
    int prev_highlighted = 1;

    Paint_Clear(WHITE); // Clear the screen
    
    for (int i = 0; i < num_rows; i++) {
        int y = y_start + row_height * i;

        // Added a start condition here to highlight the first languages
        if(i == prev_highlighted) {
            // Draw left cell background
            Paint_DrawRectangle(0, y, cell_width, y + row_height, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            // Draw right cell background
            Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

            // Draw text with 5px padding
            Paint_DrawString_EN(0 + padding, y + padding, screen_table[i][0], &Font16, BLUE, WHITE);
            Paint_DrawString_EN(cell_width + padding, y + padding, screen_table[i][1], &Font16, BLUE, WHITE);
        } else {
            // Draw left cell background 
            Paint_DrawRectangle(0, y, cell_width, y + row_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            // Draw right cell background
            Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

            // Draw text with 5px padding
            Paint_DrawString_EN(0 + padding, y + padding, screen_table[i][0], &Font16, WHITE, BLACK);
            Paint_DrawString_EN(cell_width + padding, y + padding, screen_table[i][1], &Font16, WHITE, BLACK);
        }

    }

    Paint_DrawLine(160, 0, 160, 240, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // Vertical Line
    Paint_DrawLine(0, 26, 320, 26, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // Horizontal Line

    LCD_2IN_Display((UBYTE *)BlackImage);
    DEV_Delay_ms(1000);  // Delay 1 second between highlights


    for (int iter = 0; iter < 5; iter++) { // You can make this `while(1)` if you want it to run forever
        int highlight_index = rand() % num_rows;

        if (highlight_index == prev_highlighted || highlight_index == 0) continue;

        printf("Highlighted index: %d, prev highlighted: %d\n", highlight_index, prev_highlighted);

        int y = y_start + row_height * prev_highlighted;

        // CLEAR BOX

        // Draw left cell background 
        Paint_DrawRectangle(0, y, cell_width, y + row_height, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        // Draw right cell background
        Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // DRAW BORDER

        // Draw left cell background 
        Paint_DrawRectangle(0, y, cell_width, y + row_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        // Draw right cell background
        Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

        // DRAW TEXT

        // Draw text with 5px padding
        Paint_DrawString_EN(0 + padding, y + padding, screen_table[prev_highlighted][0], &Font16, WHITE, BLACK);
        Paint_DrawString_EN(cell_width + padding, y + padding, screen_table[prev_highlighted][1], &Font16, WHITE, BLACK);


        y = y_start + row_height * highlight_index;

        // Draw left cell background
        Paint_DrawRectangle(0, y, cell_width, y + row_height, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        // Draw right cell background
        Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // Draw text with 5px padding
        Paint_DrawString_EN(0 + padding, y + padding, screen_table[highlight_index][0], &Font16, BLUE, WHITE);
        Paint_DrawString_EN(cell_width + padding, y + padding, screen_table[highlight_index][1], &Font16, BLUE, WHITE);

        prev_highlighted = highlight_index;

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
