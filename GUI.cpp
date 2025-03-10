#include "ADFWriter.h"
#include "ArduinoInterface.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <unistd.h>
#include <limits.h>

#define MAX_PORTS 10

#include "utils.hpp"
#include <pthread.h>
#include <iostream>
#include <filesystem>

#include "common.hpp"

static const char __attribute__((used)) *version = "$VER: Waffle Copy Professional GUI Version 2.8.8 for AmigaOS4 (" __DATE__ ")";

static void __attribute__((constructor (99))) gui_constructor (void) {
    setenv("LIBGL_NOBANNER", "1", 1);
}

using namespace ArduinoFloppyReader;

char fileNameWrite[PATH_MAX] = "", fileNameRead[PATH_MAX] = "";
bool loadF2DFile = false, loadD2FFile = false;
pthread_mutex_t arrayMutex; // Mutex to protect the array
bool stopWorking = false;
bool isWorking = false;
bool isReading = false;
bool isWriting = false;
pthread_t workerThread;

typedef struct __attribute__ ((packed)) {
    const char *fileName;
    const char *portName;
    int32_t mode;
    bool hdMode;
    bool verify;
    bool precomp;
    bool tracks82;
    bool *running;
    int* tracksA; // Pointer to tracksA array
    int* tracksB; // Pointer to tracksB array
} ThreadParams;

static void LoadF2DFile() {
    GetASLFilename("File To Disk", fileNameWrite, PATH_MAX, FALSE);
}
static void LoadD2FFile() {
    GetASLFilename("Disk to File", fileNameRead, PATH_MAX, TRUE);
}

// Worker thread function
void* writeFunction(void* arg) {
    // Cast the argument back to the struct
    ThreadParams* params = (ThreadParams*) arg;
    ADFWriter *taskWriter = new ADFWriter();
    std::string filename = params->fileName;
    std::string portName = params->portName;
    int32_t mode = params->mode;
    bool verify = params->verify;
    bool precomp = params->precomp;
    bool hdMode = false;
    bool isSCP = true;
    bool isIPF = false;
    ArduinoFloppyReader::FirmwareVersion v;
    if (!taskWriter->openDevice(portName)) {
        ShowMessage("Write Error", "Error opening selected port", "OK");
        goto out;
    }

    if (!((mode == MODE_IPF) || (mode == MODE_SCP)))
    {
        if (!verify) {
            ShowMessage("Writing", "WARNING: It is STRONGLY recommended to write with verify turned on", "OK");
        }
    }

    // Detect disk speed/density
    v = taskWriter->getFirwareVersion();
    if (((v.major == 1) && (v.minor >= 9)) || (v.major > 1))
    {
        if ((!isSCP) && (!isIPF))
            if (taskWriter->GuessDiskDensity(hdMode) != ArduinoFloppyReader::ADFResult::adfrComplete)
            {
                ShowMessage("Error Writing", "Unable to work out the density of the disk inserted", "OK");
                goto out;
            }
    }

    struct sched_param param;
    param.sched_priority = 10;
    pthread_setschedparam(pthread_self(), 0, &param);

    ADFResult result;

    switch (mode)
    {
        case MODE_IPF:
        {
            result = taskWriter->IPFToDisk(filename, false, [params](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
            {
                if (isVerifyError) {
                    int ret = ShowMessage("Writing Error", "Disk write verify error on current track", "Retry|Ignore|Abort");
                    switch (ret)
                    {
                        case 0:
                            return WriteResponse::wrAbort;
                        case 2:
                            pthread_mutex_lock(&arrayMutex);
                            if (currentSide == DiskSurface::dsUpper)
                                params->tracksA[currentTrack] = 2;
                            else
                                params->tracksB[currentTrack] = 2;
                            pthread_mutex_unlock(&arrayMutex);
                            return WriteResponse::wrSkipBadChecksums;
                        case 1:
                            return WriteResponse::wrRetry;
                    }
                }
                pthread_mutex_lock(&arrayMutex);
                if (currentSide == DiskSurface::dsUpper)
                    params->tracksA[currentTrack] = 1;
                else
                    params->tracksB[currentTrack] = 1;
                pthread_mutex_unlock(&arrayMutex);
                if (*params->running)
                    return WriteResponse::wrContinue;
                else
                    return WriteResponse::wrAbort;
            });
        }
        break;

        case MODE_SCP:
        {
            result = taskWriter->SCPToDisk(filename, false, [params](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
            {
                pthread_mutex_lock(&arrayMutex);
                if (currentSide == DiskSurface::dsUpper)
                    params->tracksA[currentTrack] = 1;
                else
                    params->tracksB[currentTrack] = 1;
                pthread_mutex_unlock(&arrayMutex);

                if (*params->running)
                    return WriteResponse::wrContinue;
                else
                    return WriteResponse::wrAbort;
            });
        }
            break;
        case MODE_ADF:
        {
            result = taskWriter->ADFToDisk(filename, hdMode, verify, true, precomp, true, [params](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
            {
                if (isVerifyError) {
                    int ret = ShowMessage("Writing Error", "Disk write verify error on current track", "Retry|Ignore|Abort");
                    switch (ret)
                    {
                        case 0:
                            return WriteResponse::wrAbort;
                        case 2:
                            pthread_mutex_lock(&arrayMutex);
                            if (currentSide == DiskSurface::dsUpper)
                                params->tracksA[currentTrack] = 2;
                            else
                                params->tracksB[currentTrack] = 2;
                            pthread_mutex_unlock(&arrayMutex);
                            return WriteResponse::wrSkipBadChecksums;
                        case 1:
                            return WriteResponse::wrRetry;
                    }
                }
                pthread_mutex_lock(&arrayMutex);
                if (currentSide == DiskSurface::dsUpper)
                    params->tracksA[currentTrack] = 1;
                else
                    params->tracksB[currentTrack] = 1;
                pthread_mutex_unlock(&arrayMutex);

                if (*params->running)
                    return WriteResponse::wrContinue;
                else
                    return WriteResponse::wrAbort;
            });
            break;
        }
        case MODE_IMG:
        case MODE_ST:
        {
            result = taskWriter->sectorFileToDisk(filename, hdMode, verify, true, false, mode == MODE_ST, [params](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
            {
                if (isVerifyError) {
                    int ret = ShowMessage("Writing Error", "Disk write verify error on current track", "Retry|Ignore|Abort");
                    switch (ret)
                    {
                        case 0:
                            return WriteResponse::wrAbort;
                        case 2:
                            pthread_mutex_lock(&arrayMutex);
                            if (currentSide == DiskSurface::dsUpper)
                                params->tracksA[currentTrack] = 2;
                            else
                                params->tracksB[currentTrack] = 2;
                            pthread_mutex_unlock(&arrayMutex);
                            return WriteResponse::wrSkipBadChecksums;
                        case 1:
                            return WriteResponse::wrRetry;
                    }
                }
                pthread_mutex_lock(&arrayMutex);
                if (currentSide == DiskSurface::dsUpper)
                    params->tracksA[currentTrack] = 1;
                else
                    params->tracksB[currentTrack] = 1;
                pthread_mutex_unlock(&arrayMutex);

                if (*params->running)
                    return WriteResponse::wrContinue;
                else
                    return WriteResponse::wrAbort;
            });
            break;
        }
    }

    switch (result)
    {
        case ADFResult::adfrBadSCPFile:
            ShowMessage("Write", "Bad, invalid or unsupported SCP file", "OK");
            break;
        case ADFResult::adfrComplete:
            ShowMessage("Write", "File written to disk", "OK");
            break;
        case ADFResult::adfrExtendedADFNotSupported:
            ShowMessage("Write", "Extended ADF files are not currently supported", "OK");
            break;
        case ADFResult::adfrMediaSizeMismatch:
            if (isIPF)
                ShowMessage("Write", "IPF writing is only supported for DD disks and images", "OK");
            else if (isSCP)
                ShowMessage("Write", "SCP writing is only supported for DD disks and images", "OK");
            else if (hdMode)
                ShowMessage("Write", "Disk in drive was detected as HD, but a DD ADF file supplied", "OK");
            else
                ShowMessage("Write", "Disk in drive was detected as DD, but an HD ADF file supplied", "OK");
            break;
        case ADFResult::adfrFirmwareTooOld:
            ShowMessage("Write", "Cannot write this file, you need to upgrade the firmware first", "OK");
            break;
        case ADFResult::adfrCompletedWithErrors:
            ShowMessage("Write", "File written to disk but there were errors during verification", "OK");
            break;
        case ADFResult::adfrAborted:
            ShowMessage("Write", "Writing aborted", "OK");
            break;
        case ADFResult::adfrFileError:
            ShowMessage("Write", "Error opening file", "OK");
            break;
        case ADFResult::adfrIPFLibraryNotAvailable:
            ShowMessage("Write", "IPF CAPSImg from Software Preservation Society Library Missing", "OK");
            break;
        case ADFResult::adfrDriveError:
            ShowMessage("Write", "Error communicating with the DrawBridge interface", "OK");
            break;
        case ADFResult::adfrDiskWriteProtected:
            ShowMessage("Write", "Error, disk is write protected", "OK");
            break;
        default:
            ShowMessage("Write", "An unknown error occurred ", "OK");
            break;
    }
out:
    delete taskWriter;

    // Mark the work as done
    isWorking = false;

    // Free the allocated memory for the struct
    if (params != NULL) {
        free(params);
        params = NULL;
    }

    return NULL;
}

void* readFunction(void* arg) {
    // Cast the argument back to the struct
    ThreadParams* params = (ThreadParams*) arg;
    ADFWriter *taskWriter = new ADFWriter();
    std::string file = params->fileName;
    std::string portName = params->portName;
    int32_t mode = params->mode;
    bool hdMode = params->hdMode;
    bool tracks82 = params->tracks82;

    if (!taskWriter->openDevice(portName)) {
        ShowMessage("Write Error", "Error opening selected port", "OK");
        isReading = false;
        return NULL;
    }

    // Detect disk speed
    const ArduinoFloppyReader::FirmwareVersion v = taskWriter->getFirwareVersion();

    // Get the current firmware version.  Only valid if openDevice is successful
    if ((v.major == 1) && (v.minor < 8))
    {
        if (mode == MODE_SCP)
        {
            ShowMessage("Error reading","This requires firmware V1.8 or newer.", "OK");
            isReading = false;
            return NULL;
        }
    }

    struct sched_param param;
    param.sched_priority = 10;
    pthread_setschedparam(pthread_self(), 0, &param);

    auto callback = [params](const int currentTrack, const DiskSurface currentSide, const int retryCounter, const int sectorsFound, const int badSectorsFound, const int totalSectors, const CallbackOperation operation) -> WriteResponse
    {
        if (retryCounter > 20)
        {
            int ret = ShowMessage("Read Error", "Disk has checksum errors/missing data.", "Retry|Ignore|Abort");
            switch (ret)
            {
                case 0:
                    return WriteResponse::wrAbort;
                case 2:
                    pthread_mutex_lock(&arrayMutex);
                    if (currentSide == DiskSurface::dsUpper)
                        params->tracksA[currentTrack] = 2;
                    else
                        params->tracksB[currentTrack] = 2;
                    pthread_mutex_unlock(&arrayMutex);
                    return WriteResponse::wrSkipBadChecksums;
                case 1:
                    return WriteResponse::wrRetry;
            }
        }

        pthread_mutex_lock(&arrayMutex);
        if (currentSide == DiskSurface::dsUpper)
            params->tracksA[currentTrack] = 1;
        else
            params->tracksB[currentTrack] = 1;
        pthread_mutex_unlock(&arrayMutex);

        if (*params->running)
            return WriteResponse::wrContinue;
        else
            return WriteResponse::wrAbort;
    };
    
    int tracks = tracks82 ? 82 : 80;
    ADFResult result;
    switch (mode)
    {
        case MODE_ADF:
            result = taskWriter->DiskToADF(params->fileName, hdMode, tracks, callback);
            break;
        case MODE_SCP:
            result = taskWriter->DiskToSCP(params->fileName, hdMode, tracks, 3, callback);
            break;
        case MODE_ST:
        case MODE_IMG:
            result = taskWriter->diskToIBMST(params->fileName, hdMode, callback);
            break;
    }

    switch (result)
    {
        case ADFResult::adfrComplete:
            ShowMessage("Reading", "File created successfully", "OK");
            break;
        case ADFResult::adfrAborted:
            ShowMessage("Reading", "File aborted", "OK");
            break;
        case ADFResult::adfrFileError:
            ShowMessage("Reading", "Error creating file", "OK");
            break;
        case ADFResult::adfrFileIOError:
            ShowMessage("Reading", "Error writing to file", "OK");
            break;
        case ADFResult::adfrFirmwareTooOld:
            ShowMessage("Reading", "This requires firmware V1.8 or newer", "OK");
            break;
        case ADFResult::adfrCompletedWithErrors:
            ShowMessage("Reading", "File created with partial success", "OK");
            break;
        case ADFResult::adfrDriveError:
            ShowMessage("Reading", "Error communicating with the Arduino interface", "OK");
            break;
        default:
            ShowMessage("Reading", "An unknown error occured", "OK");
            break;
    }

    delete taskWriter;

    // Mark the work as done
    isWorking = false;

    // Free the allocated memory for the struct
    if (params != NULL) {
        free(params);
        params = NULL;
    }

    return NULL;
}

static void StartWrite(std::string portName, bool verify, bool pcw, int tracksA[83], int tracksB[83]) {
    stopWorking = false;
    if (!std::filesystem::exists(fileNameWrite)) {
        ShowMessage("Write Error", "The selected file doesn't exists! Cannot write the file to floppy disk", "OK");
        isWriting = false;
        return;
    }
    if (fileNameWrite[0] == '\0') {
        isWriting = false;
        return;
    }

    const char *extension = strstr(fileNameWrite, ".");
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
        ShowMessage("Error writing", "File extension not recognised. It must be one of: .ADF, .IMG, .IMA, .ST or .SCP", "OK");

        isWriting = false;
        return;
    }

    bool hdMode = false;

    /* reset tracks */
    pthread_mutex_lock(&arrayMutex);
    for (int i = 0; i < 83; i++) {
        tracksA[i] = 0;
        tracksB[i] = 0;
    }
    pthread_mutex_unlock(&arrayMutex);

    isWorking = true;

    // Allocate memory for the thread parameters
    ThreadParams* params = (ThreadParams*) malloc(sizeof(ThreadParams));
    if (params != NULL) {
        params->tracksA = tracksA;
        params->tracksB = tracksB;
        params->fileName = fileNameWrite;
        params->running = &isWorking;
        params->mode = mode;
        params->hdMode = hdMode;
        params->portName = portName.c_str();
        params->verify = verify;
        params->precomp = pcw;
        pthread_create(&workerThread, NULL, writeFunction, params);
    }
    else {
        isWorking = false;
        ShowMessage("Error writing", "Error allocating memory for the I/O thread", "OK");
    }
    isWriting = false;
}

static void StartRead(std::string portName, bool verify, bool tracks82, int tracksA[83], int tracksB[83]) {
    stopWorking = false;

    if (fileNameRead[0] == '\0') {
        isReading = false;
        return;
    }

    const char *extension = strstr(fileNameRead, ".");
    int32_t mode = -1;
    bool hdMode = false;

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

    if (mode < 0) {
        ShowMessage("Error reading", "File extension not recognised. It must be one of: .ADF, .IMG, .IMA, .ST or .SCP", "OK");
        isReading = false;
        return;
    }

    /* reset tracks */
    pthread_mutex_lock(&arrayMutex);
    for (int i = 0; i < 83; i++) {
        tracksA[i] = 0;
        tracksB[i] = 0;
    }
    pthread_mutex_unlock(&arrayMutex);

    isWorking = true;

    // Allocate memory for the thread parameters
    ThreadParams* params = (ThreadParams*) malloc(sizeof(ThreadParams));
    if (params != NULL) {
        params->tracksA = tracksA;
        params->tracksB = tracksB;
        params->fileName = fileNameRead;
        params->running = &isWorking;
        params->mode = mode;
        params->hdMode = hdMode;
        params->portName = portName.c_str();
        params->verify = verify;
        params->tracks82 = tracks82;
        int pid = pthread_create(&workerThread, NULL, readFunction, params);
    }
    else {
        isWorking = false;
        ShowMessage("Error writing", "Error allocating memory for the I/O thread", "OK");
    }

    isReading = false;
}

int main() {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1024;
    const int screenHeight = 594;
    float startTracksAx = 392.0, startTracksAy = 237.0;
    float startTracksBx = 721.0, startTracksBy = 237.0;
    float gap = 6.7f;
    const float trackRectWidth = 21.0, trackRectHeight = 21.0;
    bool writeEditMode = false, readEditMode = false, verify = true, pcw = true, tracks82 = false, hdSelection = false;
    int portIndex = 0, portNumbers = 0, dPortIndex = 0;
    const char *portList[MAX_PORTS];
    int tracksA[83] = {0}, tracksB[83] = {0};

    SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "WAFFLE-Copy-Professional");

    Texture2D background = LoadTexture("WaffleUI/main.png");
    Texture2D leftLogo = LoadTexture("WaffleUI/leftLogo.png");
    Texture2D rightLogo = LoadTexture("WaffleUI/rightLogo.png");
    Texture2D waffleCopyPRO = LoadTexture("WaffleUI/waffleCopyPRO.png");
    Texture2D start = LoadTexture("WaffleUI/Start.png");
    Texture2D nStart = LoadTexture("WaffleUI/sStart.png");
    Texture2D serialPort = LoadTexture("WaffleUI/SerialPort.png");
    Font topazFont = LoadFont("WaffleUI/TopazPlus_a500_v1.0.ttf");
    GuiSetFont(topazFont);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xffffffff);
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, 0x686868ff);
    GuiSetStyle(DROPDOWNBOX, TEXT_COLOR_NORMAL, 0x686868ff);
    GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED, 0xffffffff);

    //SetTargetFPS(15);

    const char *floatingText = "--- WAFFLE Copy Professional -- The essential USB floppy drive solution for the real Amiga user."
                               " It allows you to write from ADF files, to read floppy disks as ADF, IPF, IMA, IMG, ST and SCP format and, thanks to a specific version Amiga Emulator, like WinUAE (by Toni Wilen) or AmiBerry (by MiDWaN), "
                               " it works like a real Amiga disk drive allowing you to directly read and write your floppies through an emulator! "
                               "Sometime you may need a special USB cable (Y-Type) with the possibility of "
                               "double powering if the USB port of the PC is not powerful enough.";
    int textWidth = MeasureText(floatingText, 14);
    Vector2 textPosition = { 1000, 550 };
    float amplitude = 15.0f; // Vertical oscillation amplitude
    float frequency = 1.4f;   // Speed of oscillation
    float time = 0.0f;        // Time counter for sine wave
    float scrollSpeed = 50.0f; // Horizontal scroll speed (pixels per second)

    std::vector<std::string> portsList;
    ArduinoFloppyReader::ArduinoInterface::enumeratePorts(portsList);
    if (portsList.size() > 0) {
        for (const std::string &port : portsList) {
            portList[portNumbers] = (const char *) malloc(128);
            memset((void *) portList[portNumbers], 0, 128);
            strncpy((char *) portList[portNumbers], port.c_str(), 127);
            portNumbers++;
        }
    }

    //--------------------------------------------------------------------------------------
    // Main game loop
    while (!WindowShouldClose() || isWorking)    // Detect window close button or ESC key
    {
        if (portNumbers > 0) {
            if (!isWorking) {
                GuiEnable();
                if (loadF2DFile)
                    LoadF2DFile();
                if (loadD2FFile)
                    LoadD2FFile();
                if (isWriting) {
                    StartWrite(portList[portIndex], verify, pcw, tracksA, tracksB);
                }
                if (isReading) {
                    StartRead(portList[portIndex], verify, tracks82, tracksA, tracksB);
                }
            }
            else
                GuiDisable();
        }
        else
            GuiDisable();

        if (stopWorking) {
            isWorking = false;
            pthread_join(workerThread, NULL);
        }

        BeginDrawing();
            DrawTexture(background, 0, 0, WHITE);
            DrawTexture(waffleCopyPRO, 165, 15, WHITE);
            DrawTexture(leftLogo, 15, 15, WHITE);
            DrawTexture(rightLogo, 865, 15, WHITE);
            for (int i = 0; i < 83; i++) {
                // Calculate row and column from the index
                int row = i / 10; // Integer division to get the row
                int col = i % 10; // Modulo operation to get the column

                // Adjust position and size to account for the gap
                float x = col * (trackRectWidth + gap);
                float y = row * (trackRectHeight + gap);

                Vector2 posA = {startTracksAx + x, startTracksAy + y};
                Vector2 posB = {startTracksBx + x, startTracksBy + y};

                Vector2 size = {trackRectWidth, trackRectHeight};
                pthread_mutex_lock(&arrayMutex);
                if (tracksA[i] == 1) {
                    DrawRectangleV(posA, size, GREEN);
                }
                else if (tracksA[i] == 2) {
                    DrawRectangleV(posA, size, RED);
                }
                else {
                    DrawRectangleV(posA, size, BLACK);
                }

                if (tracksB[i] == 1) {
                    DrawRectangleV(posB, size, GREEN);
                }
                else if (tracksB[i] == 2) {
                    DrawRectangleV(posB, size, RED);
                }
                else {
                    DrawRectangleV(posB, size, BLACK);
                }
                pthread_mutex_unlock(&arrayMutex);
            }

            if (GuiTextBox((Rectangle){ 20, 170, 290, 25 }, fileNameWrite, PATH_MAX - 1, writeEditMode))
                writeEditMode = !writeEditMode;
            if (!writeEditMode && fileNameWrite[0] == '\0') {
                DrawTextEx(topazFont, "Select ADF to write to Waffle", (Vector2) {22.0, 176.0}, 13, 1, GRAY);
            }

            if (GuiButton((Rectangle){ 310, 170, 25, 25 }, "..."))
                loadF2DFile = true;

            if (GuiButton((Rectangle){ 120, 300, 105, 20 }, "Start Write"))
                isWriting = true;

            if (GuiButton((Rectangle){ 120, 450, 105, 20 }, "Start Read"))
                isReading = true;

            GuiCheckBox((Rectangle) { 20, 275, 15, 15 }, "Verify", &verify);
            GuiCheckBox((Rectangle) { 260, 275, 15, 15 }, "PCW", &pcw);

            if (GuiTextBox((Rectangle){ 20, 330, 290, 25 }, fileNameRead, PATH_MAX - 1, readEditMode))
                readEditMode = !readEditMode;
            if (!readEditMode && fileNameRead[0] == '\0') {
                DrawTextEx(topazFont, "Select file name to read from Waffle", (Vector2) {22.0, 336.0}, 13, 1, GRAY);
            }

            if (GuiButton((Rectangle){ 310, 330, 25, 25 }, "..."))
                loadD2FFile = true;

            if (GuiCheckBox((Rectangle) { 20, 452, 15, 15 }, "Tracks: 82", &tracks82)) tracks82 = !tracks82;
            if (GuiCheckBox((Rectangle) { 650, 515, 15, 15 }, "HIGH DENSITY DISK SELECTION", &hdSelection)) hdSelection = !hdSelection;
            GuiLabel((Rectangle){ 70, 510, 150, 25 }, "WAFFLE DRIVE PORT: ");
            if (!isReading && !isWriting)
                GuiDropdownBox((Rectangle){ 200, 510, 150, 25 }, TextJoin(portList, portNumbers, ";"), &portIndex, false);
            else
                GuiDropdownBox((Rectangle){ 200, 510, 150, 25 }, TextJoin(portList, portNumbers, ";"), &dPortIndex, false);

            if (isWorking) {
                if (GuiEnabledButton((Rectangle) {screenWidth / 2 - 60, 510, 120, 25}, "STOP"))
                    stopWorking = true;
            }

            float deltaTime = GetFrameTime(); // Get time since last frame
            time += deltaTime; // Increment time for sine wave

            // Update horizontal position (scroll from right to left)
            textPosition.x -= scrollSpeed * deltaTime;

            // Reset position to the right if the text goes off the screen
            if (textPosition.x + textWidth < 0) {
                textPosition.x = screenWidth - 20;
            }

            textPosition.y = (float)550 + sinf(time * frequency) * amplitude;
            DrawTextEx(topazFont, floatingText, textPosition, 12, 2, WHITE);
            DrawRectangle(0, 500, 12, 84, BLACK);
            DrawRectangle(screenWidth - 12, 500, 20, 84, BLACK);

            DrawRectangleLinesEx((Rectangle) {12, 497, 1000, 84}, 3, GetColor(0xF3F3F3FF));

            //DrawFPS(30, screenHeight - 40);
        EndDrawing();
    }

    if (isWorking) {
        isWorking = false;
        pthread_join(workerThread, NULL);
    }

    if (portNumbers > 0) {
        for (int i = 0; i < portNumbers; i++) {
            if (portList[i] != NULL)
                free((void *)portList[i]);
        }
    }
    UnloadFont(topazFont);

    UnloadTexture(serialPort);
    UnloadTexture(start);
    UnloadTexture(nStart);
    UnloadTexture(waffleCopyPRO);
    UnloadTexture(leftLogo);
    UnloadTexture(rightLogo);
    UnloadTexture(background);

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}