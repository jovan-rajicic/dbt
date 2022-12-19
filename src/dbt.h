#ifndef _DBT_MAIN_H_
#define _DBT_MAIN_H_


/* Dependencies */
#include <ncurses.h>
#include <jansson.h>


/* Definitions */
#ifndef CTRL
#define CTRL(c) (c & 037)
#endif

#ifndef DBT_VERSION
#define DBT_VERSION "v0.1.0"
#endif


/* Enums */
enum dbt_windows {
	DBT_WIN_SERVERS,
	DBT_WIN_DATABASES,
	DBT_WIN_SCHEMAS,
	DBT_WIN_TABLESVIEWS,
	DBT_WIN_COLUMNS,
	DBT_WIN_ACTION,
	DBT_WIN_RESULT,
	DBT_WIN_MAX
};
enum dbt_mode {
	DBT_MODE_NORMAL,
	DBT_MODE_SERVER_SELECT,
	DBT_MODE_DATABASE_SELECT,
	DBT_MODE_SCHEMA_SELECT,
	DBT_MODE_TABLEVIEW_SELECT,
	DBT_MODE_COLUMN_SELECT
};


/* Structs */
struct dbt_adapter {
	void *conn_handle;
	void *db_conn_handle;

	const char *host;
	const char *user;
	const char *pass;

	json_t *(*load_database_list)(struct dbt_adapter *self);
	void (*connect_to_db)(const char *database, struct dbt_adapter *self);
	json_t *(*load_schema_list)(struct dbt_adapter *self);
	json_t *(*load_table_list)(const char *schema, struct dbt_adapter *self);
};
struct dbt_session {
	WINDOW *app_windows[DBT_WIN_MAX]; 

	enum dbt_mode mode;
	char input_buffer[64];
	short int buffer_head;

	json_t *config;
	json_t *current_server;
	json_t *database_list;
	const char *current_database;
	json_t *schema_list;
	const char *current_schema;
	json_t *table_list;
	const char *current_table;

	struct dbt_adapter adapter_handle;
};


/* Functions */
int dbt_session_init(const char *config_path, struct dbt_session *session);
int dbt_session_handle_input(int input, struct dbt_session *session);


void dbt_adapter_psql_init(struct dbt_session *session);


int dbt_servers_refresh(struct dbt_session *session);
int dbt_servers_select(const char *server, struct dbt_session *session);


int dbt_databases_refresh(struct dbt_session *session);
int dbt_databases_select(const char *database, struct dbt_session *session);


int dbt_schemas_refresh(struct dbt_session *session);
int dbt_schemas_select(const char *schema, struct dbt_session *session);


int dbt_tables_refresh(struct dbt_session *session);
int dbt_tables_select(const char *tables, struct dbt_session *session);

#endif
