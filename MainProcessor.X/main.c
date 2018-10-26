
#include <xc.h>

#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config BOREN = SBORDIS
#pragma config WDTE = OFF

void longDelay() {
    long int i;
    for (i = 0; i < 400000; i += 1);
}

void shortDelay() {
    long int i;
    for (i = 0; i < 100000; i += 1);
}

int main() {
    TRISD0 = 0;
    RD0 = 1;
    longDelay();
    RD0 = 0;
    shortDelay();
    while (1) {
        RD0 = 1;
        shortDelay();
        RD0 = 0;
        shortDelay();
    }
    return 0;
}


