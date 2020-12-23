#include "Loader.h"

namespace Loader
{
    HINTERNET InetAccessPoint = NULL;
    bool ReadyToUse = false;
    std::regex FileProtocolMatch;
}

void Loader::configure(std::string ApplicationAgentName)
{
    if (Loader::ReadyToUse != true)
    {
        Loader::FileProtocolMatch = std::regex("(?:https:|http:|ftp:)//(?:.+)", std::regex_constants::icase | std::regex_constants::ECMAScript);

        if (Loader::InetAccessPoint != NULL)
            return;

        Loader::InetAccessPoint = InternetOpen(ApplicationAgentName.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

        if (!Loader::InetAccessPoint)
        {
            std::cout << "Connection Failed or Syntax Error";
            return;
        }

        Loader::ReadyToUse = true;
    }
}

std::string Loader::loadFileFromURL(std::string URL)
{
    bool IsOnlineInetFile = std::regex_match(URL, Loader::FileProtocolMatch);

    if (IsOnlineInetFile)
    {
        /*InetFile* File = new InetFile(InetAccessPoint, FileName);

        if (!File->LoadFileData())
        {
            delete File;
            return 0;
        }

        Internet_Files_Data.push_back(File->GetFileData());
        delete File;
        return Internet_Files_Data.size() - 1;*/
        if (Loader::InetAccessPoint == NULL || URL == "")
            return "";

        HINTERNET InetFileOpenAddress = InternetOpenUrl(Loader::InetAccessPoint, URL.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);

        if (!InetFileOpenAddress)
        {
            DWORD ErrorNum = GetLastError();
            std::cout << "Failed to open URL \nError no: " << ErrorNum;
            return "";
        }

        char DataReceived[1024];
        DWORD NumberOfBytesRead = 0;
        std::string InetFileData = "";
        while (InternetReadFile(InetFileOpenAddress, DataReceived, 1024, &NumberOfBytesRead) && NumberOfBytesRead)
        {
            //cout << DataReceived;
            InetFileData.append(DataReceived, NumberOfBytesRead);
        }

        if (InternetCloseHandle(InetFileOpenAddress) != TRUE)
        {
            DWORD ErrorNum = GetLastError();
            std::cout << "Failed to close handle \nError no: " << ErrorNum;
            return "";
        }

        return InetFileData;
    }
    return "";
}

void Loader::loadFileFromURL(std::string URL, std::function<void (char, bool)> CB)
{
    bool IsOnlineInetFile = std::regex_match(URL, Loader::FileProtocolMatch);

    if (IsOnlineInetFile)
    {
        if (Loader::InetAccessPoint == NULL || URL == "")
        {
            CB('\u0000', true);
            return;
        }

        HINTERNET InetFileOpenAddress = InternetOpenUrl(Loader::InetAccessPoint, URL.c_str(), NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);

        if (!InetFileOpenAddress)
        {
            DWORD ErrorNum = GetLastError();
            std::cout << "Failed to open URL \nError no: " << ErrorNum;
            CB('\u0000', true);
            return;
        }

        char DataReceived[1];
        DWORD NumberOfBytesRead = 0;
        while (InternetReadFile(InetFileOpenAddress, DataReceived, 1, &NumberOfBytesRead) && NumberOfBytesRead)
        {
            CB(DataReceived[0], false);
        }
        CB('\u0000', true);

        if (InternetCloseHandle(InetFileOpenAddress) != TRUE)
        {
            DWORD ErrorNum = GetLastError();
            std::cout << "Failed to close handle \nError no: " << ErrorNum;
        }
    }
}

void Loader::close()
{
    Loader::ReadyToUse = false;

    if (InternetCloseHandle(Loader::InetAccessPoint) != TRUE)
    {
        DWORD ErrorNum = GetLastError();
        std::cout << "Failed to open URL \nError no: " << ErrorNum;
        return;
    }
}
