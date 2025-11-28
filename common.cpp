#include "ADFWriter.h"
#include "ArduinoInterface.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>

#include "common.hpp"

using namespace ArduinoFloppyReader;

static struct termios old, current;

ADFWriter writer;

// All Settings - Note: Using static strings for initialization, actual localized strings fetched at runtime
static const int SettingMsgIds[MAX_SETTINGS][2] = {
    {MSG_SETTING_DISKCHANGE_NAME, MSG_SETTING_DISKCHANGE_DESC},
    {MSG_SETTING_PLUS_NAME, MSG_SETTING_PLUS_DESC},
    {MSG_SETTING_ALLDD_NAME, MSG_SETTING_ALLDD_DESC},
    {MSG_SETTING_SLOW_NAME, MSG_SETTING_SLOW_DESC},
    {MSG_SETTING_INDEX_NAME, MSG_SETTING_INDEX_DESC}};

static const char *ModeNames[] = {"ADF", "IMG", "ST", "SCP", "IPF"};

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
    tcgetattr(0, &old);         /* grab old terminal i/o settings */
    current = old;              /* make new settings same as old settings */
    current.c_lflag &= ~ICANON; /* disable buffered i/o */
    if (echo)
    {
        current.c_lflag |= ECHO; /* set echo mode */
    }
    else
    {
        current.c_lflag &= ~ECHO; /* set no echo mode */
    }
    tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
    tcsetattr(0, TCSANOW, &old);
}

char _getChar()
{
    char ch;
    initTermios(0);
    ch = getchar();
    resetTermios();
    return ch;
}

// Stolen from https://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c
// A wide-string case insensative compare of strings
bool iequals(const std::string &a, const std::string &b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](wchar_t a, wchar_t b)
                      { return tolower(a) == tolower(b); });
}

void internalListSettings(ArduinoFloppyReader::ArduinoInterface &io)
{
    for (int a = 0; a < MAX_SETTINGS; a++)
    {
        bool value = false;
        ;
        switch (a)
        {
        case 0:
            io.eeprom_IsAdvancedController(value);
            break;
        case 1:
            io.eeprom_IsDrawbridgePlusMode(value);
            break;
        case 2:
            io.eeprom_IsDensityDetectDisabled(value);
            break;
        case 3:
            io.eeprom_IsSlowSeekMode(value);
            break;
        case 4:
            io.eeprom_IsIndexAlignMode(value);
            break;
        }
        printf("[%s] %s \t%s\n", value ? "X" : " ",
               GetString(SettingMsgIds[a][0]),
               GetString(SettingMsgIds[a][1]));
    }
    printf("\n");
}

void listSettings(const std::string &port)
{
    ArduinoFloppyReader::ArduinoInterface io;
    printf(GetString(MSG_ATTEMPTING_READ_SETTINGS), port.c_str());
    printf("\n\n");

    ArduinoFloppyReader::DiagnosticResponse resp = io.openPort(port);
    if (resp != ArduinoFloppyReader::DiagnosticResponse::drOK)
    {
        printf("%s\n", GetString(MSG_UNABLE_CONNECT_DEVICE));
        printf(io.getLastErrorStr().c_str());
        return;
    }

    ArduinoFloppyReader::FirmwareVersion version = io.getFirwareVersion();
    if ((version.major == 1) && (version.minor < 9))
    {
        printf("%s\n", GetString(MSG_FIRMWARE_V19_REQUIRED));
        return;
    }

    internalListSettings(io);
}

void programmeSetting(const std::string &port, const std::string &settingName, const bool settingValue)
{
    ArduinoFloppyReader::ArduinoInterface io;
    printf(GetString(MSG_ATTEMPTING_SET_SETTINGS), port.c_str());
    printf("\n\n");
    ArduinoFloppyReader::DiagnosticResponse resp = io.openPort(port);
    if (resp != ArduinoFloppyReader::DiagnosticResponse::drOK)
    {
        printf("%s\n", GetString(MSG_UNABLE_CONNECT_DEVICE));
        printf(io.getLastErrorStr().c_str());
        return;
    }

    ArduinoFloppyReader::FirmwareVersion version = io.getFirwareVersion();
    if ((version.major == 1) && (version.minor < 9))
    {
        printf("%s\n", GetString(MSG_FIRMWARE_V19_REQUIRED));
        return;
    }

    // See which one it is
    for (int a = 0; a < MAX_SETTINGS; a++)
    {
        std::string s = GetString(SettingMsgIds[a][0]);
        if (iequals(s, settingName))
        {

            switch (a)
            {
            case 0:
                io.eeprom_SetAdvancedController(settingValue);
                break;
            case 1:
                io.eeprom_SetDrawbridgePlusMode(settingValue);
                break;
            case 2:
                io.eeprom_SetDensityDetectDisabled(settingValue);
                break;
            case 3:
                io.eeprom_SetSlowSeekMode(settingValue);
                break;
            case 4:
                io.eeprom_SetIndexAlignMode(settingValue);
                break;
            }

            printf("%s\n\n", GetString(MSG_SETTING_SET));
            internalListSettings(io);
            return;
        }
    }
    printf(GetString(MSG_SETTING_NOT_FOUND), settingName.c_str());
    printf("\n\n");
}

// Read an ADF/SCP/IPF/IMG/IMA/ST file and write it to disk
void file2Disk(const std::string &filename, bool verify)
{
    const char *extension = strstr(filename.c_str(), ".");
    int32_t mode = -1;

    if (extension)
    {
        extension++;
        if (iequals(extension, "SCP"))
            mode = MODE_SCP;
        else if (iequals(extension, "ADF"))
            mode = MODE_ADF;
        else if (iequals(extension, "IMG"))
            mode = MODE_IMG;
        else if (iequals(extension, "IMA"))
            mode = MODE_IMG;
        else if (iequals(extension, "ST"))
            mode = MODE_ST;
        else if (iequals(extension, "IPF"))
            mode = MODE_IPF;
    }
    if (mode < 0)
    {
        printf("%s\n", GetString(MSG_FILE_EXT_NOT_RECOGNIZED_WRITE));
        printf("\n");
        return;
    }
    bool hdMode = false;
    bool isSCP = true;
    bool isIPF = false;

    printf("\n");
    printf(GetString(MSG_WRITING_FILE_TO_DISK), ModeNames[mode]);
    printf("\n\n");
    if (!((mode == MODE_IPF) || (mode == MODE_SCP)))
    {
        if (!verify)
        {
            printf(GetString(MSG_WARNING_VERIFY_RECOMMENDED));
            printf("\r\n\r\n");
        }
    }

    // Detect disk speed/density
    const ArduinoFloppyReader::FirmwareVersion v = writer.getFirwareVersion();
    if (((v.major == 1) && (v.minor >= 9)) || (v.major > 1))
    {
        if ((!isSCP) && (!isIPF))
            if (writer.GuessDiskDensity(hdMode) != ArduinoFloppyReader::ADFResult::adfrComplete)
            {
                printf("%s\n", GetString(MSG_UNABLE_DENSITY));
                return;
            }
    }

    ADFResult result;

    switch (mode)
    {
    case MODE_IPF:
    {
        result = writer.IPFToDisk(filename, false, [](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
                                  {
                if (isVerifyError) {
                    char input;
                    do {
                        printf("\r");
                        printf(GetString(MSG_DISK_VERIFY_ERROR_PROMPT), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                        input = toupper(_getChar());
                    } while ((input != 'R') && (input != 'S') && (input != 'A'));

                    switch (input) {
                        case 'R': return WriteResponse::wrRetry;
                        case 'I': return WriteResponse::wrSkipBadChecksums;
                        case 'A': return WriteResponse::wrAbort;
                    }
                }
                printf("\r");
                printf(GetString(MSG_WRITING_TRACK), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                fflush(stdout);
                return WriteResponse::wrContinue; });
    }
    break;

    case MODE_SCP:
    {
        result = writer.SCPToDisk(filename, false, [](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
                                  {
                printf("\n");
                printf(GetString(MSG_WRITING_TRACK), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                fflush(stdout);
                return WriteResponse::wrContinue; });
    }
    break;
    case MODE_ADF:
    {
        result = writer.ADFToDisk(filename, hdMode, verify, true, false, true, [](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
                                  {
                if (isVerifyError) {
                    char input;
                    do {
                        printf("\n");
                        printf(GetString(MSG_DISK_VERIFY_ERROR_PROMPT), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                        input = toupper(_getChar());
                    } while ((input != 'R') && (input != 'S') && (input != 'A'));

                    switch (input) {
                        case 'R': return WriteResponse::wrRetry;
                        case 'I': return WriteResponse::wrSkipBadChecksums;
                        case 'A': return WriteResponse::wrAbort;
                    }
                }
                printf("\r");
                printf(GetString(MSG_WRITING_TRACK), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                fflush(stdout);
                return WriteResponse::wrContinue; });
        break;
    }
    case MODE_IMG:
    case MODE_ST:
    {
        result = writer.sectorFileToDisk(filename, hdMode, verify, true, false, mode == MODE_ST, [](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
                                         {
                if (isVerifyError) {
                    char input;
                    do {
                        printf("\n");
                        printf(GetString(MSG_DISK_VERIFY_ERROR_PROMPT), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                        input = toupper(_getChar());
                    } while ((input != 'R') && (input != 'S') && (input != 'A'));

                    switch (input) {
                        case 'R': return WriteResponse::wrRetry;
                        case 'I': return WriteResponse::wrSkipBadChecksums;
                        case 'A': return WriteResponse::wrAbort;
                    }
                }
                printf("\r");
                printf(GetString(MSG_WRITING_TRACK), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
                fflush(stdout);
                return WriteResponse::wrContinue; });
        break;
    }
    }

    switch (result)
    {
    case ADFResult::adfrBadSCPFile:
        printf("\n%s", GetString(MSG_BAD_SCP_FILE));
        break;
    case ADFResult::adfrComplete:
        printf("\n%s", GetString(MSG_FILE_WRITTEN));
        break;
    case ADFResult::adfrExtendedADFNotSupported:
        printf("\n%s", GetString(MSG_EXTENDED_ADF_NOT_SUPPORTED));
        break;
    case ADFResult::adfrMediaSizeMismatch:
        if (isIPF)
            printf("\n%s", GetString(MSG_IPF_ONLY_DD));
        else if (isSCP)
            printf("\n%s", GetString(MSG_SCP_ONLY_DD));
        else if (hdMode)
            printf("\n%s", GetString(MSG_DISK_HD_FILE_DD));
        else
            printf("\n%s", GetString(MSG_DISK_DD_FILE_HD));
        break;
    case ADFResult::adfrFirmwareTooOld:
        printf("\n%s", GetString(MSG_FIRMWARE_TOO_OLD));
        break;
    case ADFResult::adfrCompletedWithErrors:
        printf("\n%s", GetString(MSG_FILE_WRITTEN_ERRORS));
        break;
    case ADFResult::adfrAborted:
        printf("\n%s", GetString(MSG_WRITING_ABORTED));
        break;
    case ADFResult::adfrFileError:
        printf("\n%s", GetString(MSG_ERROR_OPENING_FILE));
        break;
    case ADFResult::adfrIPFLibraryNotAvailable:
        printf("\n%s", GetString(MSG_IPF_LIBRARY_MISSING));
        break;
    case ADFResult::adfrDriveError:
        printf("\n%s", GetString(MSG_ERROR_COMM_DRAWBRIDGE));
        printf("\n%s", writer.getLastError().c_str());
        break;
    case ADFResult::adfrDiskWriteProtected:
        printf("\n%s", GetString(MSG_DISK_WRITE_PROTECTED));
        break;
    default:
        printf("\n%s", GetString(MSG_UNKNOWN_ERROR));
        break;
    }
}

// Read a disk and save it to ADF/SCP/IMG/IMA/ST files
void disk2file(const std::string &filename)
{
    const char *extension = strstr(filename.c_str(), ".");
    int32_t mode = -1;

    if (extension)
    {
        extension++;
        if (iequals(extension, "ADF"))
            mode = MODE_ADF;
        else if (iequals(extension, "SCP"))
            mode = MODE_SCP;
        else if (iequals(extension, "IMG"))
            mode = MODE_IMG;
        else if (iequals(extension, "IMA"))
            mode = MODE_IMG;
        else if (iequals(extension, "ST"))
            mode = MODE_ST;
    }
    if (mode < 0)
    {
        printf("%s\n\n", GetString(MSG_FILE_EXT_NOT_RECOGNIZED));
        return;
    }

    printf("\n");
    printf(GetString(MSG_CREATING_FILE_FROM_DISK), ModeNames[mode]);
    printf("\n\n");

    bool hdMode = false;

    // Detect disk speed
    const ArduinoFloppyReader::FirmwareVersion v = writer.getFirwareVersion();

    // Get the current firmware version.  Only valid if openDevice is successful
    if ((v.major == 1) && (v.minor < 8))
    {
        if (mode == MODE_SCP)
        {
            printf("%s\n", GetString(MSG_FIRMWARE_V18_REQUIRED));
            return;
        }
        else
        {
            // improved disk timings in 1.8, so make them aware
            printf("%s\n", GetString(MSG_FIRMWARE_RECOMMEND_V18));
            printf("%s\n", GetString(MSG_FIRMWARE_BETTER_READING));
        }
    }

    auto callback = [mode, hdMode](const int currentTrack, const DiskSurface currentSide, const int retryCounter, const int sectorsFound, const int badSectorsFound, const int totalSectors, const CallbackOperation operation) -> WriteResponse
    {
        if (retryCounter > 20)
        {
            char input;
            do
            {
                printf("\r");
                printf(GetString(MSG_CHECKSUM_ERROR_PROMPT));
                input = toupper(_getChar());
            } while ((input != 'R') && (input != 'I') && (input != 'A'));
            switch (input)
            {
            case 'R':
                return WriteResponse::wrRetry;
            case 'I':
                return WriteResponse::wrSkipBadChecksums;
            case 'A':
                return WriteResponse::wrAbort;
            }
        }
        if (mode == MODE_SCP)
        {
            printf("\r");
            printf(GetString(MSG_READING_TRACK), hdMode ? GetString(MSG_HD) : GetString(MSG_DD), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER));
        }
        else
        {
            printf("\r");
            printf(GetString(MSG_READING_TRACK_DETAILED), hdMode ? GetString(MSG_HD) : GetString(MSG_DD), currentTrack, (currentSide == DiskSurface::dsUpper) ? GetString(MSG_SIDE_UPPER) : GetString(MSG_SIDE_LOWER), retryCounter, sectorsFound, totalSectors, badSectorsFound);
        }
        fflush(stdout);
        return WriteResponse::wrContinue;
    };

    ADFResult result;

    switch (mode)
    {
    case MODE_ADF:
        result = writer.DiskToADF(filename, hdMode, 80, callback);
        break;
    case MODE_SCP:
        result = writer.DiskToSCP(filename, hdMode, 80, 3, callback);
        break;
    case MODE_ST:
    case MODE_IMG:
        result = writer.diskToIBMST(filename, hdMode, callback);
        break;
    }

    switch (result)
    {
    case ADFResult::adfrComplete:
        printf("\r%s", GetString(MSG_FILE_CREATED));
        break;
    case ADFResult::adfrAborted:
        printf("\r%s", GetString(MSG_FILE_ABORTED));
        break;
    case ADFResult::adfrFileError:
        printf("\r%s", GetString(MSG_ERROR_CREATING_FILE));
        break;
    case ADFResult::adfrFileIOError:
        printf("\r%s", GetString(MSG_ERROR_WRITING_FILE));
        break;
    case ADFResult::adfrFirmwareTooOld:
        printf("\r%s", GetString(MSG_FIRMWARE_V18_REQUIRED_DOT));
        break;
    case ADFResult::adfrCompletedWithErrors:
        printf("\r%s", GetString(MSG_FILE_CREATED_PARTIAL));
        break;
    case ADFResult::adfrDriveError:
        printf("\r%s", GetString(MSG_ERROR_COMM_ARDUINO));
        printf("\n%s", writer.getLastError().c_str());
        break;
    default:
        printf("\r%s", GetString(MSG_UNKNOWN_ERROR_OCCURRED));
        break;
    }
}

// Run drive cleaning action
void runCleaning(const std::string &port)
{
    printf("\r");
    printf(GetString(MSG_RUNNING_CLEANING), port.c_str());
    printf("\n\n");
    writer.runClean([](const uint16_t position, const uint16_t maxPosition) -> bool
                    {
        printf("\r");
        printf(GetString(MSG_PROGRESS), (position * 100) / maxPosition);
        fflush(stdout);
        return true; });
    printf("\r%s\n\n", GetString(MSG_CLEANING_COMPLETED));
}

// Run the diagnostics module
void runDiagnostics(const std::string &port)
{
    printf("\r");
    printf(GetString(MSG_RUNNING_DIAGNOSTICS), port.c_str());
    printf("\n");
    writer.runDiagnostics(port, [](bool isError, const std::string message) -> void
                          {
        if (isError)
        {
            printf(GetString(MSG_DIAGNOSTICS_FAILED), message.c_str());
            printf("\n");
        }
        else
            printf("%s\n", message.c_str()); }, [](bool isQuestion, const std::string question) -> bool
                          {
                              if (isQuestion)
                                  printf("%s [Y/N]: ", question.c_str());
                              else
                                  printf("%s [Enter/ESC]: ", question.c_str());

                              char c;
                              do {
                                  c = toupper(_getChar());
                              } while ((c != 'Y') && (c != 'N') && (c != '\n') && (c != '\r') && (c != '\x1B'));
                              printf("%c\n", c);

                              return (c == 'Y') || (c == '\n') || (c == '\r') || (c == '\x1B'); });

    writer.closeDevice();
}