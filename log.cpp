#include "log.h"

#include "raylib.h"

#include <cstdarg>
#include <string>

extern float g_since_start;

void LogMat4(const char* name, const Matrix& m4)
{
    Log("%s:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", name,
        m4.m0, m4.m1, m4.m2, m4.m3,
        m4.m4, m4.m5, m4.m6, m4.m7,
        m4.m8, m4.m9, m4.m10, m4.m11,
        m4.m12, m4.m13, m4.m14, m4.m15
    );
}

void Log(const char* format, ...)
{
    char buffer[1024];
    strcpy_s(buffer, 1024, "%0.3f   ");

    va_list args;
    va_start(args, format);
    vsprintf_s(buffer + strlen(buffer), 1024 - strlen(buffer), format, args);
    va_end(args);

    TraceLog(LOG_DEBUG, buffer, g_since_start);
}