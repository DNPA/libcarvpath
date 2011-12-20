#ifndef _CARVPATH_LONGTOKEN_H
#define _CARVPATH_LONGTOKEN_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sqlite3.h>


int carvpath_longtoken_opendb(sqlite3 **db);
char *  carvpath_longtoken_lookup(const char *relpath,sqlite3 *db);
int carvpath_longtoken_fixup(char *longtoken,sqlite3 *db);

#ifdef __cplusplus
}
#endif
#endif
