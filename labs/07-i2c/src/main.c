/***********************************************************************
 * 
 * The I2C (TWI) bus scanner tests all addresses and detects devices
 * that are connected to the SDA and SCL signals.
 * 
 * ATmega328P (Arduino Uno), 16 MHz, PlatformIO
 *
 * Copyright (c) 2023 Tomas Fryza
 * Dept. of Radio Electronics, Brno University of Technology, Czechia
 * This work is licensed under the terms of the MIT license.
 * 
 **********************************************************************/


/* Defines -----------------------------------------------------------*/
#ifndef F_CPU
# define F_CPU 16000000  // CPU frequency in Hz required for UART_BAUD_SELECT
#endif


/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include "timer.h"          // Timer library for AVR-GCC
#include <twi.h>            // I2C/TWI library for AVR-GCC
#include <uart.h>           // Peter Fleury's UART library
#include <stdlib.h>         // C library. Needed for number conversions
#include <oled.h>

/* Global variables --------------------------------------------------*/
// Declaration of "dht12" variable with structure "DHT_values_structure"
struct DHT_values_structure {
   uint8_t hum_int;
   uint8_t hum_dec;
   uint8_t temp_int;
   uint8_t temp_dec;
   uint8_t checksum;
} dht12;

// Flag for printing new data from sensor
volatile uint8_t new_sensor_data = 0;


// Slave and internal addresses of temperature/humidity sensor DHT12
#define SENSOR_ADR 0x5c
#define SENSOR_HUM_MEM 0
#define SENSOR_TEMP_MEM 2
#define SENSOR_CHECKSUM 4

/* Function definitions ----------------------------------------------*/
/**********************************************************************
 * Function: Main function where the program execution begins
 * Purpose:  Call function to test all I2C (TWI) combinations and send
 *           detected devices to UART.
 * Returns:  none
 * 
 * Some known devices:
 *     0x3c - OLED display
 *     0x57 - EEPROM
 *     0x5c - Temp+Humid
 *     0x68 - RTC
 *     0x68 - GY521
 *     0x76 - BME280
 *
 **********************************************************************/
int main(void)
{
    char string[2];  // For converting numbers by itoa()

    oled_init(OLED_DISP_ON);
    oled_clrscr();
    oled_charMode(DOUBLESIZE);
    oled_puts("OLED disp.");
    oled_charMode(NORMALSIZE);

    twi_init();

    // Initialize USART to asynchronous, 8-N-1, 115200 Bd
    uart_init(UART_BAUD_SELECT(115200, F_CPU));
    sei();  // Needed for UART

    // Test if sensor is ready
    if (twi_test_address(SENSOR_ADR) == 0)
        uart_puts("I2C sensor detected\r\n");
    else {
        uart_puts("[ERROR] I2C device not detected\r\n");
        while (1);
    }

    // Timer1
    TIM1_OVF_1SEC
    TIM1_OVF_ENABLE


    uart_puts("Scanning I2C... ");
    for (uint8_t sla = 8; sla < 120; sla++) {
        if (twi_test_address(sla) == 0) {  // ACK from Slave
            uart_puts("\r\n");
            itoa(sla, string, 16);
            uart_puts(string);
        }
    }

    
    // oled_gotoxy(x, y)
    oled_gotoxy(0, 2);
    oled_puts("128x64, SHH1106");

    // oled_drawLine(x1, y1, x2, y2, color)
    oled_drawLine(0, 25, 120, 25, WHITE);

    oled_gotoxy(0, 4);
    oled_puts("BPC-DE2, Brno");

    // Copy buffer to display RAM
    oled_display();
    oled_clrscr();

    oled_gotoxy(0, 3);
    oled_puts("Temperature: ");
    oled_gotoxy(15, 3);
    oled_puts(".");
    oled_gotoxy(17, 3);
    oled_puts(" °C");
    
    oled_gotoxy(0, 4);
    oled_puts("Humidity: ");
    oled_gotoxy(12, 4);
    oled_puts(".");
    oled_gotoxy(14, 4);
    oled_puts(" %");

    while (1) {
        if (new_sensor_data == 1) {
            // oled_clrscr();
            itoa(dht12.temp_int, string, 10);
            uart_puts(string);
            uart_puts(".");
            // oled_puts("Temperature: ");
            oled_gotoxy(13, 3);
            oled_puts(string);
            // oled_puts(".");

            itoa(dht12.temp_dec, string, 10);
            uart_puts(string);
            uart_puts(" °C\t");
            oled_gotoxy(16, 3);
            oled_puts(string);
            // oled_puts(" °C");


            itoa(dht12.hum_int, string, 10);
            // oled_puts("Humidity: ");
            oled_gotoxy(10, 4);
            uart_puts(string);
            uart_puts(".");
            
            oled_puts(string);
            // oled_puts(".");
            

            itoa(dht12.hum_dec, string, 10);
            uart_puts(string);
            uart_puts(" %\r\n");
            oled_gotoxy(13, 4);
            oled_puts(string);
            // oled_puts(" %");

            // Copy buffer to display RAM
            oled_display();
            // Do not print it again and wait for the new data
            new_sensor_data = 0;
        }
    }

    return 0;
}


/* Interrupt service routines ----------------------------------------*/
ISR(TIMER1_OVF_vect)
{
    

    // Test ACK from sensor
    twi_start();
    if (twi_write((SENSOR_ADR<<1) | TWI_WRITE) == 0) {
        // Set internal memory location
        twi_write(SENSOR_HUM_MEM);
        twi_stop();
        // Read data from internal memory
        twi_start();
        twi_write((SENSOR_ADR<<1) | TWI_READ);
        dht12.hum_int = twi_read(TWI_ACK);
        dht12.hum_dec = twi_read(TWI_ACK);
        dht12.temp_int = twi_read(TWI_ACK);
        dht12.temp_dec = twi_read(TWI_NACK);
        
        new_sensor_data = 1;
    }

    
}