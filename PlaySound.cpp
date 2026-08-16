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

    bool tryPlayDesktopBell();

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

    void writeLittleEndian16(std::ofstream &output, std::uint16_t value)
    {
        output.put(static_cast<char>(value & 0xFF));
        output.put(static_cast<char>((value >> 8) & 0xFF));
    }

    void writeLittleEndian32(std::ofstream &output, std::uint32_t value)
    {
        output.put(static_cast<char>(value & 0xFF));
        output.put(static_cast<char>((value >> 8) & 0xFF));
        output.put(static_cast<char>((value >> 16) & 0xFF));
        output.put(static_cast<char>((value >> 24) & 0xFF));
    }

    bool createToneFile(const std::string &path)
    {
        constexpr int sampleRate = 44100;
        constexpr double frequency = 880.0;
        constexpr double durationSeconds = 0.18;
        constexpr std::uint16_t bitsPerSample = 16;
        constexpr std::uint16_t channels = 1;
        constexpr std::uint16_t blockAlign = channels * bitsPerSample / 8;
        constexpr std::uint32_t byteRate = sampleRate * blockAlign;
        const std::uint32_t sampleCount = static_cast<std::uint32_t>(sampleRate * durationSeconds);
        const std::uint32_t dataSize = sampleCount * blockAlign;

        std::ofstream output(path, std::ios::binary);
        if (!output)
        {
            return false;
        }

        output.write("RIFF", 4);
        writeLittleEndian32(output, 36 + dataSize);
        output.write("WAVE", 4);
        output.write("fmt ", 4);
        writeLittleEndian32(output, 16);
        writeLittleEndian16(output, 1);
        writeLittleEndian16(output, channels);
        writeLittleEndian32(output, sampleRate);
        writeLittleEndian32(output, byteRate);
        writeLittleEndian16(output, blockAlign);
        writeLittleEndian16(output, bitsPerSample);
        output.write("data", 4);
        writeLittleEndian32(output, dataSize);

        for (std::uint32_t i = 0; i < sampleCount; ++i)
        {
            const double time = static_cast<double>(i) / sampleRate;
            const double sample = std::sin(2.0 * 3.14159265358979323846 * frequency * time);
            const auto value = static_cast<std::int16_t>(sample * 18000.0);
            writeLittleEndian16(output, static_cast<std::uint16_t>(value));
        }

        return static_cast<bool>(output);
    }

    void playTerminalBellTimes(int count)
    {
        bool usedDesktopBell = false;

        for (int i = 0; i < count; ++i)
        {
            if (tryPlayDesktopBell())
            {
                usedDesktopBell = true;
            }
            else
            {
                std::cout << '\a' << std::flush;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        std::cout << std::endl;

        if (!usedDesktopBell)
        {
            std::cerr << "Desktop bell backend not available. Used terminal bell fallback." << std::endl;
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

        // Universal players that often exist across desktop distros.
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

    bool tryPlayDesktopBell()
    {
        if (commandExists("canberra-gtk-play"))
        {
            return std::system("canberra-gtk-play -i bell >/dev/null 2>&1") == 0;
        }
        if (commandExists("canberra-gtk-play-3"))
        {
            return std::system("canberra-gtk-play-3 -i bell >/dev/null 2>&1") == 0;
        }
        return false;
    }

    std::optional<std::string> findExistingBellSample()
    {
        const std::vector<std::string> candidates = {
            "/usr/share/sounds/freedesktop/stereo/bell.oga",
            "/usr/share/sounds/freedesktop/stereo/message.oga",
            "/usr/share/sounds/ubuntu/stereo/bell.ogg",
            "/usr/share/sounds/ubuntu/stereo/message.ogg",
            "/usr/share/sounds/alsa/Front_Center.wav"};

        for (const std::string &candidate : candidates)
        {
            if (fileExists(candidate))
            {
                return candidate;
            }
        }
        return std::nullopt;
    }

    void beepTimesPreferAudio(int count)
    {
        const std::string toneFile = "/tmp/PlaySound-tone.wav";
        const bool canUseTone = createToneFile(toneFile);
        const std::optional<std::string> packagedBell = findExistingBellSample();

        for (int i = 0; i < count; ++i)
        {
            bool played = false;

            if (!played && tryPlayDesktopBell())
            {
                played = true;
            }

            if (!played && packagedBell.has_value() && tryPlayFile(packagedBell.value()))
            {
                played = true;
            }

            if (!played && canUseTone && tryPlayFile(toneFile))
            {
                played = true;
            }

            if (!played)
            {
                std::cout << '\a' << std::flush;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        std::remove(toneFile.c_str());
        std::cout << std::endl;
    }
}

int main()
{
    std::cout << "Select an option:" << std::endl;
    std::cout << "  0) Quit Program" << std::endl;
    std::cout << "  1) Beep 10 times" << std::endl;
    std::cout << "  2) Play an audio file (.mp3 or .wav)" << std::endl;
    std::cout << "  3) Try playTerminalBellTimes" << std::endl;

    do
    {
        std::cout << "Choice: " << std::flush;

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "0")
        {
            return 0;
        }

        if (choice == "1")
        {
            std::cout << "Playing beep 10 times (audio first, terminal bell fallback)..." << std::endl;
            beepTimesPreferAudio(10);
            std::cout << "Done." << std::endl;
        }

        if (choice == "3")
        {
            std::cout << "Playing terminal bell 10 times..." << std::endl;
            playTerminalBellTimes(10);
            std::cout << "Done." << std::endl;
        }

        if (choice == "2")
        {
            std::cout << "Enter full path to .mp3 or .wav file: " << std::flush;
            std::string path;
            std::getline(std::cin, path);

            if (path.empty())
            {
                std::cerr << "No file path provided." << std::endl;
            }
            else if (!fileExists(path))
            {
                std::cerr << "File does not exist: " << path << std::endl;
            }
            else
            {
                const std::string ext = getExtension(path);
                if (ext != "mp3" && ext != "wav")
                {
                    std::cerr << "Unsupported file type. Please use .mp3 or .wav." << std::endl;
                }

                if (tryPlayFile(path))
                {
                    std::cout << "Playback finished." << std::endl;
                }
                else
                {
                    std::cerr << "Playback failed for file: " << path << std::endl;
                }
            }
        }

    } while (true);

    return 1;
}
