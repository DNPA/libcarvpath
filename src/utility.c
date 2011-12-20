//Fraglib fragment path translation library.
//Copyright (C) KLPD 2006  <ocfa@dnpa.nl>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "libcarvpath.h"
#include "longtoken.h"

//#####################################################################################
//#                             Utility functions                                     #
//#####################################################################################
//This function returns the index of the first occurence of a seperator character.
int _carvpath_utility_get_first_seperator_index(const char *relpath,char seperator){
   int x;
   int slen=strlen(relpath);
   for (x=0;x<slen-1;x++) {
      if (relpath[x] == seperator) {
         return x;
      }
   }
   return -1;
}

//This function returns a newly allocated string with the part
//of the input string before the first seperator character.
char * _carvpath_utility_get_toplayer_string(const char *relpath,char seperator) {
  int subsize=strlen(relpath);
  int sepindex=_carvpath_utility_get_first_seperator_index(relpath,seperator);
  if (sepindex != -1) {
      if ((sepindex == 0) && (subsize > 1)) {
         return _carvpath_utility_get_toplayer_string(relpath+1,seperator);
      }
      subsize=sepindex;   
  }
  char *rval=(char *) calloc(1,subsize+1);
  if (rval == 0) {
     errno=CARVPATH_ERR_ALLOC;
     return 0;
  }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_string_count++;
#endif
  strncpy(rval,relpath,subsize);
  return rval;
}
//This function returns a newly allocated string with the part
////of the input string after the first seperator character.
char * _carvpath_utility_get_remaininglayers_string(const char *relpath,char seperator){
  int subsize=0;
  int substart=0;
  int sepindex=_carvpath_utility_get_first_seperator_index(relpath,seperator);
  if (sepindex != -1) {
     if ((sepindex == 0) && (subsize > 1)) {
        return _carvpath_utility_get_remaininglayers_string(relpath+1,seperator);
     }
     subsize=strlen(relpath) - sepindex -1;
     substart=sepindex+1;
  }
  char *rval=(char *) calloc(1,subsize+1);
  if (rval == 0) {
    errno=CARVPATH_ERR_ALLOC;
    return 0;
  }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_string_count++;
#endif
  strncpy(rval,relpath+substart,subsize);
  return rval;
}

//This function concattenates the string representation of a layer of
////carvpath indirection to the string representation of the parent layer.
char * _carvpath_util_new_pathstring(char *parentstr,char *fragmentsstr){
  if (parentstr == 0) {
      errno=CARVPATH_ERR_NOSTRING_INT;
      return 0;
  }
  if (fragmentsstr == 0) {
      errno=CARVPATH_ERR_NOSTRING_INT;
      return 0;
  }
  //The old string ;engths plus the seperator character plus the trailing 0 character.
  int newsize=strlen(parentstr)+strlen(fragmentsstr) + 2;
  char *newstring=(char *) calloc(1,newsize);
  if (newstring == 0) {
     errno=CARVPATH_ERR_ALLOC;
     return 0;
  }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_string_count++;
#endif
  //Concatenate using the OS dependant seperator character. This will normaly be the '/'
  //character, unless we are building on an MS Windows machine, in which case it will 
  //be the '\' character.
  sprintf(newstring,"%s%c%s",parentstr,_carvpath_seperator_char,fragmentsstr);
  //Return the new string and relinguish responsibility for it to the calling party.
  return newstring;
}
