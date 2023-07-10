#include <stdio.h>
#include <termio.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#include "snake_game.h"

#define LOOP_INTERVAL_USEC			(1000000 / 30)	/* 30 FPS */

static int input_init(void);
static void input_deinit(void);
static void input_from_keyboard(SnakeGame *game);
static int input_hit(void);
static void print_tip(void);
static void fps_restrict(void);

static struct timeval fps_tv = {0};
static struct termios orig_termios;
static unsigned long orig_flags;
static SnakeGame *game;

static void
signal_handler(int sig)
{
	snake_game_over(game, "Signal interrupt.");
}

static int
signal_init(void)
{
	signal(SIGINT, signal_handler);

	return 0;
}

static int
input_init(void)
{
	struct termios new_termios;

	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(orig_termios));

	/* No echo */
	new_termios.c_lflag &= ~ECHO;
	//new_termios.c_lflag &= ~ISIG;
	new_termios.c_lflag &= ~ICANON;

	tcsetattr(0, TCSANOW, &new_termios);

	/* Non-blocking stdin */
	orig_flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, orig_flags | O_NONBLOCK);

	return 0;
}

static void
input_deinit(void)
{
	tcsetattr(0, TCSANOW, &orig_termios);
	fcntl(0, F_SETFL, orig_flags);
}

static void
fps_restrict(void)
{
	long usec_pass;
	struct timeval now_tv;

	gettimeofday(&now_tv, NULL);

	usec_pass = TV_USEC_DIFF(&fps_tv, &now_tv);
	if (usec_pass < LOOP_INTERVAL_USEC)
		usleep(LOOP_INTERVAL_USEC - usec_pass);

	fps_tv = now_tv;
}

static void
input_from_keyboard(SnakeGame *game)
{
	int dir = DIRECTION_NONE;
	char input;
	input = getchar();
	switch (toupper(input))
	{
		case 'W':
			dir = DIRECTION_UP;
			break;
		case 'A':
			dir = DIRECTION_LEFT;
			break;
		case 'S':
			dir = DIRECTION_DOWN;
			break;
		case 'D':
			dir = DIRECTION_RIGHT;
			break;
		case 'Q':
			snake_game_over(game, "User quit.\n");
			break;
	}

	if (dir != DIRECTION_NONE)
		snake_game_set_direction(game, dir);
}

static int
input_hit(void)
{
	char input;
	input = getchar();

	if (input < 0)
		return 0;

	return 1;
}

static void
print_tip(void)
{
	printf("W: Up, A: Left, S: Down, D: Right, Q: Quit\n");
	printf("Press any key to start\n");
	while (!input_hit())
		;
}

int main(void)
{
	/* Init */
	srand(time(NULL));
	input_init();
	signal_init();

	print_tip();

	game = snake_game_create(32, 16, 8, 500);
	snake_game_show(game);
	/* Loop */
	while (!snake_game_is_over(game))
	{
		fps_restrict();

		input_from_keyboard(game);

		snake_game_update(game);
	}

	printf("Game over: %s\n", snake_game_get_game_over_reason(game));
	snake_game_free(game);

	/* De-initialize */
	input_deinit();

	return 0;
}
