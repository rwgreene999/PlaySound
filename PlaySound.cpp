#include "AudioPlayback.h"
#include "BellAudio.h"

#include <iostream>
#include <string>

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
