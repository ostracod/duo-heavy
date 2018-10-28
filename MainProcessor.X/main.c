
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

#define MEMORY_OE_TRIS TRISC7
#define MEMORY_OE_LAT LATC7

#define MEMORY_WE_TRIS TRISE2
#define MEMORY_WE_LAT LATE2

void delayMs(uint32_t milliseconds) {
    uint32_t index = milliseconds * 850;
    while (index > 0) {
        index -= 1;
    }
}

void sendDumbTerminalCharacter(uint8_t character) {
    ADDRESS_BUS_2_LAT = (ADDRESS_BUS_2_LAT & 0xE0) | 0x18;
    DATA_BUS_TRIS = 0x00;
    DATA_BUS_LAT = character;
    Nop(); Nop(); Nop(); Nop(); Nop();
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

void sendDumbTerminalNumber(uint8_t number) {
    sendDumbTerminalCharacter('0' + (number / 100));
    sendDumbTerminalCharacter('0' + ((number / 10) % 10));
    sendDumbTerminalCharacter('0' + (number % 10));
    sendDumbTerminalCharacter('\n');
}

void setMemoryAddress(uint32_t address) {
    ADDRESS_BUS_0_LAT = address & 0x000000FF;
    ADDRESS_BUS_1_LAT = (address & 0x0000FF00) >> 8;
    ADDRESS_BUS_2_LAT = (ADDRESS_BUS_2_LAT & 0xE0) | ((address & 0x001F0000) >> 16);
}

uint8_t readMemory(uint32_t address) {
    setMemoryAddress(address);
    DATA_BUS_TRIS = 0xFF;
    MEMORY_OE_LAT = 0;
    Nop(); Nop(); Nop(); Nop(); Nop();
    uint8_t output = DATA_BUS_PORT;
    MEMORY_OE_LAT = 1;
    return output;
}

void writeMemory(uint32_t address, uint8_t data) {
    setMemoryAddress(address);
    DATA_BUS_TRIS = 0x00;
    DATA_BUS_LAT = data;
    Nop(); Nop();
    MEMORY_WE_LAT = 0;
    Nop(); Nop(); Nop(); Nop(); Nop();
    MEMORY_WE_LAT = 1;
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
    MEMORY_OE_LAT = 1;
    MEMORY_WE_LAT = 1;
    
    DATA_BUS_TRIS = 0x00;
    ADDRESS_BUS_0_TRIS = 0x00;
    ADDRESS_BUS_1_TRIS = 0x00;
    ADDRESS_BUS_2_TRIS &= 0xE0;
    
    HOST_CLOCK_TRIS = 1;
    DEVICE_CLOCK_TRIS = 0;
    MEMORY_OE_TRIS = 0;
    MEMORY_WE_TRIS = 0;
    
    sendDumbTerminalCharacter(128); // Clear the display.
    
    sendDumbTerminalCharacter('F');
    sendDumbTerminalCharacter('\n');
    uint8_t tempDelay;
    tempDelay = 3;
    while (tempDelay > 0) {
        sendDumbTerminalNumber(tempDelay);
        delayMs(1000);
        tempDelay -= 1;
    }
    
    uint8_t tempChipNumber;
    uint32_t tempAddressMask;
    uint32_t tempAddress;
    uint8_t tempCount;
    
    /*
    tempChipNumber = 1;
    tempAddressMask = 0x00080000;
    while (tempChipNumber <= 3) {
        
        sendDumbTerminalNumber(tempChipNumber);
        // Erase the whole flash chip.
        writeMemory(0x00005555 | tempAddressMask, 0xAA);
        writeMemory(0x00002AAA | tempAddressMask, 0x55);
        writeMemory(0x00005555 | tempAddressMask, 0x80);
        writeMemory(0x00005555 | tempAddressMask, 0xAA);
        writeMemory(0x00002AAA | tempAddressMask, 0x55);
        writeMemory(0x00005555 | tempAddressMask, 0x10);
        delayMs(200);
        
        tempAddress = 1;
        tempCount = 0;
        while (tempCount <= 18) {
            // Program a byte.
            writeMemory(0x00005555 | tempAddressMask, 0xAA);
            writeMemory(0x00002AAA | tempAddressMask, 0x55);
            writeMemory(0x00005555 | tempAddressMask, 0xA0);
            writeMemory(tempAddress | tempAddressMask, (tempCount + 1) * 5 + tempChipNumber);
            delayMs(1);
            tempAddress <<= 1;
            tempCount += 1;
        }
        
        tempChipNumber += 1;
        tempAddressMask += 0x00080000;
    }
    */
    
    tempChipNumber = 1;
    tempAddressMask = 0x00080000;
    while (tempChipNumber <= 3) {
        
        sendDumbTerminalCharacter('\n');
        sendDumbTerminalNumber(tempChipNumber);
        
        tempAddress = 1;
        tempCount = 0;
        while (tempCount <= 18) {
            uint8_t tempValue = readMemory(tempAddress | tempAddressMask);
            sendDumbTerminalNumber(tempValue);
            tempAddress <<= 1;
            tempCount += 1;
        }
        
        tempDelay = 12;
        while (tempDelay > 0) {
            delayMs(1000);
            tempDelay -= 1;
        }
        
        tempChipNumber += 1;
        tempAddressMask += 0x00080000;
    }
    
    while (1) {
        delayMs(10);
    }
    
    return 0;
}


