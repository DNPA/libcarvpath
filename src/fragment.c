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

//#####################################################################################
//#             carvpath_fragment related functions (pseudo methods                   #
//#####################################################################################

//This function will fill the string representation stored in fragments if they are
//set to 0. Each fragment should hold the representation of the full fragment list
//upto itself. This means that the true reperentation of the fragment list is stored 
//with the last fragment of the fragment list.
int _carvpath_fragment_update_strings_if_needed(carvpath_fragment *self,const char *prefix) {
  if (self == 0) {
     errno=CARVPATH_ERR_NOFRAG_INT;
     return 0;
  }
  if (prefix == 0) {
     errno=CARVPATH_ERR_NOSTRING_INT;
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
     errno=CARVPATH_ERR_NOFRAG_INT;
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
     errno=CARVPATH_ERR_NOFRAG_INT;
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
      errno=CARVPATH_ERR_NOFRAG_INT;
      return 0;
   }
   if (newfrag == 0) {
      errno=CARVPATH_ERR_NOFRAG_INT;
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
      errno=CARVPATH_ERR_NOFRAG_INT;
      return 0;
   }
   if (lfrag == 0) {
      errno=CARVPATH_ERR_NOFRAG_INT;
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



