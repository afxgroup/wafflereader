#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "gui_common.hpp"

extern bool loadF2DFile;
extern bool loadD2FFile;

bool GetASLFilename(const char *title, char *buf, int bufSize, BOOL saveMode);
int ShowMessage(const char *title, const char *message, const char *button);

#ifdef RAGUI
void UpdateTrack(int *tracks, int side, int track, int value, struct Window *window);
#endif

#define FILTER_PATTERN "#?.(adf|scp|img|ima|st|ipf)"

#endif