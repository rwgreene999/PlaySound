#include "AudioPlayback.h"

#include <cmath>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
    struct Backend
    {
        std::string name;
        std::string command;
    };

    bool commandExists(const std::string &name)
    {
        const std::string command = "command -v " + name + " >/dev/null 2>&1";
        return std::system(command.c_str()) == 0;
    }

    std::string shellQuote(const std::string &value)
    {
        std::string quoted;
        quoted.reserve(value.size() + 2);
        quoted.push_back('\'');

        for (char ch : value)
        {
            if (ch == '\'')
            {
                quoted += "'\\''";
            }
            else
            {
                quoted.push_back(ch);
            }
        }

        quoted.push_back('\'');
        return quoted;
    }

    std::string toLower(std::string value)
    {
        for (char &ch : value)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return value;
    }

}

std::string getExtension(const std::string &path)
{
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= path.size())
    {
        return "";
    }
    return toLower(path.substr(dot + 1));
}

bool fileExists(const std::string &path)
{
    std::ostringstream command;
    command << "test -f " << shellQuote(path) << " >/dev/null 2>&1";
    return std::system(command.str().c_str()) == 0;
}

bool tryPlayFile(const std::string &path)
{
    const std::string ext = getExtension(path);
    const std::string pathArg = shellQuote(path);
    std::vector<Backend> backends;

    if (ext == "wav")
    {
        if (commandExists("paplay"))
        {
            backends.push_back({"paplay", "paplay " + pathArg + " >/dev/null 2>&1"});
        }
        if (commandExists("pw-play"))
        {
            backends.push_back({"pw-play", "pw-play " + pathArg + " >/dev/null 2>&1"});
        }
        if (commandExists("aplay"))
        {
            backends.push_back({"aplay", "aplay -q " + pathArg + " >/dev/null 2>&1"});
        }
    }
    else if (ext == "mp3")
    {
        if (commandExists("mpg123"))
        {
            backends.push_back({"mpg123", "mpg123 -q " + pathArg + " >/dev/null 2>&1"});
        }
        if (commandExists("mpg321"))
        {
            backends.push_back({"mpg321", "mpg321 -q " + pathArg + " >/dev/null 2>&1"});
        }
    }

    if (commandExists("ffplay"))
    {
        backends.push_back({"ffplay", "ffplay -nodisp -autoexit -loglevel quiet " + pathArg + " >/dev/null 2>&1"});
    }
    if (commandExists("mplayer"))
    {
        backends.push_back({"mplayer", "mplayer -really-quiet " + pathArg + " >/dev/null 2>&1"});
    }
    if (commandExists("cvlc"))
    {
        backends.push_back({"cvlc", "cvlc --play-and-exit --quiet " + pathArg + " >/dev/null 2>&1"});
    }
    if (commandExists("mpv"))
    {
        backends.push_back({"mpv", "mpv --no-video --really-quiet " + pathArg + " >/dev/null 2>&1"});
    }
    if (commandExists("play"))
    {
        backends.push_back({"play", "play -q " + pathArg + " >/dev/null 2>&1"});
    }

    for (const Backend &backend : backends)
    {
        if (std::system(backend.command.c_str()) == 0)
        {
            std::cout << "Used backend: " << backend.name << std::endl;
            return true;
        }
    }

    return false;
}
