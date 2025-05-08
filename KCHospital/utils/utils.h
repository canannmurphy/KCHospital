#pragma once
#include <sstream>
#include <cstdlib>
#include <ctime>

inline std::string getCurrentTimestamp()
{
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}