#include <stdio.h>
#include <errno.h>
#define _LIBCARVPATH_DEBUG_ALLOC
#include "libcarvpath.h"

void basetest(carvpath_entity *entity) {
   errno=0;
   char *entitystr=carvpath_get_as_path(entity);
   fprintf(stderr,"BASETEST : '%s'\n",entitystr);
   if (entitystr == 0) {
       fprintf(stderr,"Problem getting string from entity, errno=%d\n",errno);
       return;
   }
   //sprintf(stderr,"entity string = '%s'\n",entitystr);
   //fprintf(stderr,"basetest 3\n");
   int failure=0;
   carvpath_get_size(entity,&failure);
   if (failure) {
       fprintf(stderr,"FAILURE :\n");
       fprintf(stderr,"Problem getting size from entity, errno=%i\n",errno);
       return;
   }
   size_t fragcount=carvpath_get_fragcount(entity,&failure);
   if (failure) {
       fprintf(stderr,"Problem getting fragment count from entity, errno=%i\n",errno);
       return;
   }
   size_t fragindex;
   for (fragindex=0;fragindex < fragcount;fragindex++) {
       carvpath_fragment_get_size(entity,fragindex,&failure);
       if (failure) {
          fprintf(stderr,"Problem getting fragment size for entity fragment, errno=%i\n",errno);
	  return;
       }
       carvpath_fragment_get_offset(entity,fragindex,&failure);
       if (failure) {
          fprintf(stderr,"Problem getting fragment offset for entity fragment, errno=%i\n",errno);
          return;
       }
       if (carvpath_fragment_is_sparse(entity,fragindex,&failure)) {
          fprintf(stderr,"Fragment is sparse.\n");
       } else {
	  if (failure) {
             fprintf(stderr,"Problem getting fragment type for entity fragment, errno=%i\n",errno);
	     return;
	  }
          fprintf(stderr,"Fragment is regular.\n");
       }

   }
   carvpath_entity *derived=carvpath_derive(entity,123,321,CARVPATH_OOR_FAIL);
   if (derived == 0) {
      fprintf(stderr,"Problem deriving small derived entity, errno=%i\n",errno);
      return;
   }
   carvpath_append(derived,444,111,&failure,CARVPATH_OOR_FAIL);
   if (failure) {
      fprintf(stderr,"Problem appending to small derived entity, errno=%d\n",errno);
      return;
   }
   carvpath_append_sparse(derived,445,&failure);
   if (failure) {
      fprintf(stderr,"Problem appending sparse to small derived entity, errno=%d\n",errno);
      return;
   }

   return;
}

int main(int argc, char *argv[]) {
  int repeat;
  for (repeat=0;repeat < argc;repeat++) {
   int failure=0;
   carvpath_library *libcp=carvpath_init(1,1);
   if (libcp == 0) {
      fprintf(stderr,"Problem initializing carvpath library, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *image=carvpath_top_entity(1234567890,"/mnt/demo/CarvFs",libcp);
   if (image == 0) {
      fprintf(stderr,"Problem creating top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_grow_top(image,1234567891,&failure);
   if (failure) {
      fprintf(stderr,"Problem deleting top node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *partition=carvpath_parse(image,"512+4096_S8192_987654+4096",CARVPATH_OOR_FAIL);
   if (partition == 0) {
      fprintf(stderr,"Problem parsing relative path '%s', errno=%d\n","512+4096_987654+4096",errno);
      return -errno;
   }
   carvpath_entity *file=carvpath_parse(image,"512+4096_S8192_987654+4096/2048+12000",CARVPATH_OOR_FAIL);
   if (file == 0) {
      fprintf(stderr,"Problem parsing relative path '%s', errno=%d\n","512+4096_S8192_987654+4096/2048+12000",errno);
      return -errno;
   }
   carvpath_entity *sparsepart=carvpath_parse(image,"512+4096_S8192_987654+4096/2048+12000/3000+3210",CARVPATH_OOR_FAIL);
   if (sparsepart == 0) {
      fprintf(stderr,"Problem parsing relative path '%s', errno=%d\n","512+4096_S8192_987654+4096/2048+12000/3000+3210",errno);
      return -errno;
   }
   carvpath_entity *bigsparse=carvpath_new_sparse_entity(99999999999999,image);
   if (bigsparse == 0) {
      fprintf(stderr,"Problem creating big sparse entity.errno=%d\n",errno);
      return -errno;
   }
   carvpath_entity *invalident=carvpath_parse(image,"512+4096_Dikkerdjedapzatopdetrap",CARVPATH_OOR_FAIL);
   if (invalident==0) {
       fprintf(stderr,"Problem creating entity from invalid carvpath : '%s'\n",carvpath_error_as_string(errno)); 
   } else {
      fprintf(stderr,"Hmm? No error from creating entity from invalid carvpath\n");
   }
   fprintf(stderr,"BASETEST: Processing image /mnt/demo/CarvFs with size 1234567890 -> 1234567891\n");
   basetest(image);
   fprintf(stderr,"BASETEST: Processing partition 512+4096_S8192_987654+4096 \n");
   basetest(partition);
   fprintf(stderr,"BASETEST: Processing file 512+4096_S8192_987654+4096/2048+12000\n");
   basetest(file);
   fprintf(stderr,"BASETEST: Processing sparse file part 512+4096_S8192_987654+4096/2048+12000/3000+3210\n");
   basetest(sparsepart);
   fprintf(stderr,"Processing big sparse entity\n");
   basetest(bigsparse);
   fprintf(stderr,"done with basic processing\n");
   carvpath_free(sparsepart,&failure);
   if (failure) {
      fprintf(stderr,"Problem deleting sparsepart node, errno=%d\n",errno);
      return -errno;
   }
   carvpath_free(file,&failure);
   if (failure) {
      fprintf(stderr,"Problem deleting file node, errno=%d\n",errno);
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
   fprintf(stderr,"Carvpath entities left : %i\n",_carvpath_entity_count);
   fprintf(stderr,"Carvpath fragments left : %i\n",_carvpath_fragment_count);
   fprintf(stderr,"Carvpath strings left : %i\n",_carvpath_string_count);
   carvpath_finish(libcp);
 }
 return 0;
}
