// NurOS Ruzen42 2026 apg/db.c
// Last change: Jan 15

#include <lmdb.h>
#include <stdbool.h>

#include "../include/apg/db.h"
#include "../include/apg/package.h"


MDB_env * 
init(
bool 
add_package(struct package *pkg, MDB_env *env)
{
  MDB_dbi dbi;
  MDB_txn *txn;
  MDB_val key, data;

  if (mdb_txn_begin(env, NULL, 0, &txn)) 
    return false;

  if (mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi)) {
    mdb_txn_abort(txn);
    return false;
  }


}

bool 
remove_package(char *pkg_name, MDB_env *env)
{

}

struct package *
get_all_packages(MDB_env *env)
{
}

struct package 
get_package_by_name(char *name, MDB_env *env)
{
}

