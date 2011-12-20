#include "longtoken.h"
#include "libcarvpath.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <errno.h>
#include <openssl/evp.h>


int carvpath_longtoken_opendb(sqlite3 **db) {
  char *usedir=0;
#ifndef _WIN32
  char *cpdbdir = getenv("LONGPATHDIR");
  if (cpdbdir) {
    usedir=(char *) calloc(strlen(cpdbdir)+1,1);
    if (usedir == 0) {
       errno=CARVPATH_ERR_ALLOC;
       return 0;
    }
    strcpy(usedir,cpdbdir);
  } 
  if (usedir == 0) {
      char *homedir=getenv("HOME");
      if (homedir) {
        usedir=(char *) calloc(strlen(homedir)+strlen("/.carvpath")+1,1);
        if (usedir == 0) {
           errno=CARVPATH_ERR_ALLOC;
           return 0;
        }
        sprintf(usedir,"%s/.carvpath",homedir) ;
      }
  }
#endif
  if (usedir == 0) {
     usedir=(char *) calloc(strlen( CARVPATH_PLATFORM_VARDIR )+1,1);
     if (usedir == 0) {
         errno=CARVPATH_ERR_ALLOC;
         return 0;
     }
     strcpy(usedir,CARVPATH_PLATFORM_VARDIR );
  }
#ifndef _WIN32
  mkdir(usedir,0755);  // dir probably already excists, but just to make sure.  
#endif
  char *dbpath=(char *) calloc(strlen(usedir) + strlen("longtokens") + 2,1);
  if (dbpath == 0) {
     free(usedir);
     errno=CARVPATH_ERR_ALLOC;
     return 0;
  }
  sprintf(dbpath,"%s%clongtokens",usedir,_carvpath_seperator_char);
  free(usedir);
  if (sqlite3_open(dbpath,db) == SQLITE_OK) {
     free(dbpath);
     sqlite3_exec(*db,"CREATE TABLE longpath ( sha1 TEXT, path TEXT, PRIMARY KEY(sha1))",0,0,0);
     return SQLITE_OK;
  }
  free(dbpath);
  return SQLITE_ERROR;
}

static int _carvpath_longtoken_lookup_result(void *dp,int ccount,char **valdata,char ** colnames) {
   char **result=(char **)dp;
   if (*result) {
      free(*result);
      *result=0;
   }
   *result=calloc(strlen(valdata[0])+1,1);
   strcpy(*result,valdata[0]);
   return 0;
}

char *  carvpath_longtoken_lookup(const char *relpath,sqlite3 *db) {
   char *result=0;
   char query[101];
   snprintf(query,100,"select path from longpath where sha1='%.40s'",relpath+1);
   char *errstr=0;
   if (sqlite3_exec(db,query,_carvpath_longtoken_lookup_result,&result,&errstr)!=0) {
      sqlite3_free(errstr);
   }
   return result;
}

/* Note: longtoken is overwitten by this function !!  */
int carvpath_longtoken_fixup(char *longtoken,sqlite3 *db) {
  EVP_MD_CTX CtxSHA1;
  EVP_DigestInit(&CtxSHA1, EVP_sha1());
  EVP_DigestUpdate(&CtxSHA1,longtoken, strlen(longtoken));
  unsigned char sha1Buffer[EVP_MAX_MD_SIZE];
  unsigned int sha1l;
  EVP_DigestFinal(&CtxSHA1, sha1Buffer, &sha1l);
  if (strlen(longtoken) < 41)
     return 0; 
  int i; 
  char *ltoken=0;
  if (db) {
    ltoken=(char *) calloc(strlen(longtoken)+1,1);
    if (ltoken == 0) {
       errno=CARVPATH_ERR_ALLOC;
       return 0;
    }
    strcpy(ltoken,longtoken);
  }
  longtoken[0]='D';
  for (i = 0; i < 20; i++) {
      sprintf((longtoken + 1 + (i * 2)), "%.2x", sha1Buffer[i]);
  } 
  if (db) {
    char *query=(char *) calloc(1,strlen(longtoken) + strlen(ltoken) + 100);
    if (query == 0) {
       errno=CARVPATH_ERR_ALLOC;
       return 0;
    }
    sprintf(query,"INSERT OR REPLACE INTO longpath (sha1,path) VALUES('%s','%s')",longtoken+1,ltoken);
    char *errstr=0;
    if (sqlite3_exec(db,query,0,0,&errstr) != 0) {
       sqlite3_free(errstr);
    }
    free(query);
    free(ltoken);
  }
  return 1;
}
