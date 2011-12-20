#include <libcarvpath.h>
#include <stdio.h>

void testflatten(carvpath_entity *top,char *inpath,char *expected) {
   carvpath_entity *subpath=carvpath_parse(top,inpath);
   if (subpath == 0) {
      fprintf(stderr,"Problem parsing input carvpath:%s\n",inpath);
      return;
   }
   carvpath_entity *flatpath=carvpath_flaten(subpath);
   if (flatpath == 0) {
      fprintf(stderr,"Problem flattening carvpath:%s\n",inpath);
      carvpath_free(subpath);
      return;
   }
   if (strcmp(carvpath_get_as_path(flatpath),expected) == 0) {
     fprintf(stdout,"OK : %s flattens to %s\n",inpath,carvpath_get_as_path(flatpath));
   } else {
      fprintf(stdout,"FAIL : %s flattens to %s while expected is %s\n",inpath,carvpath_get_as_path(flatpath),expected);
   }
   carvpath_free(subpath);
   carvpath_free(flatpath);
}

int main(int argc, char *argv[])
{
    carvpath_entity *top=carvpath_top_entity(1000000000000,"/CarvFS",1);
    if (top==0) {
       fprintf(stderr,"ERROR: Unable to create top node\n");
       return 1;
    }
    testflatten(top,"0+20000_40000+20000/10000+20000/5000+10000/2500+5000/1250+2500/625+1250","/CarvFS/19375+625_40000+625");
    testflatten(top,"0+20000_20000+20000/0+40000","/CarvFS/0+40000");
    testflatten(top,"0+20000_20000+20000","/CarvFS/0+40000");
    testflatten(top,"S100_S200","/CarvFS/S300");
    testflatten(top,"S1_S1","/CarvFS/S2");
    testflatten(top,"0+5","/CarvFS/0+5");
    testflatten(top,"0+0","/CarvFS/0+0");
    testflatten(top,"20000+0","/CarvFS/0+0");
    testflatten(top,"S0","/CarvFS/0+0");
    testflatten(top,"20000+0_89765+0","/CarvFS/0+0");
    testflatten(top,"1000+0_2000+0/0+0","/CarvFS/0+0");
    testflatten(top,"0+0/0+0","/CarvFS/0+0");
    testflatten(top,"0+100_101+100_202+100_303+100_404+100_505+100_606+100_707+100_808+100_909+100_1010+100_1111+100_1212+100_1313+100_1414+100_1515+100_1616+100_1717+100_1818+100_1919+100_2020+100_2121+100_2222+100_2323+100_2424+100","/CarvFS/Dfa357c3b750fcb2244b113d4ac904f757232ae43");
    return 0;
}


