char *check_object(char dst[BUFSIZE], struct stage_t *stage, enum object_t object)
{
	int count = 0;
	char src[BUFSIZE];
	struct cell_t *cellp;

	memset(src, 0, BUFSIZE);
	memset(dst, 0, BUFSIZE);

	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, row, col);

			if (cellp->object == object) {
				if (count != 0)
					strlcat(dst, ", ", BUFSIZE);
				snprintf(src, BUFSIZE, "[%d,%d]", col + 1, row + 1);
				strlcat(dst, src, BUFSIZE);
				count++;
			}
		}
	}
	return dst;
}

char *check_bomb(char dst[BUFSIZE], struct stage_t *stage)
{
	int count = 0;
	char src[BUFSIZE];
	struct cell_t *cellp;

	memset(src, 0, BUFSIZE);
	memset(dst, 0, BUFSIZE);

	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, row, col);

			if (cellp->object == BOMB) {
				if (count != 0)
					strlcat(dst, ", ", BUFSIZE);
				snprintf(src, BUFSIZE, "{\"pos\":{\"x\": %d, \"y\": %d}, \"timer\": %d, \"power\": %d}",
					col + 1, row + 1, cellp->bomb.timer, cellp->bomb.power);
				strlcat(dst, src, BUFSIZE);
				count++;
			}
		}
	}
	return dst;
}

char *check_item(char dst[BUFSIZE], struct stage_t *stage)
{
	int count = 0;
	char src[BUFSIZE];
	struct cell_t *cellp;

	memset(src, 0, BUFSIZE);
	memset(dst, 0, BUFSIZE);

	for (int row = 0; row < STAGE_HEIGHT; row++) {
		for (int col = 0; col < STAGE_WIDTH; col++) {
			cellp = get_cell(stage, row, col);

			if (cellp->item != NO_ITEM) {
				if (count != 0)
					strlcat(dst, ", ", BUFSIZE);
				snprintf(src, BUFSIZE, "{\"pos\":{\"x\": %d, \"y\": %d}, \"name\": \"%s\"}",
					col + 1, row + 1, (cellp->item == IPOWER) ? "力": "弾");
				strlcat(dst, src, BUFSIZE);
				count++;
			}
		}
	}
	return dst;
}

char *check_player(char dst[BUFSIZE], struct player_t player[NPLAYER])
{
	char src[BUFSIZE];

	memset(src, 0, BUFSIZE);
	memset(dst, 0, BUFSIZE);

	for (int p = 0; p < NPLAYER; p++) {
		if (p != 0)
			strlcat(dst, ",", BUFSIZE);
		snprintf(src, BUFSIZE, "{\"id\": %d, \"name\": \"%d\", \"pos\": {\"x\": %d, \"y\": %d}, "
			"\"power\": %d, \"setBombLimit\": %d, \"setBombCount\": %d, \"isAlive\": %s}",
			p, p, player[p].x + 1, player[p].y + 1,
			player[p].bomb_power, player[p].bomb_limit, player[p].set_bomb_count,
			player[p].is_alive ? "true": "false");
		strlcat(dst, src, BUFSIZE);

		/*
		fprintf(fp, "\"name\": \"%d\", ", p);
		fprintf(fp, "\"pos\": {\"x\": %d, \"y\": %d}, ", player[p].x + 1, player[p].y + 1);
		fprintf(fp, "\"power\": %d, \"setBombLimit\": %d, \"setBombCount\": %d, ",
			player[p].bomb_power, player[p].bomb_limit, player[p].set_bomb_count);
		fprintf(fp, "\"isAlive\": %s}", player[p].is_alive ? "true": "false");
		*/
	}
	return dst;
}

void output_json(int fd, struct stage_t *stage, struct player_t player[NPLAYER])
{
	char buf[BUFSIZE], dst[BUFSIZE], src[BUFSIZE];

	memset(buf, 0, BUFSIZE);
	memset(dst, 0, BUFSIZE);

	snprintf(buf, BUFSIZE, "{\"turn\":%d,", stage->turn);

	snprintf(dst, BUFSIZE, "\"walls\": [%s],", check_object(src, stage, WALL));
	strlcat(buf, dst, BUFSIZE);

	snprintf(dst, BUFSIZE, "\"blocks\": [%s],", check_object(src, stage, BLOCK));
	strlcat(buf, dst, BUFSIZE);

	snprintf(dst, BUFSIZE, "\"players\": [%s],", check_player(src, player));
	strlcat(buf, dst, BUFSIZE);

	snprintf(dst, BUFSIZE, "\"bombs\": [%s],", check_bomb(src, stage));
	strlcat(buf, dst, BUFSIZE);

	snprintf(dst, BUFSIZE, "\"items\": [%s],", check_item(src, stage));
	strlcat(buf, dst, BUFSIZE);

	snprintf(dst, BUFSIZE, "\"fires\": [%s]}\n", check_object(src, stage, FIRE));
	strlcat(buf, dst, BUFSIZE);

	ewrite(fd, buf, strlen(buf));
	logging(DEBUG, "%s", buf);
}
