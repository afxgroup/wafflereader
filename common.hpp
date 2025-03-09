#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <string>
#include <vector>

#define MODE_ADF 0
#define MODE_IMG 1
#define MODE_ST 2
#define MODE_SCP 3
#define MODE_IPF 4

// Settings type
struct SettingName {
    const char* name;
    const char* description;
};

#define MAX_SETTINGS 5

void file2Disk(const std::string &filename, bool verify);
void disk2file(const std::string &filename);
void runCleaning(const std::string& port);
void runDiagnostics(const std::string &port);
void listSettings(const std::string &port);
void programmeSetting(const std::string &port, const std::string &settingName, const bool settingValue);
bool iequals(const std::string &a, const std::string &b);

#endif
