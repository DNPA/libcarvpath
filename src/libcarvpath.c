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
#include "utility.h"
#include "fragment.h"
#include "entity.h"

//This function will create a new fully flattened version of a carvpath entity.
carvpath_entity *carvpath_flatten(carvpath_entity *self) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  //We can not flatten top entities any further.
  carvpath_entity *parent=self->parent;
  if (parent == 0) {
      carvpath_entity *rval=_carvpath_entity_new_reference(self);
      return rval;
  }
  //We can not flatten level 1 entities any further.
  carvpath_entity *grandparent=parent->parent;
  if (grandparent==0) {
      _carvpath_entity_longtoken_fixup(self);
      carvpath_entity *rval=_carvpath_entity_new_reference(self);
      return rval;
  }
  //Create a new empty entity bound to the grand parent of our original one.
  carvpath_entity *flattened=_carvpath_entity_new(0,grandparent);
  if (flattened == 0) {
      return 0;
  }
  //Copy converted copies of all fragments of the original to the newly created
  //entity, factoring out the original parent from our set of indirections.
  if (_carvpath_entity_fragment_flatten_frags_copy_one_layer_up(flattened,self)==0) {
      carvpath_free(flattened,0);
      return 0;
  } 
  //Flatten any possible remaining layers of indirection untill we get a level 1
  //carvpath_entity back.
  carvpath_entity *rval=carvpath_flatten(flattened);
  //Drop our temporary (one-layer off) result.
  carvpath_free(flattened,0);
  if (_carvpath_entity_longtoken_fixup(rval)==0) {
     carvpath_free(rval,0);
     return 0;
  }
  return rval;
}

carvpath_library *carvpath_init(int uselongtokendb,int compatibilitymode){
  int errval=0;
  for (errval=0;errval < (CARVPATH_ERR_MAXERRNO+2);errval++) {
    _carvpath_error_strings[errval]=0;
  }
  for (errval=0;errval < (CARVPATH_ERR_MAXERRNO+2);errval++) {
     _carvpath_error_strings[errval]=calloc(1,128);
     if (_carvpath_error_strings[errval] == 0) {
        errno=CARVPATH_ERR_ALLOC;
	return 0;
     }
     if (errval == 0) {
        snprintf(_carvpath_error_strings[errval],127,"Everything is irie.");
     } else {
        snprintf(_carvpath_error_strings[errval],127,"No such errno defined for libcarvpath.");
     }
  }
  snprintf(_carvpath_error_strings[CARVPATH_ERR_ALLOC],127,"Allocation error, insuficient memory.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOENT],127,"Libcarvpath function invoked with NULL value for entity.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOSTRING_INT],127,"Internal libcarvpath function invoked with NULL value for string.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOLIB],127,"Libcarvpath function invoked with NULL value for carvpath library structure.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_OUT_OF_RANGE],127,"Value(s) out of range.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_ISTOP],127,"Operation not allowed or possible for top node.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_SQLITE],127,"Problem opening longtoken SQL database.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_INCOMPLETE_ELEMENT],127,"Carvpath element found to be inconsistent.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOLIB],127,"Function invoked with NULL carvpath library.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_ATTRIBUTES],127,"Invalid combination of attributes (parent and entity size) for creation of new entity.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NO_FRAGMENTS],127,"Element has zero fragments.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NONTOP],127,"Operation not allowed or possible for non-top node.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_LONGTOKEN],127,"Problem fixing up long token and/or putting longtoken digest into the longtoken database.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_INVALID_FRAGMENT_TOKEN],127,"Invalid format for fragment token.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NODB],127,"Unable to parse digest type carvpath tokens while not in longtoken database mode.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_DBLOOKUP],127,"Unable to locate the given digest node in the longtoken database.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOENT_INT],127,"Internal libcarvpath function invoked with NULL value for entity.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOFRAG_INT],127,"Internal libcarvpath function invoked with NULL value for fragment.");
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOSTRING_INT],127,"Internal libcarvpath function invoked with NULL value for string.");
  carvpath_library *self=(carvpath_library *)calloc(1, sizeof(carvpath_library));
  if (self == 0) {
     errno=CARVPATH_ERR_ALLOC;
     return 0;
  }
  self->longtokendb=0;
  if (uselongtokendb) {
      if (carvpath_longtoken_opendb(&(self->longtokendb)) == SQLITE_ERROR) {
          free(self);
	  errno=CARVPATH_ERR_SQLITE;
          return 0;
      }
  }
  self->maxtokensize=CARVPATH_MAX_TOKEN_SIZE ;
  if (compatibilitymode) {
     //In compatibility mode we need to support the max token size for windows.
     self->maxtokensize=WINDOWS_EXPORTABLE_MAX_TOKEN_SIZE ;
  }
  return self;
}

void carvpath_finish(carvpath_library *self) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOLIB;
     return;
  }
  int errval=0;
  for (errval=0;errval < (CARVPATH_ERR_MAXERRNO+2);errval++) {
          free(_carvpath_error_strings[errval]);
          _carvpath_error_strings[errval]=0;
  }
  if (self->longtokendb) {
    sqlite3_close(self->longtokendb); 
  }
  free(self);
}

carvpath_entity *carvpath_top_entity(off_t fullsize,const char *topentitypath,const carvpath_library *lib){
  //Create a new top entity, this new top entity will not yet have its path_cache defined.
  carvpath_entity *entity=_carvpath_entity_new_top(fullsize,lib);
  if (entity == 0) {
     return 0;
  }
  if (lib == 0) {
     errno=CARVPATH_ERR_NOLIB;
     return 0;
  }
  //Propperly set the path_cache for the new top entity.
  if (_carvpath_entity_set_path_cache(entity,topentitypath)==0) {
     carvpath_free(entity,0);
     return 0;
  }
  return entity;
}

off_t carvpath_get_size(const carvpath_entity *ent,int *failure) {
   return ent->totalsize;
}

size_t carvpath_get_fragcount(const carvpath_entity *ent,int *failure) {
  return ent->fragcount;
}

off_t carvpath_fragment_get_size(const carvpath_entity *ent,size_t index,int *failure) {
   //Lookup the propper fragment.
   carvpath_fragment *indexedfragment=_carvpath_entity_get_fragment(ent,index);
   if (indexedfragment == 0) {
      return 0;
   }
   //Return the size of that fragment.
   return indexedfragment->size;
}

off_t carvpath_fragment_get_offset(const carvpath_entity *ent,size_t index,int *failure) {
  //Lookup the propper fragment.
  carvpath_fragment *indexedfragment=_carvpath_entity_get_fragment(ent,index);
  if (indexedfragment == 0) {
    return 0;
  }
  //Return the offset of that fragment.
  return indexedfragment->offset;
}

int carvpath_fragment_is_sparse(const carvpath_entity *ent,size_t index,int *failure) {
   //Lookup the propper fragment.
   carvpath_fragment *indexedfragment=_carvpath_entity_get_fragment(ent,index);
   if (indexedfragment == 0) {
	return 0;
   }
   //Return wetter that fragment is sparse.
   if (indexedfragment->ftype == CARVPATH_FRAGTYPE_SPARSE) {
        return 1;
   }
   return 0;
}

carvpath_entity *carvpath_new_sparse_entity(off_t size,carvpath_entity *anyent){
   //Create a new entity.
   carvpath_entity *entity=_carvpath_entity_new(0,anyent);
   if (entity == 0) {
     return 0;
   }
   //Add a sparse fragment to that entity.
   if (_carvpath_entity_add_sparse_fragment(entity,size) == 0) {
      carvpath_free(entity,0);
      return 0;
   }
   return entity;
}

carvpath_entity *carvpath_derive(carvpath_entity *ent,off_t offset,off_t size,carvpath_out_of_range_action onoor){
   //Create a new entity.
   carvpath_entity *newentity=_carvpath_entity_new(0,ent);
   if (newentity == 0) {
     return 0;
   }
   //Add a regular fragment to that entity.
   if (_carvpath_entity_add_regular_fragment(newentity,offset,size,onoor) == 0) {
      carvpath_free(newentity,0);
      return 0;
   }
   return newentity;
}

void carvpath_append_sparse(carvpath_entity *target,off_t size,int *failure){
   if (failure) { *failure=0; }
   if (target == 0) {
     if (failure) {
       *failure=1;
     }
     errno=CARVPATH_ERR_NOENT;
     return;
   }
   if (target->parent == 0) {
     if (failure) { *failure=1; }
     errno=CARVPATH_ERR_NOENT;
     return;
   }
   //Add a sparse fragment to the entity.
   if (_carvpath_entity_add_sparse_fragment(target,size) == 0) {
	if (failure) {
          //errno already set by _carvpath_entity_add_sparse_fragment.
          *failure=1;
	}
        return;
   }
   return;   
}

void carvpath_append(carvpath_entity *target,off_t offset,off_t size,int *failure,carvpath_out_of_range_action onoor){
  if (failure) {
    *failure=0;
  }
  if (target == 0) {
        if (failure) {
	  *failure=1;
	}
	errno=CARVPATH_ERR_NOENT;
	return;
  }
  if (target->parent == 0) {
	if (failure) { *failure=1; }
	errno=CARVPATH_ERR_NOENT;
	return;
  }
  carvpath_entity *parent=target->parent;
  //Add a regular fragment to the entity.
  if (_carvpath_entity_add_regular_fragment(target,offset,size,onoor) == 0) {
    //errno set by _carvpath_entity_add_regular_fragment
    if (failure) { *failure=1; }
  }
  return;
}

void carvpath_free(carvpath_entity *ent,int *failure){ 
  if (failure) {
     *failure=0;
  }
  //Free is only free if our reference count is zero.
  if (_carvpath_entity_decrefcount(ent)==0) {
    //errno already set by _carvpath_entity_decrefcount
    if (failure) { *failure=1; }
  }
}

char *carvpath_get_as_path(carvpath_entity *ent) {
 //First create a temporary flattened version of our entity.
 carvpath_entity *flattened_ent=carvpath_flatten(ent);
 if (flattened_ent == 0) {
	 return 0;
 }
 //We are going to have to delete the flattened version, so lets migrate the path string to our unflattened entity.
 _carvpath_entity_migrate_path_cache(ent,flattened_ent);
 //Delete the temporary entity.
 carvpath_free(flattened_ent,0);
 //Return our updated path string.
 return _carvpath_entity_get_as_path(ent);
}

carvpath_entity *carvpath_parse(carvpath_entity *parententity,const char *relpath,carvpath_out_of_range_action onoor){ 
  //Get the string section related to the level N entity
  char *toplayerstring=_carvpath_utility_get_toplayer_string(relpath,_carvpath_seperator_char);
  if (toplayerstring == 0) {
     return 0;
  }
  //Create a new entity from this string.
  carvpath_entity *curlevelentity=_carvpath_entity_new_from_string(parententity,toplayerstring,onoor);
  if (curlevelentity==0) {
     free(toplayerstring);
     return 0;
  }
  //were done with the toplayer string
  free(toplayerstring);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_string_count--;
#endif
  //path_cache should be set, otherwise we have a probem.
  if (curlevelentity->path_cache == 0) {
     carvpath_free(curlevelentity,0);
     return 0;
  }
  //Fetch the level N+1 etc entities string.
  char * remaininglevelsstring=_carvpath_utility_get_remaininglayers_string(relpath,_carvpath_seperator_char);
  if (remaininglevelsstring) {
     if (strlen(remaininglevelsstring)>0) {
	//If we have a remaininglevelsstring, call carvpath_parse and return the resulting entity.
        carvpath_entity *rval=carvpath_parse(curlevelentity,remaininglevelsstring,onoor);
        carvpath_free(curlevelentity,0);
        free(remaininglevelsstring);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
	_carvpath_string_count--;
#endif
        return rval;
     }
     free(remaininglevelsstring);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
     _carvpath_string_count--;
#endif
     //If our remaininglevelsstring is empty, return our level N entity.
     return curlevelentity;
  }
  //If _carvpath_utility_get_remaininglayers_string returned NULL, lets check for an error.
  if (errno) {
     carvpath_free(curlevelentity,0);
     return 0;
  }
  //Otherwise return our level N entity.
  return curlevelentity;
}

char *carvpath_error_as_string(int failurenum){
   if ((failurenum < 0) || (failurenum > CARVPATH_ERR_MAXERRNO)) {
      return _carvpath_error_strings[CARVPATH_ERR_MAXERRNO+1];
   } 
   return _carvpath_error_strings[failurenum];
}

void carvpath_grow_top(carvpath_entity *top,off_t newsize,int *failure){
   if (failure) {
       *failure=0;
   }
   if (top == 0) {
     if (failure) {
       *failure=1;
     }
     errno=CARVPATH_ERR_NOENT;
     return;
   }
   //Only the top entity can be grown.
   if (top->parent != 0) {
     if (failure) { *failure=1; }
     errno=CARVPATH_ERR_NONTOP;
     return;
   }
   //Grow only, no shrinking.
   if (carvpath_get_size(top,0) > newsize) {
      if (failure) { *failure=1; }
      errno=CARVPATH_ERR_OUT_OF_RANGE;
      return;
   }
   //Update our size.
   _carvpath_entity_set_size(top,newsize);
   return;
}
