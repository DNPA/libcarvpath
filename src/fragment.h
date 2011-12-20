#ifndef _CARVPATH_FRAGMENT_H
#define _CARVPATH_FRAGMENT_H
//This function will fill the string representation stored in fragments if they are
//set to 0. Each fragment should hold the representation of the full fragment list
//upto itself. This means that the true reperentation of the fragment list is stored 
//with the last fragment of the fragment list.
int _carvpath_fragment_update_strings_if_needed(carvpath_fragment *self,const char *prefix);
//This function returns the string representation of a fragment list.
char *_carvpath_fragment_get_as_string(carvpath_fragment *self);
//This function creates a new fragment struct.
carvpath_fragment *_carvpath_fragment_new(carvpath_fragtype ftype,off_t offset, off_t size);
//This function returns the last fragment of a fragment list.
carvpath_fragment *_carvpath_fragment_find_last(carvpath_fragment *self);
//This function destroy (part of) a fragment list.
void _carvpath_fragment_destroy(carvpath_fragment *self);
//This function will look if a new fragment can be merged with this fragment.
//If merging is possible, the new fragment is merged with the current one and
//the new fragment is destroyed. If merging is not possible, 0 is returned and the
//new fragment is left untouched.
int _carvpath_fragment_merge(carvpath_fragment *self,carvpath_fragment *newfrag);
//This function determines if a migratable fragment overlaps with a parent fragment. If it does
//it will return a new parent fragment for usage in a new flattened entity.
carvpath_fragment *_carvpath_fragment_overlap(carvpath_fragment *self,off_t loffset,carvpath_fragment *lfrag);
//This function will create a new fragment struct from a string representation.
carvpath_fragment *_carvpath_fragment_new_from_string(const char *fragmentstring);
#endif


