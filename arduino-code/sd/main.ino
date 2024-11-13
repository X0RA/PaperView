#include "sdcard.h"


void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }
    setupSD();
}

void loop()
{
    testSD();
    sleep(1000);
}
