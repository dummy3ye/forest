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

#include <math.h>

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
		ExitProcess(0);
		return TRUE;
	}
	return FALSE;
}
#else
void handle_sigint(int sig) {
	(void)sig;
	printf("\x1b[?25h\x1b[0m\x1b[2J\x1b[H");
	exit(0);
}
#endif

#define MAX_PETALS 150

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
	printf("\x1b[?25l");
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hOut, &csbi);
	term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	term_h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
	signal(SIGINT, handle_sigint);
	printf("\x1b[?25l");
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	term_w = w.ws_col;
	term_h = w.ws_row;
#endif

	VirtualBitmap *bmp = create_bitmap(term_w * 2, term_h * 4);
	SakuraTree *sakura = create_sakura(bmp->width / 2.0f, bmp->height - 12.0f);

	Petal petals[MAX_PETALS];
	for (int i=0; i<MAX_PETALS; i++) petals[i].active = 0;

	float dt = 0.033f; // ~30fps
	float total_time = 0;

	while(1) {
		total_time += dt;
		clear_bitmap(bmp);

		// Draw static tree
		draw_sakura(sakura, bmp);

		// Update and draw petals
		for (int i=0; i<MAX_PETALS; i++) {
			if (!petals[i].active) {
				if (rand() % 50 == 0) {
					petals[i].active = 1;
					petals[i].x = (float)(rand() % bmp->width);
					petals[i].y = -10.0f;
					petals[i].vx = (float)(rand() % 10 - 5) * 0.5f;
					petals[i].vy = (float)(rand() % 10 + 10) * 0.5f;
					petals[i].phase = (float)(rand() % 100);
				}
				continue;
			}

			// Wobble physics
			petals[i].vx += sinf(total_time * 2.0f + petals[i].phase) * 0.2f;
			petals[i].x += petals[i].vx;
			petals[i].y += petals[i].vy;

			if (petals[i].y > bmp->height || petals[i].x < 0 || petals[i].x > bmp->width) {
				petals[i].active = 0;
			} else {
				draw_pixel_ext(bmp, (int)petals[i].x, (int)petals[i].y, MAT_SAKURA);
			}
		}

		VirtualBitmap *layers[1] = { bmp };
		render_to_console(layers, 1);

#ifdef _WIN32
		Sleep(33);
#else
		usleep(33000);
#endif

		// Check for resize (simplified)
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
			free_bitmap(bmp);
			bmp = create_bitmap(term_w * 2, term_h * 4);
		}
	}

	return 0;
}
