#include <libpq-fe.h>

#include "../dbt.h"


struct json_t *load_database_list(struct dbt_adapter *adapter) {
	/* Fetch databases */
	const char *sql = 
		" SELECT datname FROM pg_database"
		" WHERE NOT datistemplate AND datdba IN"
		" (SELECT usesysid FROM pg_user WHERE usename = current_user)"
		" ORDER BY datname;";

	PGresult *res = PQexec(adapter->conn_handle, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		return 0;
	}


	/* Append database names to array */
	json_t *database_list = json_array();
	int row_count = PQntuples(res);
	for (int i=0; i < row_count; i++) {
		char *db_name = PQgetvalue(res, i, 0);
		json_array_append_new(database_list, json_string(db_name));
	}


	/* Clear result */
	PQclear(res);


	return database_list;
}
static void connect_to_db(const char *database, struct dbt_adapter *adapter) {
	/* Connect to database */
	PGconn *conn = PQsetdbLogin(adapter->host, 0, 0, 0, database, adapter->user, adapter->pass);
	if (PQstatus(conn) != CONNECTION_OK) {
		/* TODO: handle */
		return;
	}
	adapter->db_conn_handle = conn;
}
static json_t *load_schema_list(struct dbt_adapter *adapter) {
	/* Fetch schemas */
	const char *sql =
		" SELECT schema_name FROM information_schema.schemata"
		" WHERE schema_owner = current_user OR schema_name = 'public'"
		" ORDER BY schema_name;";

	PGresult *res = PQexec(adapter->db_conn_handle, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		return 0;
	}


	/* Append schema names to array */
	json_t *schema_list = json_array();
	int row_count = PQntuples(res);
	for (int i=0; i < row_count; i++) {
		char *schema_name = PQgetvalue(res, i, 0);
		json_array_append_new(schema_list, json_string(schema_name));
	}


	/* Clear result */
	PQclear(res);


	return schema_list;
}
static json_t *load_table_list(const char *schema, struct dbt_adapter *adapter) {
	/* Fetch tables */
	const char *sql =
		" SELECT tablename FROM pg_catalog.pg_tables"
		" WHERE tableowner = current_user AND schemaname = $1"
		" ORDER BY tablename;";
	
	const char **params = {
		&schema
	};

	PGresult *res = PQexecParams(adapter->db_conn_handle, sql, 1, 0, params, 0, 0, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		return 0;
	}


	/* Append table names to array */
	json_t *table_list = json_array();
	int row_count = PQntuples(res);
	for (int i=0; i < row_count; i++) {
		char *table_name = PQgetvalue(res, i, 0);
		json_array_append_new(table_list, json_string(table_name));
	}


	/* Clear result */
	PQclear(res);


	return table_list;
}


void dbt_adapter_psql_init(struct dbt_session *session) {
	/* Init values */
	session->adapter_handle.conn_handle = 0;
	session->adapter_handle.db_conn_handle = 0;
	session->adapter_handle.load_database_list = load_database_list;
	session->adapter_handle.connect_to_db = connect_to_db;
	session->adapter_handle.load_schema_list = load_schema_list;
	session->adapter_handle.load_table_list = load_table_list;


	/* Check input */
	if (!session || !session->current_server) return;


	/* Load connection details */
	const char *host = json_string_value(json_object_get(session->current_server, "host"));
	const char *user = json_string_value(json_object_get(session->current_server, "user"));
	const char *pass = json_string_value(json_object_get(session->current_server, "pass"));


	/* Connect to database */
	PGconn *conn = PQsetdbLogin(host, 0, 0, 0, "postgres", user, pass);
	if (PQstatus(conn) != CONNECTION_OK) {
		/* TODO: handle */
		return;
	}
	session->adapter_handle.conn_handle = conn;
	session->adapter_handle.host = host;
	session->adapter_handle.user = user;
	session->adapter_handle.pass = pass;


	return;
}
