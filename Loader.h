#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#include <string>
#include <windows.h>
#include <wininet.h>
#include <regex>
#include <vector>
#include <iostream>
#include <functional>

namespace Loader
{
    void configure(std::string ApplicationAgentName);
    std::string loadFileFromURL(std::string URL);
    void loadFileFromURL(std::string URL, std::function<void (char, bool)> CB);
    void close();
}

#endif // LOADER_H_INCLUDED
