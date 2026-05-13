#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif

#include "../include/render.h"
#include "../include/config.h"
#include "../sakura/sakura.h"

typedef struct {
	float x, y;
	float vx, vy;
	float phase;
	int active;
} Petal;

#ifdef _WIN32
HANDLE hGlobalOut;
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
	if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_CLOSE_EVENT) {
		printf("\x1b[?25h\x1b[0m\x1b[2J\x1b[H");
		fflush(stdout);
		ExitProcess(0);
		return TRUE;
	}
	return FALSE;
}
#else
void handle_sigint(int sig) {
	(void)sig;
	printf("\x1b[?25h\x1b[0m\x1b[2J\x1b[H");
	fflush(stdout);
	exit(0);
}
#endif

#define MAX_PETALS 200

int main() {
	srand((unsigned int)time(NULL));

	int term_w, term_h;
#ifdef _WIN32
	SetConsoleOutputCP(65001);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hGlobalOut = hOut;
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	printf("\x1b[?25l\x1b[2J");
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hOut, &csbi);
	term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	term_h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	if (term_w > 4) term_w -= 2; // Prevent wrapping on some terminals
#else
	signal(SIGINT, handle_sigint);
	printf("\x1b[?25l\x1b[2J");
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	term_w = w.ws_col;
	term_h = w.ws_row;
	if (term_w > 4) term_w -= 2;
#endif

	VirtualBitmap *tree_bmp = create_bitmap(term_w * 2, term_h * 4);
	VirtualBitmap *petal_bmp = create_bitmap(term_w * 2, term_h * 4);
	// Move root up so it's clearly visible (75% down the screen)
	SakuraTree *sakura = create_sakura(tree_bmp->width / 2.0f, tree_bmp->height * 0.75f);

	// Pre-draw the static tree
	draw_sakura(sakura, tree_bmp);

	Petal petals[MAX_PETALS];
	for (int i=0; i<MAX_PETALS; i++) petals[i].active = 0;

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

	while(1) {
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
		if (dt > 0.1f) dt = 0.1f; // Cap dt to avoid teleports on lag

		total_time += dt;
		if (total_time > 1000.0f) total_time -= 1000.0f; // Wrap time
		clear_bitmap(petal_bmp);

		float wind = sinf(total_time * 0.5f) * 0.5f + 0.5f; // Constant but varying wind

		// Update and draw petals
		for (int i=0; i<MAX_PETALS; i++) {
			if (!petals[i].active) {
				if (sakura->foliage_count > 0 && rand() % 20 == 0) {
					int idx = rand() % sakura->foliage_count;
					petals[i].active = 1;
					petals[i].x = sakura->foliage_points[idx].x;
					petals[i].y = sakura->foliage_points[idx].y;
					petals[i].vx = (float)(rand() % 10 - 5) * 0.1f;
					petals[i].vy = (float)(rand() % 10 + 2) * 0.1f; // Slow fall
					petals[i].phase = (float)(rand() % 100);
				}
				continue;
			}

			// Wobble and Wind physics
			float wobble = sinf(total_time * 3.0f + petals[i].phase) * 0.3f;
			petals[i].vx += (wind + wobble - petals[i].vx) * 0.05f;
			petals[i].vy += (0.8f - petals[i].vy) * 0.05f; // Terminal velocity
			
			petals[i].x += petals[i].vx;
			petals[i].y += petals[i].vy;

			if (petals[i].y > petal_bmp->height || petals[i].x < -10 || petals[i].x > petal_bmp->width + 10) {
				petals[i].active = 0;
			} else {
				draw_pixel_ext(petal_bmp, (int)petals[i].x, (int)petals[i].y, MAT_SAKURA);
			}
		}

		VirtualBitmap *layers[2] = { petal_bmp, tree_bmp };
		render_to_console(layers, 2);

		/* Throttle frame rate */
		float target_frame_time = 1.0f / 30.0f;
		if (dt < target_frame_time) {
#ifdef _WIN32
			Sleep((DWORD)((target_frame_time - dt) * 1000));
#else
			usleep((unsigned int)((target_frame_time - dt) * 1000000));
#endif
		}

		// Check for resize
#ifdef _WIN32
		GetConsoleScreenBufferInfo(hOut, &csbi);
		int nw = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		int nh = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		int nw = w.ws_col;
		int nh = w.ws_row;
#endif
		if (nw != term_w || nh != term_h) {
			term_w = nw; term_h = nh;
			if (term_w > 4) term_w -= 2; 
			free_bitmap(tree_bmp);
			free_bitmap(petal_bmp);
			tree_bmp = create_bitmap(term_w * 2, term_h * 4);
			petal_bmp = create_bitmap(term_w * 2, term_h * 4);
			free_sakura(sakura);
			sakura = create_sakura(tree_bmp->width / 2.0f, tree_bmp->height * 0.75f);
			draw_sakura(sakura, tree_bmp);
		}
	}

	// Unreachable in current loop, but for completeness:
	printf("\x1b[?25h\x1b[0m\x1b[2J\x1b[H");
	free_bitmap(tree_bmp);
	free_bitmap(petal_bmp);
	free_sakura(sakura);

	return 0;
}
