#include <string.h>

#include "dbt.h"


int dbt_columns_refresh(struct dbt_session *session) {
	/* Check input */
	if (!session) return 1;


	/* Load tables */
	session->column_list = session->adapter_handle.load_column_list(session->current_schema, session->current_table, &session->adapter_handle);
	if (!json_is_array(session->column_list)) return 1;


	/* Clear previous list */
	wclear(session->app_windows[DBT_WIN_COLUMNS]);
	box(session->app_windows[DBT_WIN_COLUMNS], 0, 0);
	mvwprintw(session->app_windows[DBT_WIN_COLUMNS], 0, 2, "Columns");


	/* Print new column list */
	size_t list_size = json_array_size(session->column_list);
	for (size_t i=0; i < list_size; i++) {
		json_t *column = json_array_get(session->column_list, i);
		const char *column_name = json_string_value(json_object_get(column, "name"));
		const char *column_type = json_string_value(json_object_get(column, "datatype"));
		const char *column_maxlength = json_string_value(json_object_get(column, "max_length"));
		const char *column_nullable = json_string_value(json_object_get(column, "nullable"));
		const char *column_identity = json_string_value(json_object_get(column, "is_identity"));

		mvwprintw(session->app_windows[DBT_WIN_COLUMNS], i+1, 2, "[ ] %s - %s", column_name, column_type);

		if (!strcmp(column_type, "varchar") || !strcmp(column_type, "nvarchar"))
			wprintw(session->app_windows[DBT_WIN_COLUMNS], "(%s)", column_maxlength);

		if (!strcmp(column_nullable, "NO"))
			wprintw(session->app_windows[DBT_WIN_COLUMNS], "/REQ");

		if (!strcmp(column_identity, "YES"))
			wprintw(session->app_windows[DBT_WIN_COLUMNS], "/ID");
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_COLUMNS]);


	return 0;
}


int dbt_columns_select(const char *column, struct dbt_session *session) {
	/* Check input */
	if (!column || !session) return 1;
	else if (!json_is_array(session->column_list)) return 1;


	/* Load requested column */
	int found_column = 0;
	size_t list_count = json_array_size(session->column_list);
	for (size_t i=0; i < list_count; i++) {
		json_t *column_obj = json_array_get(session->column_list, i);
		const char *column_name = json_string_value(json_object_get(column_obj, "name"));

		if (!strcmp(column_name, column)) {
			/* Set found table flag */
			found_column = 1;


			/* Mark as selected */
			mvwprintw(session->app_windows[DBT_WIN_COLUMNS], i+1, 2, "[*]");


			/* Store as current */
			session->current_column = column_name;


			/* Refresh properties window */
		} else mvwprintw(session->app_windows[DBT_WIN_COLUMNS], i+1, 2, "[ ]");
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_COLUMNS]);


	/* Clear windows/variables if column not found */
	if (!found_column) {
		session->current_column = 0;
	}



	return !found_column;
}
