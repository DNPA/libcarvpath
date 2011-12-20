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

carvpath_fragment *
_carvpath_flaten_overlap (off_t offset1, carvpath_fragment * parentfrag,
			  carvpath_fragment * curfrag)
{
  off_t overlapoffset = 0;
  off_t overlapsize = 0;
  off_t parentoffset = parentfrag->offset;
  off_t size1 = parentfrag->decodedsize;
  off_t offset2 = curfrag->offset;
  off_t size2 = curfrag->size;
  int parfrag = 1;
  int cldfrag = 1;
  if (offset1 == offset2)
    {
      if (size1 == size2)
	{
	  overlapsize = size1;
	  overlapoffset = offset1;
	  parfrag = 0;
	  cldfrag = 0;
	}
      else if (size1 < size2)
	{
	  overlapsize = size1;
	  overlapoffset = offset1;
	  parfrag = 0;
	}
      else
	{
	  overlapsize = size2;
	  overlapoffset = offset2;
	  cldfrag = 0;
	}
    }
  else if (offset1 > offset2)
    {
      if ((offset1 + size1) == (offset2 + size2))
	{
	  overlapsize = size1;
	  overlapoffset = offset1;
	  parfrag = 0;
	}
      else if ((offset1 + size1) > (offset2 + size2))
	{
	  overlapsize = offset2 + size2 - offset1;
	  if (overlapsize <= 0)
	    return 0;
	  overlapoffset = offset1;
	}
      else
	{
	  overlapsize = size1;
	  overlapoffset = offset1;
	  parfrag = 0;
	}
    }
  else
    {
      if ((offset1 + size1) >= (offset2 + size2))
	{
	  overlapsize = size2;
	  overlapoffset = offset2;
	  cldfrag = 0;
	}
      else
	{
	  overlapsize = offset1 + size1 - offset2;
	  overlapoffset = offset2;
	}
    }
  if (overlapsize <= 0)
    return 0;
  carvpath_fragment *rval =
    (carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
  if (rval == 0)
    return 0;
  if (parentfrag->ftype == CARVPATH_FRAGTYPE_SPARSE)
    {
      if ((curfrag->ftype == CARVPATH_FRAGTYPE_REGULAR)
	  || (curfrag->ftype == CARVPATH_FRAGTYPE_UNKNOWN))
	{
	  rval->size = 0;
	  rval->decodedsize = overlapsize;
	  rval->ftype = CARVPATH_FRAGTYPE_SPARSE;
	  rval->offset = 0;
	}
      else
	{
          fprintf(stderr,"inconsistent path: only regular and unknown entities can be mapped to sparse\n");
	  rval->ftype = CARVPATH_FRAGTYPE_INVALID;
	}
    }
  else if ((parentfrag->ftype == CARVPATH_FRAGTYPE_REGULAR)
	   && (curfrag->ftype == CARVPATH_FRAGTYPE_REGULAR))
    {
      off_t parentfragoffset = parentoffset - offset1 + overlapoffset;
      rval->size = overlapsize;
      rval->decodedsize = overlapsize;
      rval->ftype = CARVPATH_FRAGTYPE_REGULAR;
      rval->offset = parentfragoffset;
    }
  else if ((parentfrag->ftype == CARVPATH_FRAGTYPE_REGULAR)
	   && (curfrag->ftype == CARVPATH_FRAGTYPE_COMPRESSED_NTFS))
    {
      if (cldfrag)
	{
          fprintf(stderr,"unflatable path: compressed child can only be mapped to regular parent if child remains unfragmented\n"); 
	  rval->ftype = CARVPATH_FRAGTYPE_INVALID;
	}
      else
	{
	  rval->size = curfrag->size;
	  rval->decodedsize = curfrag->decodedsize;
	  rval->offset = parentfrag->offset;
	  rval->ftype = CARVPATH_FRAGTYPE_COMPRESSED_NTFS;
	}
    }
  else if ((parentfrag->ftype == CARVPATH_FRAGTYPE_COMPRESSED_NTFS)
	   && (curfrag->ftype == CARVPATH_FRAGTYPE_REGULAR))
    {
      if (parfrag)
	{
          fprintf(stderr,"unflatable path: regular child can only be mapped to compressed parent if parent remains unfragmented\n");
	  rval->ftype = CARVPATH_FRAGTYPE_INVALID;
	}
      else
	{
	  rval->size = parentfrag->size;
	  rval->decodedsize = parentfrag->decodedsize;
	  rval->offset = parentfrag->offset;
	  rval->ftype = CARVPATH_FRAGTYPE_COMPRESSED_NTFS;
	}
    }
  else
    {
      fprintf(stderr,"invalid path\n"); 
      rval->ftype = CARVPATH_FRAGTYPE_INVALID;
    }
  return rval;
}

int
carvpath_append (carvpath_entity * target, carvpath_entity * source,
		 off_t offset, off_t rsize)
{
  if (target == 0)
    return CARVPATH_ERR_NOENT;
  if (source == 0)
    return CARVPATH_ERR_NOENT;
  if (target->parent != source)
    return CARVPATH_ERR_BAD_PARENT;
  off_t size = rsize;
  if (offset > source->totalsize)
    return CARVPATH_ERR_OUT_OF_RANGE;
  if ((offset + size) > source->totalsize)
    size = source->totalsize - offset;
  carvpath_fragment *lastfrag = target->fraglist;
  while (lastfrag->next)
    lastfrag = lastfrag->next;
  if (((lastfrag->offset + lastfrag->size) == offset)
      && (lastfrag->ftype == CARVPATH_FRAGTYPE_REGULAR))
    {
      lastfrag->size += size;
      target->totalsize += size;
      return 0;
    }
  carvpath_fragment *cfrag =
    (carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
  if (cfrag == 0)
    return CARVPATH_ERR_ALLOC;
  cfrag->ftype = CARVPATH_FRAGTYPE_REGULAR;
  cfrag->size = size;
  cfrag->decodedsize = size;
  cfrag->offset = offset;
  lastfrag->next = cfrag;
  target->totalsize += size;
  target->fragcount++;
  return 0;
}

int carvpath_append_encoded(carvpath_entity *target,carvpath_entity *source,
                             off_t offset,
                             off_t rsize,
                             off_t decsize,
                             carvpath_fragtype enctype) {
  if (target == 0)
    return CARVPATH_ERR_NOENT;
  if (source == 0)
    return CARVPATH_ERR_NOENT;
  if (target->parent != source)
    return CARVPATH_ERR_BAD_PARENT;
  off_t size = rsize;
  if (offset > source->totalsize)
    return CARVPATH_ERR_OUT_OF_RANGE;
  if ((offset + size) > source->totalsize)
    size = source->totalsize - offset;
  carvpath_fragment *lastfrag = target->fraglist;
  while (lastfrag->next)
    lastfrag = lastfrag->next;
  if (((lastfrag->offset + lastfrag->size) == offset)
      && (lastfrag->ftype == CARVPATH_FRAGTYPE_REGULAR))
    {
      lastfrag->size += size;
      target->totalsize += size;
      return 0;
    }
  carvpath_fragment *cfrag =
    (carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
  if (cfrag == 0)
    return CARVPATH_ERR_ALLOC;
  cfrag->ftype = enctype;
  cfrag->size = size;
  cfrag->decodedsize = decsize;
  cfrag->offset = offset;
  lastfrag->next = cfrag;
  target->totalsize += size;
  target->fragcount++;
  return 0;
}

size_t
_carvpath_flaten_fragment (carvpath_fragment * pfraglist,
			   carvpath_entity * entity,
			   carvpath_fragment * curfrag)
{
  off_t totalsize = 0;
  if (curfrag->ftype == CARVPATH_FRAGTYPE_UNKNOWN)
    return 1;
  carvpath_fragment *parentfrag = pfraglist;
  carvpath_fragment *lastfrag = entity->fraglist;
  if (lastfrag)
    {
      while (lastfrag->next)
	lastfrag = lastfrag->next;
    }
  if (curfrag->ftype == CARVPATH_FRAGTYPE_SPARSE)
    {
      carvpath_fragment *newsparse =
	(carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
      newsparse->size = 0;
      newsparse->decodedsize = curfrag->decodedsize;
      newsparse->offset = 0;
      newsparse->ftype = CARVPATH_FRAGTYPE_SPARSE;
      if (lastfrag)
	lastfrag->next = newsparse;
      else
	entity->fraglist = newsparse;
      return 0;
    }
  while ((parentfrag) && (totalsize < (curfrag->offset + curfrag->decodedsize)))
    {
      if (lastfrag)
	{
	  lastfrag->next =
	    _carvpath_flaten_overlap (totalsize, parentfrag, curfrag);
	  if (lastfrag->next)
	    {
	      lastfrag = lastfrag->next;
	      entity->fragcount++;
	    }
	}
      else
	{
	  lastfrag =
	    _carvpath_flaten_overlap (totalsize, parentfrag, curfrag);
	  if (lastfrag)
	    {
	      entity->fraglist = lastfrag;
	      entity->fragcount++;
	    }
          else {
             fprintf(stderr,"no overlap\n");
          }
	}
      if ((lastfrag) && (lastfrag->ftype == CARVPATH_FRAGTYPE_INVALID))
	return 1;
      totalsize += parentfrag->decodedsize;
      parentfrag = parentfrag->next;
    }
  return 0;
}

void
_carvpath_merge_interlocking (carvpath_entity * entity)
{
  if (entity == 0)
    return;
  carvpath_fragment *curfrag = entity->fraglist;
  while (curfrag)
    {
      while ((curfrag->next) &&
	     (((carvpath_fragment *) (curfrag->next))->offset ==
	      (curfrag->offset + curfrag->size))
	     && (((carvpath_fragment *) (curfrag->next))->ftype ==
		 curfrag->ftype)
	     && (curfrag->ftype == CARVPATH_FRAGTYPE_REGULAR))
	{
	  curfrag->size += ((carvpath_fragment *) (curfrag->next))->size;
	  curfrag->decodedsize +=
	    ((carvpath_fragment *) (curfrag->next))->decodedsize;
	  entity->fragcount--;
	  carvpath_fragment *dfrag = ((carvpath_fragment *) (curfrag->next));
	  curfrag->next = dfrag->next;
	  free (dfrag);
	}
      while ((curfrag->next) &&
	     (((carvpath_fragment *) (curfrag->next))->ftype ==
	      curfrag->ftype) && (curfrag->ftype == CARVPATH_FRAGTYPE_SPARSE))
	{
	  curfrag->decodedsize +=
	    ((carvpath_fragment *) (curfrag->next))->decodedsize;
	  entity->fragcount--;
	  carvpath_fragment *dfrag = ((carvpath_fragment *) (curfrag->next));
	  curfrag->next = dfrag->next;
	  free (dfrag);
	}
      curfrag = curfrag->next;
    }
}

char *
_carvpath_parse_entity (carvpath_entity * entity, char *parsepointer)
{
  if (entity == 0)
    return 0;
  if (parsepointer == 0)
    return 0;
  if (parsepointer[0] == 0)
    return 0;
  if (strlen (parsepointer) < 3)
    return 0;
  char *offsetstring = parsepointer;
  char *sizestring = 0;
  char *decodedsizestring = 0;
  size_t index = 0;
  off_t fragsize = 0;
  off_t fragoffset = 0;
  off_t decodedsize = 0;
  carvpath_fragtype ftype = CARVPATH_FRAGTYPE_INVALID;
  size_t max = strlen (parsepointer);
  if (parsepointer[0] == 'S')
    {
      ftype = CARVPATH_FRAGTYPE_SPARSE;
      decodedsizestring = parsepointer + 1;
      index = 1;
      for (;
	   ((index < max) && (parsepointer[index] != '/')
	    && (parsepointer[index] != '_')); index++)
	{
	  ;
	}

    }
  else
    {
      if (parsepointer[0] == 'U')
	{
	  index = 1;
	  decodedsizestring = parsepointer + 1;
	  ftype = CARVPATH_FRAGTYPE_UNKNOWN;
	}
      else if (parsepointer[0] == 'N')
	{
	  index = 1;
	  decodedsizestring = parsepointer + 1;
	  ftype = CARVPATH_FRAGTYPE_COMPRESSED_NTFS;
	}
      else
	{
	  ftype = CARVPATH_FRAGTYPE_REGULAR;
	}
      for (;
	   ((index < max) && (parsepointer[index] != '/')
	    && (parsepointer[index] != '_')); index++)
	{
	  if (parsepointer[index] == '=')
	    {
	      offsetstring = parsepointer + index + 1;
	    }
	  else if (parsepointer[index] == '+')
	    {
	      sizestring = parsepointer + index + 1;
	    }
	  else
	    {
	      if ((parsepointer[index] < '0') || (parsepointer[index] > '9'))
		{
                  fprintf(stderr,"Invalid character in carvpath '%c' numeric expected\n",parsepointer[index]);
		  return 0;
		}
	    }
	}
      if (index < 3)
	{
          fprintf(stderr,"Invalid carvpath : to small token to be valid '%s'\n",parsepointer);
	  return 0;
	}
      if (sizestring == 0)
	{
          fprintf(stderr,"Invalid carvpath : no size string in token '%s'\n",parsepointer);
	  return 0;
	}
      if ((offsetstring[0] < '0') || (offsetstring[0] > '9'))
	{
          fprintf(stderr,"Invalid first character in offset token '%c' numeric expected\n",offsetstring[0]);
	  return 0;
	}
      if ((sizestring[0] < '1') || (sizestring[0] > '9'))
	{
          fprintf(stderr,"Invalid first character in size token '%c' numeric expected\n",sizestring[0]);
	  return 0;
	}
      fragsize = atoll (sizestring);
      fragoffset = atoll (offsetstring);
      if ((entity->parent)
	  && (((carvpath_entity *) (entity->parent))->totalsize <
	      (fragoffset + fragsize)))
	{
	  carvpath_fragment *dfrag = entity->fraglist;
	  while (dfrag)
	    {
	      entity->fraglist = dfrag->next;
	      free (dfrag);
	      dfrag = entity->fraglist;
	    }
          fprintf(stderr,"Invalid carvpath, out of parent range\n");
	  return 0;
	}
    }
  if (decodedsizestring)
    {
      fprintf(stderr,"decodedsizestring='%s'\n",decodedsizestring);
      decodedsize = atoll (decodedsizestring);
    }
  else
    {
      decodedsize = fragsize;
    }
  carvpath_fragment *newfrag =
    (carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
  if (newfrag == 0) {
    fprintf(stderr,"allocation error\n");
    return 0;
  }
  newfrag->offset = fragoffset;
  newfrag->size = fragsize;
  newfrag->decodedsize = decodedsize;
  newfrag->ftype = ftype;
  if (entity->fraglist == 0)
    {
      entity->fraglist = newfrag;
    }
  else
    {
      carvpath_fragment *lastfrag = entity->fraglist;
      while (lastfrag)
	{
	  if (lastfrag->next == 0)
	    {
	      lastfrag->next = newfrag;
	      lastfrag = 0;
	    }
	  else
	    {
	      lastfrag = lastfrag->next;
	    }
	}
    }
  entity->fragcount++;
  entity->totalsize += decodedsize;
  if (parsepointer[index] == '_')
    {
      return _carvpath_parse_entity (entity, parsepointer + index + 1);
    }
  if (parsepointer[index] == '/')
    return parsepointer + index + 1;
  return parsepointer + max;
}

carvpath_entity *
carvpath_top_entity (off_t fullsize, char *mountpoint,int uselongtokendb)
{
  carvpath_entity *cent =
    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
  if (cent == 0)
    return 0;
  cent->totalsize = fullsize;
  cent->fragcount = 0;
  cent->refcount = 1;
  
  cent->path_cache = (char *) calloc (1, strlen (mountpoint) + 1);
  if (cent->path_cache == 0)
    {
      free (cent);
      return 0;
    }
  strcpy (cent->path_cache, mountpoint);
  cent->fraglist = 0;
  cent->parent = 0;
  if (cent->path_cache == 0)
    {
      free (cent);
      return 0;
    }
  if (uselongtokendb)
     carvpath_longtoken_opendb(&(cent->longtokendb));
  return cent;
}

carvpath_entity *
carvpath_parse (carvpath_entity * top, char *relpath)
{
  if (top == 0)
    return 0;
  carvpath_entity *cent =
    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
  if (cent == 0)
    return 0;
  cent->fragcount = 0;
  cent->totalsize = 0;
  cent->refcount = 1;
  cent->path_cache = 0;
  cent->fraglist = 0;
  cent->parent = top;
  cent->longtokendb = top->longtokendb;
  top->refcount++;
  char * trelpath=relpath;
  if (relpath[0] == 'D') {
     trelpath = carvpath_longtoken_lookup(relpath,top->longtokendb);
  }
  char *parsepointer = _carvpath_parse_entity (cent, trelpath);
  if (parsepointer == 0)
    {
      carvpath_free (cent);
      return 0;
    }
  if (parsepointer[0] == 0)
    {
      if (cent->fragcount)
	{
	  return cent;
	}
      else
	{
	  return 0;
	}
    }
  cent->refcount = 0;
  carvpath_entity *rval = carvpath_parse (cent, parsepointer);
  if (relpath[0] == 'D') {
     free(trelpath);
  }
  return rval;
}

char *
carvpath_get_as_path (carvpath_entity * ent)
{
  if (ent == 0)
    return 0;
  char *parentstr = 0;
  if (ent->parent) {
    parentstr = carvpath_get_as_path (ent->parent);
  }
  if (ent->path_cache)
    {
      if (ent->parent)
	{
	  free (ent->path_cache);
	  ent->path_cache = 0;
	}
    }
  if (ent->path_cache == 0)
    {
      size_t ssize = strlen (parentstr) + ent->fragcount * 40 + 1;
      ent->path_cache = calloc (1, ssize);
      if (ent->path_cache == 0)
	return 0;
      sprintf (ent->path_cache, "%s%c", parentstr, _carvpath_seperator_char);
      carvpath_fragment *frag = ent->fraglist;
      while (frag)
	{
	  size_t start = strlen (ent->path_cache);
	  if (frag->ftype == CARVPATH_FRAGTYPE_REGULAR)
	    {
	      if (frag->next)
		{
		  sprintf (ent->path_cache + start,
			   "%" PRId64 "+%" PRId64 "_", frag->offset,
			   frag->size);
		}
	      else
		{
		  sprintf (ent->path_cache + start, "%" PRId64 "+%" PRId64,
			   frag->offset, frag->size);
		}
	    }
	  else if (frag->ftype == CARVPATH_FRAGTYPE_SPARSE)
	    {
	      if (frag->next)
		{
		  sprintf (ent->path_cache + start, "S%" PRId64 "_",
			   frag->decodedsize);
		}
	      else
		{
		  sprintf (ent->path_cache + start, "S%" PRId64,
			   frag->decodedsize);
		}
	    }
	  else if (frag->ftype == CARVPATH_FRAGTYPE_COMPRESSED_NTFS)
	    {
	      if (frag->next)
		{
		  sprintf (ent->path_cache + start,
			   "N%" PRId64 "=%" PRId64 "+%" PRId64 "_",
			   frag->decodedsize, frag->offset, frag->size);
		}
	      else
		{
		  sprintf (ent->path_cache + start,
			   "N%" PRId64 "=%" PRId64 "+%" PRId64,
			   frag->decodedsize, frag->offset, frag->size);
		}
	    }
	  else
	    {
	      if (frag->next)
		{
		  sprintf (ent->path_cache + start,
			   "U%" PRId64 "=%" PRId64 "+%" PRId64 "_",
			   frag->decodedsize, frag->offset, frag->size);
		}
	      else
		{
		  sprintf (ent->path_cache + start,
			   "U%" PRId64 "=%" PRId64 "+%" PRId64,
			   frag->decodedsize, frag->offset, frag->size);
		}
	    }
	  frag = frag->next;
	}
    }
  if ((ent->path_cache) && (parentstr)  &&
     ((strlen(ent->path_cache) - strlen(parentstr) -1) > CARVPATH_MAX_TOKEN_SIZE)) {
     carvpath_longtoken_fixup(ent->path_cache + strlen(parentstr)+1,ent->longtokendb);
  }
  return ent->path_cache;
}

off_t
carvpath_get_size (carvpath_entity * ent)
{
  if (ent == 0)
    return CARVPATH_ERR_NOENT;
  return ent->totalsize;;
}

size_t
carvpath_get_fragcount (carvpath_entity * ent)
{
  if (ent == 0)
    return CARVPATH_ERR_NOENT;
  return ent->fragcount;;
}

off_t
carvpath_fragment_get_size (carvpath_entity * ent, size_t index)
{
  if (ent == 0)
    return CARVPATH_ERR_NOENT;
  carvpath_fragment *cfrag = ent->fraglist;
  size_t iter;
  for (iter = 0; iter <= index; iter++)
    {
      if (cfrag == 0)
	return CARVPATH_ERR_OUT_OF_RANGE;
      if (iter == index)
	return cfrag->size;
      cfrag = (carvpath_fragment *) cfrag->next;
    }
  return CARVPATH_ERR_OUT_OF_RANGE;
}

off_t
carvpath_fragment_get_decoded_size (carvpath_entity * ent, size_t index)
{
  if (ent == 0)
    return CARVPATH_ERR_NOENT;
  carvpath_fragment *cfrag = ent->fraglist;
  size_t iter;
  for (iter = 0; iter <= index; iter++)
    {
      if (cfrag == 0)
        return CARVPATH_ERR_OUT_OF_RANGE;
      if (iter == index)
        return cfrag->decodedsize;
      cfrag = (carvpath_fragment *) cfrag->next;
    }
  return CARVPATH_ERR_OUT_OF_RANGE;
}

carvpath_fragtype
carvpath_fragment_get_type (carvpath_entity * ent, size_t index)
{
  if (ent == 0)
    return CARVPATH_ERR_NOENT;
  carvpath_fragment *cfrag = ent->fraglist;
  size_t iter;
  for (iter = 0; iter <= index; iter++)
    {
      if (cfrag == 0)
        return CARVPATH_ERR_OUT_OF_RANGE;
      if (iter == index)
        return cfrag->ftype;
      cfrag = (carvpath_fragment *) cfrag->next;
    }
  return CARVPATH_ERR_OUT_OF_RANGE;
}


off_t
carvpath_fragment_get_offset (carvpath_entity * ent, size_t index)
{
  if (ent == 0)
    return CARVPATH_ERR_NOENT;
  carvpath_fragment *cfrag = ent->fraglist;
  size_t iter;
  for (iter = 0; iter <= index; iter++)
    {
      if (cfrag == 0)
	return CARVPATH_ERR_OUT_OF_RANGE;
      if (iter == index)
	return cfrag->offset;
      cfrag = (carvpath_fragment *) cfrag->next;
    }
  return CARVPATH_ERR_OUT_OF_RANGE;
}

carvpath_entity *
carvpath_derive (carvpath_entity * ent, off_t offset, off_t rsize)
{
  if (ent == 0)
    return 0;
  off_t size = rsize;
  if (offset > ent->totalsize)
    return 0;
  if ((offset + size) > ent->totalsize)
    size = ent->totalsize - offset;
  if (ent == 0)
    return 0;
  carvpath_fragment *cfrag =
    (carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
  if (cfrag == 0)
    return 0;
  cfrag->offset = offset;
  cfrag->size = size;
  cfrag->decodedsize = size;
  cfrag->ftype = CARVPATH_FRAGTYPE_REGULAR;
  cfrag->next = 0;
  carvpath_entity *cent =
    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
  if (cent == 0)
    {
      free (cfrag);
      return 0;
    }
  cent->totalsize = size;
  cent->fragcount = 1;
  cent->refcount = 1;
  cent->path_cache = 0;
  cent->fraglist = cfrag;
  cent->parent = (void *) ent;
  cent->longtokendb = ent->longtokendb;
  ent->refcount++;
  return cent;
}
carvpath_entity *carvpath_derive_encoded(carvpath_entity *ent,
                             off_t offset,
                             off_t rsize,
                             off_t decsize,
                             carvpath_fragtype enctype) {
  if (ent == 0)
    return 0;
  off_t size = rsize;
  if (offset > ent->totalsize)
    return 0;
  if ((offset + size) > ent->totalsize)
    size = ent->totalsize - offset;
  if (ent == 0)
    return 0;
  carvpath_fragment *cfrag =
    (carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
  if (cfrag == 0)
    return 0;
  cfrag->offset = offset;
  cfrag->size = size;
  cfrag->decodedsize = decsize;
  cfrag->ftype = enctype;
  cfrag->next = 0;
  carvpath_entity *cent =
    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
  if (cent == 0)
    {
      free (cfrag);
      return 0;
    }
  cent->totalsize = decsize;
  cent->fragcount = 1;
  cent->refcount = 1;
  cent->path_cache = 0;
  cent->fraglist = cfrag;
  cent->parent = (void *) ent;
  cent->longtokendb = ent->longtokendb;
  ent->refcount++;
  return cent;
}



carvpath_entity *
carvpath_sequence (size_t fragcount, carvpath_entity ** fraglist)
{
  carvpath_entity *cent =
    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
  if (cent == 0)
    return 0;
  cent->totalsize = 0;
  cent->fragcount = 0;
  cent->refcount = 1;
  cent->path_cache = 0;
  cent->fraglist = 0;
  cent->parent = 0;
  size_t iter;
  carvpath_fragment *lastfrag = 0;
  for (iter = 0; iter < fragcount; iter++)
    {
      if (fraglist[iter] != 0)
	{

	  if (cent->parent == 0)
	    {
	      cent->parent = fraglist[iter]->parent;
              cent->longtokendb =  fraglist[iter]->longtokendb;
	      ((carvpath_entity *) (cent->parent))->refcount++;
	    }
	  if (cent->parent != fraglist[iter]->parent)
	    {
	      carvpath_free (cent);
	      return 0;
	    }
	  if (cent->parent == 0)
	    {
	      carvpath_free (cent);
	      return 0;
	    }
	  cent->totalsize += fraglist[iter]->totalsize;
	  cent->fragcount += fraglist[iter]->fragcount;
	  carvpath_fragment *flfrag = fraglist[iter]->fraglist;
	  while (flfrag)
	    {
	      carvpath_fragment *newfrag =
		(carvpath_fragment *) calloc (1, sizeof (carvpath_fragment));
	      if (newfrag == 0)
		{
		  carvpath_free (cent);
		  return 0;
		}
	      newfrag->next = 0;
	      newfrag->offset = flfrag->offset;
	      newfrag->size = flfrag->size;
	      newfrag->ftype = flfrag->ftype;
	      newfrag->decodedsize = flfrag->decodedsize;
	      if (lastfrag)
		{
		  lastfrag->next = (void *) newfrag;
		}
	      else
		{
		  cent->fraglist = newfrag;
		}
	      lastfrag = newfrag;
	      flfrag = (carvpath_fragment *) flfrag->next;
	    }
	}
    }
  _carvpath_merge_interlocking (cent);
  return cent;
}

void
carvpath_free (carvpath_entity * ent)
{
  if (ent == 0)
    return;
  ent->refcount--;
  if (ent->refcount == 0)
    {
      if (ent->parent)
	{
	  carvpath_free (ent->parent);
	}
      else 
        {
          sqlite3_close(ent->longtokendb);            
        }
      carvpath_fragment *frag = ent->fraglist;
      while (frag)
	{
	  carvpath_fragment *tfrag = frag->next;
	  free (frag);
	  frag = tfrag;
	}
      if (ent->path_cache)
	{
	  free (ent->path_cache);
	  ent->path_cache = 0;
	}
      free (ent);
    }
}
carvpath_entity *
carvpath_flaten (carvpath_entity * ent)
{
  if (ent == 0)
    {
      return 0;
    }
  if (((carvpath_entity *) (ent->parent))->parent == 0)
    {
      return ent;
    }
  carvpath_entity *cent =
    (carvpath_entity *) calloc (1, sizeof (carvpath_entity));
  if (cent == 0)
    return 0;
  cent->parent = ((carvpath_entity *) (ent->parent))->parent;
  cent->longtokendb = ((carvpath_entity *) (ent->parent))->longtokendb;
  ((carvpath_entity *) (cent->parent))->refcount++;
  cent->totalsize = ent->totalsize;
  cent->fragcount = 0;
  cent->fraglist = 0;
  cent->refcount = 1;
  carvpath_fragment *migratefraglist =
    ((carvpath_entity *) (ent->parent))->fraglist;
  carvpath_fragment *curfrag = ent->fraglist;
  size_t errcount = 0;
  while (curfrag)
    {
      errcount += _carvpath_flaten_fragment (migratefraglist, cent, curfrag);
      curfrag = curfrag->next;
    }
  if (errcount)
    {
      carvpath_free (cent);
      return 0;
    }
  carvpath_entity *rval = carvpath_flaten (cent);
  if (cent != rval)
    {
      carvpath_free (cent);
    }
  _carvpath_merge_interlocking (rval);
  return rval;
}
