
#include <stdint.h>
#include <xc.h>

#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config BOREN = SBORDIS
#pragma config WDTE = OFF

#define DATA_BUS_ANSEL ANSELD
#define DATA_BUS_TRIS TRISD
#define DATA_BUS_LAT LATD
#define DATA_BUS_PORT PORTD

#define ADDRESS_BUS_0_TRIS TRISA
#define ADDRESS_BUS_0_LAT LATA

#define ADDRESS_BUS_1_TRIS TRISB
#define ADDRESS_BUS_1_LAT LATB

// Be careful: Only the lower 5 bits are in the address bus.
#define ADDRESS_BUS_2_TRIS TRISC
#define ADDRESS_BUS_2_LAT LATC

#define HOST_CLOCK_ANSEL ANSELEbits.ANSELE0
#define HOST_CLOCK_TRIS TRISE0
#define HOST_CLOCK_PORT PORTEbits.RE0

#define DEVICE_CLOCK_TRIS TRISE1
#define DEVICE_CLOCK_LAT LATE1

void delayMs(uint32_t milliseconds) {
    uint32_t index = milliseconds * 850;
    while (index > 0) {
        index -= 1;
    }
}

void sendDumbTerminalCharacter(uint8_t character) {
    DATA_BUS_LAT = character;
    DEVICE_CLOCK_LAT = 0;
    while (1) {
        if (!HOST_CLOCK_PORT) {
            break;
        }
    }
    DEVICE_CLOCK_LAT = 1;
    while (1) {
        if (HOST_CLOCK_PORT) {
            break;
        }
    }
}

int main() {
    
    delayMs(100); // Let everything stabilize a little.
    
    // Disable ADC.
    ADCON0bits.ADON = 0;
    CM1CON0bits.C1EN = 0;
    CM2CON0bits.C2EN = 0;
    DATA_BUS_ANSEL = 0x00;
    HOST_CLOCK_ANSEL = 0;
    
    ADDRESS_BUS_0_LAT = 0x00;
    ADDRESS_BUS_1_LAT = 0x00;
    ADDRESS_BUS_2_LAT = (ADDRESS_BUS_2_LAT & 0xE0) | 0x18;
    
    DEVICE_CLOCK_LAT = 1;
    
    DATA_BUS_TRIS = 0x00;
    ADDRESS_BUS_0_TRIS = 0x00;
    ADDRESS_BUS_1_TRIS = 0x00;
    ADDRESS_BUS_2_TRIS &= 0xE0;
    
    HOST_CLOCK_TRIS = 1;
    DEVICE_CLOCK_TRIS = 0;
    
    sendDumbTerminalCharacter(128); // Clear the display.
    
    while (1) {
        sendDumbTerminalCharacter('A');
        delayMs(50);
        sendDumbTerminalCharacter('B');
        delayMs(50);
        sendDumbTerminalCharacter('C');
        delayMs(50);
        sendDumbTerminalCharacter('D');
        delayMs(50);
    }
    
    return 0;
}


