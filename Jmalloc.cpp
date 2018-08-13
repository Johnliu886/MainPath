//
// Jmalloc.cpp
//

//
// Revision History:
// 2013/01/26               : the codes are added 2013/01/26 in order to debug a nasty memory problem
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"

extern FILE *logstream;

#ifdef MDEBUG
extern FILE *mdebugstream;
#endif MDEBUG

#define MAX_N_ALLOCATION 100000
#define MAX_FUNCINFO 80
#define TYPE_MALLOC 1
#define TYPE_FREED 2
struct MEMORYINFO
{
	int status;	// 1 = malloc, 2 = freed
	void *addr;	// address
	int size;	// original size
	int nsize;	// actual allocated size
	wchar_t funcinfo[MAX_FUNCINFO];
};

int mindx;	// current total of malloc() and free()
struct MEMORYINFO *memoryinfo;

//
// assign a big chunk of memory to store memory allocation information
//
int Jmemory()
{
	int size;

	memoryinfo = (struct MEMORYINFO *)malloc(MAX_N_ALLOCATION * sizeof(struct MEMORYINFO));
	mindx = 0;

	return 0;
}


//
// Jmalloc() calls standard malloc(), and collects memory allocation information at the same time
//
#define SIZE_EXTRA 10
void *Jmalloc(size_t size, wchar_t *pname)
{
	void *addr;

#ifdef MDEBUG
	int i, nsize;
	char *ptr;
	nsize = size + SIZE_EXTRA;
	addr = (void *)malloc(nsize);
	ptr = (char *)addr;
	for (i = 0; i < SIZE_EXTRA; i++) 
		ptr[size+i] = 'X';
	if (mindx < MAX_N_ALLOCATION)
	{
		memoryinfo[mindx].status = TYPE_MALLOC;
		memoryinfo[mindx].addr = addr;
		memoryinfo[mindx].size = size;
		memoryinfo[mindx].nsize = nsize;
		wcscpy(memoryinfo[mindx].funcinfo, pname);
		mindx++;
	}
#else
	addr = (void *)malloc(size);
	//fwprintf(logstream, L"### malloc %s, addr=%0x, size=%I64d ###\n", pname, addr, size); fflush(logstream);
#endif MDEBUG

	return addr;
}

//
// Jfree() calls standard free(), but collects memory free information at the same time
//
int Jfree(void *addr, wchar_t *pname)
{
	int ret;

	ret = 0;

	//fwprintf(logstream, L"### free   %s, addr=%0x ###\n", pname, addr); fflush(logstream);
	free(addr);
#ifdef MDEBUG
	int i;
	for (i = 0; i < mindx; i++)
	{
		if (memoryinfo[i].addr == addr)
			memoryinfo[i].status = TYPE_FREED;
	}
#endif MDEBUG

	return ret;
}

#ifdef MDEBUG
//
// check the heap of allocated memory
//
int Jheap_check()
{
	int i, k;
	int size;
	char *ptr;

	for (i = 0; i < mindx; i++)
	{
		if (memoryinfo[i].status == TYPE_MALLOC)
		{
			//fwprintf(mdebugstream, L"%d\t%xd\t(%s)\t%d bytes of memory allocated.\n", 
			//	i, memoryinfo[i].addr,  memoryinfo[i].funcinfo, memoryinfo[i].size); 
			ptr = (char *)memoryinfo[i].addr;
			size = memoryinfo[i].size;
			ptr = ptr + size;
			for (k = 0; k < SIZE_EXTRA; k++) 
			{
				if (ptr[k] != 'X')
					fwprintf(mdebugstream, L"WARNING!!!\n");
			}
			//fflush(mdebugstream);
		}
		else if (memoryinfo[i].status == TYPE_FREED)
		{
			//fwprintf(mdebugstream, L"%d\t%xd\t(%s)\t%d bytes of memory allocated and freed.\n", 
			//	i, memoryinfo[i].addr, memoryinfo[i].funcinfo, memoryinfo[i].size); 
			//fflush(mdebugstream);
		}
	}

	return 0;
}
#endif MDEBUG
