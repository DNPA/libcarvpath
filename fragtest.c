#include <stdio.h>
#include "libcarvpath.h"
#ifdef _WIN32
#define MP_STR "C:\\\\dummymount"
#define ENT1_STR "0+256_512+256\\128:256"
#define ENT2_STR "0+256_256+256\\128:256"
#else
#define MP_STR "/dummymount"
#define ENT1_STR "0+100000000000/0+50000000"
#define ENT2_STR "0+100000000000/0+50000000/D011af7373292cf9c11b10018d49e693be988cef7/1024+4096"
#endif
 
int main(int argc, char *argv[]) {
 carvpath_entity *top=carvpath_top_entity(200000000000,MP_STR,1);
 if (top == 0) {
     fprintf(stderr,"problem creating top entity\n");
     return 1;
 }
 fprintf(stderr,"created top\n");
 _carvpath_debug_dump(top,1,"top");
 carvpath_entity *ent1=carvpath_parse(top,ENT1_STR);
 if (ent1 == 0) {
     fprintf(stderr,"problem parsing %s\n",ENT1_STR);
     return 0;
 }
 
 fprintf(stderr,"parsed \"%s\" and created ent1\n",ENT1_STR);
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 carvpath_entity *ent2=carvpath_parse(top,ENT2_STR);
 if (ent2 == 0) {
     fprintf(stderr,"problem parsing %s\n",ENT2_STR);
     return 1;
 }
 fprintf(stderr,"parsed \"%s\" and created ent2\n",ENT2_STR);
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(ent2,1,"ent2");
 carvpath_free(top); 
 fprintf(stderr,"released top entity\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(ent2,1,"ent2");
 carvpath_entity *ent1flat=carvpath_flaten(ent1);
 if (ent1flat == 0) {
   fprintf(stderr,"problem flatening entity 1\n");
   return 1;
 }
 fprintf(stderr,"flattened entity ent1\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(ent2,1,"ent2");
 _carvpath_debug_dump(ent1flat,1,"ent1flat");
 fprintf(stdout,"ent1    =%s\n",carvpath_get_as_path(ent1));
 fprintf(stdout,"ent1flat=%s\n",carvpath_get_as_path(ent1flat));
 carvpath_free(ent1flat);
 fprintf(stderr,"released ent1flat entity\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(ent2,1,"ent2");
 carvpath_entity *ent2flat=carvpath_flaten(ent2);
 if (ent2flat == 0) {
   fprintf(stderr,"problem flatening entity 2\n");
   return 1;
 }
 fprintf(stderr,"flattened entity ent2\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(ent2,1,"ent2");
 _carvpath_debug_dump(ent2flat,1,"ent2flat");
 fprintf(stdout,"ent2    =%s\n",carvpath_get_as_path(ent2));
 carvpath_free(ent2); 
 fprintf(stderr,"released ent2 entity\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(ent2flat,1,"ent2flat");
 fprintf(stdout,"ent2flat=%s\n",carvpath_get_as_path(ent2flat));
 carvpath_free(ent2flat);
 fprintf(stderr,"released ent2flat entity\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 carvpath_entity *chunks[3];
 chunks[0]=carvpath_derive(ent1,0,64);
 fprintf(stderr,"derived first chunk\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunks[0],1,"chunks[0]");
 chunks[1]=carvpath_derive(ent1,192,128);
 fprintf(stderr,"derived second chunk\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunks[0],1,"chunks[0]");
 _carvpath_debug_dump(chunks[1],1,"chunks[1]");
 chunks[2]=carvpath_derive(ent1,384,64);
 fprintf(stderr,"derived third chunk\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunks[0],1,"chunks[0]");
 _carvpath_debug_dump(chunks[1],1,"chunks[1]");
 _carvpath_debug_dump(chunks[2],1,"chunks[2]");
 size_t index;
 for (index=0;index<3;index++) {
    if (chunks[index] == 0) 
      fprintf(stderr,"problem with deriving chunk %zd , probably out of range\n",index);
 }
 fprintf(stdout,"chunk 0 : %s\n", carvpath_get_as_path(chunks[0]));
 fprintf(stdout,"chunk 1 : %s\n", carvpath_get_as_path(chunks[1]));
 fprintf(stdout,"chunk 2 : %s\n", carvpath_get_as_path(chunks[2]));
 carvpath_entity *ent3=carvpath_derive(ent1,0,1000000);
 int x;
 for (x=1;x<20;x++) {
   carvpath_append(ent3,ent1,x*2000000,1000000);
 }
 fprintf(stdout,"ent3    =%s\n",carvpath_get_as_path(ent3));
 _carvpath_debug_dump(ent3,1,"ent3");
 carvpath_free(ent1);
 fprintf(stderr,"released ent1 entity\n");
 carvpath_entity *chunkset=carvpath_sequence(2,chunks);
 fprintf(stderr,"created sequence entity from chunks\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunks[0],1,"chunks[0]");
 _carvpath_debug_dump(chunks[1],1,"chunks[1]");
 _carvpath_debug_dump(chunks[2],1,"chunks[2]");
 _carvpath_debug_dump(chunkset,1,"chunkset");
 if (chunkset == 0) {
   fprintf(stderr,"problem with sequence\n");
   return 1;
 }
 carvpath_free(chunks[0]);
 fprintf(stderr,"released first chunk\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunks[1],1,"chunks[1]");
 _carvpath_debug_dump(chunks[2],1,"chunks[2]");
 _carvpath_debug_dump(chunkset,1,"chunkset");
 carvpath_free(chunks[1]);
 fprintf(stderr,"released second chunk\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunks[2],1,"chunks[2]");
 _carvpath_debug_dump(chunkset,1,"chunkset");
 carvpath_free(chunks[2]);
 fprintf(stderr,"released third chunk\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunkset,1,"chunkset");
 carvpath_entity *chunksetflat=carvpath_flaten(chunkset);
 fprintf(stderr,"flattened sequence\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunkset,1,"chunkset");
 _carvpath_debug_dump(chunksetflat,1,"chunksetflat");
 if (chunksetflat == 0) {
   fprintf(stderr,"problem with flatening of chunkset\n");
   return 1;
 }
 fprintf(stdout,"chunkset    =%s\n",carvpath_get_as_path(chunkset));
 fprintf(stdout,"chunksetflat=%s\n",carvpath_get_as_path(chunksetflat));
 carvpath_free(chunksetflat);
 fprintf(stderr,"released flattened sequence\n");
 _carvpath_debug_dump(top,1,"top");
 _carvpath_debug_dump(ent1,1,"ent1");
 _carvpath_debug_dump(chunkset,1,"chunkset");
 carvpath_free(chunkset);
 fprintf(stderr,"released sequence\n");
 return 0;
}
