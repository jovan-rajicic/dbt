#include <string.h>

#include "dbt.h"


int dbt_databases_refresh(struct dbt_session *session) {
	/* Check input */
	if (!session || !session->current_server) return 1;


	/* Load server type */
	const char *server_type = json_string_value(json_object_get(session->current_server, "type"));


	/* Init adapter for server */
	if (!strcmp(server_type, "psql")) dbt_adapter_psql_init(session);
	//else if (!strcmp(server_type, "mssql")) dbt_adapter_mssql_init(session);
	//else if (!strcmp(server_type, "mysql")) dbt_adapter_mysql_init(session);
	//else if (!strcmp(server_type, "sqlite")) dbt_adapter_sqlite_init(session);


	/* Load databases */
	session->database_list = session->adapter_handle.load_database_list(&session->adapter_handle);
	if (!json_is_array(session->database_list)) return 1;


	/* Clear previous list */
	wclear(session->app_windows[DBT_WIN_DATABASES]);
	box(session->app_windows[DBT_WIN_DATABASES], 0, 0);
	mvwprintw(session->app_windows[DBT_WIN_DATABASES], 0, 2, "Databases");


	/* Print new database list */
	size_t list_size = json_array_size(session->database_list);
	for (size_t i=0; i < list_size; i++) {
		const char *db_name = json_string_value(json_array_get(session->database_list, i));

		mvwprintw(session->app_windows[DBT_WIN_DATABASES], i+1, 2, "[ ] %s", db_name);
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_DATABASES]);


	return 0;
}


int dbt_databases_select(const char *database, struct dbt_session *session) {
	/* Check input */
	if (!database || !session) return 1;
	else if (!json_is_array(session->database_list)) return 1;


	/* Load requested database */
	int found_db = 0;
	size_t database_count = json_array_size(session->database_list);
	for (size_t i=0; i < database_count; i++) {
		const char *db_name = json_string_value(json_array_get(session->database_list, i));
		if (!strcmp(db_name, database)) {
			/* Set found_db flag */
			found_db = 1;


			/* Mark as selected */
			mvwprintw(session->app_windows[DBT_WIN_DATABASES], i+1, 2, "[*]");


			/* Store as current */
			session->current_database = db_name;


			/* Connect to db */
			session->adapter_handle.connect_to_db(db_name, &session->adapter_handle);


			/* Refresh schemas */
			dbt_schemas_refresh(session);
		} else mvwprintw(session->app_windows[DBT_WIN_DATABASES], i+1, 2, "[ ]");
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_DATABASES]);


	/* Clear windows/variables if database not found */
	if (!found_db) {
		session->current_database = 0;
	}


	return !found_db;
}
