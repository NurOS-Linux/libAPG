// NurOS Ruzen42 2026 apg/db.c
// Last change: Feb 2 

#include <lmdb.h>
#include <stdbool.h>

#include "../include/apg/db.h"
#include "../include/apg/package.h"
#include "../include/apg/json.h"
#include <string.h>


MDB_env *
init_db_env(const char *db_path)
{
    MDB_env *env;
    mdb_env_create(&env);
    mdb_env_set_mapsize(env, 10485760);
    mdb_env_open(env, db_path, 0, 0664);
    return env;
}

bool
add_package(struct package *pkg, MDB_env *env)
{
    MDB_dbi dbi;
    MDB_val key, data;
    MDB_txn *txn;

    mdb_txn_begin(env, NULL, 0, &txn);
    mdb_dbi_open(txn, NULL, 0, &dbi);
    mdb_txn_commit(txn);

    // write
    char *s_key = pkg->meta->name;
    char *s_value = json_to_string(package_to_json(pkg));
    key.mv_size = strlen(s_key);
    key.mv_data = s_key;
    data.mv_size = strlen(s_value);
    data.mv_data = s_value;

    mdb_txn_begin(env, NULL, 0, &txn);
    mdb_put(txn, dbi, &key, &data, 0);
    mdb_txn_commit(txn);

    // read
    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    if (mdb_get(txn, dbi, &key, &data) == MDB_SUCCESS) {
        printf("Key: %.*s, Value: %.*s\n",
               (int)key.mv_size, (char *)key.mv_data,
               (int)data.mv_size, (char *)data.mv_data);
    }
    mdb_txn_abort(txn);

    mdb_dbi_close(env, dbi);
    mdb_env_close(env);

    return true;
}

