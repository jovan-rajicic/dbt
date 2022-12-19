#include <string.h>

#include "dbt.h"


int dbt_schemas_refresh(struct dbt_session *session) {
	/* Check input */
	if (!session) return 1;


	/* Load schemas */
	session->schema_list = session->adapter_handle.load_schema_list(&session->adapter_handle);
	if (!json_is_array(session->schema_list)) return 1;


	/* Clear previous list */
	wclear(session->app_windows[DBT_WIN_SCHEMAS]);
	box(session->app_windows[DBT_WIN_SCHEMAS], 0, 0);
	mvwprintw(session->app_windows[DBT_WIN_SCHEMAS], 0, 2, "Schemas");


	/* Print new schema list */
	size_t list_size = json_array_size(session->schema_list);
	for (size_t i=0; i < list_size; i++) {
		const char *schema_name = json_string_value(json_array_get(session->schema_list, i));

		mvwprintw(session->app_windows[DBT_WIN_SCHEMAS], i+1, 2, "[ ] %s", schema_name);
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_SCHEMAS]);


	return 0;
}

int dbt_schemas_select(const char *schema, struct dbt_session *session) {
	/* Check input */
	if (!schema || !session) return 1;
	else if (!json_is_array(session->schema_list)) return 1;


	/* Load requested schema */
	int found_schema = 0;
	size_t schema_count = json_array_size(session->schema_list);
	for (size_t i=0; i < schema_count; i++) {
		const char *schema_name = json_string_value(json_array_get(session->schema_list, i));
		if (!strcmp(schema_name, schema)) {
			/* Set found schema flag */
			found_schema = 1;


			/* Mark as selected */
			mvwprintw(session->app_windows[DBT_WIN_SCHEMAS], i+1, 2, "[*]");


			/* Store as current */
			session->current_schema = schema_name;


			/* Refresh tables */
			dbt_tables_refresh(session);
		} else mvwprintw(session->app_windows[DBT_WIN_SCHEMAS], i+1, 2, "[ ]");
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_SCHEMAS]);


	/* Clear windows/variables if schema not found */
	if (!found_schema) {
		session->current_schema = 0;
	}



	return !found_schema;
}
