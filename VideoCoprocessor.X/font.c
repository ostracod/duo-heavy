
#include "main.h"
#include "font.h"

const uint8_t fontData[] = {
    // A.
    0b00000100,
    0b00001010,
    0b00010001,
    0b00011111,
    0b00010001,
    0b00010001,
    0b00010001,
    
    // B.
    0b00011110,
    0b00010001,
    0b00010001,
    0b00011110,
    0b00010001,
    0b00010001,
    0b00011110,
    
    // C.
    0b00001110,
    0b00010001,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00010001,
    0b00001110,
    
};

void drawCharacter(uint8_t posX, uint8_t offset) {
    // TODO: Write the actual function.
    videoData[31 * 10 + posX] = fontData[0 + offset * 7];
    videoData[31 * 11 + posX] = fontData[1 + offset * 7];
    videoData[31 * 12 + posX] = fontData[2 + offset * 7];
    videoData[31 * 13 + posX] = fontData[3 + offset * 7];
    videoData[31 * 14 + posX] = fontData[4 + offset * 7];
    videoData[31 * 15 + posX] = fontData[5 + offset * 7];
    videoData[31 * 16 + posX] = fontData[6 + offset * 7];
}


