#include <stdlib.h>
#include <string.h>

#include "dbt.h"



/* Helper functions */
static void app_exit(int reason) {
	endwin();
	exit(reason);
}


/* Entry point */
int main(int argc, const char **argv) {
	/* Init ncurses */
	initscr();
	raw();
	noecho();
	refresh();


	/* Init session */
	struct dbt_session session;
	if (dbt_session_init(argc > 1 ? argv[1] : 0, &session)) app_exit(1);


	/* Load servers */
	if (dbt_servers_refresh(&session)) app_exit(2);


	/* Start main loop */
	for (;;) {
		/* Get input */
		int input = getchar();


		/* Handle quit or mode-quit */
		if (session.mode == DBT_MODE_NORMAL && input == 'q') break;
		else if (input == CTRL('c')) { 
			session.mode = DBT_MODE_NORMAL;

			
			/* Reset input */
			for (size_t i=0; i <= session.buffer_head; i++) session.input_buffer[i] = 0;
			session.buffer_head = 0;


			/* Clear input area */
			move(LINES-1, 0);
			clrtoeol();


			/* Hide cursor */
			curs_set(0);
			refresh();

			continue;
		}


		/* Handle input */
		if (dbt_session_handle_input(input, &session)) break;
	}


	/* Cleanup */
	if (session.config) json_decref(session.config);
	endwin();
	return 0;
}
