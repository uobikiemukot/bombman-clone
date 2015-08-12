struct action_t random_action(struct stage_t *stage, struct player_t player[NPLAYER], int p)
{
	int count = 0;
	struct cell_t *cellp;
	struct action_t action[NDIRECTION];

	memset(action, 0, sizeof(struct action_t) * NDIRECTION);

	/* STAY */
	action[0].direction = STAY;
	count++;

	/* LEFT */
	cellp = get_cell(stage, player[p].y, player[p].x - 1);
	if (cellp != NULL && cellp->object == NONE) {
		action[count].direction = LEFT;
		count++;
	}

	/* RIGHT */
	cellp = get_cell(stage, player[p].y, player[p].x + 1);
	if (cellp != NULL && cellp->object == NONE) {
		action[count].direction = RIGHT;
		count++;
	}

	/* UP */
	cellp = get_cell(stage, player[p].y - 1, player[p].x);
	if (cellp != NULL && cellp->object == NONE) {
		action[count].direction = UP;
		count++;
	}

	/* DOWN */
	cellp = get_cell(stage, player[p].y + 1, player[p].x);
	if (cellp != NULL && cellp->object == NONE) {
		action[count].direction = DOWN;
		count++;
	}

	return action[rand() % NDIRECTION];
}

struct action_t manual_action(struct stage_t *stage, struct player_t player[NPLAYER], int p)
{
	struct timeval tv;
	fd_set fds;
	struct action_t action;
	int size;
	char buf[BUFSIZE], c;

	(void) stage;

	memset(&action, 0, sizeof(struct action_t));
	memset(buf, 0, BUFSIZE);

	FD_ZERO(&fds);
	FD_SET(player[p].fds.rfd, &fds);

	tv.tv_sec  = 0;
	tv.tv_usec = SELECT_TIMEOUT;

	eselect(player[p].fds.rfd + 1, &fds, NULL, NULL, &tv);

	if (FD_ISSET(player[p].fds.rfd, &fds)
		&& (size = eread(player[p].fds.rfd, buf, BUFSIZE)) > 0) {

		if (buf[0] == ' ') {
			action.set_bomb = true;
			c = buf[1];
		} else {
			c = buf[0];
		}

		switch(c) {
		case 'h':
			action.direction = LEFT;
			break;
		case 'j':
			action.direction = DOWN;
			break;
		case 'k':
			action.direction = UP;
			break;
		case 'l':
			action.direction = RIGHT;
			break;
		case ' ':
			break;
		}
	}

	return action;
}

int not_comma(int c)
{
	if (iscntrl(c) || c == ',')
		return false;
	else
		return true;
}

struct action_t parse_subprocess_input(int fd)
{
	char buf[BUFSIZE], *cp;
	int size;
	struct action_t action;
	struct parm_t parm;

	memset(buf, 0, BUFSIZE);
	memset(&action, 0, sizeof(struct action_t));
	reset_parm(&parm);

	if ((size = eread(fd, buf, BUFSIZE)) > 0) {
		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';
		logging(DEBUG, "read data from subprocess: %s\n", buf);
		parse_arg(buf, &parm, ',', not_comma);

		if (parm.argc >= 2) {
			if (strncmp(parm.argv[0], "UP", 2) == 0)
				action.direction = UP;
			else if (strncmp(parm.argv[0], "DOWN", 4) == 0)
				action.direction = DOWN;
			else if (strncmp(parm.argv[0], "LEFT", 4) == 0)
				action.direction = LEFT;
			else if (strncmp(parm.argv[0], "RIGHT", 5) == 0)
				action.direction = RIGHT;
			else
				action.direction = STAY;

			if (strncmp(parm.argv[1], "true", 4) == 0)
				action.set_bomb = true;
			else
				action.set_bomb = false;

		}

		if (parm.argc >= 3) {
			strncpy(action.comment, parm.argv[2], BUFSIZE - 1);
		}
	}

	return action;
}

struct action_t subprocess_action(struct stage_t *stage, struct player_t player[NPLAYER], int p)
{
	fd_set fds;
	struct timeval tv;
	struct action_t action;

	memset(&action, 0, sizeof(struct action_t));

	/* write date */
	logging(DEBUG, "write data to subprocess[%d]\n", p);
	output_json(player[p].fds.wfd, stage, player);

	/* read data */
	FD_ZERO(&fds);
	FD_SET(player[p].fds.rfd, &fds);

	tv.tv_sec  = 0;
	tv.tv_usec = SELECT_TIMEOUT;

	eselect(player[p].fds.rfd + 1, &fds, NULL, NULL, &tv);

	if (FD_ISSET(player[p].fds.rfd, &fds))
		action = parse_subprocess_input(player[p].fds.rfd);
	else
		logging(WARN, "no data from subprocess[%d]\n", p);

	return action;
}
