#ifndef _CARVPATH_UTILITY_H
#define _CARVPATH_UTILITY_H
//#####################################################################################
//This function returns the index of the first occurence of a seperator character.
int _carvpath_utility_get_first_seperator_index(const char *relpath,char seperator);
//This function returns a newly allocated string with the part
//of the input string before the first seperator character.
char * _carvpath_utility_get_toplayer_string(const char *relpath,char seperator);
//This function returns a newly allocated string with the part
////of the input string after the first seperator character.
char * _carvpath_utility_get_remaininglayers_string(const char *relpath,char seperator);
//This function concattenates the string representation of a layer of
////carvpath indirection to the string representation of the parent layer.
char * _carvpath_util_new_pathstring(char *parentstr,char *fragmentsstr);
#endif
