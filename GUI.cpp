#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "gui_common.hpp"
#include "common.hpp"

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
                    GetASLFilename("File To Disk", fileNameWrite, PATH_MAX, FALSE);
                if (loadD2FFile)
                    GetASLFilename("Disk to File", fileNameRead, PATH_MAX, TRUE);
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