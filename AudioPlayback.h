#pragma once

#include <string>

std::string getExtension(const std::string &path);
bool fileExists(const std::string &path);
bool isAudioFormat(const std::string &ext);
bool tryPlayFile(const std::string &path);
