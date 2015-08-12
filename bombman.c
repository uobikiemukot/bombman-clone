#include "error.h"
#include "bombman.h"
#include "util.h"
#include "strl.h"
#include "ui.h"
#include "io.h"
#include "parse_arg.h"
#include "ai.h"

void sig_handler(int signo)
{
	extern volatile sig_atomic_t reload_action;
	extern volatile sig_atomic_t turn_limit;

    if (signo == SIGALRM)
		reload_action = true;
	else if (signo == SIGINT)
		turn_limit = 0;
}

bool set_signal(int signo, void (*handler)(int signo), int flags)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags   = flags;

	if (esigaction(signo, &sigact, NULL) == -1) {
		logging(ERROR, "signo: %d\n", signo);
		return false;
	}
	return true;
}

bool set_timer(int timer_type, time_t sec, suseconds_t usec)
{
	struct itimerval itimer;

	itimer.it_interval.tv_sec  = sec;
	itimer.it_interval.tv_usec = usec;
	itimer.it_value = itimer.it_interval;

	if (esetitimer(timer_type, &itimer, NULL) == -1)
		return false;

	return true;
}

void set_rawmode(int fd, struct termios *save_tm)
{
    struct termios tm;

    etcgetattr(fd, save_tm);
    tm = *save_tm;
    //tm.c_iflag     = tm.c_oflag = 0;
    //tm.c_cflag    &= ~CSIZE;
    //tm.c_cflag    |= CS8;
    //tm.c_lflag    &= ~(ECHO | ISIG | ICANON);
    tm.c_lflag    &= ~(ECHO | ICANON);
    tm.c_cc[VMIN]  = 1; /* min data size (byte) */
    tm.c_cc[VTIME] = 0; /* time out */
    etcsetattr(fd, TCSAFLUSH, &tm);
}

void set_object_randomly(struct stage_t *stage, enum object_t object, int limit)
{
	int y, x, count = 0, fail_count = 0;
	struct cell_t *cellp;

	while (count < limit) {
		if (fail_count > FAIL_LIMIT)
			break;

		y = rand() % STAGE_HEIGHT;
		x = rand() % STAGE_WIDTH;
		cellp = get_cell(stage, y, x);

		if (cellp->object != NONE || is_near_default_pos(y, x)) {
			fail_count++;
			continue;
		}

		cellp->object = object;
		count++;
	}
}

void set_item_randomly(struct stage_t *stage, enum item_t item, int limit)
{
	int y, x, count = 0, fail_count = 0;
	struct cell_t *cellp;

	while (count < limit) {
		if (fail_count > FAIL_LIMIT)
			break;

		y = rand() % STAGE_HEIGHT;
		x = rand() % STAGE_WIDTH;
		cellp = get_cell(stage, y, x);

		if (cellp->object != BLOCK) {
			fail_count++;
			continue;
		}

		cellp->item = item;
		count++;
	}
}

void init_stage(struct stage_t *stage, struct player_t player[NPLAYER]) {
	struct cell_t *cellp;

	memset(stage->cells, 0, sizeof(stage->cells));

	stage->turn = 0;

	/* add players */
	for (int p = 0; p < NPLAYER; p++)
		get_cell(stage, player[p].y, player[p].x)->player = &player[p];

	/* add walls and blocks */
	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, row, col);
			if (cellp->object != NONE)
				continue;

			if (row == 0 || row == STAGE_HEIGHT - 1
				|| col == 0 || col == STAGE_WIDTH - 1
				|| ((row % 2) == 0 && (col % 2) == 0)) {
				cellp->object = WALL;
			}
		}
	}

	/* set items */
	set_object_randomly(stage, BLOCK, NBLOCK);
	set_item_randomly(stage, IBOMB, NBOMB);
	set_item_randomly(stage, IPOWER, NFIRE);
}

bool fork_subprocess(struct player_t *player, const char *player_command)
{
	pid_t pid;
	int rfd[2], wfd[2];

	if (pipe(rfd) < 0 || pipe(wfd) < 0) {
		perror("pipe");
		return false;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		return false;
	} else if (pid == 0) { /* child */
		close(rfd[R]);
		close(wfd[W]);

		if (dup2(rfd[W], STDOUT_FILENO) < 0
			|| dup2(wfd[R], STDIN_FILENO < 0)) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}

		close(rfd[W]);
		close(wfd[R]);

		if (execl(player_command, player_command, NULL) < 0) {
			perror("execl");
			exit(EXIT_FAILURE);
		}
	}
	/* parent */
	close(rfd[W]);
	close(wfd[R]);

	player->type     = SUBPROCESS;
	player->fds.rfd  = rfd[R];
	player->fds.wfd  = wfd[W];
	player->child_id = pid;

	logging(DEBUG, "subprocess rfd:%d wfd:%d pid:%d\n",
		player->fds.rfd, player->fds.wfd, player->child_id);

	return true;
}

void manual_player_init(struct player_t *player)
{
	player->type     = MANUAL;
	player->fds.rfd  = STDIN_FILENO;
	player->fds.wfd  = -1;
	player->child_id = -1;
}

void random_player_init(struct player_t *player)
{
	player->type     = RANDOM;
	player->fds.rfd  = -1;
	player->fds.wfd  = -1;
	player->child_id = -1;
}

bool init_player(struct player_t player[NPLAYER], const char *player_command[NPLAYER])
{
	char buf[BUFSIZE];
	int size;

	memset(player, 0, sizeof(struct player_t) * NPLAYER);

	for (int p = 0; p < NPLAYER; p++) {
		player[p].y = default_pos[p][Y];
		player[p].x = default_pos[p][X];
		player[p].bomb_power     = DEFAULT_BOMB_POWER;
		player[p].bomb_limit     = DEFAULT_BOMB_LIMIT;
		player[p].set_bomb_count = 0;
		player[p].is_alive       = true;

		if (strncmp(player_command[p], "manual", 6) == 0) {
			manual_player_init(&player[p]);
		} else if (strncmp(player_command[p], "random", 6) == 0) {
			random_player_init(&player[p]);
		} else if (fork_subprocess(&player[p], player_command[p]) == true) {
			if (eread(player[p].fds.rfd, buf, BUFSIZE) > 0)
				logging(DEBUG, "client[%d] name: %s", p, buf);
			/* send client id */
			size = snprintf(buf, BUFSIZE, "%d\n", p);
			if (ewrite(player[p].fds.wfd, buf, size) != size)
				logging(WARN, "write size error\n");
		} else {
			logging(ERROR, "init player[%d] failed\n", p);
			return false;
		}
	}
	return true;
}

struct action_t get_next_action(struct stage_t *stage, struct player_t player[NPLAYER], int p)
{
	struct action_t action;

	if (player[p].type == MANUAL)
		return manual_action(stage, player, p);
	else if (player[p].type == RANDOM)
		return random_action(stage, player, p);
	else if (player[p].type == SUBPROCESS)
		return subprocess_action(stage, player, p);

	memset(&action, 0, sizeof(struct action_t));
	return action;
}

bool move(struct stage_t *stage, struct player_t *player, enum direction_t direction)
{
	int y, x;
	struct cell_t *cellp;
	static const int direction2offset[][2] = {
		[STAY]  = {0 , 0 },
		[LEFT]  = {0 , -1},
		[RIGHT] = {0 , +1},
		[UP]    = {-1, 0 },
		[DOWN]  = {+1, 0 },
	};

	/* check next pos */
	y = player->y + direction2offset[direction][Y];
	x = player->x + direction2offset[direction][X];

	cellp = get_cell(stage, y, x);

	if (is_valid_pos(y, x) == false)
		return false;

	if (cellp->object == WALL
		|| cellp->object == BLOCK
		|| cellp->object == BOMB)
		return false;
	
	if (cellp->player)
		return false;

	/* remove object at previous pos */
	get_cell(stage, player->y, player->x)->player = NULL;

	/* set object at new pos */
	player->y = y;
	player->x = x;
	cellp = get_cell(stage, y, x);
	cellp->player = player;

	/* get item */
	if (cellp->item != NO_ITEM) {
		if (cellp->item == IPOWER) {
			player->bomb_power += 1;
			cellp->item = NONE;
		} else if (cellp->item == IBOMB) {
			player->bomb_limit += 1;
			cellp->item = NONE;
		}
	}

	return true;
}

bool set_bomb(struct stage_t *stage, struct player_t *player)
{
	struct cell_t *cellp = get_cell(stage, player->y, player->x);

	if (player->bomb_limit == player->set_bomb_count)
		return false;

	if (cellp->object != NONE)
		return false;

	cellp->object = BOMB;
	cellp->bomb.power = player->bomb_power;
	cellp->bomb.timer = EXPLODE_TIME;
	cellp->bomb.owner = player;

	player->set_bomb_count += 1;

	return true;
}

bool sweep_cell(struct cell_t *cellp)
{
	if (cellp->object == WALL)
		return false;

	if (cellp->object == BLOCK) {
		cellp->object = FIRE;
		return false;
	}

	if (cellp->player) {
		cellp->player->is_alive = false;
		cellp->player->y = -1;
		cellp->player->x = -1;
		cellp->player = NULL;
	}

	cellp->object = FIRE;
	//cellp->item   = NO_ITEM;

	return true;
}

void explode(struct stage_t *stage, struct bomb_t *bomb, int y, int x)
{
	int target_y, target_x;
	struct cell_t *cellp;

	sweep_cell(get_cell(stage, y, x));

	for (int row = 1; row <= bomb->power; row++) {
		target_y = y + row;
		target_x = x;
		if ((cellp = get_cell(stage, target_y, target_x)) == NULL)
			break;

		/* induced explosion */
		if (cellp->object == BOMB)
			explode(stage, &cellp->bomb, target_y, target_x);

		if (sweep_cell(cellp) == false)
			break;
	}

	for (int row = -1; row >= -bomb->power; row--) {
		target_y = y + row;
		target_x = x;
		if ((cellp = get_cell(stage, target_y, target_x)) == NULL)
			break;

		/* induced explosion */
		if (cellp->object == BOMB)
			explode(stage, &cellp->bomb, target_y, target_x);

		if (sweep_cell(cellp) == false)
			break;
	}

	for (int col = 1; col <= bomb->power; col++) {
		target_y = y;
		target_x = x + col;
		if ((cellp = get_cell(stage, target_y, target_x)) == NULL)
			break;

		/* induced explosion */
		if (cellp->object == BOMB)
			explode(stage, &cellp->bomb, target_y, target_x);

		if (sweep_cell(cellp) == false)
			break;
	}

	for (int col = -1; col >= -bomb->power; col--) {
		target_y = y;
		target_x = x + col;
		if ((cellp = get_cell(stage, target_y, target_x)) == NULL)
			break;

		/* induced explosion */
		if (cellp->object == BOMB)
			explode(stage, &cellp->bomb, target_y, target_x);

		if (sweep_cell(cellp) == false)
			break;
	}

	bomb->owner->set_bomb_count -= 1;
	bomb->exploded_turn = stage->turn;
}

void extinguish(struct stage_t *stage, struct bomb_t *bomb, int y, int x)
{
	struct cell_t *cellp;

	for (int row = -bomb->power; row <= bomb->power; row++) {
		if ((cellp = get_cell(stage, y + row, x)) == NULL)
			continue;

		if (cellp->object == FIRE)
			cellp->object = NONE;
	}

	for (int col = -bomb->power; col <= bomb->power; col++) {
		if ((cellp = get_cell(stage, y, x + col)) == NULL)
			continue;

		if (cellp->object == FIRE)
			cellp->object = NONE;
	}

	memset(bomb, 0, sizeof(struct bomb_t));
}

void next(struct stage_t *stage, struct player_t player[NPLAYER], struct action_t action[NPLAYER])
{
	struct cell_t *cellp;

	/* parse each player's action */
	for (int p = 0; p < NPLAYER; p++) {
		if (player[p].is_alive == false)
			continue;

		if (action[p].set_bomb)
			set_bomb(stage, &player[p]);

		if (action[p].direction != STAY)
			move(stage, &player[p], action[p].direction);
	}

	/* check all cells */
	//logging(DEBUG, "--- turn:%d ---\n", stage->turn);
	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, row, col);

			if (cellp->object == BOMB) {
				cellp->bomb.timer -= 1;
				if (cellp->bomb.timer <= 0) {
					//logging(DEBUG, "explode (%d, %d) timer:%d\n",
						//col, row, cellp->bomb.timer);
					explode(stage, &cellp->bomb, row, col);
				}
			} else if (cellp->bomb.exploded_turn != 0 && cellp->bomb.exploded_turn < stage->turn) {
				//logging(DEBUG, "extinguish (%d, %d) exploded turn:%d\n",
					//col, row, cellp->bomb.exploded_turn);
				extinguish(stage, &cellp->bomb, row, col);
			}
		}
	}

	stage->turn += 1;
}

void suspend(int signo)
{
	sigset_t sigset;

	sigfillset(&sigset);
	sigdelset(&sigset, signo);
	sigsuspend(&sigset);
}

void kill_subprocess(struct player_t player[NPLAYER])
{
	for (int p = 0; p < NPLAYER; p++) {
		if (player[p].type == SUBPROCESS) {
			//kill(player[p].child_id, SIGKILL);
			kill(player[p].child_id, SIGTERM);
			close(player[p].fds.rfd);
			close(player[p].fds.wfd);
		}
	}
}

int main()
{
	struct stage_t stage;
	struct player_t player[NPLAYER];
	struct action_t action[NPLAYER];
	struct termios tm;
	extern volatile sig_atomic_t reload_action;
	extern volatile sig_atomic_t turn_limit;
	extern const char *player_command[];

	srand(time(NULL));
	if (set_signal(SIGALRM, sig_handler, SA_RESTART) == false
		|| set_signal(SIGINT, sig_handler, SA_RESTART) == false
		|| set_timer(ITIMER_REAL, REFRESH_INTERVAL, REFRESH_UINTERVAL) == false)
		return EXIT_FAILURE;
	set_rawmode(STDIN_FILENO, &tm);
	logging(DEBUG, "sigaction(), setitimer(), tcsetattr() ok\n");

	/* init */
	if (init_player(player, player_command) == false)
		goto init_player_failed;
	init_stage(&stage, player);
	logging(DEBUG, "init_player(), init_stage() ok\n");

	clear_screen();
	show_map(&stage);
	show_player(player);

	while (num_of_players(player) > 1 && stage.turn < turn_limit) {
		if (reload_action) {
			reload_action = false;

			for (int p = 0; p < NPLAYER; p++) {
				if (player[p].is_alive == false)
					continue;
				action[p] = get_next_action(&stage, player, p);
			}
			next(&stage, player, action);

			clear_screen();
			show_map(&stage);
			show_player(player);

			//suspend(SIGALRM);
		}
	}

	//output_json(stderr, &stage, player);
	kill_subprocess(player);
    etcsetattr(STDIN_FILENO, TCSAFLUSH, &tm);

	return EXIT_SUCCESS;

init_player_failed:
	kill_subprocess(player);
    etcsetattr(STDIN_FILENO, TCSAFLUSH, &tm);

	return EXIT_FAILURE;
}
