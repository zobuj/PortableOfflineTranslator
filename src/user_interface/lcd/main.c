#include "DEV_Config.h"
#include "LCD_2inch.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <math.h>
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define SCREEN_ROWS 9  // 1 header + 8 visible rows

void initTermios(int echo) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ICANON;
    if (echo) tty.c_lflag |= ECHO;
    else tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void resetTermios() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ICANON;
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

char getch() {
    char ch;
    read(STDIN_FILENO, &ch, 1);
    return ch;
}

void handle_sigusr1(int sig) {
    if (sig == SIGUSR1) {
        printf("Received SIGUSR1 — forwarding data...\n");
    }
}


void LCD_2IN_test(void)
{
    // Exception handling:ctrl + c
    signal(SIGINT, Handler_2IN_LCD);

    signal(SIGUSR1, handle_sigusr1);
    
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
        "Chinese",
        "Korean",
        "French",
        "Italian",
        "Hindi",
        "Greek",
        "Thai"
    };

    char *screen_table[SCREEN_ROWS][2];

    // Fill in header row
    screen_table[0][0] = "Source";
    screen_table[0][1] = "Destination";

    // Fill in initial view
    for (int i = 1; i < SCREEN_ROWS; i++) {
        screen_table[i][0] = languages[i - 1];     // Source column
        screen_table[i][1] = languages[i - 1];     // Destination column
    }
    
    int num_languages = sizeof(languages) / sizeof(languages[0]);
    int num_rows = sizeof(screen_table) / sizeof(screen_table[0]);
    int y_start = 0;
    int row_height = 26;
    int cell_width = 160;
    int padding = 5;
    
    int highlight_index_src = 1; // Skip header row

    int highlight_index_dest = 1; // Skip header row

    int scroll_offset_src = 0;
    int scroll_offset_dest = 0;

    Paint_Clear(WHITE); // Clear the screen
    
    for (int i = 0; i < num_rows; i++) {
        int y = y_start + row_height * i;

        // Added a start condition here to highlight the first languages
        if(i == 1) {
            // Draw left cell background
            Paint_DrawRectangle(0, y, cell_width, y + row_height, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            // Draw right cell background
            Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);

            // Draw text with 5px padding
            Paint_DrawString_EN(0 + padding, y + padding, screen_table[i][0], &Font16, BLUE, WHITE);
            Paint_DrawString_EN(cell_width + padding, y + padding, screen_table[i][1], &Font16, RED, WHITE);
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
    
    initTermios(0); // No echo, non-canonical input
    
    while (1) {
        char ch = getch(); // wait for input
        if (ch == 'q') break; // quit
    
        if (ch == 'w') {
            if (highlight_index_src > 1) {
                highlight_index_src--; // Move up within screen
            } else if (scroll_offset_src > 0) {
                scroll_offset_src--;   // Scroll up
            }
        } else if (ch == 's') {
            // Only scroll if we're already at the bottom of the screen
            if (highlight_index_src == SCREEN_ROWS - 1 && scroll_offset_src + SCREEN_ROWS - 1 < num_languages) {
                scroll_offset_src++;   // Scroll down
            } else if (highlight_index_src < SCREEN_ROWS - 1 && 
                       scroll_offset_src + highlight_index_src < num_languages - 1) {
                highlight_index_src++; // Move down within screen
            }
        }        
    
        if (ch == 'i') {
            if (highlight_index_dest > 1) {
                highlight_index_dest--; // Move up within screen
            } else if (scroll_offset_dest > 0) {
                scroll_offset_dest--;   // Scroll up
            }
        } else if (ch == 'k') {
            // Only scroll if we're already at the bottom of the screen
            if (highlight_index_dest == SCREEN_ROWS - 1 && scroll_offset_dest + SCREEN_ROWS - 1 < num_languages) {
                scroll_offset_dest++;   // Scroll down
            } else if (highlight_index_dest < SCREEN_ROWS - 1 && 
                       scroll_offset_dest + highlight_index_dest < num_languages - 1) {
                highlight_index_dest++; // Move down within screen
            }
        }      
        
        // Redraw source (if changed)
        for (int i = 1; i < SCREEN_ROWS; i++) {
            int lang_index = scroll_offset_src + i - 1;
            if (lang_index < num_languages) {
                screen_table[i][0] = languages[lang_index];
            } else {
                screen_table[i][0] = "";
            }
        }

        for(int i = 0; i < SCREEN_ROWS; i++) {
            int y = row_height * i;

            UWORD bg_src = (i == highlight_index_src) ? BLUE : WHITE;
            UWORD fg_src = (i == highlight_index_src) ? WHITE : BLACK;

            // Clear Cell 
            Paint_DrawRectangle(0, y, cell_width, y + row_height, bg_src, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            // Draw Cell Border
            Paint_DrawRectangle(0, y, cell_width, y + row_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            Paint_DrawString_EN(padding, y + padding, screen_table[i][0], &Font16, bg_src, fg_src);

        }

        // Redraw destination (if changed)
        for (int i = 1; i < SCREEN_ROWS; i++) {
            int lang_index = scroll_offset_dest + i - 1;
            if (lang_index < num_languages) {
                screen_table[i][1] = languages[lang_index];
            } else {
                screen_table[i][1] = "";
            }
        }

        for(int i = 0; i < SCREEN_ROWS; i++) {
            int y = row_height * i;

            UWORD bg_dest = (i == highlight_index_dest) ? RED : WHITE;
            UWORD fg_dest = (i == highlight_index_dest) ? WHITE : BLACK;

            // Clear Cell 
            Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, bg_dest, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            // Draw Cell Border
            Paint_DrawRectangle(cell_width, y, 2 * cell_width, y + row_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            Paint_DrawString_EN(cell_width + padding, y + padding, screen_table[i][1], &Font16, bg_dest, fg_dest);

        }

        LCD_2IN_Display((UBYTE *)BlackImage);
        DEV_Delay_ms(1000);  // Delay 1 second between highlights
        printf("Currently Selected Languages: %s (Source) --> %s (Destination)\n",
            screen_table[highlight_index_src][0],
            screen_table[highlight_index_dest][1]);

        // Write selected languages to config.txt
        FILE *config_file = fopen("config.txt", "w");  // You can change the path if needed
        if (config_file != NULL) {
            fprintf(config_file, "source=%s\ndestination=%s\n",
                screen_table[highlight_index_src][0],
                screen_table[highlight_index_dest][1]);
            fclose(config_file);
        } else {
            perror("Failed to open config.txt for writing");
        }
    }
    
    resetTermios();

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
