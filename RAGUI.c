#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_common.hpp"
#include "reaction.hpp"

Object *Objects[OBJ_MAX];
#define OBJ(x) Objects[x]
#define GAD(x) (struct Gadget *)Objects[x]

#define TRACKH CHILD_MaxWidth, 25,  \
               CHILD_MaxHeight, 25, \
               CHILD_MinWidth, 25,  \
               CHILD_MinHeight, 25,

#define PALOBJ(X) StartMember, OBJ(OBJ_TRACK_START + X) = PaletteObject,                     \
                                                     GA_ID, OBJ_TRACK_START + X,             \
                                                     PALETTE_Color, 0,                       \
                                                     PALETTE_NumColors, 1,                   \
                                                     GA_UserData, 0,                         \
                                                     PALETTE_RenderHook, &PaletteRenderHook, \
                                                     EndMember,

static LONG PaletteRenderHookFunction(struct Hook *hook, APTR reserved, struct PBoxDrawMsg *msg)
{
    struct RastPort *rp;
    struct Rectangle *rect;

    rp = msg->pbdm_RastPort;
    rect = &(msg->pbdm_Bounds);
    LONG pen = (LONG)msg->pbdm_Gadget->UserData;

    if (pen == 1)
    {
        RectFillColor(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, 0x0002b505);
    }
    else if (pen == 2)
    {
        RectFillColor(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY, 0x00CD0404);
    }
    else
    {
        SetAPen(rp, 0);
        RectFill(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
    }

    return (PBCB_OK);
}

static struct Hook PaletteRenderHook = {
    {NULL, NULL},
    (HOOKFUNC)PaletteRenderHookFunction,
    NULL,
    NULL};

#define TRACK(X) \
    TRACKH       \
    PALOBJ(X)

static void
GetArchive(int archiveType, struct Window *window)
{
    int aslType = archiveType == 0 ? OBJ_SELECT_READ_FILE : OBJ_SELECT_WRITE_FILE;
    ULONG temp;

    if (gfRequestFile(OBJ(aslType), (ULONG)window))
    {
        GetAttr(GETFILE_FullFile, OBJ(aslType), &temp);
        if (temp)
        {
            if (archiveType == 0)
                strlcpy(fileNameRead, (char *)temp, PATH_MAX);
            else
                strlcpy(fileNameWrite, (char *)temp, PATH_MAX);
        }
    }
}

int main(void)
{
    struct Window *window;
    ULONG signal, result;
    ULONG done = FALSE;
    const char *portList[MAX_PORTS];
    int portNumbers = 0, portIndex = -1;
    bool verify = true, pcw = true, tracks82 = false, hdSelection = false;
    int tracksA[83] = {0}, tracksB[83] = {0};

    InitLocaleLibrary();

    struct Screen *screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        DebugPrintF("%s\n", GetString(MSG_DEBUG_FAILED_LOCK_SCREEN));
        return 0;
    }

    for (int i = 0; i < MAX_PORTS; i++)
        portList[i] = NULL;

    std::vector<std::string> portsList;
    ArduinoFloppyReader::ArduinoInterface::enumeratePorts(portsList);
    if (portsList.size() > 0)
    {
        for (const std::string &port : portsList)
        {
            portList[portNumbers] = (const char *)malloc(128);
            memset((void *)portList[portNumbers], 0, 128);
            strncpy((char *)portList[portNumbers], port.c_str(), 127);
            portNumbers++;
            if (portNumbers == MAX_PORTS - 1)
                break;
        }
        portIndex = 0;
    }

    OBJ(OBJ_MENU) = MStrip,
        MA_AddChild, MTitle(GetString(MSG_MENU_PROJECT)),
            MA_AddChild, MItem(GetString(MSG_MENU_ABOUT)),
                MA_ID, MID_ABOUT,
            MEnd,

            MA_AddChild, MSeparator,
            MEnd,

            MA_AddChild, MItem(GetString(MSG_MENU_QUIT)),
                MA_ID, MID_QUIT,
            MEnd,
        MEnd,
    MEnd;

	OBJ(OBJ_MAIN_WINDOW) = WindowObject,
		WA_ScreenTitle, PROGRAM_NAME,
		WA_Title, PROGRAM_NAME,
		WA_SizeGadget, FALSE,
		WA_DepthGadget, TRUE,
		WA_DragBar, TRUE,
		WA_CloseGadget, TRUE,
		WA_Activate, TRUE,
        WA_BackFill, LAYERS_NOBACKFILL,
		WINDOW_Position, WPOS_CENTERWINDOW,
        WINDOW_MenuStrip, OBJ(OBJ_MENU),
		WINDOW_ParentGroup, OBJ(OBJ_MAIN_LAYOUT) = VLayoutObject,
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_AddChild, ButtonObject,
                BUTTON_BevelStyle, BVS_NONE,
                BUTTON_Transparent, TRUE,
                BUTTON_RenderImage,
                OBJ(OBJ_LOGO_IMAGE) = BitMapObject,
                    BITMAP_SourceFile, "PROGDIR:WaffleUI/waffleCopyPRO_n.png",
                    BITMAP_Height, 145,
                    BITMAP_Masking, TRUE,
                    BITMAP_Screen, screen,
                BitMapEnd,
            End,
            CHILD_MinHeight, 150,
            LAYOUT_AddChild, HLayoutObject,
                LAYOUT_AddChild, OBJ(OBJ_LEFT_COL) = VLayoutObject,
                    LAYOUT_AddChild, VLayoutObject,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_Label, GetString(MSG_GROUP_WRITE_ADF),
                        LAYOUT_AddChild, VLayoutObject,
                            LAYOUT_HorizAlignment, LALIGN_CENTER,
                            LAYOUT_AddChild, HLayoutObject,
                                LAYOUT_AddChild, OBJ(OBJ_SELECT_WRITE_FILE) = GetFileObject,
                                    GA_ID,                  OBJ_SELECT_WRITE_FILE,
                                    GA_RelVerify,           TRUE,
                                    GA_TabCycle,            TRUE,
                                    GETFILE_ReadOnly,       TRUE,
                                    GETFILE_RejectIcons,    TRUE,
                                    GETFILE_DoPatterns,     TRUE,
                                    GETFILE_Pattern,        FILTER_PATTERN,
                                    GETFILE_TitleText,      GetString(MSG_SELECT_FILE_READ_DISK),
                                End,
                            End,
                            LAYOUT_AddChild, HLayoutObject,
                                LAYOUT_AddChild, OBJ(OBJ_WRITE_VERIFY) = CheckBoxObject,
                                    GA_ID,          OBJ_WRITE_VERIFY,
                                    GA_RelVerify,   TRUE,
                                    GA_TabCycle,    TRUE,
                                    GA_Selected,    TRUE,
                                    GA_Text,        GetString(MSG_VERIFY),
                                End,
                                LAYOUT_AddChild, OBJ(OBJ_WRITE_PCW) = CheckBoxObject,
                                    GA_ID,          OBJ_WRITE_PCW,
                                    GA_RelVerify,   TRUE,
                                    GA_TabCycle,    TRUE,
                                    GA_Selected,    TRUE,
                                    GA_Text,        GetString(MSG_PCW),
                                End,
                            End,
                            LAYOUT_AddChild, OBJ(OBJ_START_WRITE) = ButtonObject,
                                GA_ID, OBJ_START_WRITE,
                                GA_TabCycle, TRUE,
                                GA_RelVerify, TRUE,
                                GA_Text, GetString(MSG_START_WRITE),
                                GA_Disabled, portNumbers == 0,
                            End,
                            CHILD_WeightedWidth, 0,
                            CHILD_WeightedHeight, 0,
                        End,
                    End,
                    CHILD_MinWidth, 300,
                    LAYOUT_AddChild, VLayoutObject,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_Label, GetString(MSG_GROUP_CREATE_ADF),
                        LAYOUT_AddChild, VLayoutObject,
                            LAYOUT_HorizAlignment, LALIGN_CENTER,
                            LAYOUT_AddChild, OBJ(OBJ_SELECT_READ_FILE) = GetFileObject,
                                GA_ID,                  OBJ_SELECT_READ_FILE,
                                GA_RelVerify,           TRUE,
                                GA_TabCycle,            TRUE,
                                GETFILE_ReadOnly,       TRUE,
                                GETFILE_RejectIcons,    TRUE,
                                GETFILE_TitleText,      GetString(MSG_SELECT_FILE_WRITE_DISK),
                                GETFILE_DoPatterns,     TRUE,
                                GETFILE_Pattern,        FILTER_PATTERN,
                            End,
                            LAYOUT_AddChild, HLayoutObject,
                                LAYOUT_AddChild, OBJ(OBJ_READ_TRACKS82) = CheckBoxObject,
                                    GA_ID, OBJ_READ_TRACKS82,
                                    GA_TabCycle, TRUE,
                                    GA_RelVerify, TRUE,
                                    GA_Text, GetString(MSG_82_TRACKS),
                                End,
                            End,
                            LAYOUT_AddChild, OBJ(OBJ_START_READ) = ButtonObject,
                                GA_ID, OBJ_START_READ,
                                GA_TabCycle, TRUE,
                                GA_RelVerify, TRUE,
                                GA_Text, GetString(MSG_START_READ),
                                GA_Disabled, portNumbers == 0,
                            End,
                            CHILD_WeightedWidth, 0,
                            CHILD_WeightedHeight, 0,
                        End,
                    End,
                End,
                LAYOUT_AddChild, HLayoutObject,
                    TAligned,
                    LAYOUT_AddChild, VLayoutObject,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_Label, GetString(MSG_GROUP_UPPER_SIDE),
                        LAYOUT_FixedHoriz, TRUE,
                        /* Tracks Row 1 */
                        StartHGroup,
                            TRACK(1)
                            TRACK(2)
                            TRACK(3)
                            TRACK(4)
                            TRACK(5)
                            TRACK(6)
                            TRACK(7)
                            TRACK(8)
                            TRACK(9)
                            TRACK(10)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 2 */
                        StartHGroup,
                            TRACK(11)
                            TRACK(12)
                            TRACK(13)
                            TRACK(14)
                            TRACK(15)
                            TRACK(16)
                            TRACK(17)
                            TRACK(18)
                            TRACK(19)
                            TRACK(20)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 3 */
                        StartHGroup,
                            TRACK(21)
                            TRACK(22)
                            TRACK(23)
                            TRACK(24)
                            TRACK(25)
                            TRACK(26)
                            TRACK(27)
                            TRACK(28)
                            TRACK(29)
                            TRACK(30)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 4 */
                        StartHGroup,
                            TRACK(31)
                            TRACK(32)
                            TRACK(33)
                            TRACK(34)
                            TRACK(35)
                            TRACK(36)
                            TRACK(37)
                            TRACK(38)
                            TRACK(39)
                            TRACK(40)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 5 */
                        StartHGroup,
                            TRACK(41)
                            TRACK(42)
                            TRACK(43)
                            TRACK(44)
                            TRACK(45)
                            TRACK(46)
                            TRACK(47)
                            TRACK(48)
                            TRACK(49)
                            TRACK(50)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 6 */
                        StartHGroup,
                            TRACK(51)
                            TRACK(52)
                            TRACK(53)
                            TRACK(54)
                            TRACK(55)
                            TRACK(56)
                            TRACK(57)
                            TRACK(58)
                            TRACK(59)
                            TRACK(60)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 7 */
                        StartHGroup,
                            TRACK(61)
                            TRACK(62)
                            TRACK(63)
                            TRACK(64)
                            TRACK(65)
                            TRACK(66)
                            TRACK(67)
                            TRACK(68)
                            TRACK(69)
                            TRACK(70)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 8 */
                        StartHGroup,
                            TRACK(71)
                            TRACK(72)
                            TRACK(73)
                            TRACK(74)
                            TRACK(75)
                            TRACK(76)
                            TRACK(77)
                            TRACK(78)
                            TRACK(79)
                            TRACK(80)
                            TRACKH // Needed for padding
                        End,
                        /* Tracks Row 9 */
                        StartHGroup,
                            TRACK(81)
                            TRACK(82)
                            TRACK(83)
                            TRACK(84)
                            TRACKH // Needed for padding
                        End,
                        CHILD_WeightedWidth, 0,
                        CHILD_WeightedHeight, 0,
                    End,
                End,
                LAYOUT_AddChild, VLayoutObject,
                    LAYOUT_BevelStyle, BVS_GROUP,
                    LAYOUT_Label, GetString(MSG_GROUP_LOWER_SIDE),
                    LAYOUT_FixedHoriz, TRUE,
                    /* Tracks Row 1 */
                    StartHGroup,
                        TRACK(90)
                        TRACK(91)
                        TRACK(92)
                        TRACK(93)
                        TRACK(94)
                        TRACK(95)
                        TRACK(96)
                        TRACK(97)
                        TRACK(98)
                        TRACK(99)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 2 */
                    StartHGroup,
                        TRACK(100)
                        TRACK(101)
                        TRACK(102)
                        TRACK(103)
                        TRACK(104)
                        TRACK(105)
                        TRACK(106)
                        TRACK(107)
                        TRACK(108)
                        TRACK(109)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 3 */
                    StartHGroup,
                        TRACK(110)
                        TRACK(111)
                        TRACK(112)
                        TRACK(113)
                        TRACK(114)
                        TRACK(115)
                        TRACK(116)
                        TRACK(117)
                        TRACK(118)
                        TRACK(119)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 4 */
                    StartHGroup,
                        TRACK(120)
                        TRACK(121)
                        TRACK(122)
                        TRACK(123)
                        TRACK(124)
                        TRACK(125)
                        TRACK(126)
                        TRACK(127)
                        TRACK(128)
                        TRACK(129)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 5 */
                    StartHGroup,
                        TRACK(130)
                        TRACK(131)
                        TRACK(132)
                        TRACK(133)
                        TRACK(134)
                        TRACK(135)
                        TRACK(136)
                        TRACK(137)
                        TRACK(138)
                        TRACK(139)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 6 */
                    StartHGroup,
                        TRACK(140)
                        TRACK(141)
                        TRACK(142)
                        TRACK(143)
                        TRACK(144)
                        TRACK(145)
                        TRACK(146)
                        TRACK(147)
                        TRACK(148)
                        TRACK(149)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 7 */
                    StartHGroup,
                        TRACK(150)
                        TRACK(151)
                        TRACK(152)
                        TRACK(153)
                        TRACK(154)
                        TRACK(155)
                        TRACK(156)
                        TRACK(157)
                        TRACK(158)
                        TRACK(159)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 8 */
                    StartHGroup,
                        TRACK(160)
                        TRACK(161)
                        TRACK(162)
                        TRACK(163)
                        TRACK(164)
                        TRACK(165)
                        TRACK(166)
                        TRACK(167)
                        TRACK(168)
                        TRACK(169)
                        TRACKH // Needed for padding
                    End,
                    /* Tracks Row 9 */
                    StartHGroup,
                        TRACK(170)
                        TRACK(171)
                        TRACK(172)
                        TRACK(173)
                        TRACKH // Needed for padding
                    End,
                    CHILD_WeightedWidth, 0,
                    CHILD_WeightedHeight, 0,
                End,
            End,
            LAYOUT_AddChild, OBJ(OBJ_BOTTOM_ROW) = HLayoutObject,
                LAYOUT_AddChild, OBJ(OBJ_PORT_LIST) = ChooserObject,
                    GA_ID, OBJ_PORT_LIST,
                    GA_TabCycle, TRUE,
                    GA_RelVerify, TRUE,
                    (portList && portList[0]) ? CHOOSER_LabelArray : TAG_IGNORE, portList,
                    CHOOSER_Selected, 0,
                    GA_Underscore,
                End,
                Label(GetString(MSG_WAFFLE_SERIAL_PORT)),
                LAYOUT_AddChild, SpaceObject,
                    SPACE_MinWidth, 100,
                End,
                LAYOUT_AddChild, OBJ(OBJ_HD_MODE) = CheckBoxObject,
                    GA_ID, OBJ_HD_MODE,
                    GA_TabCycle, TRUE,
                    GA_RelVerify, TRUE,
                    GA_Text, GetString(MSG_HIGH_DENSITY_SELECTION),
                End,
                CHILD_NominalSize, TRUE,
            End,
        End,
    EndWindow;

    if (OBJ(OBJ_MAIN_WINDOW))
    {
        if (window = (struct Window *)IDoMethod(OBJ(OBJ_MAIN_WINDOW), WM_OPEN))
        {
            ULONG wait, temp, id;
            WORD code;

            GetAttr(WINDOW_SigMask, OBJ(OBJ_MAIN_WINDOW), &signal);
            while (!done)
            {
                wait = Wait(signal | SIGBREAKF_CTRL_C);

                if (wait & SIGBREAKF_CTRL_C)
                    done = TRUE;
                else
                {
                    while ((result = IDoMethod(OBJ(OBJ_MAIN_WINDOW), WM_HANDLEINPUT, &code)))
                    {
                        switch (result & WMHI_CLASSMASK)
                        {
                        case WMHI_IGNORE:
                            continue;
                        case WMHI_CLOSEWINDOW:
                            done = TRUE;
                            break;

                        case WMHI_MENUPICK:
                            id = NO_MENU_ID;
                            while ((id = IDoMethod(OBJ(OBJ_MENU), MM_NEXTSELECT, 0, id)) != NO_MENU_ID)
                            {
                                switch (id)
                                {
                                case MID_ABOUT:
                                    ShowMessage(PROGRAM_NAME, GetString(MSG_ABOUT_TEXT), GetString(MSG_BUTTON_OK));
                                    break;

                                case MID_QUIT:
                                    done = TRUE;
                                    break;
                                }
                            }
                            break;

                        case WMHI_GADGETUP:
                            switch (result & WMHI_GADGETMASK)
                            {
                            case OBJ_SELECT_READ_FILE:
                                GetArchive(0, window);
                                break;
                            case OBJ_SELECT_WRITE_FILE:
                                GetArchive(1, window);
                                break;
                            case OBJ_WRITE_VERIFY:
                                verify = code;
                                break;
                            case OBJ_WRITE_PCW:
                                pcw = code;
                                break;
                            case OBJ_READ_TRACKS82:
                                tracks82 = code;
                                break;
                            case OBJ_HD_MODE:
                                hdSelection = code;
                                break;
                            case OBJ_PORT_LIST:
                                portIndex = code;
                                break;
                            case OBJ_START_READ:
                                if (portIndex >= 0)
                                {
                                    if (fileNameRead != NULL && !fileNameRead[0] == '\0')
                                    {
                                        if (!isWorking)
                                            StartRead(portList[portIndex], verify, tracks82, tracksA, tracksB, window);
                                        else
                                        {
                                            isWorking = FALSE;
                                        }
                                    }
                                    else
                                        ShowMessage(PROGRAM_NAME, GetString(MSG_ENTER_FILENAME_READ), GetString(MSG_BUTTON_OK));
                                }
                                else
                                {
                                    ShowMessage(PROGRAM_NAME, GetString(MSG_NO_PORT_SELECTED), GetString(MSG_BUTTON_OK));
                                }
                                break;
                            case OBJ_START_WRITE:
                                if (portIndex >= 0)
                                {
                                    if (fileNameWrite != NULL && !fileNameWrite[0] == '\0')
                                    {
                                        if (!isWorking)
                                            StartWrite(portList[portIndex], verify, pcw, tracksA, tracksB, window);
                                        else
                                        {
                                            isWorking = FALSE;
                                        }
                                    }
                                    else
                                        ShowMessage(PROGRAM_NAME, GetString(MSG_ENTER_FILENAME_WRITE), GetString(MSG_BUTTON_OK));
                                }
                                else
                                {
                                    ShowMessage(PROGRAM_NAME, GetString(MSG_NO_PORT_SELECTED), GetString(MSG_BUTTON_OK));
                                }
                                break;
                            default:
                                Printf(GetString(MSG_DEBUG_UNKNOWN_GADGET), result & WMHI_GADGETMASK);
                            }
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            Printf("%s\n", GetString(MSG_ERROR_FAILED_OPEN_WINDOW));
        }

        if (OBJ(OBJ_LOGO_IMAGE))
            DisposeObject(OBJ(OBJ_LOGO_IMAGE));

        /* Disposing of the window object will
         * also close the window if it is
         * already opened and it will dispose of
         * all objects attached to it.
         */
        DisposeObject(OBJ(OBJ_MAIN_WINDOW));
    }
    else
    {
        Printf("%s\n", GetString(MSG_ERROR_FAILED_CREATE_WINDOW));
    }

    if (screen)
        UnlockPubScreen(NULL, screen);

    if (portNumbers > 0)
    {
        for (int i = 0; i < portNumbers; i++)
        {
            if (portList[i] != NULL)
                free((void *)portList[i]);
        }
    }

    CloseLocaleLibrary();
}
