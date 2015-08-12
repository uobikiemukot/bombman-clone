#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

enum misc_t {
	NPLAYER            = 4,
	STAGE_WIDTH        = 15,
	STAGE_HEIGHT       = 15,
	NBLOCK             = 89, /* default: 89 */
	NBOMB              = 10, /* default: 10 */
	NFIRE              = 10, /* default: 10 */
	DEFAULT_BOMB_POWER = 2,
	DEFAULT_BOMB_LIMIT = 2,
	EXPLODE_TIME       = 10,
	FIRE_TIME          = 1,
	TURN_LIMIT         = 1000,
	FAIL_LIMIT         = 1024,
	REFRESH_INTERVAL   = 1,     /* sec */
	REFRESH_UINTERVAL  = 0,     /* usec */
	BUFSIZE            = 8192,
	SELECT_TIMEOUT     = 500000, /* usec */
};

enum index_misc_t {
	Y = 0, /* for default_pos[][2] */
	X = 1,
	R = 0, /* for pipe() */
	W = 1,
};

enum item_t {
	NO_ITEM = 0,
	IBOMB,
	IPOWER,
	NITEM,
};

enum object_t {
	NONE = 0,
	WALL,
	BLOCK,
	//PLAYER,
	BOMB,
	FIRE,
	ITEM,
	NOBJECT,
};

enum player_type_t {
	UNKNOWN = 0,
	MANUAL,
	RANDOM,
	SUBPROCESS,
	NTYPE,
};

enum direction_t {
	STAY = 0,
	LEFT,
	RIGHT,
	DOWN,
	UP,
	NDIRECTION,
};

/*
struct pos_t {
	int y, x;
};
*/

struct fds_t {
	int rfd; /* read-only */
	int wfd; /* write-only */
	//int efd; /* read-only */
};

struct player_t {
	//struct pos_t pos;
	int y, x;
	int bomb_power;
	int bomb_limit;
	int set_bomb_count;
	bool is_alive;
	enum player_type_t type;
	struct fds_t fds;
	pid_t child_id;
};

struct bomb_t {
	int power;
	int timer;
	int exploded_turn;
	struct player_t *owner;
};

struct cell_t {
	enum object_t object;
	enum item_t item;
	struct bomb_t bomb;
	struct player_t *player;
};

struct stage_t {
	int turn;
	struct cell_t cells[STAGE_WIDTH * STAGE_HEIGHT];
};


struct action_t {
	enum direction_t direction;
	bool set_bomb;
	char comment[BUFSIZE];
};

static const char object2char[] = {
	[NONE]    = ' ',
	[WALL]    = '#',
	[BLOCK]   = 'x',
	[BOMB]    = 'B',
	[FIRE]    = 'F',
	[ITEM]    = '?',
	//[PLAYER]  = '@',
};

static const char item2char[] = {
	[NO_ITEM] = ' ',
	[IBOMB]   = 'b',
	[IPOWER]   = 'f',
};

/*
static const struct pos_t default_pos[] = {
	[0] = {.y = 1               , .x = 1              },
	[1] = {.y = 1               , .x = STAGE_WIDTH - 2},
	[2] = {.y = STAGE_HEIGHT - 2, .x = 1              },
	[3] = {.y = STAGE_HEIGHT - 2, .x = STAGE_WIDTH - 2},
}
*/
static const int default_pos[][2] = {
	[0] = {1               , 1              },
	[1] = {1               , STAGE_WIDTH - 2},
	[2] = {STAGE_HEIGHT - 2, 1              },
	[3] = {STAGE_HEIGHT - 2, STAGE_WIDTH - 2},
};

volatile sig_atomic_t reload_action = false;
volatile sig_atomic_t turn_limit    = TURN_LIMIT;

static const char *player_command[] = {
	[0] = "manual",
	[1] = "./nobrain",
	[2] = "random",
	[3] = "random",
};
