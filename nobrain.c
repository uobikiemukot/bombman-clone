#include "error.h"
#include "bombman.h"
#include "util.h"

int main()
{
	int rsize;
	char buf[BUFSIZE];
	static const char *directions[] = {
		"STAY", "LEFT", "RIGHT", "UP", "DOWN",
	};

	(void) player_command;

	//setvbuf(stdin, NULL, _IONBF, 0);
	//setvbuf(stdout, NULL, _IONBF, 0);

	if (ewrite(STDOUT_FILENO, "nobrain\n", 8) < 0)
		return EXIT_FAILURE;

	if (eread(STDIN_FILENO, buf, BUFSIZE) < 0)
		return EXIT_FAILURE;
	/*
	if (fgets(buf, BUFSIZE, stdin) == NULL)
		return EXIT_FAILURE;
	*/

	srand(time(NULL));

	while (1) {
		/*
		if (fgets(buf, BUFSIZE, stdin) == NULL)
			break;
		fprintf(stdout, "%s,false,read size:%d\n",
			directions[rand() % (sizeof(directions) / sizeof(char *))], strlen(buf));
		*/

		if ((rsize = eread(STDIN_FILENO, buf, BUFSIZE)) > 0) {
			snprintf(buf, BUFSIZE, "%s,false,%d\n",
				directions[rand() % (sizeof(directions) / sizeof(char *))], rsize);
			if (ewrite(STDOUT_FILENO, buf, strlen(buf)) < 0)
				break;
		}
	}
	return EXIT_SUCCESS;
}
