//
// author alias.cpp
//
// this file incdlues functions to handle author alias, keyword alias, etc.
//


//
// Revision History:
// 2016/01/25 Modification  : changed the file opening to Unicode mode (ccs=UTF-8)
// 2016/02/01 Modification  : added codes to check if there is byte-order-mark, this is necessary after the modification of openning file in UTF-8 type
// 2017/06/08 Fixed problem : added codes to ignore a line in the author alis file which begins with a space or a tab
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include "network.h"

int parse_aalias(wchar_t *, wchar_t *, wchar_t *);
int compare_authoralias(const void *, const void *);
int compare_keywordalias(const void *, const void *);
int compare_departmentalias(const void *, const void *);
int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
int keywordalias_search(struct KEYWORD_ALIAS *, int, wchar_t *);
int departmentalias_search(struct DEPARTMENT_ALIAS *, int, wchar_t *);

int nauthoralias;	// number of items in the author alias table
struct AUTHOR_ALIAS *authoralias;
int nkeywordalias;	// number of items in the keyword alias table
struct KEYWORD_ALIAS *keywordalias;
int ndepartmentalias;	// number of items in the department alias table, added 2016/01/26
struct DEPARTMENT_ALIAS *departmentalias;	// added 2016/01/26

extern FILE *logstream;

extern int parse_aalias(wchar_t *, wchar_t *, wchar_t *);

//
// read the author alias file
//
int prepare_author_alias_list()
{
	int i;
	FILE *astream;
	wchar_t tmps[LBUF_SIZE], *sp, *tline;

	// open the "Author alias.txt" file, ignore if it does not exist
	if (_wfopen_s(&astream, L"Author alias.txt", L"rt, ccs=UTF-8") != 0)
	{
		fwprintf(logstream, L"WARNING: \"Author alias.txt\" is not provided.\n");
		nauthoralias = 0;
	}
	else
	{
		fwprintf(logstream, L"Found and read the file: \"Author alias.txt\".\n");
		// count the number of items in the alias table
		nauthoralias = 0;
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, astream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r' || tmps[0] == ' ' || tmps[0] == '\t')	// modified 2017/06/08
				continue;
			nauthoralias++;
		}
		// allocate memory for the alias table
		authoralias = (struct AUTHOR_ALIAS *)malloc(nauthoralias * sizeof(struct AUTHOR_ALIAS));	
		if (authoralias == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(astream);	// point back to the begining of the assignee alias file
		nauthoralias = 0;	
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, astream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r' || tmps[0] == ' ' || tmps[0] == '\t')	// modified 2017/06/08
				continue;
			tline = tmps;
			if (nauthoralias == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
			{
				if (tmps[0] == 0xfeff || tmps[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
					tline = &tmps[1];	// skip the BOM
			}
			parse_aalias(tline, authoralias[nauthoralias].name, authoralias[nauthoralias].alias);
			nauthoralias++;		
		}

		for (i = 0; i < nauthoralias; i++)	// change all text to lower case
		{
			sp = authoralias[i].name; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
			sp = authoralias[i].alias; while (*sp != '\0') { *sp = towlower(*sp); sp++; } 
		}
		// sort the table by the original name
		qsort((void *)authoralias, (size_t)nauthoralias, sizeof(struct AUTHOR_ALIAS), compare_authoralias);
		fwprintf(logstream, L"Author alias table\n");
		for (i = 0; i < nauthoralias; i++)
		{
			fwprintf(logstream, L"[%s]=>[%s]\n", authoralias[i].name, authoralias[i].alias);
			fflush(logstream);
		}

	}
	fclose(astream);

	return 0;
}

//
// read the keyword alias file
//
int prepare_keyword_alias_list()
{
	int i;
	FILE *kstream;
	wchar_t tmps[LBUF_SIZE], *sp, *tline;

	// open the "Keyword alias.txt" file, ignore if it does not exist
	if (_wfopen_s(&kstream, L"Keyword alias.txt", L"rt, ccs=UTF-8") != 0)
	{
		fwprintf(logstream, L"\nWARNING: \"Keyword alias.txt\" is not provided.\n");
		nkeywordalias = 0;
	}
	else
	{
		fwprintf(logstream, L"\nFound and read the file: \"Keyword alias.txt\".\n");
		// count the number of items in the alias table
		nkeywordalias = 0;
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, kstream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			nkeywordalias++;
		}
		// allocate memory for the alias table
		keywordalias = (struct KEYWORD_ALIAS *)malloc(nkeywordalias * sizeof(struct KEYWORD_ALIAS));
		if (keywordalias == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(kstream);	// point back to the begining of the assignee alias file
		nkeywordalias = 0;	
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, kstream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			tline = tmps;
			if (nkeywordalias == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
			{
				if (tmps[0] == 0xfeff || tmps[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
					tline = &tmps[1];	// skip the BOM
			}
			parse_aalias(tline, keywordalias[nkeywordalias].name, keywordalias[nkeywordalias].alias);
			nkeywordalias++;		
		}
		for (i = 0; i < nkeywordalias; i++)	// change all text to lower case
		{
			sp = keywordalias[i].name; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
			sp = keywordalias[i].alias; while (*sp != '\0') { *sp = towlower(*sp); sp++; } 
		}
		// sort the table by the original name
		qsort((void *)keywordalias, (size_t)nkeywordalias, sizeof(struct KEYWORD_ALIAS), compare_keywordalias);
		fwprintf(logstream, L"Keyword alias table\n");
		for (i = 0; i < nkeywordalias; i++)
			fwprintf(logstream, L"[%s]=>[%s]\n", keywordalias[i].name, keywordalias[i].alias);
	}
	fclose(kstream);

	return 0;
}

//
// read the department alias file (only for 台灣碩博士資料)
//
int prepare_department_alias_list()
{
	int i;
	FILE *dstream;
	wchar_t tmps[LBUF_SIZE], *sp, *tline;

	// open the "Department alias.txt" file, ignore if it does not exist
	if (_wfopen_s(&dstream, L"Department alias.txt", L"rt, ccs=UTF-8") != 0)
	{
		fwprintf(logstream, L"WARNING: \"Department alias.txt\" is not provided.\n");
		ndepartmentalias = 0;
	}
	else
	{
		fwprintf(logstream, L"\nFound and read the file: \"Department alias.txt\".\n");
		// count the number of items in the alias table
		ndepartmentalias = 0;
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, dstream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			ndepartmentalias++;
		}
		// allocate memory for the alias table
		departmentalias = (struct DEPARTMENT_ALIAS *)malloc(ndepartmentalias * sizeof(struct DEPARTMENT_ALIAS));	
		if (departmentalias == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(dstream);	// point back to the begining of the assignee alias file
		ndepartmentalias = 0;	
		while (TRUE)
		{
			if(fgetws(tmps, LBUF_SIZE, dstream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r' || tmps[0] == '\t' || tmps[0] == ' ')
				continue;
			tline = tmps;
			if (ndepartmentalias == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
			{
				if (tmps[0] == 0xfeff || tmps[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
					tline = &tmps[1];	// skip the BOM
			}
			parse_aalias(tline, departmentalias[ndepartmentalias].name, departmentalias[ndepartmentalias].alias);
			ndepartmentalias++;		
		}
		//for (i = 0; i < ndepartmentalias; i++)	// change all text to lower case
		//{
		//	sp = departmentalias[i].name; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
		//	sp = departmentalias[i].alias; while (*sp != '\0') { *sp = towlower(*sp); sp++; } 
		//}
		// sort the table by the original name
		qsort((void *)departmentalias, (size_t)ndepartmentalias, sizeof(struct DEPARTMENT_ALIAS), compare_departmentalias);
		fwprintf(logstream, L"Department alias table\n");
		for (i = 0; i < ndepartmentalias; i++)
			fwprintf(logstream, L"[%s]=>[%s]\n", departmentalias[i].name, departmentalias[i].alias);

	}
	fclose(dstream);

	return 0;
}

//
// parse the assignee, author, or keyword alias string
// the string is delimited by a tab
//
int parse_aalias(wchar_t *istr, wchar_t *oname, wchar_t *alias)
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
int compare_authoralias(const void *n1, const void *n2)
{
	struct AUTHOR_ALIAS *t1, *t2;
	
	t1 = (struct AUTHOR_ALIAS *)n1;
	t2 = (struct AUTHOR_ALIAS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_keywordalias(const void *n1, const void *n2)
{
	struct KEYWORD_ALIAS *t1, *t2;
	
	t1 = (struct KEYWORD_ALIAS *)n1;
	t2 = (struct KEYWORD_ALIAS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_departmentalias(const void *n1, const void *n2)
{
	struct DEPARTMENT_ALIAS *t1, *t2;
	
	t1 = (struct DEPARTMENT_ALIAS *)n1;
	t2 = (struct DEPARTMENT_ALIAS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of an author alias in an "AUTHOR_ALIAS" array
//
int authoralias_search(struct AUTHOR_ALIAS d[], int num, wchar_t *str)
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

//
// use binary search to find the proper position of a keyword alias in a "KEWORD_ALIAS" array
//
int keywordalias_search(struct KEYWORD_ALIAS d[], int num, wchar_t *str)
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

//
// use binary search to find the proper position of an department alias in an "DEPARTMENT_ALIAS" array
//
int departmentalias_search(struct DEPARTMENT_ALIAS d[], int num, wchar_t *str)
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
