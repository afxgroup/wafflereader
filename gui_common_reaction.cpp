#include <stdlib.h>
#include "gui_common.hpp"
#include "common.hpp"

#include <proto/intuition.h>
#include <intuition/gadgetclass.h>

static const char __attribute__((used)) *version = "$VER: Waffle Copy Professional GUI Version 2.8.8 for AmigaOS4 (" __DATE__ ")";

using namespace ArduinoFloppyReader;

char fileNameWrite[PATH_MAX] = "", fileNameRead[PATH_MAX] = "";
bool loadF2DFile = false, loadD2FFile = false;
bool stopWorking = false;
bool isWorking = false;
bool isWriting = false;
pthread_t workerThread;

extern Object *Objects[OBJ_MAX];
#define GAD(x) (struct Gadget *) Objects[x]

// Worker thread function
void* writeFunction(void *args) {
    ThreadParams* params = (ThreadParams*) args;

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
        ShowMessage(PROGRAM_NAME, "Error opening selected port", "OK");
        goto out;
    }

    if (!((mode == MODE_IPF) || (mode == MODE_SCP)))
    {
        if (!verify) {
            ShowMessage(PROGRAM_NAME, "WARNING: It is STRONGLY recommended to write with verify turned on", "OK");
        }
    }

    // Detect disk speed/density
    v = taskWriter->getFirwareVersion();
    if (((v.major == 1) && (v.minor >= 9)) || (v.major > 1))
    {
        if ((!isSCP) && (!isIPF))
            if (taskWriter->GuessDiskDensity(hdMode) != ArduinoFloppyReader::ADFResult::adfrComplete)
            {
                ShowMessage(PROGRAM_NAME, "Unable to work out the density of the disk inserted", "OK");
                goto out;
            }
    }

    SetGadgetAttrs(GAD(OBJ_LEFT_COL), params->window, NULL, GA_Disabled, TRUE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_BOTTOM_ROW), params->window, NULL, GA_Disabled, TRUE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_START_WRITE), params->window, NULL, GA_Disabled, FALSE, GA_Text, "Stop", TAG_DONE);
    RefreshGList(GAD(OBJ_LEFT_COL), params->window, NULL, -1);
    RefreshGList(GAD(OBJ_BOTTOM_ROW), params->window, NULL, -1);

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
                            if (currentSide == DiskSurface::dsUpper)
                                UpdateTrack(params->tracksA, 0, currentTrack, 2, params->window);
                            else
                                UpdateTrack(params->tracksB, 1, currentTrack, 2, params->window);
                            return WriteResponse::wrSkipBadChecksums;
                        case 1:
                            return WriteResponse::wrRetry;
                    }
                }
                if (currentSide == DiskSurface::dsUpper)
                    UpdateTrack(params->tracksA, 0, currentTrack, 1, params->window);
                else
                    UpdateTrack(params->tracksB, 1, currentTrack, 1, params->window);
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
                if (currentSide == DiskSurface::dsUpper)
                    UpdateTrack(params->tracksA, 0, currentTrack, 1, params->window);
                else
                    UpdateTrack(params->tracksB, 1, currentTrack, 1, params->window);
        
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
                    int ret = ShowMessage(PROGRAM_NAME, "Disk write verify error on current track", "Retry|Ignore|Abort");
                    switch (ret)
                    {
                        case 0:
                            return WriteResponse::wrAbort;
                        case 2:
                            if (currentSide == DiskSurface::dsUpper)
                                UpdateTrack(params->tracksA, 0, currentTrack, 2, params->window);
                            else
                                UpdateTrack(params->tracksB, 1, currentTrack, 2, params->window);
                            return WriteResponse::wrSkipBadChecksums;
                        case 1:
                            return WriteResponse::wrRetry;
                    }
                }
                if (currentSide == DiskSurface::dsUpper)
                    UpdateTrack(params->tracksA, 0, currentTrack, 1, params->window);
                else
                    UpdateTrack(params->tracksB, 1, currentTrack, 1, params->window);
        
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
                    int ret = ShowMessage(PROGRAM_NAME, "Disk write verify error on current track", "Retry|Ignore|Abort");
                    switch (ret)
                    {
                        case 0:
                            return WriteResponse::wrAbort;
                        case 2:
                            if (currentSide == DiskSurface::dsUpper)
                                UpdateTrack(params->tracksA, 0, currentTrack, 2, params->window);
                            else
                                UpdateTrack(params->tracksB, 1, currentTrack, 2, params->window);
                            return WriteResponse::wrSkipBadChecksums;
                        case 1:
                            return WriteResponse::wrRetry;
                    }
                }
                if (currentSide == DiskSurface::dsUpper)
                    UpdateTrack(params->tracksA, 0, currentTrack, 1, params->window);
                else
                    UpdateTrack(params->tracksB, 1, currentTrack, 1, params->window);
        
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
            ShowMessage(PROGRAM_NAME, "Bad, invalid or unsupported SCP file", "OK");
            break;
        case ADFResult::adfrComplete:
            ShowMessage(PROGRAM_NAME, "File written to disk", "OK");
            break;
        case ADFResult::adfrExtendedADFNotSupported:
            ShowMessage(PROGRAM_NAME, "Extended ADF files are not currently supported", "OK");
            break;
        case ADFResult::adfrMediaSizeMismatch:
            if (isIPF)
                ShowMessage(PROGRAM_NAME, "IPF writing is only supported for DD disks and images", "OK");
            else if (isSCP)
                ShowMessage(PROGRAM_NAME, "SCP writing is only supported for DD disks and images", "OK");
            else if (hdMode)
                ShowMessage(PROGRAM_NAME, "Disk in drive was detected as HD, but a DD ADF file supplied", "OK");
            else
                ShowMessage(PROGRAM_NAME, "Disk in drive was detected as DD, but an HD ADF file supplied", "OK");
            break;
        case ADFResult::adfrFirmwareTooOld:
            ShowMessage(PROGRAM_NAME, "Cannot write this file, you need to upgrade the firmware first", "OK");
            break;
        case ADFResult::adfrCompletedWithErrors:
            ShowMessage(PROGRAM_NAME, "File written to disk but there were errors during verification", "OK");
            break;
        case ADFResult::adfrAborted:
            ShowMessage(PROGRAM_NAME, "Writing aborted", "OK");
            break;
        case ADFResult::adfrFileError:
            ShowMessage(PROGRAM_NAME, "Error opening file", "OK");
            break;
        case ADFResult::adfrIPFLibraryNotAvailable:
            ShowMessage(PROGRAM_NAME, "IPF CAPSImg from Software Preservation Society Library Missing", "OK");
            break;
        case ADFResult::adfrDriveError:
            ShowMessage(PROGRAM_NAME, "Error communicating with the DrawBridge interface", "OK");
            break;
        case ADFResult::adfrDiskWriteProtected:
            ShowMessage(PROGRAM_NAME, "Error, disk is write protected", "OK");
            break;
        default:
            ShowMessage(PROGRAM_NAME, "An unknown error occurred ", "OK");
            break;
    }
out:

    SetGadgetAttrs(GAD(OBJ_LEFT_COL), params->window, NULL, GA_Disabled, FALSE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_BOTTOM_ROW), params->window, NULL, GA_Disabled, FALSE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_START_WRITE), params->window, NULL, GA_Disabled, FALSE, GA_Text, "Start Write", TAG_DONE);
    RefreshGList(GAD(OBJ_LEFT_COL), params->window, NULL, -1);
    RefreshGList(GAD(OBJ_BOTTOM_ROW), params->window, NULL, -1);

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

void* readFunction(void *args) {
    ThreadParams* params = (ThreadParams*) args;
    ADFWriter *taskWriter = new ADFWriter();
    std::string file = params->fileName;
    std::string portName = params->portName;
    int32_t mode = params->mode;
    bool hdMode = params->hdMode;
    bool tracks82 = params->tracks82;

    if (!taskWriter->openDevice(portName)) {
        ShowMessage(PROGRAM_NAME, "Error opening selected port", "OK");
        return NULL;
    }

    // Detect disk speed
    const ArduinoFloppyReader::FirmwareVersion v = taskWriter->getFirwareVersion();

    // Get the current firmware version.  Only valid if openDevice is successful
    if ((v.major == 1) && (v.minor < 8))
    {
        if (mode == MODE_SCP)
        {
            ShowMessage(PROGRAM_NAME,"This requires firmware V1.8 or newer.", "OK");
            return NULL;
        }
    }

    SetGadgetAttrs(GAD(OBJ_LEFT_COL), params->window, NULL, GA_Disabled, TRUE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_BOTTOM_ROW), params->window, NULL, GA_Disabled, TRUE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_START_READ), params->window, NULL, GA_Disabled, FALSE, GA_Text, "Stop", TAG_DONE);
    RefreshGList(GAD(OBJ_LEFT_COL), params->window, NULL, -1);
    RefreshGList(GAD(OBJ_BOTTOM_ROW), params->window, NULL, -1);

    auto callback = [params](const int currentTrack, const DiskSurface currentSide, const int retryCounter, const int sectorsFound, const int badSectorsFound, const int totalSectors, const CallbackOperation operation) -> WriteResponse
    {
        if (retryCounter > 20)
        {
            int ret = ShowMessage(PROGRAM_NAME, "Disk has checksum errors/missing data.", "Retry|Ignore|Abort");
            switch (ret)
            {
                case 0:
                    return WriteResponse::wrAbort;
                case 2:
                    if (currentSide == DiskSurface::dsUpper) {
                        UpdateTrack(params->tracksA, 0, currentTrack, 2, params->window);
                    }
                    else {
                        UpdateTrack(params->tracksB, 1, currentTrack, 2, params->window);
                    }
                    return WriteResponse::wrSkipBadChecksums;
                case 1:
                    return WriteResponse::wrRetry;
            }
        }

        if (currentSide == DiskSurface::dsUpper) {
            UpdateTrack(params->tracksA, 0, currentTrack, 1, params->window);
        }
        else {
            UpdateTrack(params->tracksB, 1, currentTrack, 1, params->window);
        }

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
            ShowMessage(PROGRAM_NAME, "File created successfully", "OK");
            break;
        case ADFResult::adfrAborted:
            ShowMessage(PROGRAM_NAME, "File aborted", "OK");
            break;
        case ADFResult::adfrFileError:
            ShowMessage(PROGRAM_NAME, "Error creating file", "OK");
            break;
        case ADFResult::adfrFileIOError:
            ShowMessage(PROGRAM_NAME, "Error writing to file", "OK");
            break;
        case ADFResult::adfrFirmwareTooOld:
            ShowMessage(PROGRAM_NAME, "This requires firmware V1.8 or newer", "OK");
            break;
        case ADFResult::adfrCompletedWithErrors:
            ShowMessage(PROGRAM_NAME, "File created with partial success", "OK");
            break;
        case ADFResult::adfrDriveError:
            ShowMessage(PROGRAM_NAME, "Error communicating with the Arduino interface", "OK");
            break;
        default:
            ShowMessage(PROGRAM_NAME, "An unknown error occured", "OK");
            break;
    }

    SetGadgetAttrs(GAD(OBJ_LEFT_COL), params->window, NULL, GA_Disabled, FALSE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_BOTTOM_ROW), params->window, NULL, GA_Disabled, FALSE, TAG_DONE);
    SetGadgetAttrs(GAD(OBJ_START_READ), params->window, NULL, GA_Disabled, FALSE, GA_Text, "Start Read", TAG_DONE);
    RefreshGList(GAD(OBJ_LEFT_COL), params->window, NULL, -1);
    RefreshGList(GAD(OBJ_BOTTOM_ROW), params->window, NULL, -1);

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

void StartWrite(std::string portName, bool verify, bool pcw, int tracksA[83], int tracksB[83], struct Window *window) {
    stopWorking = false;
    if (!std::filesystem::exists(fileNameWrite)) {
        ShowMessage(PROGRAM_NAME, "The selected file doesn't exists! Cannot write the file to floppy disk", "OK");
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
    for (int i = 0; i < 83; i++) {
        tracksA[i] = 0;
        tracksB[i] = 0;
    }

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
        params->window = window;
        pthread_create(&workerThread, NULL, writeFunction, params);
    }
    else {
        isWorking = false;
        ShowMessage(PROGRAM_NAME, "Error allocating memory for the I/O thread", "OK");
    }
    isWriting = false;
}

void StartRead(std::string portName, bool verify, bool tracks82, int tracksA[83], int tracksB[83], struct Window *window) {
    stopWorking = false;

    if (fileNameRead[0] == '\0') {
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
        ShowMessage(PROGRAM_NAME, "File extension not recognised. It must be one of: .ADF, .IMG, .IMA, .ST or .SCP", "OK");
        return;
    }

    /* reset tracks */
    for (int i = 0; i < 83; i++) {
        UpdateTrack(tracksA, 0, i, 0, window);
        UpdateTrack(tracksB, 1, i, 0, window);
    }

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
        params->window = window;
        int pid = pthread_create(&workerThread, NULL, readFunction, params);
    }
    else {
        isWorking = false;
        ShowMessage(PROGRAM_NAME, "Error allocating memory for the I/O thread", "OK");
    }
}