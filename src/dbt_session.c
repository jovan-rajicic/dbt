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
		case DBT_MODE_COLUMN_SELECT:
			return dbt_columns_select(session->input_buffer, session);
		default:
			break;
	}

	return 0;
}

static int dbt_session_commit_query(struct dbt_session *session) {
	/* Perform query */
	const char *query = session->q_buffers[session->q_buffer_ind];
	json_t *result = session->adapter_handle.perform_query(query, &session->adapter_handle);
	if (!json_is_object(result)) return 1;


	/* Clear previous result */
	wclear(session->app_windows[DBT_WIN_RESULT]);
	box(session->app_windows[DBT_WIN_RESULT], 0, 0);
	mvwprintw(session->app_windows[DBT_WIN_RESULT], 0, 2, "Result (1/7)");


	/* Print columns */
	json_t *column_list = json_object_get(result, "columns");
	size_t column_count = json_array_size(column_list);
	wmove(session->app_windows[DBT_WIN_RESULT], 2, 2);
	
	for (size_t i=0; i < column_count; i++) {
		const char *col_name = json_string_value(json_array_get(column_list, i));
		wprintw(session->app_windows[DBT_WIN_RESULT], "\t%s\t", col_name);
	}


	/* Print separator row */
	int maxX = getmaxx(session->app_windows[DBT_WIN_RESULT]);
	for (size_t i=1; i < maxX-1; i++) mvwprintw(session->app_windows[DBT_WIN_RESULT], 3, i, "+");


	/* Print rows */
	json_t *row_list = json_object_get(result, "rows");
	size_t row_count = json_array_size(row_list);

	for (size_t i=0; i < row_count; i++) {
		wmove(session->app_windows[DBT_WIN_RESULT], 4+i, 2);

		json_t *row_values = json_array_get(row_list, i);
		for (size_t j=0; j < column_count; j++) {
			const char *cell_value = json_string_value(json_array_get(row_values, j));
			wprintw(session->app_windows[DBT_WIN_RESULT], "\t%s\t", cell_value);
		}	
	}


	/* Refresh output */
	wrefresh(session->app_windows[DBT_WIN_RESULT]);


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
			case 'i':
				/* Enter query mode */
				session->mode = DBT_MODE_QUERY;
				break;
		}


		/* Check if mode changed */
		if (session->mode == DBT_MODE_QUERY) {	
			/* Init query buffer (TODO: improve) */
			if (!session->q_buffers[session->q_buffer_ind]) {
				session->q_buffers[session->q_buffer_ind] = (char *)calloc(4096, sizeof(char));
				session->q_buffer_head = 0;
			}


			/* Show cursor */
			curs_set(1);


			/* Move cursor to query window */
			wmove(session->app_windows[DBT_WIN_QUERY], 1, 2);
			wrefresh(session->app_windows[DBT_WIN_QUERY]);
		} else if (session->mode != DBT_MODE_NORMAL) {
			/* Set input prompt */
			switch (session->mode) {
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
				default:
					break;
			}


			/* Show cursor */
			curs_set(1);
			refresh();
		}

		return 0;
	}


	/* Handle input for query mode */
	if (session->mode == DBT_MODE_QUERY) {
		if (input == CTRL(13)) {
			/* Commit (CTRL + ENTER) */
			dbt_session_commit_query(session);

			return 0;
		} else if (input == 8 || input == 127) {
			/* Backspace */
			if (session->q_buffer_head <= 0) return 0;
			session->q_buffers[session->q_buffer_ind][--session->q_buffer_head] = 0;

			
			/* Move to previous character */
			int cur_y, cur_x;
			getyx(session->app_windows[DBT_WIN_QUERY], cur_y, cur_x);
			wmove(session->app_windows[DBT_WIN_QUERY], cur_y, cur_x-1);


			/* Delete character */
			waddch(session->app_windows[DBT_WIN_QUERY], ' ');
			wmove(session->app_windows[DBT_WIN_QUERY], cur_y, cur_x-1);
			wrefresh(session->app_windows[DBT_WIN_QUERY]);

			return 0;
		} else if (input < ' ' || input > '~') {
			/* Out of range of supported ascii characters */
			return 0;
		}


		/* Append to buffer (TODO: auto-newline, realloc, etc) */
		if (session->q_buffer_head >= 4095) return 0;
		session->q_buffers[session->q_buffer_ind][session->q_buffer_head++] = (char)input;


		/* Print */
		waddch(session->app_windows[DBT_WIN_QUERY], input);
		wrefresh(session->app_windows[DBT_WIN_QUERY]);

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
	session->app_windows[DBT_WIN_COLUMNS] = dbt_generate_window(LINES-1, 50, 0, 30, "Columns");
	session->app_windows[DBT_WIN_PROPERTIES] = dbt_generate_window(30, 40, 0, 80, "Properties");
	session->app_windows[DBT_WIN_QUERY] = dbt_generate_window(30, COLS-120, 0, 120, "Query (1/7)");
	session->app_windows[DBT_WIN_RESULT] = dbt_generate_window(LINES-31, COLS-80, 30, 80, "Results (1/7)");


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

	session->q_buffers[0] = 0;
	session->q_buffers[1] = 0;
	session->q_buffers[2] = 0;
	session->q_buffers[3] = 0;
	session->q_buffers[4] = 0;
	session->q_buffers[5] = 0;
	session->q_buffers[6] = 0;
	session->q_buffer_head = 0;
	session->q_buffer_ind = 0;


	/* Put cursor to resting position (and hide) */
	move(LINES-1, 0);
	curs_set(0);
	refresh();

	return 0;
}
