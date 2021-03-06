#pragma once
#include "asmfunc_def.h"
#include "memory_s.h"

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

unsigned int memtest(unsigned int start, unsigned int end);
// unsigned int memtest_sub(unsigned int start, unsigned end);

void memory_manage_init(MEM_MAN* man);
unsigned int memory_manage_total(MEM_MAN* man);
unsigned int memory_manage_alloc(MEM_MAN* man, unsigned int size);
int memory_manage_free(MEM_MAN* man, unsigned int addr, unsigned int size);
unsigned int memory_manage_alloc_4k(MEM_MAN* man, unsigned int size);
int memory_manage_free_4k(MEM_MAN* man, unsigned int addr, unsigned int size);
