#include "ADFWriter.h"
#include "ArduinoInterface.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>

#include "common.hpp"

using namespace ArduinoFloppyReader;

static struct termios old, current;

ADFWriter writer;

// All Settings
const SettingName AllSettings[MAX_SETTINGS] = {
        {"DISKCHANGE","Force DiskChange Detection Support (used if pin 12 is not connected to GND)"},
        {"PLUS","Set DrawBridge Plus Mode (when Pin 4 and 8 are swapped for improved accuracy)"},
        {"ALLDD","Force All Disks to be Detected as Double Density (faster if you don't need HD support)"},
        {"SLOW","Use Slower Disk Seeking (for slow head-moving floppy drives - rare)"},
        {"INDEX","Always Index-Align Writing to Disks (not recommended unless you know what you are doing)"}
};

static const char *ModeNames[] = {"ADF", "IMG", "ST", "SCP", "IPF"};

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
    tcgetattr(0, &old);			/* grab old terminal i/o settings */
    current = old;				/* make new settings same as old settings */
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
        printf("[%s] %s \t%s\n", value ? "X" : " ", AllSettings[a].name, AllSettings[a].description);
    }
    printf("\n");
}

void listSettings(const std::string &port)
{
    ArduinoFloppyReader::ArduinoInterface io;
    printf("Attempting to read settings from device on port: %s\n\n", port.c_str());

    ArduinoFloppyReader::DiagnosticResponse resp = io.openPort(port);
    if (resp != ArduinoFloppyReader::DiagnosticResponse::drOK)
    {
        printf("Unable to connect to device: \n");
        printf(io.getLastErrorStr().c_str());
        return;
    }

    ArduinoFloppyReader::FirmwareVersion version = io.getFirwareVersion();
    if ((version.major == 1) && (version.minor < 9))
    {
        printf("This feature requires firmware V1.9\n");
        return;
    }

    internalListSettings(io);
}

void programmeSetting(const std::string &port, const std::string &settingName, const bool settingValue)
{
    ArduinoFloppyReader::ArduinoInterface io;
    printf("Attempting to set settings to device on port: %s\n\n", port.c_str());
    ArduinoFloppyReader::DiagnosticResponse resp = io.openPort(port);
    if (resp != ArduinoFloppyReader::DiagnosticResponse::drOK)
    {
        printf("Unable to connect to device: \n");
        printf(io.getLastErrorStr().c_str());
        return;
    }

    ArduinoFloppyReader::FirmwareVersion version = io.getFirwareVersion();
    if ((version.major == 1) && (version.minor < 9))
    {
        printf("This feature requires firmware V1.9\n");
        return;
    }

    // See which one it is
    for (int a = 0; a < MAX_SETTINGS; a++)
    {
        std::string s = AllSettings[a].name;
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

            printf("Settng Set.  Current settings are now:\n\n");
            internalListSettings(io);
            return;
        }
    }
    printf("Setting %s was not found.\n\n", settingName.c_str());
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
        printf("File extension not recognised. It must be one of:\n\n");
        printf(" .ADF, .IMG, .IMA, .ST, .SCP or .IPF\n\n");
        return;
    }
    bool hdMode = false;
    bool isSCP = true;
    bool isIPF = false;

    printf("\nWriting %s file to disk\n\n", ModeNames[mode]);
    if (!((mode == MODE_IPF) || (mode == MODE_SCP)))
    {
        if (!verify)
            printf("WARNING: It is STRONGLY recommended to write with verify turned on.\r\n\r\n");
    }

    // Detect disk speed/density
    const ArduinoFloppyReader::FirmwareVersion v = writer.getFirwareVersion();
    if (((v.major == 1) && (v.minor >= 9)) || (v.major > 1))
    {
        if ((!isSCP) && (!isIPF))
            if (writer.GuessDiskDensity(hdMode) != ArduinoFloppyReader::ADFResult::adfrComplete)
            {
                printf("Unable to work out the density of the disk inserted.\n");
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
                        printf("\rDisk write verify error on track %i, %s side. [R]etry, [S]kip, [A]bort?                                   ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
                        input = toupper(_getChar());
                    } while ((input != 'R') && (input != 'S') && (input != 'A'));

                    switch (input) {
                        case 'R': return WriteResponse::wrRetry;
                        case 'I': return WriteResponse::wrSkipBadChecksums;
                        case 'A': return WriteResponse::wrAbort;
                    }
                }
                printf("\rWriting Track %i, %s side     ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
                fflush(stdout);
                return WriteResponse::wrContinue; });
        }
            break;

        case MODE_SCP:
        {
            result = writer.SCPToDisk(filename, false, [](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
            {
                printf("\nWriting Track %i, %s side     ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
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
                        printf("\nDisk write verify error on track %i, %s side. [R]etry, [S]kip, [A]bort?                                   ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
                        input = toupper(_getChar());
                    } while ((input != 'R') && (input != 'S') && (input != 'A'));

                    switch (input) {
                        case 'R': return WriteResponse::wrRetry;
                        case 'I': return WriteResponse::wrSkipBadChecksums;
                        case 'A': return WriteResponse::wrAbort;
                    }
                }
                printf("\rWriting Track %i, %s side     ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
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
                        printf("\nDisk write verify error on track %i, %s side. [R]etry, [S]kip, [A]bort?                                   ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
                        input = toupper(_getChar());
                    } while ((input != 'R') && (input != 'S') && (input != 'A'));

                    switch (input) {
                        case 'R': return WriteResponse::wrRetry;
                        case 'I': return WriteResponse::wrSkipBadChecksums;
                        case 'A': return WriteResponse::wrAbort;
                    }
                }
                printf("\rWriting Track %i, %s side     ", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
                fflush(stdout);
                return WriteResponse::wrContinue; });
            break;
        }
    }

    switch (result)
    {
        case ADFResult::adfrBadSCPFile:
            printf("\nBad, invalid or unsupported SCP file                                               ");
            break;
        case ADFResult::adfrComplete:
            printf("\nFile written to disk                                                               ");
            break;
        case ADFResult::adfrExtendedADFNotSupported:
            printf("\nExtended ADF files are not currently supported.                                    ");
            break;
        case ADFResult::adfrMediaSizeMismatch:
            if (isIPF)
                printf("\nIPF writing is only supported for DD disks and images                          ");
            else if (isSCP)
                printf("\nSCP writing is only supported for DD disks and images                             ");
            else if (hdMode)
                printf("\nDisk in drive was detected as HD, but a DD ADF file supplied.                      ");
            else
                printf("\nDisk in drive was detected as DD, but an HD ADF file supplied.                     ");
            break;
        case ADFResult::adfrFirmwareTooOld:
            printf("\nCannot write this file, you need to upgrade the firmware first.                    ");
            break;
        case ADFResult::adfrCompletedWithErrors:
            printf("\nFile written to disk but there were errors during verification                      ");
            break;
        case ADFResult::adfrAborted:
            printf("\nWriting aborted                                                                    ");
            break;
        case ADFResult::adfrFileError:
            printf("\nError opening file.                                                                ");
            break;
        case ADFResult::adfrIPFLibraryNotAvailable:
            printf("\nIPF CAPSImg from Software Preservation Society Library Missing                     ");
            break;
        case ADFResult::adfrDriveError:
            printf("\nError communicating with the DrawBridge interface.                                 ");
            printf("\n%s                                                  ", writer.getLastError().c_str());
            break;
        case ADFResult::adfrDiskWriteProtected:
            printf("\nError, disk is write protected!                                                    ");
            break;
        default:
            printf("\nAn unknown error occured                                                           ");
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
        printf("File extension not recognised. It must be one of:\n\n");
        printf(" .ADF, .IMG, .IMA, .ST or .SCP\n\n");
        return;
    }

    printf("\nCreating %s file from disk\n\n", ModeNames[mode]);

    bool hdMode = false;

    // Detect disk speed
    const ArduinoFloppyReader::FirmwareVersion v = writer.getFirwareVersion();

    // Get the current firmware version.  Only valid if openDevice is successful
    if ((v.major == 1) && (v.minor < 8))
    {
        if (mode == MODE_SCP)
        {
            printf("This requires firmware V1.8 or newer.\n");
            return;
        }
        else
        {
            // improved disk timings in 1.8, so make them aware
            printf("Rob strongly recommends updating the firmware on your Arduino to at least V1.8.\n");
            printf("That version is even better at reading old disks.\n");
        }
    }

    auto callback = [mode, hdMode](const int currentTrack, const DiskSurface currentSide, const int retryCounter, const int sectorsFound, const int badSectorsFound, const int totalSectors, const CallbackOperation operation) -> WriteResponse
    {
        if (retryCounter > 20)
        {
            char input;
            do
            {
                printf("\rDisk has checksum errors/missing data.  [R]etry, [I]gnore, [A]bort?                                      ");
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
            printf("\rReading %s Track %i, %s side   ", hdMode ? "HD" : "DD", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower");
        }
        else
        {
            printf("\rReading %s Track %i, %s side (retry: %i) - Got %i/%i sectors (%i bad found)   ", hdMode ? "HD" : "DD", currentTrack, (currentSide == DiskSurface::dsUpper) ? "Upper" : "Lower", retryCounter, sectorsFound, totalSectors, badSectorsFound);
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
            printf("\rFile created successfully.                                                     ");
            break;
        case ADFResult::adfrAborted:
            printf("\rFile aborted.                                                                  ");
            break;
        case ADFResult::adfrFileError:
            printf("\rError creating file.                                                           ");
            break;
        case ADFResult::adfrFileIOError:
            printf("\rError writing to file.                                                         ");
            break;
        case ADFResult::adfrFirmwareTooOld:
            printf("\rThis requires firmware V1.8 or newer.                                          ");
            break;
        case ADFResult::adfrCompletedWithErrors:
            printf("\rFile created with partial success.                                             ");
            break;
        case ADFResult::adfrDriveError:
            printf("\rError communicating with the Arduino interface.                                ");
            printf("\n%s                                                  ", writer.getLastError().c_str());
            break;
        default:
            printf("\rAn unknown error occured.                                                      ");
            break;
    }
}

// Run drive cleaning action
void runCleaning(const std::string& port) {
    printf("\rRunning drive head cleaning on COM port: %s\n\n", port.c_str());
    writer.runClean([](const uint16_t position, const uint16_t maxPosition)->bool {
        printf("\rProgress: %i%%    ", (position * 100) / maxPosition);
        fflush(stdout);
        return true;
    });
    printf("\rCleaning cycle completed.\n\n");
}

// Run the diagnostics module
void runDiagnostics(const std::string &port)
{
    printf("\rRunning diagnostics on COM port: %s\n", port.c_str());
    writer.runDiagnostics(port, [](bool isError, const std::string message) -> void
    {
        if (isError)
            printf("DIAGNOSTICS FAILED: %s\n",message.c_str());
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