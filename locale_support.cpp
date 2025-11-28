#include "locale_support.h"
#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/locale.h>
#include <libraries/locale.h>

struct Catalog *catalog = NULL;
struct LocaleIFace *ILocale = NULL;
struct Library *LocaleBase = NULL;
struct Locale *locale = NULL;

// Default English strings - must match the order in locale_support.h enum
static const char *DefaultStrings[] = {
    "Waffle Copy Professional",                                                                                   // MSG_PROGRAM_NAME
    "Select ADF to write to Waffle",                                                                              // MSG_SELECT_ADF_WRITE
    "Select file name to read from Waffle",                                                                       // MSG_SELECT_FILE_READ
    "Start Write",                                                                                                // MSG_START_WRITE
    "Start Read",                                                                                                 // MSG_START_READ
    "Stop",                                                                                                       // MSG_STOP
    "Verify",                                                                                                     // MSG_VERIFY
    "PCW",                                                                                                        // MSG_PCW
    "Tracks: 82",                                                                                                 // MSG_TRACKS_82
    "HIGH DENSITY DISK SELECTION",                                                                                // MSG_HD_DISK_SELECTION
    "WAFFLE DRIVE PORT: ",                                                                                        // MSG_WAFFLE_DRIVE_PORT
    "File To Disk",                                                                                               // MSG_FILE_TO_DISK
    "Disk to File",                                                                                               // MSG_DISK_TO_FILE
    "Project",                                                                                                    // MSG_MENU_PROJECT
    "A|About",                                                                                                    // MSG_MENU_ABOUT
    "Q|Quit",                                                                                                     // MSG_MENU_QUIT
    "Write ADF to Disk",                                                                                          // MSG_GROUP_WRITE_ADF
    "Create ADF from Disk",                                                                                       // MSG_GROUP_CREATE_ADF
    "UPPER SIDE",                                                                                                 // MSG_GROUP_UPPER_SIDE
    "LOWER SIDE",                                                                                                 // MSG_GROUP_LOWER_SIDE
    "Select file to read from disk",                                                                              // MSG_SELECT_FILE_READ_DISK
    "Select file to write to disk",                                                                               // MSG_SELECT_FILE_WRITE_DISK
    "82 Tracks",                                                                                                  // MSG_82_TRACKS
    "High Density disk selection",                                                                                // MSG_HIGH_DENSITY_SELECTION
    "Waffle Serial Port",                                                                                         // MSG_WAFFLE_SERIAL_PORT
    "Please enter filename to read",                                                                              // MSG_ENTER_FILENAME_READ
    "Please enter filename to write",                                                                             // MSG_ENTER_FILENAME_WRITE
    "No COM port selected",                                                                                       // MSG_NO_PORT_SELECTED
    "Waffle Copy Professional\n\nVersion 2.8.8\n\n(c) 2025 Amigasoft.net\n(c) 2025 Acube Systems",                // MSG_ABOUT_TEXT
    "OK",                                                                                                         // MSG_BUTTON_OK
    "Retry",                                                                                                      // MSG_BUTTON_RETRY
    "Ignore",                                                                                                     // MSG_BUTTON_IGNORE
    "Abort",                                                                                                      // MSG_BUTTON_ABORT
    "Retry|Ignore|Abort",                                                                                         // MSG_BUTTONS_RETRY_IGNORE_ABORT
    "Error opening selected port",                                                                                // MSG_ERROR_OPENING_PORT
    "Error communicating with the DrawBridge interface",                                                          // MSG_ERROR_COMM_DRAWBRIDGE
    "Error communicating with the Arduino interface",                                                             // MSG_ERROR_COMM_ARDUINO
    "Error allocating memory for the I/O thread",                                                                 // MSG_ERROR_ALLOCATING_MEMORY
    "Unable to connect to device: ",                                                                              // MSG_UNABLE_CONNECT_DEVICE
    "The selected file doesn't exists! Cannot write the file to floppy disk",                                     // MSG_FILE_DOESNT_EXIST
    "Error opening file",                                                                                         // MSG_ERROR_OPENING_FILE
    "Error creating file",                                                                                        // MSG_ERROR_CREATING_FILE
    "Error writing to file",                                                                                      // MSG_ERROR_WRITING_FILE
    "File extension not recognised. It must be one of: .ADF, .IMG, .IMA, .ST or .SCP",                            // MSG_FILE_EXT_NOT_RECOGNIZED
    "File extension not recognised. It must be one of: .ADF, .IMG, .IMA, .ST, .SCP or .IPF",                      // MSG_FILE_EXT_NOT_RECOGNIZED_WRITE
    "Error, disk is write protected",                                                                             // MSG_DISK_WRITE_PROTECTED
    "Disk write verify error on current track",                                                                   // MSG_DISK_VERIFY_ERROR
    "Disk has checksum errors/missing data.",                                                                     // MSG_DISK_CHECKSUM_ERROR
    "Unable to work out the density of the disk inserted",                                                        // MSG_UNABLE_DENSITY
    "WARNING: It is STRONGLY recommended to write with verify turned on",                                         // MSG_WARNING_VERIFY_RECOMMENDED
    "File written to disk",                                                                                       // MSG_FILE_WRITTEN
    "File created successfully",                                                                                  // MSG_FILE_CREATED
    "File written to disk but there were errors during verification",                                             // MSG_FILE_WRITTEN_ERRORS
    "File created with partial success",                                                                          // MSG_FILE_CREATED_PARTIAL
    "Writing aborted",                                                                                            // MSG_WRITING_ABORTED
    "File aborted",                                                                                               // MSG_FILE_ABORTED
    "Bad, invalid or unsupported SCP file",                                                                       // MSG_BAD_SCP_FILE
    "Extended ADF files are not currently supported",                                                             // MSG_EXTENDED_ADF_NOT_SUPPORTED
    "IPF writing is only supported for DD disks and images",                                                      // MSG_IPF_ONLY_DD
    "SCP writing is only supported for DD disks and images",                                                      // MSG_SCP_ONLY_DD
    "Disk in drive was detected as HD, but a DD ADF file supplied",                                               // MSG_DISK_HD_FILE_DD
    "Disk in drive was detected as DD, but an HD ADF file supplied",                                              // MSG_DISK_DD_FILE_HD
    "Cannot write this file, you need to upgrade the firmware first",                                             // MSG_FIRMWARE_TOO_OLD
    "This requires firmware V1.8 or newer",                                                                       // MSG_FIRMWARE_V18_REQUIRED
    "This requires firmware V1.8 or newer.",                                                                      // MSG_FIRMWARE_V18_REQUIRED_DOT
    "This feature requires firmware V1.9",                                                                        // MSG_FIRMWARE_V19_REQUIRED
    "Rob strongly recommends updating the firmware on your Arduino to at least V1.8.",                            // MSG_FIRMWARE_RECOMMEND_V18
    "That version is even better at reading old disks.",                                                          // MSG_FIRMWARE_BETTER_READING
    "IPF CAPSImg from Software Preservation Society Library Missing",                                             // MSG_IPF_LIBRARY_MISSING
    "An unknown error occurred ",                                                                                 // MSG_UNKNOWN_ERROR
    "An unknown error occurred",                                                                                  // MSG_UNKNOWN_ERROR_OCCURRED
    "Usage: %s %s",                                                                                               // MSG_USAGE
    "No file specified.",                                                                                         // MSG_NO_FILE_SPECIFIED
    "Writing %s file to disk",                                                                                    // MSG_WRITING_FILE_TO_DISK
    "Creating %s file from disk",                                                                                 // MSG_CREATING_FILE_FROM_DISK
    "Progress: %i%%    ",                                                                                         // MSG_PROGRESS
    "Cleaning cycle completed.",                                                                                  // MSG_CLEANING_COMPLETED
    "Running diagnostics on COM port: %s",                                                                        // MSG_RUNNING_DIAGNOSTICS
    "DIAGNOSTICS FAILED: %s",                                                                                     // MSG_DIAGNOSTICS_FAILED
    "Running drive head cleaning on COM port: %s",                                                                // MSG_RUNNING_CLEANING
    "Attempting to read settings from device on port: %s",                                                        // MSG_ATTEMPTING_READ_SETTINGS
    "Attempting to set settings to device on port: %s",                                                           // MSG_ATTEMPTING_SET_SETTINGS
    "Settng Set.  Current settings are now:",                                                                     // MSG_SETTING_SET
    "Setting %s was not found.",                                                                                  // MSG_SETTING_NOT_FOUND
    "Writing Track %i, %s side     ",                                                                             // MSG_WRITING_TRACK
    "Reading %s Track %i, %s side   ",                                                                            // MSG_READING_TRACK
    "Reading %s Track %i, %s side (retry: %i) - Got %i/%i sectors (%i bad found)   ",                             // MSG_READING_TRACK_DETAILED
    "Upper",                                                                                                      // MSG_SIDE_UPPER
    "Lower",                                                                                                      // MSG_SIDE_LOWER
    "DD",                                                                                                         // MSG_DD
    "HD",                                                                                                         // MSG_HD
    "Disk write verify error on track %i, %s side. [R]etry, [S]kip, [A]bort?                                   ", // MSG_DISK_VERIFY_ERROR_PROMPT
    "Disk has checksum errors/missing data.  [R]etry, [I]gnore, [A]bort?                                      ",  // MSG_CHECKSUM_ERROR_PROMPT
    "DrawBridge aka Arduino Floppy Disk Reader/Writer V2.8.8, Copyright (C) 2017-2022 Robert Smith",             // MSG_BANNER_LINE1
    "Full sourcecode and documentation at https://amiga.robsmithdev.co.uk",                                       // MSG_BANNER_LINE2
    "This is free software licenced under the GNU General Public Licence V3",                                     // MSG_BANNER_LINE3
    "AmigaOS4 version by Andrea Palmate' - os4test@amigasoft.net",                                                // MSG_BANNER_LINE4
    "--- WAFFLE Copy Professional -- The essential USB floppy drive solution for the real Amiga user. It allows you to write from ADF files, to read floppy disks as ADF, IPF, IMA, IMG, ST and SCP format and, thanks to a specific version Amiga Emulator, like WinUAE (by Toni Wilen) or AmiBerry (by MiDWaN),  it works like a real Amiga disk drive allowing you to directly read and write your floppies through an emulator! Sometime you may need a special USB cable (Y-Type) with the possibility of double powering if the USB port of the PC is not powerful enough.", // MSG_FLOATING_TEXT
    "DISKCHANGE",                                                                                                 // MSG_SETTING_DISKCHANGE_NAME
    "Force DiskChange Detection Support (used if pin 12 is not connected to GND)",                                // MSG_SETTING_DISKCHANGE_DESC
    "PLUS",                                                                                                       // MSG_SETTING_PLUS_NAME
    "Set DrawBridge Plus Mode (when Pin 4 and 8 are swapped for improved accuracy)",                              // MSG_SETTING_PLUS_DESC
    "ALLDD",                                                                                                      // MSG_SETTING_ALLDD_NAME
    "Force All Disks to be Detected as Double Density (faster if you don't need HD support)",                     // MSG_SETTING_ALLDD_DESC
    "SLOW",                                                                                                       // MSG_SETTING_SLOW_NAME
    "Use Slower Disk Seeking (for slow head-moving floppy drives - rare)",                                        // MSG_SETTING_SLOW_DESC
    "INDEX",                                                                                                      // MSG_SETTING_INDEX_NAME
    "Always Index-Align Writing to Disks (not recommended unless you know what you are doing)",                   // MSG_SETTING_INDEX_DESC
    "Failed to lock public screen",                                                                               // MSG_DEBUG_FAILED_LOCK_SCREEN
    "Failed to open main window",                                                                                 // MSG_ERROR_FAILED_OPEN_WINDOW
    "Failed to create main window",                                                                               // MSG_ERROR_FAILED_CREATE_WINDOW
    "Gadget %ld"                                                                                                  // MSG_DEBUG_UNKNOWN_GADGET
};

void InitLocaleLibrary(void)
{
    LocaleBase = OpenLibrary("locale.library", 53);
    if (LocaleBase)
    {
        ILocale = (struct LocaleIFace *)GetInterface(LocaleBase, "main", 1, NULL);
        if (ILocale)
        {
            locale = OpenLocale(NULL);
            catalog = OpenCatalog(NULL, "waffle.catalog",
                                  OC_BuiltInLanguage, "english",
                                  OC_Version, 1,
                                  OC_BuiltInCodeSet, 4, /* ISO-8859-1 */
                                  TAG_DONE);
        }
    }
}

void CloseLocaleLibrary(void)
{
    if (catalog)
    {
        CloseCatalog(catalog);
        catalog = NULL;
    }
    if (locale)
    {
        CloseLocale(locale);
        locale = NULL;
    }
    if (ILocale)
    {
        DropInterface((struct Interface *)ILocale);
        ILocale = NULL;
    }
    if (LocaleBase)
    {
        CloseLibrary(LocaleBase);
        LocaleBase = NULL;
    }
}

const char *GetString(int stringNum)
{
    if (catalog && ILocale)
    {
        return GetCatalogStr(catalog, stringNum, DefaultStrings[stringNum]);
    }
    return DefaultStrings[stringNum];
}
