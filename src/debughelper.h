#pragma once

#ifdef LOGGING

#define DEBUG(X) (Serial.println(String(F("Debug: ")) + X))

#define INIT_DEBUGGER(BAUD) \
    {                       \
        Serial.begin(BAUD); \
        while (!Serial)     \
        {                   \
        };                  \
    }

#else

#define DEBUG(X)
#define INIT_DEBUGGER(BAUD)

#endif