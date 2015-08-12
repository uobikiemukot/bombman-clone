bool is_valid_pos(int y, int x)
{
	if (x < 0 || x >= STAGE_WIDTH
		|| y < 0 || y >= STAGE_HEIGHT)
		return false;
	else
		return true;
}

static inline struct cell_t *get_cell(struct stage_t *stage, int y, int x)
{
	if (is_valid_pos(y, x) == false)
		return NULL;
	else
		return &stage->cells[y * STAGE_WIDTH + x];
}

bool is_near_default_pos(int y, int x)
{
	int distance;
	extern const int default_pos[][2];

	for (int p = 0; p < NPLAYER; p++) {
		distance = abs(default_pos[p][Y] - y)
			+ abs(default_pos[p][X] - x);

		if (distance <= 1)
			return true;
	}
	return false;
}

void clear_screen()
{
	printf("\033[1;1H\033[2J");
}

int num_of_players(struct player_t player[NPLAYER])
{
	int sum = 0;

	for (int p = 0; p < NPLAYER; p++)
		if (player[p].is_alive)
			sum++;

	return sum;
}

/*
void show_map(struct stage_t *stage)
{
	struct cell_t *cellp;

	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, &(struct pos_t){.y = row, .x = col});

			if (cellp->player)
				printf("@");
			else if (cellp->object != NONE)
				printf("%c", object2char[cellp->object]);
			else
				printf("%c", item2char[cellp->item]);

			if (col == (STAGE_WIDTH - 1))
				printf("\n");
		}
	}

	printf("turn:%d\n", stage->turn);
}

void show_player(struct player_t player[NPLAYER])
{
	for (int p = 0; p < NPLAYER; p++) {
		printf("player[%d] pos:(%.2d, %.2d) power:%d limit:%d count:%d alive:%c\n",
			p, player[p].pos.x, player[p].pos.y,
			player[p].bomb_power, player[p].bomb_limit,
			player[p].set_bomb_count, player[p].is_alive ? 'Y': 'N');
			
	}
}
*/

/*
int my_ceil(int val, int div)
{
	if (div == 0)
		return 0;
	else
		return (val + div - 1) / div;
}
*/

int dec2num(char *str)
{
	if (str == NULL)
		return 0;

	return estrtol(str, NULL, 10);
}

/*
int hex2num(char *str)
{
	if (str == NULL)
		return 0;

	return estrtol(str, NULL, 16);
}
*/
