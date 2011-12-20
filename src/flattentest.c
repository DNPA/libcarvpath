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
   carvpath_entity *image=carvpath_top_entity(1000000,"/CarvFs",libcp);
   if (image == 0) {
      fprintf(stderr,"Problem creating top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *derived=carvpath_derive(image,100,100,CARVPATH_OOR_FAIL);
   if (derived == 0) {
       fprintf(stderr,"Problem deriving evidence, errno= %d\n",errno);
       return -errno;
   }
   carvpath_append(derived,200,100,&failure,CARVPATH_OOR_FAIL);
   if (failure) {
      fprintf(stderr,"Problem appending, errno=%d\n",errno);
      return -errno;
   }
   carvpath_append(derived,300,100,&failure,CARVPATH_OOR_FAIL);
   if (failure) {
      fprintf(stderr,"Problem appending, errno=%d\n",errno);
      return -errno;
   }
   carvpath_append(derived,500,100,&failure,CARVPATH_OOR_FAIL); 
   if (failure) {
      fprintf(stderr,"Problem appending, errno=%d\n",errno);
      return -errno;
   }
   carvpath_append(derived,600,100,&failure,CARVPATH_OOR_FAIL);  
   if (failure) {
      fprintf(stderr,"Problem appending, errno=%d\n",errno);
      return -errno;
   }
   size_t frags = carvpath_get_fragcount(derived,&failure);
   if (failure) {
      fprintf(stderr,"Problem getting fragcount, errno=%d\n",errno);
      return -errno;
   } 
   if (frags != 2) {
      fprintf(stderr,"Problem, fragcount isnt the right vallue %d  != 2\n",frags);
      return -1;
   }
   carvpath_entity *flat=carvpath_flatten(derived);
   if (flat == 0) {
      fprintf(stderr,"Problem flattening entity: errno=%d\n",frags);
      return -errno;
   }
   frags = carvpath_get_fragcount(flat,&failure);
   if (failure) {
      fprintf(stderr,"Problem getting fragcount, errno=%d\n",errno);
      return -errno;
   }
   if (frags != 2) {
      fprintf(stderr,"Problem, flattened fragcount isnt the right vallue %d  != 2, carvpath=%s\n",frags,carvpath_get_as_path(derived));
      return -1;
   }
   carvpath_free(flat,&failure);
   if (failure) {
      fprintf(stderr,"Problem deleting flattened node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_free(derived,&failure);
   if (failure) {
      fprintf(stderr,"Problem deleting derived node, errno=%d\n",errno);
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
