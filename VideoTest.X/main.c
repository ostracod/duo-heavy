
#define _XTAL_FREQ 12000000

#include <stdint.h>
#include <xc.h>
#include <math.h>

#pragma config FEXTOSC = HS
#pragma config RSTOSC = EXTOSC_4PLL
#pragma config BOREN = SBORDIS
#pragma config WDTE = OFF

#define NOP_10 Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop()

#define VIDEO_FORMAT_NTSC 0
#define VIDEO_FORMAT_VGA 1

// 31.8 microseconds.
#define INITIAL_NTSC_TIMER_VALUE ((uint32_t)190)
// Also 31.8 microseconds.
#define INITIAL_VGA_TIMER_VALUE ((uint32_t)190)

#define NTSC_PULSE_TYPE_NONE 0
#define NTSC_PULSE_TYPE_HORIZONTAL_SYNC 1
#define NTSC_PULSE_TYPE_EQUALIZING 2
#define NTSC_PULSE_TYPE_LONG 3

#define VIDEO_BUFFER_WIDTH ((240 / 8) + 1)
#define VIDEO_BUFFER_HEIGHT 240

// Video format to output.
uint8_t videoFormat = VIDEO_FORMAT_VGA;
// Pixels to display on the monitor.
uint8_t videoData[VIDEO_BUFFER_WIDTH * VIDEO_BUFFER_HEIGHT];
// Number of "half line" intervals since the beginning of field 1.
// Field 1 begins with pre-equalizing pulses.
uint16_t ntscProgress = 0;
// Type of pulse to output during the next interrupt.
uint8_t nextNtscPulseType = NTSC_PULSE_TYPE_NONE;
// Number of remaining long pulses to output.
uint8_t remainingNtscLongPulseCount = 0;
// VGA line number to output.
// This includes lines above and below the visible area.
uint16_t vgaLineNumber = 0;
// Value VSYNC should adopt during the next interrupt.
uint8_t nextVgaVerticalSyncValue = 0;
// Boolean indicating whether to start dumping pixels during the next interrupt.
uint8_t nextShouldDumpVideoData = 0;
// Index of pixels to display during the next interrupt.
uint16_t nextVideoDataIndex = 0;

void delayMs(uint32_t milliseconds) {
    uint32_t index = milliseconds * 640;
    while (index > 0) {
        index -= 1;
    }
}

// When amount is 4, the delay is 7.9 microseconds.
// When amount is 5, the delay is 9.0 microseconds.
// When amount is 6, the delay is 10.2 microseconds.
// When amount is 7, the delay is 11.4 microseconds.
// delay ~= 3 + amount * 1.2
void delayFast(uint32_t amount) {
    uint32_t index = amount;
    while (index > 0) {
        index -= 1;
    }
}

void drawSinglePixel(uint8_t posX, uint8_t posY, uint8_t value) {
    uint16_t index = (posX >> 3) + posY * VIDEO_BUFFER_WIDTH;
    if (value) {
        videoData[index] |= 0x80 >> (posX & 0x0007);
    } else {
        videoData[index] &= ~(0x80 >> (posX & 0x0007));
    }
}

void drawTestRectangle(
    uint8_t posX,
    uint8_t posY,
    uint8_t width,
    uint8_t height,
    uint8_t fillColor,
    uint8_t borderColor
) {
    uint8_t tempOffsetY = 0;
    while (tempOffsetY < height) {
        uint8_t tempOffsetX = 0;
        while (tempOffsetX < width) {
            uint8_t tempValue;
            if (tempOffsetX <= 0 || tempOffsetX >= width - 1
                    || tempOffsetY <= 0 || tempOffsetY >= height - 1) {
                tempValue = borderColor;
            } else {
                if (fillColor == 2) {
                    tempValue = (tempOffsetX & 0x01) ^ (tempOffsetY & 0x01);
                } else {
                    tempValue = fillColor;
                }
            }
            drawSinglePixel(posX + tempOffsetX, posY + tempOffsetY, tempValue);
            tempOffsetX += 1;
        }
        tempOffsetY += 1;
    }
}

void testVideo() {
    
    // Disable peripheral pin select lock.
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x00;

    RC5PPS = 0x1F; // Set up SDO.
    
    // Enable peripheral pin select lock.
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x01;
    
    SPI1CON1 = 0x00;
    SPI1CON2 = 0x02;
    if (videoFormat == VIDEO_FORMAT_NTSC) {
        SPI1BAUD = 0x03; // 6 MHz.
    } else if (videoFormat == VIDEO_FORMAT_VGA) {
        SPI1BAUD = 0x01; // 12 MHz.
    }
    SPI1CLK = 0x00;
    
    SPI1CON0 = 0x83; // Enable SPI.
    
    // Disable priority lock.
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 0;
    
    // Prioritize DMA over everything else.
    DMA1PR = 0;
    ISRPR = 1;
    MAINPR = 2;
    
    // Enable priority lock.
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 1;
    
    uint16_t posY = 0;
    while (posY < 240) {
        uint16_t posX = 0;
        while (posX < VIDEO_BUFFER_WIDTH) {
            uint16_t index = posX + posY * VIDEO_BUFFER_WIDTH;
            videoData[index] = 0;
            //if (posX < VIDEO_BUFFER_WIDTH - 1) {
                //videoData[index] = posY;
            //} else {
                //videoData[index] = 0;
            //}
            posX += 1;
        }
        posY += 1;
    }
    
    DMA1DSA = (uint32_t)(&SPI1TXB); // Set DMA destination pointer.
    
    DMA1CON1bits.SMR = 0; // Read from GPR.
    DMA1CON1bits.SMODE = 1; // Increment after access.
    DMA1CON1bits.DMODE = 0; // Do not change after access.
    
    // Set DMA data size.
    DMA1SSZ = VIDEO_BUFFER_WIDTH;
    DMA1DSZ = 1;
    
    DMA1CON1bits.SSTP = 1; // Stop DMA after reload.
    DMA1CON1bits.DSTP = 0; // Continue DMA after reload.
    
    DMA1SIRQ = 21; // DMA start interrupt is SPI.
    
    DMA1CON0bits.EN = 1; // Enable DMA.
    
    // Use an 8 bit timer.
    T0CON0bits.MD16 = 0;
    // Use F_osc / 4. In our case, F_osc / 4 = 12 MHz.
    T0CON1bits.CS = 2;
    // Use scale of 1/2.
    T0CON1bits.CKPS = 1;
    // Set timer period.
    if (videoFormat == VIDEO_FORMAT_NTSC) {
        TMR0H = INITIAL_NTSC_TIMER_VALUE;
    } else if (videoFormat == VIDEO_FORMAT_VGA) {
        TMR0H = INITIAL_VGA_TIMER_VALUE;
    }
    // Reset timer counter.
    TMR0L = 0;
    
    TMR0IE = 1; // Enable timer interrupts.
    GIE = 1; // Enable global interrupts.
    T0CON0bits.EN = 1; // Enable the timer.
    
    // Some code to test graphics.
    
    float angle = 0;
    float radius = 0;
    while (radius < 100) {
        uint16_t tempPosX = (uint16_t)(120 + cos(angle) * radius);
        uint16_t tempPosY = (uint16_t)(120 + sin(angle) * radius);
        drawSinglePixel(tempPosX + 0, tempPosY + 0, 1);
        drawSinglePixel(tempPosX + 1, tempPosY + 0, 1);
        drawSinglePixel(tempPosX + 2, tempPosY + 0, 1);
        drawSinglePixel(tempPosX + 0, tempPosY + 1, 1);
        drawSinglePixel(tempPosX + 1, tempPosY + 1, 1);
        drawSinglePixel(tempPosX + 2, tempPosY + 1, 1);
        drawSinglePixel(tempPosX + 0, tempPosY + 2, 1);
        drawSinglePixel(tempPosX + 1, tempPosY + 2, 1);
        drawSinglePixel(tempPosX + 2, tempPosY + 2, 1);
        angle += 0.015;
        radius += 0.025;
    }
    
    /*
    while (1) {
        uint16_t posX = 0;
        while (posX < VIDEO_BUFFER_WIDTH - 1) {
            videoData[posX] = 0;
            posX += 1;
        }
        uint8_t tempCount = 0;
        while (tempCount < 3) {
            uint8_t posX = (uint8_t)((uint16_t)rand() % (uint16_t)238);
            drawSinglePixel(posX + 1, 0, 1);
            drawSinglePixel(posX, 1, 1);
            drawSinglePixel(posX + 2, 1, 1);
            drawSinglePixel(posX + 1, 2, 1);
            tempCount += 1;
        }
        uint16_t posY = 239;
        while (posY > 0) {
            uint16_t tempIndex1 = posY * VIDEO_BUFFER_WIDTH;
            uint16_t tempIndex2 = (posY - 1) * VIDEO_BUFFER_WIDTH;
            uint16_t posX = 0;
            while (posX < VIDEO_BUFFER_WIDTH - 1) {
                videoData[tempIndex1] = videoData[tempIndex2];
                tempIndex1 += 1;
                tempIndex2 += 1;
                posX += 1;
            }
            posY -= 1;
        }
    }
    */
    
    /*
    while (1) {
        uint8_t width = 10 + (uint8_t)((uint16_t)rand() % (uint16_t)50);
        uint8_t height = 10 + (uint8_t)((uint16_t)rand() % (uint16_t)50);
        uint8_t posX = (uint8_t)((uint16_t)rand() % (uint16_t)(240 - width));
        uint8_t posY = (uint8_t)((uint16_t)rand() % (uint16_t)(240 - height));
        uint8_t fillColor = (uint8_t)rand() % (uint8_t)3;
        uint8_t borderColor;
        if (fillColor == 0) {
            borderColor = 1;
        }
        if (fillColor == 1) {
            borderColor = 0;
        }
        if (fillColor == 2) {
            borderColor = (uint8_t)rand() % (uint8_t)2;
        }
        drawTestRectangle(posX, posY, width, height, fillColor, borderColor);
    }
    */
}

void advanceNtscProgress() {
    ntscProgress += 1;
    if (ntscProgress >= 1050) {
        ntscProgress = 0;
    }
    uint8_t tempFieldNumber;
    uint16_t tempFieldProgress;
    if (ntscProgress < 525) {
        tempFieldNumber = 0;
        tempFieldProgress = ntscProgress;
    } else {
        tempFieldNumber = 1;
        tempFieldProgress = ntscProgress - 525;
    }
    if (tempFieldProgress >= 18) {
        nextShouldDumpVideoData = 0;
        if ((tempFieldProgress & 0x0001) == tempFieldNumber) {
            nextNtscPulseType = NTSC_PULSE_TYPE_HORIZONTAL_SYNC;
            if (tempFieldProgress >= 43 && tempFieldProgress < 524) {
                nextShouldDumpVideoData = 1;
                nextVideoDataIndex = VIDEO_BUFFER_WIDTH * ((tempFieldProgress - 44) >> 1);
            }
        } else {
            nextNtscPulseType = NTSC_PULSE_TYPE_NONE;
        }
    } else if (tempFieldProgress >= 6 && tempFieldProgress < 12) {
        nextNtscPulseType = NTSC_PULSE_TYPE_LONG;
        remainingNtscLongPulseCount = 6;
    } else {
        nextNtscPulseType = NTSC_PULSE_TYPE_EQUALIZING;
    }
}

void advanceVgaProgress() {
    vgaLineNumber += 1;
    if (vgaLineNumber >= 525) {
        vgaLineNumber = 0;
    }
    if (vgaLineNumber < 2) {
        nextVgaVerticalSyncValue = 0;
    } else {
        nextVgaVerticalSyncValue = 1;
        nextShouldDumpVideoData = (vgaLineNumber >= 35 && vgaLineNumber < 515);
        nextVideoDataIndex = VIDEO_BUFFER_WIDTH * ((vgaLineNumber - 35) >> 1);
    }
}

// Baseline with 10 NOP: 1.0 microseconds
// 20 NOP: 1.8 microseconds
// 30 NOP: 2.7 microseconds
// 40 NOP: 3.5 microseconds
// delay ~= amount * 0.08333 (This is about 1/12, which makes sense)

// Service TIMER0 interrupts.
void __interrupt(high_priority, irq(31)) serviceInterrupt(void) {
    
    // This is some REAL Tom-foolery.
    // The idea is that the interrupt may occur during a 1 or 2 cycle instruction.
    // We want to compensate for the timing inconsistency.
    Nop();
    uint8_t tempTimerValue = TMR0L;
    if (tempTimerValue > 3) {
        Nop();
    } else {
        Nop(); Nop();
    }
    
    if (videoFormat == VIDEO_FORMAT_VGA) {
        // Horizontal sync pulse. (3.8 microseconds)
        LATC6 = 0;
        NOP_10; NOP_10; NOP_10; NOP_10;
        Nop(); Nop(); Nop(); Nop();
        LATC6 = 1;
        LATC7 = nextVgaVerticalSyncValue;
        
        // Back porch. (1.9 microseconds)
        NOP_10;
        
        if (nextShouldDumpVideoData) {
            // Pixel data.
            DMA1CON0bits.EN = 0; // Disable DMA.
            DMA1SSA = (uint16_t)videoData + nextVideoDataIndex; // Set DMA source pointer.
            DMA1CON0bits.EN = 1; // Enable DMA.
            DMA1CON0bits.DMA1SIRQEN = 1; // Send DMA data over SPI once.
        }
        
        advanceVgaProgress();
        
    } else if (videoFormat == VIDEO_FORMAT_NTSC) {
        if (nextNtscPulseType == NTSC_PULSE_TYPE_NONE) {
            advanceNtscProgress();
        } else {
            // All of the other pulse types begin with 0 volts.
            // We want them to begin with consistent timing.
            LATC2 = 0;
            
            if (nextNtscPulseType == NTSC_PULSE_TYPE_EQUALIZING) {
                
                // Equalizing pulse. (2.3 microseconds)
                NOP_10;
                Nop(); Nop(); Nop(); Nop(); Nop();
                Nop();
                LATC2 = 1;
                
                advanceNtscProgress();
                
            } else if (nextNtscPulseType == NTSC_PULSE_TYPE_HORIZONTAL_SYNC) {
                
                // Horizontal sync pulse. (4.7 microseconds)
                NOP_10; NOP_10; NOP_10;
                Nop(); Nop(); Nop(); Nop(); Nop();
                Nop(); Nop(); Nop(); Nop();
                LATC2 = 1;
                
                // Back porch. (4.7 microseconds again)
                delayFast(1);
                Nop(); Nop(); Nop(); Nop();
                
                if (nextShouldDumpVideoData) {
                    // Pixel data.
                    DMA1CON0bits.EN = 0; // Disable DMA.
                    DMA1SSA = (uint16_t)videoData + nextVideoDataIndex; // Set DMA source pointer.
                    DMA1CON0bits.EN = 1; // Enable DMA.
                    DMA1CON0bits.DMA1SIRQEN = 1; // Send DMA data over SPI once.
                }
                
                advanceNtscProgress();
                
            } else if (nextNtscPulseType == NTSC_PULSE_TYPE_LONG) {
                
                // Long pulse. (27.1 microseconds)
                delayFast(18);
                NOP_10;
                Nop(); Nop(); Nop();
                LATC2 = 1;
                
                // We don't have a lot of time to advance progress after the pulse,
                // so we have to do it really fast.
                ntscProgress += 1;
                remainingNtscLongPulseCount -= 1;
                if (remainingNtscLongPulseCount <= 0) {
                    nextNtscPulseType = NTSC_PULSE_TYPE_EQUALIZING;
                }
            }
        }
    }
    
    TMR0IF = 0;
}

int main() {
    // Test LED.
    TRISC0 = 0;
    // Composite video syncing pulses.
    TRISC2 = 0;
    // Video data.
    TRISC5 = 0;
    // Horizontal VGA syncing pulses.
    TRISC6 = 0;
    // Vertical VGA syncing pulses.
    TRISC7 = 0;
    LATC0 = 0;
    LATC2 = 0;
    LATC5 = 0;
    LATC6 = 0;
    LATC7 = 0;
    delayMs(1000);
    
    LATC0 = 1;
    delayMs(1000);
    LATC0 = 0;
    delayMs(1000);
    
    testVideo();
    
    while (1) {
        delayMs(250);
    }
    return 0;
}


