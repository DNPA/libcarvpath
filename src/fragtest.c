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
   carvpath_entity *image=carvpath_top_entity(1000000000000,"/CarvFs",libcp);
   if (image == 0) {
      fprintf(stderr,"Problem creating top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *partition=carvpath_parse(image,"1457287168+266240_1504587776+4096_1511743488+4096_786477056+4096_560992256+4096_869621760+4096_858812416+1012",CARVPATH_OOR_FAIL);
   if (partition == 0) {
      fprintf(stderr,"Problem parsing relative path '%s', errno=%d : %s\n","1457287168+266240_1504587776+4096_1511743488+4096_786477056+4096_560992256+4096_869621760+4096_858812416+1012",errno,carvpath_error_as_string(errno));
      return -errno;
   }
   carvpath_entity *readchunk=carvpath_derive(partition,16384,32768,CARVPATH_OOR_TRUNCATE);
   if (readchunk == 0) {
      fprintf(stderr,"Problem deriving readchunk 2 from partition, errno=%d : %s\n",errno,carvpath_error_as_string(errno));
      return -errno;
   }
   if (carvpath_get_size(readchunk,0) != 32768) {
      fprintf(stderr,"readchunk size =%" PRId64 " while expected 32768 \n",carvpath_get_size(readchunk,0));
      return 27;
   }
   carvpath_entity *readchunk_fragments=carvpath_flatten(readchunk);
   if (readchunk_fragments == 0) {
      fprintf(stderr,"readchunk could not be flattened: errno=%d : %s\n",errno,carvpath_error_as_string(errno));
      return 28;
   }
   carvpath_free(readchunk_fragments,&failure);
   if (failure) {
        fprintf(stderr,"Problem deleting readchunk node, errno=%d\n",errno);
        return -errno;
   }
   carvpath_free(readchunk,&failure);
   if (failure) {
        fprintf(stderr,"Problem deleting readchunk node, errno=%d\n",errno);
        return -errno;
   }
   off_t fragcount=carvpath_get_fragcount(partition,&failure);
   if (failure) {
      fprintf(stderr,"Problem getting fragments from readchunk, errno=%d\n",errno);
      return -errno;
   }
   if (fragcount != 7) {
      fprintf(stderr,"Unexpected number of fragments for image errno=0, expected fragcount=7, found %" PRId64 "\n",fragcount);
      return 28;
   }
   carvpath_free(partition,&failure);
   if (failure) {
        fprintf(stderr,"Problem deleting partition node, errno=%d\n",errno);
        return -errno;
   }
   carvpath_finish(libcp);
   fprintf(stderr,"OK\n");
   return 0;
}

