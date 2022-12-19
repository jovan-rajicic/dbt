#include <string.h>

#include "dbt.h"



int dbt_servers_refresh(struct dbt_session *session) {
	/* Check input */
	json_t *config = session->config;
	if (!config) return 1;


	/* Load servers from config */
	json_t *server_list = json_object_get(config, "servers");
	if (!json_is_object(server_list)) return 1;


	/* Render server list */
	const char *server_name;
	json_t *server_info;
	size_t ind = 0;
	json_object_foreach(server_list, server_name, server_info) {
		json_t *server_type = json_object_get(server_info, "type");
		if (!json_is_string(server_type)) continue;

		const char *server_type_str = json_string_value(server_type);
		mvwprintw(session->app_windows[DBT_WIN_SERVERS], ind+1, 2, "[ ] %s - (%s)", server_name, server_type_str);

		ind++;
	}


	/* Refresh window */
	wrefresh(session->app_windows[DBT_WIN_SERVERS]);


	/* Move cursor to resting position */
	move(LINES-1, 0);
	refresh();

	return 0;
}


int dbt_servers_select(const char *server, struct dbt_session *session) {
	/* Check input */
	if (!server) return 1;
	else if (!session) return 1;

	json_t *config = session->config;
	if (!config) return 1;


	/* Load servers from config */
	json_t *server_list = json_object_get(config, "servers");
	if (!json_is_object(server_list)) return 1;


	/* Load requested server (TODO: find a better way to get index) */
	const char *server_name;
	json_t *server_info;
	size_t ind = 0;
	int found_server = 0;
	json_object_foreach(server_list, server_name, server_info) {
		/* Load server type (TODO: find better way) */
		json_t *server_type = json_object_get(server_info, "type");
		if (!json_is_string(server_type)) continue;


		if (!strcmp(server_name, server)) {
			/* Set found_server flag */
			found_server = 1;


			/* Mark as selected */
			mvwprintw(session->app_windows[DBT_WIN_SERVERS], ind+1, 2, "[*]");


			/* Store as current */
			session->current_server = server_info;


			/* Refresh databases */
			dbt_databases_refresh(session);

		} else mvwprintw(session->app_windows[DBT_WIN_SERVERS], ind+1, 2, "[ ]");

		ind++;
	}


	/* Refresh window */ 
	wrefresh(session->app_windows[DBT_WIN_SERVERS]);


	/* Clear windows/variables if server not found */
	if (!found_server) {
		session->current_server = 0;
	}


	
	return !found_server;
}
