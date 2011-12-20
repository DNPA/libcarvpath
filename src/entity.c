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

//######################################################################################
//#                   carvpath_entity related functions (pseudo methods)               #
//######################################################################################
//This function decrements the reference count for the entity object. If the reference count
//reaches zero, the carvpath_entity is destroyed.
int _carvpath_entity_decrefcount(carvpath_entity *self){
    if (self == 0) {
      errno=CARVPATH_ERR_NOENT_INT;
      return 0;
    }
    (self->refcount)--;
    if (self->refcount == 0) {
       int rval=_carvpath_entity_destroy(self);
       return rval;
    }
    return 1;
}

//This function destroys a carvpath_entity and its members.
int _carvpath_entity_destroy(carvpath_entity *self) {
   if (self == 0) {
      errno=CARVPATH_ERR_NOENT_INT;
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
    errno=CARVPATH_ERR_NOENT_INT;
    return 0;
  }
  (self->refcount)++;
  return 1;
}

//This function updates the patch_cache for a new or changed carvpath_entity.
int _carvpath_entity_update_path_cache(carvpath_entity *self){
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT_INT;
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
     errno=CARVPATH_ERR_NOENT_INT;
     return 0;
  }
  if (frag==0) {
     errno=CARVPATH_ERR_NOFRAG_INT;
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
  carvpath_entity *parent=self->parent;
  if ((frag->offset + frag->size) > carvpath_get_size(parent,0)) {
     if (frag->offset > carvpath_get_size(parent,0)) {
         //discard fragment out of range.
         free(frag);
         return 1;
     }
     frag->size = carvpath_get_size(parent,0) - frag->offset;
  } 
  //The first fragment 'is' the fragment list initially.
  if (self->fraglist == 0) {
     self->fraglist=frag;
     self->fragcount++;
  } else {
     //Other fragments should go to the end of the list.
     carvpath_fragment *lastfrag=_carvpath_fragment_find_last(self->fraglist);
     //Try to merge the new fragment with the last of the list.
     if (_carvpath_fragment_merge(lastfrag,frag)==0) {
        self->fragcount++;
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
      errno=CARVPATH_ERR_NOENT_INT;
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

int _carvpath_entity_add_fragment_respect_onoor(carvpath_entity *self,carvpath_fragment *frag,carvpath_out_of_range_action onoor) {
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT_INT;
    return 0;
  }
  carvpath_entity *parent=self->parent;
  if (parent == 0) {
    errno=CARVPATH_ERR_NOENT_INT;
    return 0;
  } 
  if ((frag->ftype == CARVPATH_FRAGTYPE_REGULAR) && (frag->offset+frag->size > parent->totalsize)) {
    switch(onoor) {
       case CARVPATH_OOR_FAIL:
           _carvpath_fragment_destroy(frag);     
           errno=CARVPATH_ERR_OUT_OF_RANGE;
           return 0;
       case CARVPATH_OOR_TRUNCATE:
           if (frag->offset >= parent->totalsize) {
              _carvpath_fragment_destroy(frag);
              return 1;
           }
           frag->size=parent->totalsize - frag->offset;
           return _carvpath_entity_add_fragment(self,frag);
       case CARVPATH_OOR_SPARSE: 
           if (frag->offset >= parent->totalsize) {
              off_t sparsesize=frag->size;
              _carvpath_fragment_destroy(frag);
              return _carvpath_entity_add_sparse_fragment(self,sparsesize);
           }
           off_t sparsesize=frag->size - (parent->totalsize - frag->offset);
           frag->size=parent->totalsize - frag->offset;
           if (_carvpath_entity_add_fragment(self,frag) == 0) {
              return 0;
           }
           return _carvpath_entity_add_sparse_fragment(self,sparsesize);
    }
  } else {
     return _carvpath_entity_add_fragment(self,frag);
  }
  
}

//This function adds a new regular fragment to the end of an entity.
int _carvpath_entity_add_regular_fragment(carvpath_entity *self,off_t offset,off_t size,carvpath_out_of_range_action onoor){
  if (self == 0) {
    errno=CARVPATH_ERR_NOENT_INT;
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
  return _carvpath_entity_add_fragment_respect_onoor(self,frag,onoor);
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
carvpath_entity *_carvpath_entity_new_from_string(carvpath_entity *parententity,char *toplayerstring,carvpath_out_of_range_action onoor){
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
     if (strcmp(firstfragstring,"0+0")) {
       carvpath_fragment *newfrag=_carvpath_fragment_new_from_string(firstfragstring);
       if (newfrag == 0) {
          return 0;
       }
       if (_carvpath_entity_add_fragment_respect_onoor(self,newfrag,onoor)== 0) {
           free(firstfragstring);
#ifdef _LIBCARVPATH_DEBUG_ALLOC
           _carvpath_string_count--;
#endif
          return 0;
       }
     }
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
    errno=CARVPATH_ERR_NOENT_INT;
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
    errno=CARVPATH_ERR_NOENT_INT;
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
     errno=CARVPATH_ERR_NOENT_INT;
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
     errno=CARVPATH_ERR_NOENT_INT;
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
     if (self->path_cache) {
        free(self->path_cache);
        self->path_cache=0;
     } 
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
    errno=CARVPATH_ERR_NOENT_INT;
    return 0;
  }
  if (pflist == 0) {
    errno=CARVPATH_ERR_NOFRAG_INT;
    return 0;
  }
  if (origfrag ==0) {
    errno=CARVPATH_ERR_NOFRAG_INT;
    return 0;
  }
  //Set where we are within in our original parent 
  off_t parentoffset=0;
  //Process each fragment in our parent fragment list.
  carvpath_fragment *parentfrag=pflist;
  while (parentfrag) {
      //Try to create a new fragment of the overlap between the original fragment and the particular
      //parent fragment we are currently processing.
      errno=0;
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
     errno=CARVPATH_ERR_NOENT_INT;
     return 0;
   }
   if (origent == 0) {
     errno=CARVPATH_ERR_NOENT_INT;
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

//This function updates the size of a (top level) entity.
void _carvpath_entity_set_size(carvpath_entity *self,off_t newsize){
  self->totalsize=newsize;
}

