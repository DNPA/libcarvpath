#ifndef _CARVPARH_ENTITY_H
#define _CARVPARH_ENTITY_H
//This function decrements the reference count for the entity object. If the reference count
//reaches zero, the carvpath_entity is destroyed.
int _carvpath_entity_decrefcount(carvpath_entity *self);
//This function destroys a carvpath_entity and its members.
int _carvpath_entity_destroy(carvpath_entity *self);
//This function increments the reference count for a carvpath_entity.
int _carvpath_entity_increfcount(carvpath_entity *self);
//This function updates the patch_cache for a new or changed carvpath_entity.
int _carvpath_entity_update_path_cache(carvpath_entity *self);
//This function is a bit of a hack, carvpath_get_as_path creates and uses a temporary (flattend)
//entity. carvpath_get_as_path can not return the path_cache of the flattened entity as it will
//need to delete the entity before returning to avoid leaking entities. To overcome the problem raised
//by this, this function migrates the path_cache of the flat entity to the original one, so that
//the flat entity can be safely deleted, and carvpath_get_as_path can return a string that still
//excists.
void _carvpath_entity_migrate_path_cache(carvpath_entity *self,carvpath_entity *flattened_self);
//This function adds a single fragment to an entity.
int _carvpath_entity_add_fragment(carvpath_entity *self,carvpath_fragment *frag);
//This function adds a new sparse fragment to the end of an entity.
int _carvpath_entity_add_sparse_fragment(carvpath_entity *self,off_t size);
//This function adds a new regular fragment to the end of an entity.
int _carvpath_entity_add_regular_fragment(carvpath_entity *self,off_t offset,off_t size,carvpath_out_of_range_action onoor);
//This function creates a new carvpath_entity.
carvpath_entity *_carvpath_entity_new(off_t entsize,carvpath_entity *parent);
//This function creates a new carvpath top entity.
carvpath_entity *_carvpath_entity_new_top(off_t entsize,const carvpath_library *lib);
//This function creates a new entity from a string.
carvpath_entity *_carvpath_entity_new_from_string(carvpath_entity *parententity,char *toplayerstring,carvpath_out_of_range_action onoor);
//This function sets the path_cache for the top node.
int _carvpath_entity_set_path_cache(carvpath_entity *self,const char *mountpoint);
//This function returns a fragment at a given index.
carvpath_fragment *_carvpath_entity_get_fragment(const carvpath_entity *self,size_t index);
//Get the entity as a path. Determining the paths is done eagerly,
//already, so this is fairly simple.
char * _carvpath_entity_get_as_path(const carvpath_entity *self);
//This function returns a new reference to a given entity.
carvpath_entity *_carvpath_entity_new_reference(carvpath_entity *self);
//This function will, if needed fixup a string representation of an entity with a longtoken version
//of this representation.
int _carvpath_entity_longtoken_fixup(carvpath_entity *self);
//This function copies a single fragment from a source entity to a new entity with one less layer
//of indirection than the original entity.
int _carvpath_entity_flatten_single_frag_copy_one_layer_up(carvpath_entity *self,carvpath_fragment *pflist,
		carvpath_fragment *origfrag);
//This function copies a fragment list, removing one layer of indirection from the parent chain while doing so.
int _carvpath_entity_fragment_flatten_frags_copy_one_layer_up(carvpath_entity *self,carvpath_entity *origent);
//This function updates the size of a (top level) entity.
void _carvpath_entity_set_size(carvpath_entity *self,off_t newsize);
#endif
