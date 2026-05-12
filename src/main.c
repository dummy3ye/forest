#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "noise.h"
#include "render.h"
#include "forest.h"
#include "config.h"

/* Global console handle for exit handler cleanup */
HANDLE hGlobalOut;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            /* Restore cursor and clear terminal before exiting */
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hGlobalOut, &cursorInfo);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(hGlobalOut, &cursorInfo);
            printf("\x1b[0m\x1b[2J\x1b[H"); 
            ExitProcess(0);
            return TRUE;
        default:
            return FALSE;
    }
}

int main() {
    srand((unsigned int)time(NULL));

    /* Console configuration */
    SetConsoleOutputCP(65001); 
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hGlobalOut = hOut;
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    /* Hide terminal cursor */
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    /* Initialize screen dimensions */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    int term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int term_h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    /* Initialize rendering buffers */
    VirtualBitmap *bmps[LAYER_COUNT];
    for (int i = 0; i < LAYER_COUNT; i++) bmps[i] = create_bitmap(term_w * 2, term_h * 4);
    
    Forest *forest = create_forest(bmps[0]->width, bmps[0]->height);

    /* Performance tracking */
    LARGE_INTEGER frequency, t1, t2;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&t1);

    float total_time = 0;
    while (1) {
        QueryPerformanceCounter(&t2);
        float dt = (float)(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
        t1 = t2;
        total_time += dt;

        /* Reallocate buffers if terminal is resized */
        GetConsoleScreenBufferInfo(hOut, &csbi);
        int nw = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int nh = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (nw != term_w || nh != term_h) {
            term_w = nw;
            term_h = nh;
            for (int i = 0; i < LAYER_COUNT; i++) {
                free_bitmap(bmps[i]);
                bmps[i] = create_bitmap(term_w * 2, term_h * 4);
            }
        }

        /* Rendering cycle: clear bitmaps, update terrain, physics, and draw */
        for (int i = 0; i < LAYER_COUNT; i++) clear_bitmap(bmps[i]);

        for (int layer = 0; layer < LAYER_COUNT; layer++) {
            draw_ground(bmps[layer], layer, forest->camera_x);
        }

        update_forest(forest, dt, total_time, bmps);

        draw_forest(forest, bmps, LAYER_COUNT);
        
        render_to_console(bmps, LAYER_COUNT);

        /* Throttle frame rate */
        float frame_time = 1.0f / (float)TARGET_FPS;
        if (dt < frame_time) {
            Sleep((DWORD)((frame_time - dt) * 1000));
        }
    }

    return 0;
}
