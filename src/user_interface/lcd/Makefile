CC = gcc
GPIO_LIB_MACRO = USE_DEV_LIB
CFLAGS += -g -O0 -Wall -D $(GPIO_LIB_MACRO)

# Add -lgpiod to link libgpiod (gpiod_chip_open, etc.)
LIBS = -llgpio -lm -lgpiod

TARGET = lcd_display

LCD_LIB = ./lib
LCD_LIB_CONFIG = $(LCD_LIB)/Config
LCD_LIB_EPD = $(LCD_LIB)/LCD
LCD_LIB_FONTS = $(LCD_LIB)/Fonts
LCD_LIB_GUI = $(LCD_LIB)/GUI

DIR_BIN = ./bin

OBJ_C = $(wildcard $(LCD_LIB_CONFIG)/*.c) \
        $(wildcard $(LCD_LIB_EPD)/*.c) \
        $(wildcard $(LCD_LIB_FONTS)/*.c) \
        $(wildcard $(LCD_LIB_GUI)/*.c) \
        main.c

OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

${TARGET}:${OBJ_O}
	$(CC) $(CFLAGS) $(OBJ_O) -o $@ $(LIBS)
    
${DIR_BIN}/main.o: main.c
	$(CC) $(CFLAGS) -I $(LCD_LIB_CONFIG) -I $(LCD_LIB_EPD) -I $(LCD_LIB_GUI) -I $(LCD_LIB_FONTS) -c $< -o $@

${DIR_BIN}/%.o:$(LCD_LIB_EPD)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ -I $(LCD_LIB_CONFIG)

${DIR_BIN}/%.o:$(LCD_LIB_FONTS)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@

${DIR_BIN}/%.o:$(LCD_LIB_GUI)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ -I $(LCD_LIB_CONFIG)  -I $(LCD_LIB_EPD)

${DIR_BIN}/%.o:$(LCD_LIB_CONFIG)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ $(LIBS)

clean :
	rm -f $(DIR_BIN)/*.* 
	rm -f $(TARGET)
