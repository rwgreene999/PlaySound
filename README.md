# PlaySound

This is a Linux Experiment, written adn only tested on Mint, but the AI says it will work on most Linux machines

PlaySound is a small C++ Linux console app that can:

- beep 10 times
- play an MP3 file
- play a WAV file

It is designed to work across many consumer Linux machines by trying multiple audio backends at runtime.

## Build

From this folder, run:

g++ -std=c++17 -O2 -Wall -Wextra -pedantic PlaySound.cpp -o PlaySound

## Run

./PlaySound

You will see a menu:

- 1. Beep 10 times
- 2. Play an audio file (.mp3 or .wav)

## Audio Backend Strategy

The program checks available tools at runtime and tries them in order until one works.

For WAV playback, it tries:

- paplay
- pw-play
- aplay
- ffplay
- mplayer
- cvlc
- mpv
- play (SoX)

For MP3 playback, it tries:

- mpg123
- mpg321
- ffplay
- mplayer
- cvlc
- mpv
- play (SoX)

For Beep option, it tries:

- desktop bell sound via canberra-gtk-play or canberra-gtk-play-3
- common packaged bell sample files
- generated temporary WAV tone
- terminal bell fallback (ASCII bell)

When playback succeeds, the app prints which backend was used.

## Notes

- The generated temporary tone file is created at /tmp/PlaySound-tone.wav during beep playback and removed when done.
- Available backends differ by distribution and installed packages.
- Seeing VLC media player in pavucontrol is normal when cvlc is the selected backend.

## Troubleshooting

If you do not hear audio:

- confirm your output device and volume in pavucontrol
- test one backend directly from terminal, for example:
  - paplay /usr/share/sounds/alsa/Front_Center.wav
  - cvlc --play-and-exit /path/to/file.mp3
- install at least one lightweight backend (for example mpg123 for MP3 or alsa-utils for aplay)
