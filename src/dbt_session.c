#include <stdio.h>
#include <string.h>

#include "dbt.h"


static WINDOW *dbt_generate_window(int height, int width, int x, int y, const char *title) {
	WINDOW *result = newwin(height, width, x, y);
	box(result, 0, 0);
	mvwprintw(result, 0, 2, "%s", title);
	
	return result;
}

static int dbt_session_commit_input(struct dbt_session *session) {
	switch (session->mode) {
		case DBT_MODE_SERVER_SELECT:
			return dbt_servers_select(session->input_buffer, session);
		case DBT_MODE_DATABASE_SELECT:
			return dbt_databases_select(session->input_buffer, session);
		case DBT_MODE_SCHEMA_SELECT:
			return dbt_schemas_select(session->input_buffer, session);
		case DBT_MODE_TABLEVIEW_SELECT:
			return dbt_tables_select(session->input_buffer, session);
		default:
			break;
	}

	return 0;
}




int dbt_session_handle_input(int input, struct dbt_session *session) {
	/* Check input */
	if (!session) return 1;


	/* Process input for normal mode */
	if (session->mode == DBT_MODE_NORMAL) {
		switch (input) {
			case 'S':
				/* Enter server mode */
				session->mode = DBT_MODE_SERVER_SELECT;
				break;
			case 'd':
				/* Enter database mode */
				session->mode = DBT_MODE_DATABASE_SELECT;
				break;
			case 's':
				/* Enter schema mode */
				session->mode = DBT_MODE_SCHEMA_SELECT;
				break;
			case 't':
				/* Enter table mode */
				session->mode = DBT_MODE_TABLEVIEW_SELECT;
				break;
			case 'c':
				/* Enter column mode */
				session->mode = DBT_MODE_COLUMN_SELECT;
				break;
		}


		/* Check if mode changed */
		if (session->mode != DBT_MODE_NORMAL) {
			/* Set input prompt */
			switch (session->mode) {
				case DBT_MODE_NORMAL:
					break;
				case DBT_MODE_SERVER_SELECT:
					printw("Server: ");
					break;
				case DBT_MODE_DATABASE_SELECT:
					printw("Database: ");
					break;
				case DBT_MODE_SCHEMA_SELECT:
					printw("Schema: ");
					break;
				case DBT_MODE_TABLEVIEW_SELECT:
					printw("Table/View: ");
					break;
				case DBT_MODE_COLUMN_SELECT:
					printw("Column: ");
					break;
			}


			/* Show cursor */
			curs_set(1);
			refresh();
		}

		return 0;
	}


	/* Handle input for other modes */
	if (input == 13) {
		/* Commit (ENTER) */
		dbt_session_commit_input(session); // TODO: do not clear if this fails


		/* Switch to normal mode */
		session->mode = DBT_MODE_NORMAL;


		/* Reset input */
		for (size_t i=0; i <= session->buffer_head; i++) session->input_buffer[i] = 0;
		session->buffer_head = 0;


		/* Clear input area */
		move(LINES-1, 0);
		clrtoeol();


		/* Hide cursor */
		curs_set(0);
		refresh();


		return 0;
	} else if (input == 8 || input == 127) {
		/* Backspace */
		if (session->buffer_head <= 0) return 0;
		session->input_buffer[--session->buffer_head] = 0;

		
		/* Move to previous character */
		int cur_y, cur_x;
		getyx(stdscr, cur_y, cur_x);
		move(cur_y, cur_x-1);


		/* Delete character */
		delch();
		refresh();


		return 0;
	} else if (input < ' ' || input > '~') {
		/* Out of range of supported ascii characters */
		return 0;
	}

	
	/* Append to buffer */
	if (session->buffer_head >= 63) return 0;
	session->input_buffer[session->buffer_head++] = (char)input;

	
	/* Print */
	addch(input);
	refresh();


	return 0;
}


int dbt_session_init(const char *config_path, struct dbt_session *session) {
	/* Check input */
	if (!session) return 1;


	/* Generate windows */
	session->app_windows[DBT_WIN_SERVERS] = dbt_generate_window(10, 30, 0, 0, "Servers");
	session->app_windows[DBT_WIN_DATABASES] = dbt_generate_window(10, 30, 10, 0, "Databases");
	session->app_windows[DBT_WIN_SCHEMAS] = dbt_generate_window(10, 30, 20, 0, "Schemas");
	session->app_windows[DBT_WIN_TABLESVIEWS] = dbt_generate_window(LINES-31, 30, 30, 0, "Tables/Views");
	session->app_windows[DBT_WIN_COLUMNS] = dbt_generate_window(LINES-1, 30, 0, 30, "Columns");
	session->app_windows[DBT_WIN_ACTION] = dbt_generate_window(30, COLS-60, 0, 60, "Action");
	session->app_windows[DBT_WIN_RESULT] = dbt_generate_window(LINES-31, COLS-60, 30, 60, "Results");


	/* Refresh windows (display) */
	size_t window_count = sizeof(session->app_windows) / sizeof(WINDOW *);
	for (size_t i=0; i < window_count; i++) wrefresh(session->app_windows[i]);


	/* Put session to 'normal' mode */
	session->mode = DBT_MODE_NORMAL;


	/* Determine final config path */
	char *final_config_path = (char *)config_path;
	if (!config_path) {
		const char *home_dir = getenv("HOME");
		size_t home_dir_len = strlen(home_dir);

		const char *dbt_config = "/.dbtui/config.json";
		size_t dbt_config_len = strlen(dbt_config);

		size_t config_path_len = home_dir_len + dbt_config_len + 1;
		final_config_path = (char *)calloc(config_path_len, sizeof(char));
		if (!final_config_path) return 1;

		strcat(final_config_path, home_dir);
		strcat(final_config_path, dbt_config);
	}


	/* Load config file */
	FILE *config_file = fopen(final_config_path, "r");
	if (!config_file) return 1;
	if (!config_path) free(final_config_path);


	/* Parse config file */
	session->config = json_loadf(config_file, 1, 0);
	fclose(config_file);
	if (!session->config) return 1;


	/* Init input buffer */
	session->input_buffer[0] = 0;
	session->buffer_head = 0;


	/* Put cursor to resting position (and hide) */
	move(LINES-1, 0);
	curs_set(0);
	refresh();

	return 0;
}
