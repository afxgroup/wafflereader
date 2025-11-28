#ifndef __GUI_COMMON_HPP__
#define __GUI_COMMON_HPP__

#include "ADFWriter.h"
#include "ArduinoInterface.h"
#include "locale_support.h"

#include <unistd.h>
#include <limits.h>

#define MAX_PORTS 10

// Helper macros for localized strings
#define LS(id) GetString(MSG_##id)
#define PROGRAM_NAME GetString(MSG_PROGRAM_NAME)

#include "utils.hpp"
#include <pthread.h>
#include <iostream>
#include <filesystem>

extern char fileNameWrite[PATH_MAX], fileNameRead[PATH_MAX];
extern bool loadF2DFile, loadD2FFile;
extern pthread_mutex_t arrayMutex;
extern bool stopWorking;
extern bool isWorking;
extern bool isReading;
extern bool isWriting;
extern pthread_t workerThread;

typedef struct __attribute__((packed))
{
    const char *fileName;
    const char *portName;
    int32_t mode;
    bool hdMode;
    bool verify;
    bool precomp;
    bool tracks82;
    bool *running;
    int *tracksA; // Pointer to tracksA array
    int *tracksB; // Pointer to tracksB array
    struct Window *window;
} ThreadParams;

enum
{
    OBJ_MAIN_WINDOW,
    OBJ_MAIN_LAYOUT,
    OBJ_LOGO_IMAGE,
    OBJ_START_WRITE,
    OBJ_START_READ,
    OBJ_SELECT_WRITE_FILE,
    OBJ_SELECT_READ_FILE,
    OBJ_WRITE_VERIFY,
    OBJ_WRITE_PCW,
    OBJ_READ_TRACKS82,
    OBJ_PORT_LIST,
    OBJ_HD_MODE,
    OBJ_LEFT_COL,
    OBJ_BOTTOM_ROW,
    OBJ_MENU,

    OBJ_MAX = 273
};

enum
{
    MID_ABOUT = 1,
    MID_QUIT
};

#define OBJ_TRACK_START 100

void *writeFunction(void *arg);
void *readFunction(void *arg);
#ifndef RAGUI
void StartWrite(std::string portName, bool verify, bool pcw, int tracksA[83], int tracksB[83]);
void StartRead(std::string portName, bool verify, bool tracks82, int tracksA[83], int tracksB[83]);
#else
void StartWrite(std::string portName, bool verify, bool pcw, int tracksA[83], int tracksB[83], struct Window *window);
void StartRead(std::string portName, bool verify, bool tracks82, int tracksA[83], int tracksB[83], struct Window *window);
#endif

#endif