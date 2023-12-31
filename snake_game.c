#include "snake_game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int _point_is_overlap(Point *a, Point *b);
static int _point_is_out_of_field(Point *p, int x, int y);
static void _point_go_random(Point *p, int x, int y);

static void _snake_eat(SnakeGame *game);
static void _snake_move(SnakeGame *game);
static int _snake_is_hitting_the_point(SnakeGame *game);
static int _snake_is_hitting_the_wall(SnakeGame *game);
static int _snake_is_hitting_itself(SnakeGame *game);
static int _snake_has_no_more_step_remain(SnakeGame *game);

static int _game_should_update(SnakeGame *game);
static void _game_point_go_random(SnakeGame *game);
static void _game_over(SnakeGame *game, const char *reason);

static int _display_get_index(SnakeGame *game, int x, int y);
static void _display_update_background(SnakeGame *game);
static void _display_update_foreground(SnakeGame *game);
static void _terminal_cursor_move(int x, int y);

static int
_point_is_overlap(Point *a, Point *b)
{
	if (a->x != b->x)
		return 0;

	if (a->y != b->y)
		return 0;

	/* Return 1 only when a->x == b->x and a->y == b->y */
	return 1;
}

static int
_point_is_out_of_field(Point *p, int x, int y)
{
	if (p->x < 0)
		return 1;

	if (p->x >= x)
		return 1;

	if (p->y < 0)
		return 1;

	if (p->y >= y)
		return 1;

	return 0;
}

static void
_point_go_random(Point *p, int x, int y)
{
	p->x = rand() % x;
	p->y = rand() % y;
}

static void
_snake_eat(SnakeGame *game)
{
	game->snake_len++;
	game->snake_step_remain = game->max_step;
	/*
	 * Initialize the last tail point by -1,
	 * -1 to skip display update until the snake move and give the point a reasonable x, y
	 */
	game->snake_body[game->snake_len - 1].x = -1;
	game->snake_body[game->snake_len - 1].y = -1;
}

static void
_snake_move(SnakeGame *game)
{
	int i;

	if (game->snake_dir == DIRECTION_NONE)
		return;

	/*
	 * Index from the snake tail(len - 1)
	 * to the point next to snake head(1)
	 */
	for (i = game->snake_len - 1; i > 0; i--)
	{
		/* Move the snake by updating its body to its next position */
		game->snake_body[i] = game->snake_body[i - 1];
	}

	/* The snake head(0) */
	switch (game->snake_dir)
	{
		case DIRECTION_UP:
			game->snake_body[0].y--;
			break;

		case DIRECTION_DOWN:
			game->snake_body[0].y++;
			break;

		case DIRECTION_LEFT:
			game->snake_body[0].x--;
			break;

		case DIRECTION_RIGHT:
			game->snake_body[0].x++;
			break;

		default:
			break;
	}

	game->snake_step_remain--;
}

static int
_snake_is_hitting_the_point(SnakeGame *game)
{
	return _point_is_overlap(&game->pt, &game->snake_body[0]);
}

static int
_snake_is_hitting_the_wall(SnakeGame *game)
{
	return _point_is_out_of_field(&game->snake_body[0], game->size_x, game->size_y);
}

static int
_snake_is_hitting_itself(SnakeGame *game)
{
	int i;
	for (i = 1; i < game->snake_len; i++)
	{
		if (_point_is_overlap(&game->snake_body[0], &game->snake_body[i]))
			return 1;
	}

	return 0;
}

static int
_snake_has_no_more_step_remain(SnakeGame *game)
{
	if (game->snake_step_remain == 0)
		return 1;

	return 0;
}

static int
_game_should_update(SnakeGame *game)
{
	struct timeval tv_now;

	/* Check how many time passed */
	gettimeofday(&tv_now, NULL);
	if (TV_USEC_DIFF(&game->tv_last_step, &tv_now) < 1000000 / game->step_per_sec)
		return 0;	/* Return 0 because it's not the time yet */

	/* Update the timestamp and return 1 */
	game->tv_last_step = tv_now;
	return 1;
}

static void
_game_point_go_random(SnakeGame *game)
{
	char c;

	/*
	 * Prevent the next random point position is in the snake body
	 */
	_point_go_random(&game->pt, game->size_x, game->size_y);
	c = game->display_bg[_display_get_index(game, game->pt.x, game->pt.y)];
	while (c != ' ' && c != '\0')
	{
		_point_go_random(&game->pt, game->size_x, game->size_y);
		c = game->display_bg[_display_get_index(game, game->pt.x, game->pt.y)];
	}
}

static void
_game_over(SnakeGame *game, const char *reason)
{
	if (reason)
		strncpy(game->game_over_reason, reason, sizeof(game->game_over_reason));

	game->game_over = 1;
}

static int
_display_get_index(SnakeGame *game, int x, int y)
{
	return y * game->size_x + x;
}

static void
_display_update_background(SnakeGame *game)
{
	int i;
	char snake_head_char = 'O';
	char snake_body_char = 'o';

	/*
	 * 0. Initialize whole display
	 */
	memset(game->display_bg, ' ', game->size_x * game->size_y);

	/*
	 * 1. Draw the snake
	 */

	if (snake_game_is_over(game))
	{
		snake_body_char = 'x';
		snake_head_char = 'X';
	}
	/* The snake head */
	if (!_point_is_out_of_field(&game->snake_body[0], game->size_x, game->size_y))
		game->display_bg[_display_get_index(game, game->snake_body[0].x, game->snake_body[0].y)] = snake_head_char;
	/* The snake body
	 * i = 1: Skip the head we already draw
	 */
	for (i = 1; i < game->snake_len; i++)
	{
		/* Skip the point which is out of field */
		if (_point_is_out_of_field(&game->snake_body[i], game->size_x, game->size_y))
			continue;

		game->display_bg[_display_get_index(game, game->snake_body[i].x, game->snake_body[i].y)] = snake_body_char;
	}

	/*
	 * 2. Draw the point
	 */
	game->display_bg[_display_get_index(game, game->pt.x, game->pt.y)] = '*';
}

static void
_display_update_foreground(SnakeGame *game)
{
	int x;
	int y;
	int char_index;

	/* Only update characters which should be replaced */
	for (y = 0; y < game->size_y; y++)
	{
		for (x = 0; x < game->size_x; x++)
		{
			char_index = _display_get_index(game, x, y);
			if (game->display_fg[char_index] != game->display_bg[char_index])
			{
				/* 
				 * Move terminal cursor
				 * plus 2: skip 0 and the border(1)
				 */
				_terminal_cursor_move(x + 2, y + 2);

				/* Print */
				putchar(game->display_bg[char_index]);

				/* Display update */
				game->display_fg[char_index] = game->display_bg[char_index];
			}
		}
	}

	_terminal_cursor_move(0, game->size_y + 3);
	printf("Step Remain: %3d, Score: %4d\n", game->snake_step_remain, snake_game_get_score(game));
}

static void
_terminal_cursor_move(int x, int y)
{
	printf("\033[%d;%dH", y, x);
}

SnakeGame *
snake_game_create(int x, int y, int step_per_sec, int max_step)
{
	SnakeGame *ng;

	if (x < 0 || y < 0)
		return NULL;

	ng = malloc(sizeof(SnakeGame));

	/* Initialize the game info */
	gettimeofday(&ng->tv_last_step, NULL);
	ng->step_per_sec = step_per_sec;
	ng->max_step = max_step;
	ng->game_over = 0;
	ng->size_x = x;
	ng->size_y = y;

	/* Initailize the snake */
	ng->snake_body = malloc(sizeof(Point) * x * y);
	ng->snake_len = 1;
	ng->snake_dir = DIRECTION_NONE;
	ng->snake_step_remain = ng->max_step;
	_point_go_random(&ng->snake_body[0], ng->size_x, ng->size_y);

	_point_go_random(&ng->pt, ng->size_x, ng->size_y);

	/* So many characters for display */
	ng->display_bg = malloc(sizeof(char) * x * y);
	ng->display_fg = malloc(sizeof(char) * x * y);
	memset(ng->display_bg, ' ', ng->size_x * ng->size_y);
	memset(ng->display_fg, ' ', ng->size_x * ng->size_y);

	return ng;
}

void
snake_game_free(SnakeGame *game)
{
	free(game->snake_body);
	free(game->display_bg);
	free(game->display_fg);

	free(game);
}

void
snake_game_set_direction(SnakeGame *game, DIRECTION dir)
{
	game->snake_dir = dir;
}

void
snake_game_update(SnakeGame *game)
{
	/* Use tv to check update or not */
	if (!_game_should_update(game))
		return;

	/* Move the snake */
	_snake_move(game);

	/* Check Snake head touch the point */
	if (_snake_is_hitting_the_point(game))
	{
		_snake_eat(game);
		_game_point_go_random(game);
	}

	/* Check if the game fail */
	if (_snake_is_hitting_itself(game))
	{
		_game_over(game, "The snake hit itself.");
	}
	else if (_snake_is_hitting_the_wall(game))
	{
		_game_over(game, "The snake hit the wall.");
	}
	else if (_snake_has_no_more_step_remain(game))
	{
		_game_over(game, "The snake died because of hunger.");
	}

	/* Draw the display in the background */
	_display_update_background(game);

	/*
	 * Show up in the foreground
	 * by updating the characters which should be updated
	 */
	_display_update_foreground(game);
}

void
snake_game_show(SnakeGame *game)
{
	int x;
	int y;
	int char_index;
	/* Clear screen and show once */
	system("clear");

	_display_update_background(game);
	memcpy(game->display_fg, game->display_bg, sizeof(char) * game->size_x * game->size_y);

	/* Upper border */
	putchar('+');
	for (x = 0; x < game->size_x; x++)
	{
		putchar('-');
	}
	putchar('+');
	putchar('\n');

	for (y = 0; y < game->size_y; y++)
	{
		/* Left border */
		putchar('|');
		for (x = 0; x < game->size_x; x++)
		{
			char_index = _display_get_index(game, x, y);
			putchar(game->display_fg[char_index]);
		}
		/* Right border */
		putchar('|');
		putchar('\n');
	}

	/* Lower border */
	putchar('+');
	for (x = 0; x < game->size_x; x++)
	{
		putchar('-');
	}
	putchar('+');
	putchar('\n');
}

void
snake_game_over(SnakeGame *game, const char *reason)
{
	_game_over(game, reason);
}

int
snake_game_is_over(SnakeGame *game)
{
	return game->game_over;
}

int
snake_game_get_score(SnakeGame *game)
{
	/* minus 1 because there's always snake head */
	return game->snake_len - 1;
}

const char *
snake_game_get_game_over_reason(SnakeGame *game)
{
	/* The string is empty, no game over reason yet */
	if (game->game_over_reason[0] == '\0')
		return NULL;

	return game->game_over_reason;
}
