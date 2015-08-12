void show_map(struct stage_t *stage)
{
	struct cell_t *cellp;

	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, row, col);

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
			p, player[p].x, player[p].y,
			player[p].bomb_power, player[p].bomb_limit,
			player[p].set_bomb_count, player[p].is_alive ? 'Y': 'N');
			
	}
}
