#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <string.h>
#include "utils.hpp"

bool GetASLFilename(const char *title, char *buf, int bufSize, BOOL saveMode) {
    bool res = FALSE;
    struct FileRequester *freq = (struct FileRequester *) AllocAslRequest(ASL_FileRequest, NULL);
    if (freq) {
        if (AslRequestTags(freq,
                           ASLFR_SleepWindow, TRUE,
                           ASLFR_TitleText, (ULONG) title,
                           ASLFR_DoSaveMode, saveMode,
                           ASLFR_RejectIcons, TRUE,
                           ASLFR_DoPatterns, TRUE,
                           ASLFR_InitialPattern,  "#?.(adf|scp|img|ima|st|ipf)",
                           TAG_DONE)) {

            strncpy(buf, freq->fr_Drawer, bufSize - 1);

            buf[bufSize - 1] = '\0';
            res = AddPart(buf, freq->fr_File, bufSize) != 0;
        }

        FreeAslRequest(freq);
    }
    loadF2DFile = false;
    loadD2FFile = false;
    return res;
}

int ShowMessage(const char *title, const char *message, const char *button) {
    struct EasyStruct easystruct;
  
    easystruct.es_StructSize = sizeof(struct EasyStruct);
    easystruct.es_Flags = 0;
    easystruct.es_Title = title;
    easystruct.es_TextFormat = message;
    easystruct.es_GadgetFormat = button;
    int ret = EasyRequest(0, &easystruct, 0);
  
    return ret;
}
