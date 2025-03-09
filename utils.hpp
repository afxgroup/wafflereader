#ifndef __UTILS_HPP__
#define __UTILS_HPP__

extern bool loadF2DFile;
extern bool loadD2FFile;

bool GetASLFilename(const char *title, char *buf, int bufSize, BOOL saveMode);
int ShowMessage(const char *title, const char *message, const char *button);

#endif