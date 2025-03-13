#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/gadgetclass.h>

#include <string.h>
#include "utils.hpp"

extern Object *Objects[OBJ_MAX];
#define OBJ(x) Objects[x]
#define GAD(x) (struct Gadget *) Objects[x]

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
                           ASLFR_InitialPattern,  FILTER_PATTERN,
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

#ifdef RAGUI
void UpdateTrack(int *tracks, int side, int track, int value, struct Window *window) {
    if (track >= 0 && track < 83) {
        tracks[track] = value;
    }

    int trackId;
    if (side == 0)
        trackId = OBJ_TRACK_START + track + 1;
    else
        trackId = OBJ_TRACK_START + 90 + track;

    SetGadgetAttrs(GAD(trackId), window, NULL, GA_UserData, value, TAG_DONE);
    RefreshGList(GAD(trackId), window, NULL, 1);
}
#endif