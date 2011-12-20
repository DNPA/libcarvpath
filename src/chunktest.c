#include <stdio.h>
#include <errno.h>
#include "libcarvpath.h"


int main(int argc, char *argv[]) {
   int failure=0;
   carvpath_library *libcp=carvpath_init(1,0);
   if (libcp == 0) {
      fprintf(stderr,"Problem initializing carvpath library, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *image=carvpath_top_entity(1234567890,"/CarvFs",libcp);
   if (image == 0) {
      fprintf(stderr,"Problem creating top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *partition=carvpath_parse(image,"0+123456",CARVPATH_OOR_FAIL);
   if (partition == 0) {
      fprintf(stderr,"Problem parsing relative path '%s', errno=%d\n","0+123456",errno);
      return -errno;
   }
   off_t fragcount=carvpath_get_fragcount(partition,&failure);
   if (failure) {
      fprintf(stderr,"Problem getting fragments from readchunk, errno=%d\n",errno);
      return -errno;
   }
   if (fragcount != 1) {
      fprintf(stderr,"Unexpected number of fragments for partition errno=0, expected fragcount=1, found %" PRId64 "\n",fragcount);
      return 28;
   }
   carvpath_entity *readchunk=carvpath_derive(partition,0,16384,CARVPATH_OOR_FAIL);
   if (readchunk == 0) {
      fprintf(stderr,"Problem deriving readchunk from partition, errno=%d\n",errno);
      return -errno;
   }
   fragcount=carvpath_get_fragcount(readchunk,&failure);
   if (failure) {
       fprintf(stderr,"Problem getting fragments from readchunk, errno=%d\n",errno);
       return -errno;
   }
   if (fragcount != 1) {
       fprintf(stderr,"Unexpected number of fragments for readchunk errno=0, expected fragcount=1, found %" PRId64 "\n",fragcount);
       return 29;
   }
   carvpath_entity *readchunk_fragments=carvpath_flatten(readchunk);
   if (readchunk_fragments == 0) {
      fprintf(stderr,"Problem flattening readchunk, errno=%d\n",errno);
      return -errno;
   }  

   fragcount=carvpath_get_fragcount(readchunk_fragments,&failure);
   if (failure) {
        fprintf(stderr,"Problem getting fragments from readchunk, errno=%d\n",errno);
        return -errno;
   }
   if (fragcount != 1) {
        fprintf(stderr,"Unexpected number of fragments for readchunk_fragments errno=0, expected fragcount=1, found %" PRId64 "\n",fragcount);
        return 30; 
   }

   carvpath_free(readchunk_fragments,&failure);
   if (failure) {
         fprintf(stderr,"Problem deleting readchunk_fragments node, errno=%d\n",errno);
         return -errno;
   }
   carvpath_free(readchunk,&failure);
   if (failure) {
        fprintf(stderr,"Problem deleting readchunk node, errno=%d\n",errno);
        return -errno;
   }
   carvpath_free(partition,&failure);
   if (failure) {
        fprintf(stderr,"Problem deleting partition node, errno=%d\n",errno);
        return -errno;
   }
   carvpath_free(image,&failure);
   if (failure) {
      fprintf(stderr,"Problem deleting top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_finish(libcp);
   fprintf(stderr,"OK\n");
   return 0;
}
