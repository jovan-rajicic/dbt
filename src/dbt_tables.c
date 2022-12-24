#include <string.h>

#include "dbt.h"


int dbt_tables_refresh(struct dbt_session *session) {
	/* Check input */
	if (!session) return 1;


	/* Load tables */
	session->table_list = session->adapter_handle.load_table_list(session->current_schema, &session->adapter_handle);
	if (!json_is_array(session->table_list)) return 1;


	/* Clear previous list */
	wclear(session->app_windows[DBT_WIN_TABLESVIEWS]);
	box(session->app_windows[DBT_WIN_TABLESVIEWS], 0, 0);
	mvwprintw(session->app_windows[DBT_WIN_TABLESVIEWS], 0, 2, "Tables/Views");


	/* Print new table list */
	size_t list_size = json_array_size(session->table_list);
	for (size_t i=0; i < list_size; i++) {
		const char *table_name = json_string_value(json_array_get(session->table_list, i));

		mvwprintw(session->app_windows[DBT_WIN_TABLESVIEWS], i+1, 2, "[ ] %s", table_name);
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_TABLESVIEWS]);


	return 0;
}

int dbt_tables_select(const char *table, struct dbt_session *session) {
	/* Check input */
	if (!table || !session) return 1;
	else if (!json_is_array(session->table_list)) return 1;


	/* Load requested table */
	int found_table = 0;
	size_t list_count = json_array_size(session->table_list);
	for (size_t i=0; i < list_count; i++) {
		const char *table_name = json_string_value(json_array_get(session->table_list, i));
		if (!strcmp(table_name, table)) {
			/* Set found table flag */
			found_table = 1;


			/* Mark as selected */
			mvwprintw(session->app_windows[DBT_WIN_TABLESVIEWS], i+1, 2, "[*]");


			/* Store as current */
			session->current_table = table_name;


			/* Refresh columns */
			dbt_columns_refresh(session);
		} else mvwprintw(session->app_windows[DBT_WIN_TABLESVIEWS], i+1, 2, "[ ]");
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_TABLESVIEWS]);


	/* Clear windows/variables if table not found */
	if (!found_table) {
		session->current_table = 0;
	}


	return !found_table;
}
