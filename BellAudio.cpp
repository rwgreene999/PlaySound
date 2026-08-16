#include "BellAudio.h"

#include "AudioPlayback.h"

#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace
{
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

    bool commandExists(const std::string &name)
    {
        const std::string command = "command -v " + name + " >/dev/null 2>&1";
        return std::system(command.c_str()) == 0;
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
