#include "log.h"

#include "raylib.h"

#include <cstdarg>
#include <string>

extern float g_since_start;

void LogMat4(const char* name, const glm::mat4& m4)
{
    Log("%s:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n", name,
        m4[0][0], m4[1][0], m4[2][0], m4[3][0],
        m4[0][1], m4[1][1], m4[2][1], m4[3][1],
        m4[0][2], m4[1][2], m4[2][2], m4[3][2],
        m4[0][3], m4[1][3], m4[2][3], m4[3][3]
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