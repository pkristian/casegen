#ifndef CASEGEN_MEM_H
#define CASEGEN_MEM_H

/* Allocation failure is the one error every module can hit and none can do anything
   about, so it lives on its own rather than in whichever module happened to need it
   first — which used to be stringlist, for no better reason than being compiled first. */

_Noreturn void outOfMemory(void);

#endif /* CASEGEN_MEM_H */
