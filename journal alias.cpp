//
// journal alias.cpp
//
// this file incdlues functions to handle the journal alias, which is used in the situation that a journal changes its name.
//
//

// Revision History:
// 
// 2015/03/31 basic function works
// 

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include "network.h"

int parse_jalias(wchar_t *, wchar_t *, wchar_t *);
int compare_journalalias(const void *, const void *);
int journalalias_search(struct JOURNAL_ALIAS *, int, wchar_t *);

int njournalalias;	// number of items in the journal alias table
struct JOURNAL_ALIAS *journalalias;

extern FILE *logstream;

//
// read the journal alias file
//
int prepare_journal_alias_list()
{
	int i;
	FILE *jstream;
	wchar_t tmps[LBUF_SIZE], *sp;

	// open the "Journal alias.txt" file, ignore if it does not exist
	if (_wfopen_s(&jstream, L"Journal alias.txt", L"r") != 0)
	{
		fwprintf(logstream, L"\nWARNING: \"Journal alias.txt\" is not provided.\n");
		njournalalias = 0;
	}
	else
	{
		fwprintf(logstream, L"\nFound and read the file: \"Journal alias.txt\".\n");
		// count the number of items in the alias table
		njournalalias = 0;
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, jstream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			njournalalias++;
		}
		// allocate memory for the alias table
		journalalias = (struct JOURNAL_ALIAS *)malloc(njournalalias * sizeof(struct JOURNAL_ALIAS));	
		if (journalalias == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(jstream);	// point back to the begining of the assignee alias file
		njournalalias = 0;	
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, jstream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			parse_jalias(tmps, journalalias[njournalalias].name, journalalias[njournalalias].alias);
			njournalalias++;		
		}
		for (i = 0; i < njournalalias; i++)	// change all text to lower case
		{
			sp = journalalias[i].name; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
			sp = journalalias[i].alias; while (*sp != '\0') { *sp = towlower(*sp); sp++; } 
		}
		// sort the table by the original name
		qsort((void *)journalalias, (size_t)njournalalias, sizeof(struct JOURNAL_ALIAS), compare_journalalias);
		fwprintf(logstream, L"Journal alias table\n");
		for (i = 0; i < njournalalias; i++)
			fwprintf(logstream, L"[%s]=>[%s]\n", journalalias[i].name, journalalias[i].alias);

	}
	fclose(jstream);

	return 0;
}

//
// parse the journal alias string
// journal alias and its authentic title are delimited by a tab
//
int parse_jalias(wchar_t *istr, wchar_t *oname, wchar_t *alias)
{
	int i;
	int state;
	wchar_t ch, *sp, *op, *ap;

	sp = istr;

	// start parsing
	op = oname;	ap = alias;
	state = 1;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		switch (state)
		{
		case 1:
			if (ch != '\t')
				*op++ = *sp++;
			else
			{
				*op = '\0'; sp++;
				state = 2;
			}
			break;
		case 2:
			*ap++ = *sp++;
			break;
		default:
			break;
		}
	}
	*ap = '\0';

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_journalalias(const void *n1, const void *n2)
{
	struct JOURNAL_ALIAS *t1, *t2;
	
	t1 = (struct JOURNAL_ALIAS *)n1;
	t2 = (struct JOURNAL_ALIAS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of an author alias in an "JOURNAL_ALIAS" array
//
int journalalias_search(struct JOURNAL_ALIAS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (num == 0) return -1;
	if (wcscmp(str, d[0].name) < 0)	return -1;

	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].name) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].name) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].name) > 0)
			low = cur;
		else
			high = cur;
	}
}
