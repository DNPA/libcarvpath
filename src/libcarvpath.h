//CarvPath library 
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
#ifndef LIBCARVPATH_H
#define LIBCARVPATH_H
#ifdef __cplusplus
extern "C" {
#endif

#if _FILE_OFFSET_BITS != 64
#error file offset should be 64 bits, add the following to your compiler flags: -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
#endif
#ifndef _LARGEFILE_SOURCE
#error you must enable large file support, add the following to your compiler flags: -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#error you must enable large file support, add the following to your compiler flags: -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
#endif
#define CARVPATH_LINUX_MAX_TOKEN_SIZE 1000
#define WINDOWS_EXPORTABLE_MAX_TOKEN_SIZE 160
#ifdef _WIN32
#define CARVPATH_MAX_TOKEN_SIZE  WINDOWS_EXPORTABLE_MAX_TOKEN_SIZE
#ifndef _WIN64
#error You have no properly 64 bit configured compiler, add the following to your compiler flags:-D_WIN64
#endif
#define CARVPATH_PLATFORM_VARDIR "C:\\Program Files\\LibCarvPath"
#define PRId64  "%I64d"
	static char _carvpath_seperator_char = '\\';
#else
#define CARVPATH_MAX_TOKEN_SIZE  CARVPATH_LINUX_MAX_TOKEN_SIZE
#define CARVPATH_PLATFORM_VARDIR "/var/carvpath"
#include <inttypes.h>
	static char _carvpath_seperator_char = '/';
#endif
#include <sys/types.h>
#include <sqlite3.h>
#ifdef _LIBCARVPATH_DEBUG_ALLOC
//The below static ints are for debugging purposes only, never change these values,
////only for libcarvpath developers to check if libcarvpath isn't leaking.
static int _carvpath_entity_count = 0;
static int _carvpath_fragment_count = 0;
static int _carvpath_string_count = 0;
#endif
#define CARVPATH_ERR_ALLOC 1
#define CARVPATH_ERR_NOENT 2
#define CARVPATH_ERR_OUT_OF_RANGE 3
#define CARVPATH_ERR_ISTOP 4
#define CARVPATH_ERR_SQLITE 5
#define CARVPATH_ERR_INCOMPLETE_ELEMENT 6
#define CARVPATH_ERR_NOLIB 7
#define CARVPATH_ERR_ATTRIBUTES 8
#define CARVPATH_ERR_NO_FRAGMENTS 9
#define CARVPATH_ERR_NONTOP 10
#define CARVPATH_ERR_LONGTOKEN 11
#define CARVPATH_ERR_INVALID_FRAGMENT_TOKEN 12
#define CARVPATH_ERR_NODB 13
#define CARVPATH_ERR_DBLOOKUP 14
#define CARVPATH_ERR_NOENT_INT 15
#define CARVPATH_ERR_NOFRAG_INT 16
#define CARVPATH_ERR_NOSTRING_INT 17
#define CARVPATH_ERR_MAXERRNO CARVPATH_ERR_NOSTRING_INT

typedef enum {
  CARVPATH_OOR_FAIL,
  CARVPATH_OOR_TRUNCATE,
  CARVPATH_OOR_SPARSE,
} carvpath_out_of_range_action;


static char* _carvpath_error_strings[CARVPATH_ERR_MAXERRNO+2];

typedef enum {
  CARVPATH_FRAGTYPE_REGULAR,
  CARVPATH_FRAGTYPE_SPARSE,
} carvpath_fragtype;


typedef struct {
   sqlite3           *longtokendb;
   int               maxtokensize;
} carvpath_library;

typedef struct {
   carvpath_fragtype ftype;
   off_t             offset;
   off_t             size;
   char              *string_cache;
   void              *next;
} carvpath_fragment;

typedef struct {
   off_t             totalsize;
   size_t            fragcount;
   int               refcount;
   char              *path_cache;
   void              *parent;
   carvpath_fragment *fraglist;
   carvpath_library const  *library_state;
} carvpath_entity;

//This method should be called once by a single application to create a valid carvpath_library struct
//@uselongtokendb: File system access in operating systems are bound to a maximum size for tokens and a 
//                 maximum size for a whole path. For this reason, the CarvFs system, that is build on 
//                 LibCarvPath can not represent highly fragmented files as a basic CarvPath. 
//                 To overcome this problem, LibCarvPath provides the possibility to represent long
//                 CarvPaths as a unique digest code that is stored in a sqlite longtoken database.
//                 Setting the uselongtokendb to non zero will enable the use of this database.
//@compatibilitymode: If the longtokendb is used, on a non Win32 operating system, the maximum size of
//                 a CarvPath can be up to 1000 characters long. It may however be the case that in a 
//                 hybrid enviroment, Win32 systems will be accessing and using CarvFs over a samba share.
//                 In win32 the total path size (that can be up to 4k on most *NIX systems) is only 255
//                 for ALL of the path. In order to allow carvpaths to be valid when used from Win32 over
//                 a smb mount, on the Win32 platform, the maximum CarvPath token size is set to only 160
//                 characters. By default on non win32 platform 1000 characters will be used unless 
//                 compatibilitymode is given a non zero value here.
//On success a pointer to a new carvpath_library is returned. On failure NULL is returned and errno is set
//appropriately.
carvpath_library *carvpath_init(int uselongtokendb,int compatibilitymode);
//Before exiting, an application should call carvpath_finish.
void carvpath_finish(carvpath_library *lib);
//define a top entity that we can derive other entities from. For this we need
//a top path, and the size of the entity represented by this top path.
//@fullsize:       The size of the image that is used as top level entity.
//@topcarvpath:    The carvpath for the image that is used as top level entity.
//@lib:            The (one per application) initialized library object.
carvpath_entity *carvpath_top_entity(off_t fullsize,const char *topcarvpath, const carvpath_library *lib);
//This function is meant for growing archives. It allows a previously created top entity
//to be grown to a new size. The new size MUST be at least as big as the old size.
//On failure errno is set appropriately and if the @failure pointer attribute is a non NULL
//pointer, the value pointed to by @failure is set to a non null vallue. 
void carvpath_grow_top(carvpath_entity *top,off_t newsize,int *failure);
//parse a carvpath string and create a derived entity from it. The given path
//is assumed to be relative to the given topcarvpath of the top entity supplied. 
//Absolute paths are not currently supported.
//@top:            The top level image entity.
//@relpath:        The relative carvpath we want to parse. This path is relative to the topcarvpath 
//		   attribute that was used when creating the top level entity.
//On success a pointer to a new carvpath_entity is returned. On failure NULL is returned and errno is set
//appropriately.
carvpath_entity *carvpath_parse(carvpath_entity *top,const char *relpath,carvpath_out_of_range_action onoor);
//convert an entity into a a fully specified (absolute) carvpath.
//Please note that the resulting carvpath uses the shortest (flattened) notation possible
//for the given carvpath top, and if configured to do so, the longtokendb shorthand notation
//for carvpaths that exceed the maximum carvpath token size for the supported platforms.
//On success a pointer to a new character string is returned. 
//On failure NULL is returned and errno is set appropriately.
char *carvpath_get_as_path(carvpath_entity *ent);
//Get the size of the entity.
//On success the size is returned, this size may be zero. On failure errno is set, zero is returned 
//and if the @failure pointer attribute is non NULL, the value pointed to by the failure attribute is
//given a non zero vallue.
off_t carvpath_get_size(const carvpath_entity *ent,int *failure);
//Get the amounth of fragments (of its direct parent) that the entity consists of. 
//On success the amounth of fragments is returned, this number may be zero. On failure errno is set, 
//zero is returned and if the @failure pointer attribute is non NULL, the value pointed to by the failure 
//attribute is given a non zero vallue.
size_t carvpath_get_fragcount(const carvpath_entity *ent,int *failure);
//Get the size of the fragment indicated by index.
//On success the size of the designated fragment is returned, this number may be zero. 
//On failure errno is set, zero is returned and if the @failure pointer attribute is non NULL, 
//the value pointed to by the failure attribute is given a non zero vallue.
off_t carvpath_fragment_get_size(const carvpath_entity *ent,size_t index,int *failure);
//Get the offset into the parent entity where the fragment resides that is indicated by index.
//@index:	The index attribute designates an individual fragment within the entity.
//              Valid values for this attribute must be in the range [0 .. $fragcount-1]
//              where $fragcount is the value returned by carvpath_get_fragcount for this entity.
//On success the offset of the designated fragment (within its parent entity) is returned, 
//this offset may be zero.
//On failure errno is set, zero is returned and if the @failure pointer attribute is non NULL, 
//the value pointed to by the failure attribute is given a non zero vallue.
off_t carvpath_fragment_get_offset(const carvpath_entity *ent,size_t index,int *failure);
//Determine if a given fragment is sparse or regular.
//@index:       The index attribute designates an individual fragment within the entity.
//              Valid values for this attribute must be in the range [0 .. $fragcount-1]
//              where $fragcount is the value returned by carvpath_get_fragcount for this entity.
//On failure errno is set, zero is returned and if the @failure pointer attribute is non NULL, 
//the value pointed to by the failure attribute is given a non zero vallue.
int carvpath_fragment_is_sparse(const carvpath_entity *ent,size_t index,int *failure);
//Create a single fragment derived entity
//@offset:      The offset of the fragment within the parent entity.
//@size:        The size of the fragment.
//On success a pointer to a new carvpath_entity is returned. On failure NULL is returned
//and errno is set appropriately.
carvpath_entity *carvpath_derive(carvpath_entity *ent,off_t offset,off_t size,carvpath_out_of_range_action onoor);
//Create a new  sparse entity.
//@size         The size of the sparse entity.
//@anyent	The top entity or any other valid entity. The sparse entity will always excist 
//              directly under the top entity.
//On success a pointer to a new carvpath_entity is returned. On failure NULL is returned
//and errno is set appropriately.
carvpath_entity *carvpath_new_sparse_entity(off_t size,carvpath_entity *anyent);
//Add an other regular fragment to an existing carvpath entity.
//@offset:      The offset of the fragment within the parent entity.
//@size:        The size of the fragment.
//On failure, errno is set, and if @failure is a non NULL pointer, the integer pointed
//to will be set to a non zero vallue.
void carvpath_append(carvpath_entity *target,off_t offset,off_t size,int *failure,carvpath_out_of_range_action onoor);
//Add an other sparse fragment to an existing carvpath entity.
//@size         The size of the sparse entity.
//On failure, errno is set, and if @failure is a non NULL pointer, the integer pointed
//to will be set to a non zero vallue.
void carvpath_append_sparse(carvpath_entity *target,off_t size,int *failure);
//Free an entity (if not otherwise referenced) and any parent entity no longer
//referenced by removing this entity.
//On failure, errno is set, and if @failure is a non NULL pointer, the integer pointed
//to will be set to a non zero vallue.
void carvpath_free(carvpath_entity *ent,int *failure);
//This utility function returns an error number as a printable string.
char *carvpath_error_as_string(int failurenum);
//Helper function to get at the shortest level 1 version of the carvpath entity.
//If you want the shortest string representation, this method is not needed as 
//carvpath_get_as_path will already invoke this function for you.
carvpath_entity *carvpath_flatten(carvpath_entity *unflattened);
#ifdef __cplusplus
}
#endif
#endif


