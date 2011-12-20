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
   for (x=0;x<slen-2;x++) {
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
      errno=CARVPATH_ERR_NOENT;
      return 0;
  }
  if (fragmentsstr == 0) {
      errno=CARVPATH_ERR_NOENT;
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
//#####################################################################################
//#             carvpath_fragment related functions (pseudo methods                   #
//#####################################################################################


carvpath_fragment *_carvpath_fragment_find_last(carvpath_fragment *first);

//This function will fill the string representation stored in fragments if they are
//set to 0. Each fragment should hold the representation of the full fragment list
//upto itself. This means that the true reperentation of the fragment list is stored 
//with the last fragment of the fragment list.
int _carvpath_fragment_update_strings_if_needed(carvpath_fragment *self,const char *prefix) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  if (prefix == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  if (self->string_cache == 0) {
     //A 64 bit unsigned integer takes upto 19 characters in its ascii representation.
     //We have two of these, one or two token seperator characters and a trailing 0.
     //41 should do the trick, lats use 42 just to make sure.
     int newssize=strlen(prefix) + 42;
     char *newstring=(char *) calloc(1,newssize);
     if (newstring == 0) {
        errno=CARVPATH_ERR_ALLOC;
	return 0;
     }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
     _carvpath_string_count++;
#endif
     //Create the string representation upto this fragment.
     if (self->ftype == CARVPATH_FRAGTYPE_REGULAR) {
       if (strlen(prefix) == 0) {
  	  sprintf(newstring,"%" PRId64 "+%" PRId64, self->offset,self->size);
       } else {
	  sprintf(newstring,"%s_%" PRId64 "+%" PRId64,prefix, self->offset,self->size);
       }
     } else {
       if (strlen(prefix) == 0) {
	 sprintf(newstring,"S%" PRId64,self->size);
       } else {
         sprintf(newstring,"%s_S%" PRId64,prefix,self->size);
       }
     }
     self->string_cache=newstring;
  }
  if (self->next) {
     //Update the rest of the fragment list.
     return _carvpath_fragment_update_strings_if_needed(self->next,self->string_cache);
  }
  return 1;
}

//This function returns the string representation of a fragment list.
char *_carvpath_fragment_get_as_string(carvpath_fragment *self){
   if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
   }
   //If needed, update the fragment internal path caches.
   if (_carvpath_fragment_update_strings_if_needed(self,"")==0) {
      return 0;
   }
   //Find the last fragment that holds the string representation of the full fragment list.
   carvpath_fragment *last=_carvpath_fragment_find_last(self);
   if (last == 0) {
      return 0;
   }
   //Return the string representation of the full fragment list.
   return last->string_cache;
}

//This function creates a new fragment struct.
carvpath_fragment *_carvpath_fragment_new(carvpath_fragtype ftype,off_t offset, off_t size) {
  carvpath_fragment *self=(carvpath_fragment *) calloc(1, sizeof(carvpath_fragment));
  if (self == 0) {
     errno=CARVPATH_ERR_ALLOC;
     return 0;
  }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_fragment_count++;
#endif
  self->ftype=ftype;
  self->offset=offset;
  self->size=size;
  self->next=0;
  self->string_cache=0;
  if (self->ftype == CARVPATH_FRAGTYPE_SPARSE) {
     self->offset=0;
  }
  return self;
}

//This function returns the last fragment of a fragment list.
carvpath_fragment *_carvpath_fragment_find_last(carvpath_fragment *self) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  carvpath_fragment *curfrag=self;
  while (curfrag->next) {
     curfrag=curfrag->next;
  }
  return curfrag;
}

//This function destroy (part of) a fragment list.
void _carvpath_fragment_destroy(carvpath_fragment *self) {
  //Destroying an empty list is not a problem.
  if (self == 0) {
     return;
  }
  //If we are not the last one in the list, lets delete the rest of this list first.
  carvpath_fragment *nextinlist=self->next;
  if (nextinlist) {
     _carvpath_fragment_destroy(nextinlist);
     self->next=0;
  }
  //Now delete the string_cache if any.
  if (self->string_cache) {
    free(self->string_cache);
    self->string_cache=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
    _carvpath_string_count--;
#endif
  }
  //And finaly delete ourselves.
  free(self);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_fragment_count--;
#endif
}

//This function will look if a new fragment can be merged with this fragment.
//If merging is possible, the new fragment is merged with the current one and
//the new fragment is destroyed. If merging is not possible, 0 is returned and the
//new fragment is left untouched.
int _carvpath_fragment_merge(carvpath_fragment *self,carvpath_fragment *newfrag) {
   if (self == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
   }
   if (newfrag == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
   }
   //fragments of different types can never be merged.
   if (self->ftype != newfrag->ftype) {
       return 0;
   }
   //Two sparse fragments can always be merged. Two regular fragments can only be merged
   //if they interlock.
   if ((self->ftype == CARVPATH_FRAGTYPE_SPARSE) ||
       ((self->ftype == CARVPATH_FRAGTYPE_REGULAR) && (newfrag->offset==(self->offset+self->size)))) {
     //merging is nothing more than simply adding sizes together.
     self->size += newfrag->size;
     //After merging we can dispose of the new fragment as it will not be needed.
     _carvpath_fragment_destroy(newfrag);
     //After merging our string_cache is no longer valid, so we should clear it, and that of all
     //remaining fragments. 
     carvpath_fragment *curfrag=self;
     while (curfrag) { 
        free(curfrag->string_cache);
        curfrag->string_cache=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
	_carvpath_string_count--;
#endif
	curfrag=curfrag->next;
     }
     return 1;
   } 
   return 0;
}

//This function determines if a migratable fragment overlaps with a parent fragment. If it does
//it will return a new parent fragment for usage in a new flattened entity.
carvpath_fragment *_carvpath_fragment_overlap(carvpath_fragment *self,off_t loffset,carvpath_fragment *lfrag) {
   if (self == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
   }
   if (lfrag == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
   }   
   //If there is no overlap, return NULL;
   if (((lfrag->offset+lfrag->size) <= loffset) || ((loffset + self->size) <= lfrag->offset)) {
      return 0; //No overlap
   }
   //First assume full overlap.
   off_t newoffset=self->offset;
   off_t newsize=self->size; 
   //Adjust size and offset at the front (if the migratable fragment starts further than
   //the parent fragment).
   if (lfrag->offset > loffset) {
       off_t difference=lfrag->offset - loffset;
       newoffset += difference;
       newsize -= difference;
   } 
   //Adjust the size at the end (If the migratable fragment ends earlier than the parent fragment).
   if ((lfrag->offset + lfrag->size) < (loffset+self->size)) {
       off_t difference = loffset+self->size-lfrag->offset-lfrag->size;
       newsize -=difference;
   } 
   //Create and return a new fragment.
   return _carvpath_fragment_new(self->ftype,newoffset,newsize);
}

//This function will create a new fragment struct from a string representation.
carvpath_fragment *_carvpath_fragment_new_from_string(const char *fragmentstring) {
   carvpath_fragtype ftype=CARVPATH_FRAGTYPE_REGULAR;
   off_t offset=0;
   off_t size=0;
   if (fragmentstring[0] == 'S') { 
      //Sparse fragment strings start off with a 'S' character followed by the size string
      ftype =CARVPATH_FRAGTYPE_SPARSE;
      if ((fragmentstring[1] > '9') || (fragmentstring[1] < '0')) {
         errno=CARVPATH_ERR_INVALID_FRAGMENT_TOKEN;
	 return 0;
      }
      size=atoll(fragmentstring+1);
   } else {
      //Regular fragments look like <offset>+<size>
      offset=atoll(fragmentstring);
      int sizeindex=1+_carvpath_utility_get_first_seperator_index(fragmentstring,'+');
      if (sizeindex < 2) {
        errno=CARVPATH_ERR_INVALID_FRAGMENT_TOKEN;
        return 0;
      } 
      size=atoll(fragmentstring+sizeindex);
   }
   return _carvpath_fragment_new(ftype,offset,size);
}

//######################################################################################
//#                   carvpath_entity related functions (pseudo methods)               #
//######################################################################################
int _carvpath_entity_destroy(carvpath_entity *self);


//This function decrements the reference count for the entity object. If the reference count
//reaches zero, the carvpath_entity is destroyed.
int _carvpath_entity_decrefcount(carvpath_entity *self){
    if (self == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
    }
    (self->refcount)--;
    if (self->refcount == 0) {
       return _carvpath_entity_destroy(self);
    }
    return 1;
}

//This function destroys a carvpath_entity and its members.
int _carvpath_entity_destroy(carvpath_entity *self) {
   if (self == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
   }
   //Destroy all fragments.
   if (self->fragcount) {
     self->fragcount=0;
     _carvpath_fragment_destroy(self->fraglist);
   }
   //Destroy the path cache.
   if (self->path_cache) {
      free(self->path_cache);
      self->path_cache=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
      _carvpath_string_count--;
#endif
   }
   //Let the parent know we no longer have a reference to it.
   if (self->parent) {
     _carvpath_entity_decrefcount(self->parent);
   }
   //Free our memory space.
   free(self);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
   _carvpath_entity_count--;
#endif
   return 1;
}

//This function increments the reference count for a carvpath_entity.
int _carvpath_entity_increfcount(carvpath_entity *self) {
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  (self->refcount)++;
  return 1;
}

//This function updates the patch_cache for a new or changed carvpath_entity.
int _carvpath_entity_update_path_cache(carvpath_entity *self){
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }  
  if (self->parent == 0) {
     //Don't touch the top entity
     return 1;
  }  
  //If we had an old likely invalid path_cache, destroy the old one and start with a clean slate.
  if (self->path_cache) {
     free(self->path_cache);
     self->path_cache=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
     _carvpath_string_count--;
#endif
  }
  //Make a new path_cache from our parent string representation and our fragmentlist string representation.
  if (self->fraglist) {
    self->path_cache=_carvpath_util_new_pathstring(carvpath_get_as_path(self->parent),
		  _carvpath_fragment_get_as_string(self->fraglist));
  } else {
    self->path_cache=_carvpath_util_new_pathstring(carvpath_get_as_path(self->parent),"0+0");
  }
  if (self->path_cache == 0) {
     return 0;
  }
  return 1;
}
//This function is a bit of a hack, carvpath_get_as_path creates and uses a temporary (flattend)
//entity. carvpath_get_as_path can not return the path_cache of the flattened entity as it will
//need to delete the entity before returning to avoid leaking entities. To overcome the problem raised
//by this, this function migrates the path_cache of the flat entity to the original one, so that
//the flat entity can be safely deleted, and carvpath_get_as_path can return a string that still
//excists.
void _carvpath_entity_migrate_path_cache(carvpath_entity *self,carvpath_entity *flattened_self){
  if (self->parent) { // Only non top entities need migration, top entities are as flat as it gets.
     if (self == flattened_self) { //If the original and the flattened entity are one and the same, nothing needs to be done.
       ;
     } else {
       if (self->path_cache) {
	  //Make sure we don't leak strings by deleting any original string from the target.
          free(self->path_cache);
	  self->path_cache=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
	  _carvpath_string_count--;
#endif
       }
       //Move the string from the source to the target.
       self->path_cache=flattened_self->path_cache;
       flattened_self->path_cache=0;
     }
  }
}

//This function adds a single fragment to an entity.
int _carvpath_entity_add_fragment(carvpath_entity *self,carvpath_fragment *frag) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  if (frag==0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  //Adding fragments to a top node is not supported.
  if (self->parent == 0) {
     errno=CARVPATH_ERR_ISTOP;
  }
  //Ignore zero size fragments.
  if (frag->size == 0) {
     return 1;
  }
  //The first fragment 'is' the fragment list initially.
  if (self->fraglist == 0) {
     self->fraglist=frag;
  } else {
     //Other fragments should go to the end of the list.
     carvpath_fragment *lastfrag=_carvpath_fragment_find_last(self->fraglist);
     //Try to merge the new fragment with the last of the list.
     if (_carvpath_fragment_merge(lastfrag,frag)==0) {
	//If merging fails, add the fragment to the end of the fragment list.
        lastfrag->next=frag;  
     }
  }
  //Update the total size of the entity.
  self->totalsize+=frag->size; 
  //As our entity is changed, the path_cache is no longer valid, so lets update it.
  return _carvpath_entity_update_path_cache(self);
}

//This function adds a new sparse fragment to the end of an entity.
int _carvpath_entity_add_sparse_fragment(carvpath_entity *self,off_t size) {
   if (self == 0) {
      errno=CARVPATH_ERR_NOENT;
      return 0;
   }
   //An empty sparse fragment requires no action.
   if (size == 0) {
      return 1;
   }
   //Create the new fragment struct.
   carvpath_fragment *frag=_carvpath_fragment_new(CARVPATH_FRAGTYPE_SPARSE,0,size);
   if (frag == 0) {
      return 0;
   }
   //Add the newly created fragment to the entity.
   return _carvpath_entity_add_fragment(self,frag);
}

//This function adds a new regular fragment to the end of an entity.
int _carvpath_entity_add_regular_fragment(carvpath_entity *self,off_t offset,off_t size){
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  //An empty regular fragment requires no action.
  if (size == 0) {
     return 1;
  }
  //Create the new fragment struct.
  carvpath_fragment *frag=_carvpath_fragment_new(CARVPATH_FRAGTYPE_REGULAR,offset,size);
  if (frag == 0) {
    return 0;
  }
  //Add the newly created fragment to the entity.
  return _carvpath_entity_add_fragment(self,frag);
}

//This function creates a new carvpath_entity.
carvpath_entity *_carvpath_entity_new(off_t entsize,carvpath_entity *parent) {
 //Only the top node has a size before it has fragments.
 if ((entsize > 0) && parent) {
    errno=CARVPATH_ERR_ATTRIBUTES;
    return 0;
 }
 //Allocate the new entity structure.
 carvpath_entity *self =
	    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
 if (self == 0) {
   errno=CARVPATH_ERR_ALLOC;
   return 0;
 }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
 _carvpath_entity_count++;
#endif
 //Fill all fields.
 self->totalsize=entsize;
 self->fragcount=0;
 self->refcount=1;
 self->path_cache=0;
 self->parent=parent;
 self->fraglist=0;
 self->library_state=0;
 if (parent) {
   self->library_state=parent->library_state;
   //Let our parent know that we have a reference to it.
   if (_carvpath_entity_increfcount(parent)==0) {
	 carvpath_free(self,0);
         return 0;
   }
   if (parent->path_cache == 0) {
         if (_carvpath_entity_update_path_cache(parent) == 0) {
	     carvpath_free(self,0);
             return 0;
         }
   }
   //Create the initial path_cache
   self->path_cache=_carvpath_util_new_pathstring(carvpath_get_as_path(parent),"0+0");
   if (self->path_cache == 0) {
     carvpath_free(self,0);
     //errno should have been set by _carvpath_util_new_pathstring.
     return 0;
   }
 }
 _carvpath_entity_update_path_cache(self); 
 return self;
}
//This function creates a new carvpath top entity.
carvpath_entity *_carvpath_entity_new_top(off_t entsize,const carvpath_library *lib) {
  carvpath_entity *newent=_carvpath_entity_new(entsize,0);
  if (newent !=0) {
    newent->library_state=lib;
  }
  else {
  }
  return newent;
}

//This function creates a new entity from a string.
carvpath_entity *_carvpath_entity_new_from_string(carvpath_entity *parententity,char *toplayerstring){
  //Create a new empty entity.
  carvpath_entity *self=_carvpath_entity_new(0,parententity);
  char *remaining=toplayerstring;
  //First, reverse any longtoken key notation.
  if (toplayerstring[0] == 'D') {
    const carvpath_library *mylib=parententity->library_state;
    //If the library was started without longtoken db support, we have a problem.
    if (mylib->longtokendb == 0) {
       errno=CARVPATH_ERR_NODB;
       return 0;
    }
    //Fetch the original long path from the longtokendb;
    remaining = carvpath_longtoken_lookup(toplayerstring,mylib->longtokendb);
    if (remaining == 0) {
       errno=CARVPATH_ERR_DBLOOKUP;
       return 0;
    }
  }
  //Eat up the string one fragment string at a time.
  while (remaining) {
     char *firstfragstring=_carvpath_utility_get_toplayer_string(remaining,'_');
     carvpath_fragment *newfrag=_carvpath_fragment_new_from_string(firstfragstring);
     if (newfrag == 0) {
        return 0;
     }
     _carvpath_entity_add_fragment(self,newfrag);
     free(firstfragstring);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
     _carvpath_string_count--;
#endif
     char *morefrags=_carvpath_utility_get_remaininglayers_string(remaining,'_');
     //Anything 'remaining' but the top layer string should get deleted here.
     if (remaining != toplayerstring) {
       free(remaining);
       remaining=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
       _carvpath_string_count--;
#endif
     }
     //Update our remaining string for the next time around.
     remaining=morefrags;
     //An empty remainder string means that we are done.
     if (remaining && (strlen(remaining) ==0)) {
       free(remaining);
       remaining=0;
#ifdef _LIBCARVPATH_DEBUG_ALLOC
       _carvpath_string_count--;
#endif
     }
  }
  return self;
}


//This function sets the path_cache for the top node.
int _carvpath_entity_set_path_cache(carvpath_entity *self,const char *mountpoint) {
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  //Directly setting the path_cache on non top entities is not supported.
  if (self->parent) {
    errno = CARVPATH_ERR_NONTOP;
    return 0;
  }
  //Allocate our new string.
  self->path_cache = (char *) calloc (1, strlen (mountpoint) + 1);
  if (self->path_cache == 0){
      errno=CARVPATH_ERR_ALLOC;
      return 0;
  }
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_string_count++;
#endif
  //Fill our string.
  strcpy (self->path_cache, mountpoint);
  return 1;
}

//This function returns a fragment at a given index.
carvpath_fragment *_carvpath_entity_get_fragment(const carvpath_entity *self,size_t index) {
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  //Check if the index is a valid index in the range [0 .. fragcount-1]
  if ((index >= self->fragcount)|| (index < 0) || (self->fragcount <=0)) {
     errno=CARVPATH_ERR_OUT_OF_RANGE;
     return 0;
  }
  //Walk to the right fragment.
  size_t curindex=0;
  carvpath_fragment *rval=self->fraglist;
  while((rval) && (curindex != index)) {
     rval=rval->next;
     curindex++;
  } 
  if (rval == 0) {
    errno=CARVPATH_ERR_NO_FRAGMENTS;
  }
  return rval;
}

//Get the entity as a path. Determining the paths is done eagerly,
//already, so this is fairly simple.
char * _carvpath_entity_get_as_path(const carvpath_entity *self) {
   if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
   }
   if (self->path_cache) {
      return self->path_cache;
   } 
   errno=CARVPATH_ERR_INCOMPLETE_ELEMENT;
   return 0;
}

//This function returns a new reference to a given entity.
carvpath_entity *_carvpath_entity_new_reference(carvpath_entity *self) {
   if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
   }
   //Increment the referencecount on the input entity.
   if (_carvpath_entity_increfcount(self)==0) {
      return 0;
   }
   //Return a reference to the updated entity.
   return self;
}

//This function will, if needed fixup a string representation of an entity with a longtoken version
//of this representation.
int _carvpath_entity_longtoken_fixup(carvpath_entity *self) {
  carvpath_library const *libstate=self->library_state;
  if (libstate->longtokendb==0) {
     return 1; //If we run without longtokendb support, no fixup is ever needed or used.
  }
  if (self->fraglist == 0) {
     return 1; //If we don't have any fragments, no fixup should be needed.
  }
  carvpath_fragment *last=_carvpath_fragment_find_last(self->fraglist);
  if (last == 0) {
     return 0;  
  }
  //If our token string size exceeds the maximum size, we need to fixup the path.
  if (strlen(last->string_cache) > libstate->maxtokensize) {
     if (carvpath_longtoken_fixup(last->string_cache,libstate->longtokendb) == 0) {
        errno=CARVPATH_ERR_LONGTOKEN;
	return 0;
     }
     //Re-create our path_cache from our parent string representation and that of our fragment list.
     self->path_cache=_carvpath_util_new_pathstring(carvpath_get_as_path(self->parent),last->string_cache);
     if (self->path_cache == 0) {
	  return 0;
     }
  }
  return 1;
}

//This function copies a single fragment from a source entity to a new entity with one less layer
//of indirection than the original entity.
int _carvpath_entity_flatten_single_frag_copy_one_layer_up(carvpath_entity *self,carvpath_fragment *pflist,
		carvpath_fragment *origfrag) {
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  if (pflist == 0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  if (origfrag ==0) {
    errno=CARVPATH_ERR_NOENT;
    return 0;
  }
  //Set where we are within in our original parent 
  off_t parentoffset=0;
  //Process each fragment in our parent fragment list.
  carvpath_fragment *parentfrag=pflist;
  while (parentfrag) {
      //Try to create a new fragment of the overlap between the original fragment and the particular
      //parent fragment we are currently processing.
      carvpath_fragment *overlapfrag=_carvpath_fragment_overlap(parentfrag,parentoffset,origfrag);
      if (overlapfrag) {
	 //If we do have an overlap fragment, add it to our new flatter entity.
         if(_carvpath_entity_add_fragment(self,overlapfrag)==0) {
           return 0;
	 }
      } else {
	 //If we don't have overlap, check for errors, as a return of null on its own is ambiguous
	 //with respect to errors.
         if (errno) {
            return 0;
	 }
      }
      //Update where we are within in our original parent
      parentoffset+=parentfrag->size;
      //Move on to the next parent fragment.
      parentfrag=parentfrag->next;
  }
  return 1;
}

//This function copies a fragment list, removing one layer of indirection from the parent chain while doing so.
int _carvpath_entity_fragment_flatten_frags_copy_one_layer_up(carvpath_entity *self,carvpath_entity *origent) {
   if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
   }
   if (origent == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
   }
   carvpath_entity *origparent=origent->parent;
   carvpath_fragment *origparentfraglist=origparent->fraglist;
   //Process each of the original fragments.
   carvpath_fragment *ofrag=origent->fraglist;
   while (ofrag) {
      if (ofrag->ftype == CARVPATH_FRAGTYPE_SPARSE) {
	//Sparse fragment can always move right up.
	int failure=0;
        carvpath_append_sparse(self,ofrag->size,&failure);
	if (failure) {
           return 0;
	}
      } else {
	//For each original non sparse fragment, process the original parent fragment list
	//to determine and copy all the overlapping fragments.
        if (_carvpath_entity_flatten_single_frag_copy_one_layer_up(self,origparentfraglist,ofrag)==0) {
           return 0;
        }
      }
      //Move on to the next of the original fragments.
      ofrag=ofrag->next;
   }
   return 1;
}

//This function will create a new fully flattened version of a carvpath entity.
carvpath_entity *carvpath_flatten(carvpath_entity *self) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOENT;
     return 0;
  }
  //We can not flatten top entities any further.
  carvpath_entity *parent=self->parent;
  if (parent == 0) {
      return _carvpath_entity_new_reference(self);
  }
  //We can not flatten level 1 entities any further.
  carvpath_entity *grandparent=parent->parent;
  if (grandparent==0) {
      _carvpath_entity_longtoken_fixup(self);
      return _carvpath_entity_new_reference(self);
  }
  //Create a new empty entity bound to the grand parent of our original one.
  carvpath_entity *flattened=_carvpath_entity_new(0,grandparent);
  if (flattened == 0) {
      errno=CARVPATH_ERR_NOENT;
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
//This function updates the size of a (top level) entity.
void _carvpath_entity_set_size(carvpath_entity *self,off_t newsize){
  self->totalsize=newsize;
}

//#################################################################################
//#                     Implementation of the interface functions                 #
//#################################################################################


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
  snprintf(_carvpath_error_strings[CARVPATH_ERR_NOENT],127,"Internal libcarvpath function invoked with NULL value for entity.");
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
  carvpath_library *self=(carvpath_library *)calloc(1, sizeof(carvpath_library));
  if (self == 0) {
     errno=CARVPATH_ERR_ALLOC;
     return 0;
  }
  self->longtokendb=0;
  if (uselongtokendb) {
      if (carvpath_longtoken_opendb(&(self->longtokendb)) == SQLITE_ERROR) {
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
     errno=CARVPATH_ERR_NOENT;
  }
  if (self->longtokendb) {
    sqlite3_close(self->longtokendb); 
  }
  int errval=0;
  for (errval=0;errval < (CARVPATH_ERR_MAXERRNO+2);errval++) {
	  free(_carvpath_error_strings[errval]=0);
	  _carvpath_error_strings[errval]=0;
  }
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
   if (entity == 0) return 0;
   //Add a sparse fragment to that entity.
   if (_carvpath_entity_add_sparse_fragment(entity,size) == 0) {
      carvpath_free(entity,0);
      return 0;
   }
   return entity;
}

carvpath_entity *carvpath_derive(carvpath_entity *ent,off_t offset,off_t size){
   //Create a new entity.
   carvpath_entity *newentity=_carvpath_entity_new(0,ent);
   if (newentity == 0) return 0;
   //Check if the new fragment would fall within the available size of the parent entity.
   if (offset > ent->totalsize) {
        errno=CARVPATH_ERR_OUT_OF_RANGE;
        return 0;
   }
   if ((offset + size) > ent->totalsize) {
      errno=CARVPATH_ERR_OUT_OF_RANGE; //replaces: size = source->totalsize - offset;
      return 0;
   }
   //Add a regular fragment to that entity.
   if (_carvpath_entity_add_regular_fragment(newentity,offset,size) == 0) {
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

void carvpath_append(carvpath_entity *target,off_t offset,off_t size,int *failure){
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
  //Check if the new fragment would fall within the available size of the parent entity.
  if (offset > parent->totalsize) {
      if (failure) { *failure=1; }
      errno=CARVPATH_ERR_OUT_OF_RANGE;
      return;
  }
  if ((offset + size) > parent->totalsize) {
      if (failure) { *failure=1; }
      errno=CARVPATH_ERR_OUT_OF_RANGE; //replaces: size = source->totalsize - offset;
      return;
  }
  //Add a regular fragment to the entity.
  if (_carvpath_entity_add_regular_fragment(target,offset,size) == 0) {
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

carvpath_entity *carvpath_parse(carvpath_entity *parententity,const char *relpath){ 
  //Get the string section related to the level N entity
  char *toplayerstring=_carvpath_utility_get_toplayer_string(relpath,_carvpath_seperator_char);
  if (toplayerstring == 0) {
     return 0;
  }
  //Create a new entity from this string.
  carvpath_entity *curlevelentity=_carvpath_entity_new_from_string(parententity,toplayerstring);
  if (curlevelentity==0) {
     return 0;
  }
  //were done with the toplayer string
  free(toplayerstring);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
  _carvpath_string_count--;
#endif
  //path_cache should be set, otherwise we have a probem.
  if (curlevelentity->path_cache == 0) {
     return 0;
  }
  //Fetch the level N+1 etc entities string.
  char * remaininglevelsstring=_carvpath_utility_get_remaininglayers_string(relpath,_carvpath_seperator_char);
  if (remaininglevelsstring) {
     if (strlen(remaininglevelsstring)>0) {
	//If we have a remaininglevelsstring, call carvpath_parse and return the resulting entity.
        carvpath_entity *rval=carvpath_parse(curlevelentity,remaininglevelsstring);
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
