#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "../include/noise.h"
#include "../include/render.h"
#include "../include/forest.h"
#include "../include/config.h"

#ifdef _WIN32
/* Global console handle for exit handler cleanup */
HANDLE hGlobalOut;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            /* Restore cursor and clear terminal before exiting */
            CONSOLE_CURSOR_INFO cursorInfo;
            if (hGlobalOut != INVALID_HANDLE_VALUE) {
                GetConsoleCursorInfo(hGlobalOut, &cursorInfo);
                cursorInfo.bVisible = TRUE;
                SetConsoleCursorInfo(hGlobalOut, &cursorInfo);
            }
            printf("\x1b[0m\x1b[2J\x1b[H");
            ExitProcess(0);
            return TRUE;
        default:
            return FALSE;
    }
}
#else
void handle_sigint(int sig) {
    (void)sig;
    /* Restore cursor and clear terminal before exiting */
    printf("\x1b[?25h\x1b[0m\x1b[2J\x1b[H");
    exit(0);
}
#endif

int main() {
    srand((unsigned int)time(NULL));

    int term_w, term_h;

#ifdef _WIN32
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
    term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    term_h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    /* Signal handler for POSIX */
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    /* Hide terminal cursor via ANSI */
    printf("\x1b[?25l");

    /* Initialize screen dimensions via ioctl */
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    term_w = w.ws_col;
    term_h = w.ws_row;
#endif

    /* Initialize rendering buffers */
    VirtualBitmap *bmps[LAYER_COUNT];
    for (int i = 0; i < LAYER_COUNT; i++) bmps[i] = create_bitmap(term_w * 2, term_h * 4);

    Forest *forest = create_forest(bmps[0]->width, bmps[0]->height);

    /* Performance tracking */
#ifdef _WIN32
    LARGE_INTEGER frequency, t1, t2;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&t1);
#else
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
#endif

    float total_time = 0;
    while (1) {
        float dt;
#ifdef _WIN32
        QueryPerformanceCounter(&t2);
        dt = (float)(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
        t1 = t2;
#else
        clock_gettime(CLOCK_MONOTONIC, &t2);
        dt = (float)(t2.tv_sec - t1.tv_sec) + (float)(t2.tv_nsec - t1.tv_nsec) * 1e-9f;
        t1 = t2;
#endif
        total_time += dt;

        /* Reallocate buffers if terminal is resized */
        int nw, nh;
#ifdef _WIN32
        GetConsoleScreenBufferInfo(hOut, &csbi);
        nw = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        nh = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        nw = w.ws_col;
        nh = w.ws_row;
#endif
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
#ifdef _WIN32
            Sleep((DWORD)((frame_time - dt) * 1000));
#else
            usleep((unsigned int)((frame_time - dt) * 1000000));
#endif
        }
    }

    return 0;
}
