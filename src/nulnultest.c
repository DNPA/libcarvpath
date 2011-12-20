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
   carvpath_entity *image=carvpath_top_entity(10000,"/CarvFs",libcp);
   if (image == 0) {
      fprintf(stderr,"Problem creating top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *partition=carvpath_parse(image,"0+0",CARVPATH_OOR_TRUNCATE);
   if (partition == 0) {
      fprintf(stderr,"Problem parsing nulnul relative path '%s', errno=%d : %s\n","0+0",errno,carvpath_error_as_string(errno));
      return -errno;
   }
   if (carvpath_get_size(partition,0) != 0) {
       fprintf(stderr,"partition size =%" PRId64 " while expected size is 0\n",carvpath_get_size(partition,0));
       return 26;
   }
   off_t fragcount=carvpath_get_fragcount(partition,&failure);
   if (failure) {
      fprintf(stderr,"Problem getting fragments from readchunk, errno=%d\n",errno);
      return -errno;
   }
   if (fragcount != 0) {
      fprintf(stderr,"Unexpected number of fragments for image errno=0, expected fragcount=0, found %" PRId64 "\n",fragcount);
      return 28;
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
