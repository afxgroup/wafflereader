#include <stdlib.h>
#include "gui_common.hpp"
#include "common.hpp"

static const char __attribute__((used)) *version = "$VER: Waffle Copy Professional GUI Version 2.8.8 for AmigaOS4 (" __DATE__ ")";

static void __attribute__((constructor(99))) gui_constructor(void)
{
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

// Worker thread function
void *writeFunction(void *arg)
{
    // Cast the argument back to the struct
    ThreadParams *params = (ThreadParams *)arg;
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
    if (!taskWriter->openDevice(portName))
    {
        ShowMessage(GetString(MSG_PROGRAM_NAME), GetString(MSG_ERROR_OPENING_PORT), GetString(MSG_BUTTON_OK));
        isWorking = false;
        goto out;
    }

    if (!((mode == MODE_IPF) || (mode == MODE_SCP)))
    {
        if (!verify)
        {
            ShowMessage(GetString(MSG_PROGRAM_NAME), GetString(MSG_WARNING_VERIFY_RECOMMENDED), GetString(MSG_BUTTON_OK));
        }
    }

    // Detect disk speed/density
    v = taskWriter->getFirwareVersion();
    if (((v.major == 1) && (v.minor >= 9)) || (v.major > 1))
    {
        if ((!isSCP) && (!isIPF))
            if (taskWriter->GuessDiskDensity(hdMode) != ArduinoFloppyReader::ADFResult::adfrComplete)
            {
                ShowMessage(GetString(MSG_PROGRAM_NAME), GetString(MSG_UNABLE_DENSITY), GetString(MSG_BUTTON_OK));
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
                    int ret = ShowMessage(PROGRAM_NAME, "Disk write verify error on current track", "Retry|Ignore|Abort");
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
                    return WriteResponse::wrAbort; });
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
                    return WriteResponse::wrAbort; });
    }
    break;
    case MODE_ADF:
    {
        result = taskWriter->ADFToDisk(filename, hdMode, verify, true, precomp, true, [params](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
                                       {
                if (isVerifyError) {
                    int ret = ShowMessage(PROGRAM_NAME, "Disk write verify error on current track", "Retry|Ignore|Abort");
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
                    return WriteResponse::wrAbort; });
        break;
    }
    case MODE_IMG:
    case MODE_ST:
    {
        result = taskWriter->sectorFileToDisk(filename, hdMode, verify, true, false, mode == MODE_ST, [params](const int currentTrack, const DiskSurface currentSide, bool isVerifyError, const CallbackOperation operation) -> WriteResponse
                                              {
                if (isVerifyError) {
                    int ret = ShowMessage(PROGRAM_NAME, "Disk write verify error on current track", "Retry|Ignore|Abort");
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
                    return WriteResponse::wrAbort; });
        break;
    }
    }

    switch (result)
    {
    case ADFResult::adfrBadSCPFile:
        ShowMessage(PROGRAM_NAME, LS(BAD_SCP_FILE), LS(BUTTON_OK));
        break;
    case ADFResult::adfrComplete:
        ShowMessage(PROGRAM_NAME, LS(FILE_WRITTEN), LS(BUTTON_OK));
        break;
    case ADFResult::adfrExtendedADFNotSupported:
        ShowMessage(PROGRAM_NAME, LS(EXTENDED_ADF_NOT_SUPPORTED), LS(BUTTON_OK));
        break;
    case ADFResult::adfrMediaSizeMismatch:
        if (isIPF)
            ShowMessage(PROGRAM_NAME, LS(IPF_ONLY_DD), LS(BUTTON_OK));
        else if (isSCP)
            ShowMessage(PROGRAM_NAME, LS(SCP_ONLY_DD), LS(BUTTON_OK));
        else if (hdMode)
            ShowMessage(PROGRAM_NAME, LS(DISK_HD_FILE_DD), LS(BUTTON_OK));
        else
            ShowMessage(PROGRAM_NAME, LS(DISK_DD_FILE_HD), LS(BUTTON_OK));
        break;
    case ADFResult::adfrFirmwareTooOld:
        ShowMessage(PROGRAM_NAME, LS(FIRMWARE_TOO_OLD), LS(BUTTON_OK));
        break;
    case ADFResult::adfrCompletedWithErrors:
        ShowMessage(PROGRAM_NAME, LS(FILE_WRITTEN_ERRORS), LS(BUTTON_OK));
        break;
    case ADFResult::adfrAborted:
        ShowMessage(PROGRAM_NAME, LS(WRITING_ABORTED), LS(BUTTON_OK));
        break;
    case ADFResult::adfrFileError:
        ShowMessage(PROGRAM_NAME, LS(ERROR_OPENING_FILE), LS(BUTTON_OK));
        break;
    case ADFResult::adfrIPFLibraryNotAvailable:
        ShowMessage(PROGRAM_NAME, LS(IPF_LIBRARY_MISSING), LS(BUTTON_OK));
        break;
    case ADFResult::adfrDriveError:
        ShowMessage(PROGRAM_NAME, LS(ERROR_COMM_DRAWBRIDGE), LS(BUTTON_OK));
        break;
    case ADFResult::adfrDiskWriteProtected:
        ShowMessage(PROGRAM_NAME, LS(DISK_WRITE_PROTECTED), LS(BUTTON_OK));
        break;
    default:
        ShowMessage(PROGRAM_NAME, LS(UNKNOWN_ERROR), LS(BUTTON_OK));
        break;
    }
out:
    delete taskWriter;

    // Mark the work as done
    isWorking = false;

    // Free the allocated memory for the struct
    if (params != NULL)
    {
        free(params);
        params = NULL;
    }

    return NULL;
}

void *readFunction(void *arg)
{
    // Cast the argument back to the struct
    ThreadParams *params = (ThreadParams *)arg;
    ADFWriter *taskWriter = new ADFWriter();
    std::string file = params->fileName;
    std::string portName = params->portName;
    int32_t mode = params->mode;
    bool hdMode = params->hdMode;
    bool tracks82 = params->tracks82;

    if (!taskWriter->openDevice(portName))
    {
        ShowMessage(PROGRAM_NAME, LS(ERROR_OPENING_PORT), LS(BUTTON_OK));
        isReading = false;
        isWorking = false;
        return NULL;
    }

    // Detect disk speed
    const ArduinoFloppyReader::FirmwareVersion v = taskWriter->getFirwareVersion();

    // Get the current firmware version.  Only valid if openDevice is successful
    if ((v.major == 1) && (v.minor < 8))
    {
        if (mode == MODE_SCP)
        {
            ShowMessage(PROGRAM_NAME, LS(FIRMWARE_V18_REQUIRED_DOT), LS(BUTTON_OK));
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
            int ret = ShowMessage(PROGRAM_NAME, LS(DISK_CHECKSUM_ERROR), "Retry|Ignore|Abort");
            switch (ret)
            {
            case 0:
                return WriteResponse::wrAbort;
            case 2:
                pthread_mutex_lock(&arrayMutex);
                if (currentSide == DiskSurface::dsUpper)
                {
                    params->tracksA[currentTrack] = 2;
                }
                else
                {
                    params->tracksB[currentTrack] = 2;
                }
                pthread_mutex_unlock(&arrayMutex);
                return WriteResponse::wrSkipBadChecksums;
            case 1:
                return WriteResponse::wrRetry;
            }
        }

        pthread_mutex_lock(&arrayMutex);
        if (currentSide == DiskSurface::dsUpper)
        {
            params->tracksA[currentTrack] = 1;
        }
        else
        {
            params->tracksB[currentTrack] = 1;
        }
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
        ShowMessage(PROGRAM_NAME, LS(FILE_CREATED), LS(BUTTON_OK));
        break;
    case ADFResult::adfrAborted:
        ShowMessage(PROGRAM_NAME, LS(FILE_ABORTED), LS(BUTTON_OK));
        std::remove(file.c_str());
        break;
    case ADFResult::adfrFileError:
        ShowMessage(PROGRAM_NAME, LS(ERROR_CREATING_FILE), LS(BUTTON_OK));
        break;
    case ADFResult::adfrFileIOError:
        ShowMessage(PROGRAM_NAME, LS(ERROR_WRITING_FILE), LS(BUTTON_OK));
        break;
    case ADFResult::adfrFirmwareTooOld:
        ShowMessage(PROGRAM_NAME, LS(FIRMWARE_V18_REQUIRED), LS(BUTTON_OK));
        break;
    case ADFResult::adfrCompletedWithErrors:
        ShowMessage(PROGRAM_NAME, LS(FILE_CREATED_PARTIAL), LS(BUTTON_OK));
        break;
    case ADFResult::adfrDriveError:
        ShowMessage(PROGRAM_NAME, LS(ERROR_COMM_ARDUINO), LS(BUTTON_OK));
        break;
    default:
        ShowMessage(PROGRAM_NAME, LS(UNKNOWN_ERROR_OCCURRED), LS(BUTTON_OK));
        break;
    }

    delete taskWriter;

    // Mark the work as done
    isWorking = false;

    // Free the allocated memory for the struct
    if (params != NULL)
    {
        free(params);
        params = NULL;
    }

    return NULL;
}

void StartWrite(std::string portName, bool verify, bool pcw, int tracksA[83], int tracksB[83])
{
    stopWorking = false;
    if (!std::filesystem::exists(fileNameWrite))
    {
        ShowMessage(PROGRAM_NAME, LS(FILE_DOESNT_EXIST), LS(BUTTON_OK));
        isWriting = false;
        return;
    }
    if (fileNameWrite[0] == '\0')
    {
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
        ShowMessage("Error writing", LS(FILE_EXT_NOT_RECOGNIZED), LS(BUTTON_OK));

        isWriting = false;
        return;
    }

    bool hdMode = false;

    /* reset tracks */
    pthread_mutex_lock(&arrayMutex);
    for (int i = 0; i < 83; i++)
    {
        tracksA[i] = 0;
        tracksB[i] = 0;
    }
    pthread_mutex_unlock(&arrayMutex);

    isWorking = true;

    // Allocate memory for the thread parameters
    ThreadParams *params = (ThreadParams *)malloc(sizeof(ThreadParams));
    if (params != NULL)
    {
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
    else
    {
        isWorking = false;
        ShowMessage(PROGRAM_NAME, LS(ERROR_ALLOCATING_MEMORY), LS(BUTTON_OK));
    }
    isWriting = false;
}

void StartRead(std::string portName, bool verify, bool tracks82, int tracksA[83], int tracksB[83])
{
    stopWorking = false;

    if (fileNameRead[0] == '\0')
    {
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

    if (mode < 0)
    {
        ShowMessage(PROGRAM_NAME, LS(FILE_EXT_NOT_RECOGNIZED), LS(BUTTON_OK));
        isReading = false;
        return;
    }

    /* reset tracks */
    pthread_mutex_lock(&arrayMutex);
    for (int i = 0; i < 83; i++)
    {
        tracksA[i] = 0;
        tracksB[i] = 0;
    }
    pthread_mutex_unlock(&arrayMutex);

    isWorking = true;

    // Allocate memory for the thread parameters
    ThreadParams *params = (ThreadParams *)malloc(sizeof(ThreadParams));
    if (params != NULL)
    {
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
    else
    {
        isWorking = false;
        ShowMessage(PROGRAM_NAME, LS(ERROR_ALLOCATING_MEMORY), LS(BUTTON_OK));
    }

    isReading = false;
}