/* parse_arg functions */
enum {
	MAX_ARGS = 16,
};

struct parm_t { /* for parse_arg() */
	int argc;
	char *argv[MAX_ARGS];
};

void reset_parm(struct parm_t *pt)
{
	pt->argc = 0;
	for (int i = 0; i < MAX_ARGS; i++)
		pt->argv[i] = NULL;
}

void add_parm(struct parm_t *pt, char *cp)
{
	if (pt->argc >= MAX_ARGS)
		return;

	logging(DEBUG, "argv[%d]: %s\n", pt->argc, (cp == NULL) ? "NULL": cp);

	pt->argv[pt->argc] = cp;
	pt->argc++;
}

void parse_arg(char *buf, struct parm_t *pt, int delim, int (is_valid)(int c))
{
	/*
		v..........v d           v.....v d v.....v ... d
		(valid char) (delimiter)
		argv[0]                  argv[1]   argv[2] ...   argv[argc - 1]
	*/
	size_t length;
	char *cp, *vp;

	if (buf == NULL)
		return;

	length = strlen(buf);
	logging(DEBUG, "parse_arg() length:%u\n", (unsigned) length);

	vp = NULL;
	for (size_t i = 0; i < length; i++) {
		cp = buf + i;

		if (vp == NULL && is_valid(*cp))
			vp = cp;

		if (*cp == delim) {
			*cp = '\0';
			add_parm(pt, vp);
			vp = NULL;
		}

		if (i == (length - 1) && (vp != NULL || *cp == '\0'))
			add_parm(pt, vp);
	}

	logging(DEBUG, "argc:%d\n", pt->argc);
}


int sum(struct parm_t *parm)
{
	int sum = 0;

	for (int i = 0; i < parm->argc; i++)
		sum += dec2num(parm->argv[i]);

	return sum;
}
