
CC = gcc
GPIO_LIB_MACRO = USE_DEV_LIB
CFLAGS += -g -O0 -Wall -D $(GPIO_LIB_MACRO)
GPIO_LIB = -llgpio -lm

TARGET = main

LCD_LIB = ./LCD_Module_RPI_code/RaspberryPi/c/lib
LCD_LIB_CONFIG = $(LCD_LIB)/Config
LCD_LIB_EPD = $(LCD_LIB)/LCD
LCD_LIB_FONTS = $(LCD_LIB)/Fonts
LCD_LIB_GUI = $(LCD_LIB)/GUI

OBJ_C = $(wildcard $(LCD_LIB_CONFIG)/*.c, $(LCD_LIB_EPD)/*.c, $(LCD_LIB_FONTS)/*.c, $(LCD_LIB_GUI)/*.c)
OBJ_O