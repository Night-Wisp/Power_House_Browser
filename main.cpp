#include <iostream>

#include "Loader.h"
#include <string>
#include "document/Document.hpp"
#include "parsers/HTML/HTMLParser.hpp"

int main() {
    std::string AppName = "PowerHouseBrowser";
    std::string URLToAccess = "https://night-wisp.github.io";
    Loader::configure(AppName);
    std::cout << Loader::loadFileFromURL(URLToAccess);
    Loader::close();

    return 0;
}
