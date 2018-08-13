// 
// USPTOdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles USPTO data (in one of the WEBPAT format)
//
//

//
// Revision History:
// 2012/04/01 Modification   : added codes and function parse_ipc() to handle IPC codes
// 2012/05/20 Added function : added codes to save "Country" to uspto[] array
// 2012/06/09 Added function : added codes to read and save assignee names, it also reads "Assignee alias.txt" file 
// 2012/06/29 Added function : added function parse_date() (in order to obtain month data)
// 2012/07/08 Added function : added to obtain patent application date data
// 2013/07/22 Fixed problems : added codes to handle empty alias names in the function parse_assignee_names() and parse_store_assignees()
// 2013/07/22 Fixed problems : fixed a problem in reading assignee alias file
// 2014/06/02 Modification   : moved the function parse_aalias() to file "author alias.cpp"
// 2016/09/15 Modification   : added codes to handle the situation when a citation file is provided
// 2016/12/07 Added function : added codes to write out university-industry relationships. this is only used for digging the U-I relationships in patents.
//                                   need to "#define UNIVERSITY_INDUSRY_PROJECT" to enable it.
//

#include "stdafx.h"
#include <string.h>
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "network.h"
#using <mscorlib.dll>

using namespace System;

#define XXLBUF_SIZE 65536
#define BUF_SIZE 1024

#define EURO_PATENT 1
#define US_PATENT 2

//
// function types
//
int parse_US_reference(wchar_t *, wchar_t *, FILE *);
int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
int parse_store_inventors(wchar_t *, int *, struct AUTHORS *);
int parse_inventor_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_store_assignees(wchar_t *, int *, struct ASSIGNEES *);
int parse_assignee_names(wchar_t *, int *, int *, int, struct ASSIGNEES *);
int parse_ipc(wchar_t *, int *, wchar_t *, int *);
int parse_date(wchar_t *, int *, int *);
int compare_author(const void *, const void *);
int compare_assignee(const void *, const void *);
int compare_aalias(const void *, const void *);
int adjust_inventor_names(wchar_t *);
int get_number_inventors(wchar_t *);
int get_number_assignees(wchar_t *);
int remove_end_str(wchar_t *);
int asgnname_search(struct ASSIGNEES *, int, wchar_t *);
int aalias_search(struct ANAME_ALIAS*, int, wchar_t *);

extern int link_author_WOS();
extern int compare_docid(const void *, const void *);
extern int compare_pid(const void *, const void *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int prepare_alias(1);
extern int ipc_statistics(struct USPTO *, int);
extern int parse_aalias(wchar_t *, wchar_t *, wchar_t *);
extern int is_university(wchar_t *name);
extern int readwrite_UI_USPTO(wchar_t *, wchar_t *, struct USPTO *);
extern int readwrite_UI_TIP(wchar_t *, wchar_t *, struct USPTO *);

//
// global variables
//
static FILE *sstream;		// for source file

int nuspto;
struct USPTO *uspto;
int filetype = US_PATENT;	// US_PATENT or EURO_PATENT
int naalias;	// number of items in the assignee alias table
struct ANAME_ALIAS *aalias;

extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern FILE *logstream;
extern int no_wos_citation_file;	// added 2016/09/15

struct PFID pfid[18] = {
L"PatentNumber     :", 1,
L"Issue_Date       :", 2,
L"Appl_Number      :", 3,
L"Appl_Date        :", 4,
L"Country          :", 5,
L"Title            :", 6,
L"Abstract         :", 7,
L"Claim            :", 8,
L"Examiner         :", 9,
L"Agent            :", 10,
L"Assignee         :", 11,
L"Foreign Reference:", 12,
L"Priority         :", 13,
L"IPC              :", 14,
L"Inventor         :", 15,
L"US Reference     :", 16,
L"US_Class         :", 17,
L"------------------", 18,
};

//
// read WOS data and put it into the wos[] array
//
int read_USPTO(wchar_t *sname)
{
	int i, k, m, ret;
	int state, lcnt, pcnt, cnt;
	int slen;
	wchar_t tmps[BUF_SIZE], *sp;
	wchar_t lptr[XXLBUF_SIZE], *line;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	FILE *cstream;		// for citation file
	FILE *astream;		// for assignee alias file

// 
// Open the source file (will fail if the file does not exist)
//	
	if (_wfopen_s(&sstream, sname, L"r") != 0)
		return MSG_WOSFILE_NOTFOUND;

	// 1st pass, count the number of patents in the file
	state = 1; lcnt = 0; pcnt = 0;
	while (TRUE)
	{		
		line = lptr;
		if(fgetws(lptr, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (lptr[0] == '\n' || line[0] == '\r')
			continue;
		switch (state)
		{
		case 1:	// patent number
			if (lcnt == 0)
			{
				if (wcsncmp(line, L"PatentNumber", 12) != 0)	// there may be 3 leading codes "EF BB BF" (in hex) at the begining of the file
					line = &lptr[3];
			}
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				if (lcnt == 0)	// check the type of file (US or EP)
				{
					if (wcsncmp(&line[18], L"EP", 2) == 0)
						filetype = EURO_PATENT;
				}
				state = 2;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;		
		case 2:	// issue date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 3; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 3:	// Appl_Number
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 4; else return MSG_WOSFILE_FORMAT_ERROR; 
			break;
		case 4:	// Appl_Date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 5; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 5:	// Country
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 6; else	return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 6:	// Title
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 7; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 7:	// Abstract
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 8; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 8:	// Claim, there are many lines in the claim
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 9; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 9:	// Examiner
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 10; 
			break; 
		case 10:// Agent
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 11;	else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 11:// Assignee
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 12; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 12:// Foreign Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 13; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// Priority 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 14; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// IPC
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 15; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 15:// Inventor
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 16; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 16:// US Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 17; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 17:// US_Class 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 18; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 18:// separation line 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				pcnt++;
				state = 1;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}
	nuspto = pcnt;

	// open the "Assignee alis.txt" file, ignore if it does not exist
	if (_wfopen_s(&astream, L"Assignee alias.txt", L"r") != 0)
	{
		fwprintf(logstream, L"WARNING: file \"Assignee alias.txt\" is not provided.\n");
		naalias = 0;
	}
	else
	{
		fwprintf(logstream, L"Found and read the file: \"Assignee alias.txt\".\n");
		// count the number of items in the alias table
		naalias = 0;
		while (TRUE)
		{
			if(fgetws(tmps, BUF_SIZE, astream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			naalias++;
		}
		// allocate memory for the alias table
		aalias = (struct ANAME_ALIAS *)malloc(naalias * sizeof(struct ANAME_ALIAS));	// estimate in average 1.2 assignees per document
		if (aalias == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(astream);	// point back to the begining of the assignee alias file
		naalias = 0;		// added 2013/07/22
		while (TRUE)
		{
			if(fgetws(tmps, BUF_SIZE, astream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			parse_aalias(tmps, aalias[naalias].name, aalias[naalias].alias);
			naalias++;		// added 2013/07/22
		}
		for (i = 0; i < naalias; i++)	// change all text to lower case, added 2017/01/10
		{
			sp = aalias[i].name; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
			sp = aalias[i].alias; while (*sp != '\0') { *sp = towlower(*sp); sp++; } 
		}
		// sort the table by the original name
		qsort((void *)aalias, (size_t)naalias, sizeof(struct ANAME_ALIAS), compare_aalias);	
		fwprintf(logstream, L"Assignee alias table\n");	// added 2017/01/10
		for (i = 0; i < naalias; i++)
			fwprintf(logstream, L"[%s]=>[%s]\n", aalias[i].name, aalias[i].alias);
	}
	
	// 2nd pass, get the inventor data and establish the inventor array
	rewind(sstream);	// point back to the begining of the file
	// allocate memory for the inventor (author) name array
	authors = (struct AUTHORS *)malloc(nuspto * 8 * sizeof(struct AUTHORS));	// estimate in average 8 inventors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// allocate memory for the assignee name array
	assignees = (struct ASSIGNEES *)malloc((int)(nuspto * 3) * sizeof(struct ASSIGNEES));	// estimate in average 3 assignees per document, increased from 1.5 to 3, 2016/12/07
	if (assignees == NULL) return MSG_NOT_ENOUGH_MEMORY;
	state = 1; lcnt = 0; pcnt = 0;
	while (TRUE)
	{		
		line = lptr;
		if(fgetws(lptr, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (lptr[0] == '\n' || line[0] == '\r')
			continue;
		switch (state)
		{
		case 1:	// patent number			
			if (lcnt == 0)
			{
				if (wcsncmp(line, L"PatentNumber", 12) != 0)	// there may be 3 leading codes "EF BB BF" (in hex) at the begining of the file
					line = &lptr[3];	
			}
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
				state = 2;
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;		
		case 2:	// issue date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 3; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 3:	// Appl_Number
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 4; else return MSG_WOSFILE_FORMAT_ERROR; 
			break;
		case 4:	// Appl_Date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 5; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 5:	// Country
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 6; else	return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 6:	// Title
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 7; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 7:	// Abstract
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 8; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 8:	// Claim, there are many lines in the claim
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 9; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 9:	// Examiner
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 10; 
			break; 
		case 10:// Agent
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 11;	else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 11:// Assignee
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				//fwprintf(logstream, L"%s", &line[18]); fflush(logstream);
				parse_store_assignees(&line[18], &nasgns, assignees);
				state = 12; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 12:// Foreign Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 13; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// Priority 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 14; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// IPC
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 15; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 15:// Inventor
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				//fwprintf(logstream, L"%s", &line[18]); fflush(logstream);
				parse_store_inventors(&line[18], &naus, authors);
				state = 16; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 16:// US Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 17; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 17:// US_Class 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 18; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 18:// separation line 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				pcnt++;
				state = 1;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}
#ifdef DEBUG
	for (i = 0; i < nasgns; i++)
		fwprintf(logstream, L"%d %s\n", i+1, assignees[i].name);
#endif DEBUG
	for (i = 0; i < naus; i++)
		adjust_inventor_names(authors[i].name);
	// sort and consolidate duplicate inventor names
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_author);
	wchar_t prev_name[MAX_AUTHOR_NAME];
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < naus; i++)
	{
		if (wcscmp(authors[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) authors[k-1].np = cnt;
			wcscpy_s(authors[k++].name, MAX_AUTHOR_NAME, authors[i].name); 
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, authors[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	authors[k-1].np = cnt;
	naus = k;

	// sort and consolidate duplicate assignee names
	qsort((void *)assignees, (size_t)nasgns, sizeof(struct ASSIGNEES), compare_assignee);
	wchar_t prev_aname[MAX_ASSIGNEE_NAME];
	prev_aname[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nasgns; i++)
	{
		if (wcscmp(assignees[i].name, prev_aname) != 0)	// hit a new name
		{
			if (k > 0) assignees[k-1].np = cnt;
			wcscpy_s(assignees[k++].name, MAX_ASSIGNEE_NAME, assignees[i].name); 
			wcscpy_s(prev_aname, MAX_ASSIGNEE_NAME, assignees[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	assignees[k-1].np = cnt;
	nasgns = k;

#ifdef DEBUG
	for (i = 0; i < nasgns; i++)
	{
		fwprintf(logstream, L"%d %s\n", i+1, assignees[i].name);
		fflush(logstream);
	}
#endif DEBUG

	// allocate memory for the list of USPTO data
	uspto = (struct USPTO *)malloc(nuspto * sizeof(struct USPTO));
	if (uspto == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// open citation file
	strip_name_extension(sname, tname, dpath);
	if (no_wos_citation_file)	// this check is added 2016/09/15
	{
		ret = swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname);
			if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"w") != 0)
			return MSG_CFILE_CANNOTOPEN;
	}

	// 3rd pass, read in the data
	rewind(sstream);
	state = 1; lcnt = 0; pcnt = 0;
	while (TRUE)
	{		
		line = lptr;
		if(fgetws(lptr, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (lptr[0] == '\n' || line[0] == '\r')
			continue;
		switch (state)
		{
		case 1:	// patent number			
			if (lcnt == 0)
			{
				if (wcsncmp(line, L"PatentNumber", 12) != 0)	// there may be 3 leading codes "EF BB BF" (in hex) at the begining of the file
					line = &lptr[3];	
			}
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{		
				slen = wcslen(&line[18]);
				wcsncpy_s(uspto[pcnt].pid, MAX_PATENT_ID, &line[18], slen-1);	// do not copy the '0x0d' at the end of the line
				state = 2;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;		
		case 2:	// issue date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				parse_date(&line[18], &uspto[pcnt].year, &uspto[pcnt].month);
				state = 3;
				//fwprintf(logstream, L"%s [%d:%d]\n", &line[18], uspto[pcnt].year, uspto[pcnt].month);
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 3:	// Appl_Number
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 4; else return MSG_WOSFILE_FORMAT_ERROR; 
			break;
		case 4:	// Appl_Date			
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				parse_date(&line[18], &uspto[pcnt].a_year, &uspto[pcnt].a_month);
				state = 5;
				//fwprintf(logstream, L"%s [%d:%d]\n", &line[18], uspto[pcnt].a_year, uspto[pcnt].a_month);
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 5:	// Country
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				uspto[pcnt].country[0] = line[18];
				uspto[pcnt].country[1] = line[18+1];
				uspto[pcnt].country[2] = '\0';
				//fwprintf(logstream, L"%s\n", uspto[pcnt].country);
				state = 6; 
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 6:	// Title
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 7; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 7:	// Abstract
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 8; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 8:	// Claim, there are many lines in the claim
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 9; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 9:	// Examiner
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 10; 
			break;
		case 10:// Agent
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 11; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 11:// Assignee
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				uspto[pcnt].nassignee = get_number_assignees(&line[18]);
				parse_assignee_names(&line[18], &uspto[pcnt].nassignee, uspto[pcnt].assignee, nasgns, assignees);
				state = 12; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 12:// Foreign Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				//fwprintf(logstream, L"%s", &line[18]); fflush(logstream);
				if (line[18] != '\0' && line[18] != '\r' && line[18] != '\n')
				{
					if (filetype == EURO_PATENT)
						parse_US_reference(&line[18+2], uspto[pcnt].pid, cstream); // in EURO_PATENT file, Foreign Reference string is lead by "$$", ignore them
				}
				state = 13; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// Priority 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 14; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// IPC
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{	
				parse_ipc(&line[18], &uspto[pcnt].nipc, uspto[pcnt].ipc, uspto[pcnt].ipc_cnt);
				state = 15; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 15:// Inventor
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				uspto[pcnt].ninventor = get_number_inventors(&line[18]);
				parse_inventor_names(&line[18], &uspto[pcnt].ninventor, uspto[pcnt].inventor, naus, authors);
				state = 16; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 16:// US Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				if (filetype == US_PATENT)
					parse_US_reference(&line[18], uspto[pcnt].pid, cstream);
				state = 17;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 17:// US_Class 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	 state = 18; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 18:// separation line 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				pcnt++;
				state = 1;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}

	fclose(sstream);	// close full record file
	if (no_wos_citation_file) fclose(cstream);	// close citation file (relationship file) if it is created


#ifdef DEBUG
	for (i = 0; i < pcnt; i++)
	{
		fwprintf(logstream, L"%d: %d %s\t", i+1, uspto[i].year, uspto[i].pid); 
		for (k = 0; k < uspto[i].nassignee; k++)
			fwprintf(logstream, L"%d [%s] ", uspto[i].assignee[k], assignees[uspto[i].assignee[k]].name);
			//fwprintf(logstream, L"%d ", uspto[i].assignee[k]);
		fwprintf(logstream, L"\n");
		fflush(logstream);
	}
#endif DEBUG

//#define UNIVERSITY_INDUSRY_PROJECT
#ifdef UNIVERSITY_INDUSRY_PROJECT	// added 2016/12/07, write out U-I relationship to the UI relationship file
	wchar_t uiname[FNAME_SIZE];
	wchar_t uiname2[FNAME_SIZE];
	FILE *uistream, *uistream2;
	wchar_t oname[FNAME_SIZE];
	int j, aj, ak;
	int UU, UI;

	swprintf_s(uiname, FNAME_SIZE, L"%s U-I relations.txt", tname);
	if (_wfopen_s(&uistream, uiname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;
	swprintf_s(uiname2, FNAME_SIZE, L"%s U-I relations 2.txt", tname);
	if (_wfopen_s(&uistream2, uiname2, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nuspto; i++) uspto[i].UI = 0;

	for (i = 0; i < nuspto; i++)
	{
		if (uspto[i].nassignee >= 2) // more than one assignees
		{
			UI = UU = 0;
			for (j = 0; j < uspto[i].nassignee; j++)
			{
				for (k = j + 1; k < uspto[i].nassignee; k++)
				{
					aj = uspto[i].assignee[j];
					ak = uspto[i].assignee[k];
					// write out relations when only one of them is an university
					if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 0)
					{
						UI = 1;
						fwprintf(uistream, L"%s\t%s\n", assignees[aj].name, assignees[ak].name);
						fwprintf(uistream2, L"%s\t%s\t%s\n", uspto[i].pid, assignees[aj].name, assignees[ak].name);
					}
					else if (is_university(assignees[aj].name) == 0 && is_university(assignees[ak].name) == 1)
					{
						UI = 1;
						fwprintf(uistream, L"%s\t%s\n", assignees[ak].name, assignees[aj].name);
						fwprintf(uistream2, L"%s\t%s\t%s\n", uspto[i].pid, assignees[ak].name, assignees[aj].name);
					}
					else if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 1)
					{
						UU = 1;
						fwprintf(uistream, L"%s\t%s\n", assignees[aj].name, assignees[ak].name);
						fwprintf(uistream2, L"%s\t%s\t%s\n", uspto[i].pid, assignees[aj].name, assignees[ak].name);
					}
				}
			}
			if (UI == 0 && UU == 1)
				uspto[i].UI = UNIVERSITY_UNIVERSITY;
			else if (UI == 1 && UU == 0)
				uspto[i].UI =UNIVERSITY_INDUSTRY;
			else if (UI == 1 && UU == 1)
				uspto[i].UI = UNIVERSITY_UNIVERSITY_INDUSTRY;
		}
	}

	swprintf_s(oname, FNAME_SIZE, L"%s U-I.txt", tname);
	readwrite_UI_USPTO(sname, oname, uspto);

	exit(0);
#endif UNIVERSITY_INDUSRY_PROJECT

	nwos = nuspto;
	// allocate memory for the list of WOS data
	wos = (struct WOS *)malloc(nwos * sizeof(struct WOS));
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// move proper information to the wos[] array, because all the following action use data in the wos[] array 
	for (i = 0; i < nuspto; i++)
	{
		wcscpy_s(wos[i].docid, MAX_DOC_ID, uspto[i].pid);
		wos[i].nau = uspto[i].ninventor;
		for (k = 0; k < uspto[i].ninventor; k++)
			wos[i].author[k] = uspto[i].inventor[k];
		wos[i].nde = uspto[i].nassignee;	// use the space for authro keyword to save assignee names
		for (k = 0; k < uspto[i].nassignee; k++)
			wos[i].DE[k] = uspto[i].assignee[k];
		wos[i].year = uspto[i].year;
		wos[i].month = uspto[i].month;		// added 2012/07/08
		wos[i].a_year = uspto[i].a_year;	// added 2012/07/08
		wos[i].a_month = uspto[i].a_month;	// added 2012/07/08
		wos[i].tc = uspto[i].tc;
		wos[i].country[0] = uspto[i].country[0]; wos[i].country[1] = uspto[i].country[1]; wos[i].country[2] = '\0';
		wos[i].nipc = uspto[i].nipc;
		for (m = 0; m < MAX_IPC*MAX_IPC_CODE; m++) wos[i].ipc[m] = uspto[i].ipc[m];
	}

	// append the year information to the patent number as alias
	//prepare_alias(1);	// if want inventor names as alias
	for (i = 0; i < nwos; i++)
		swprintf_s(wos[i].alias, MAX_ALIAS, L"%s_%d_%d", wos[i].docid, wos[i].a_year, wos[i].year);

	link_author_WOS();	

	// author statistics
	// count the number of times each author as the 1st author
	//
	for (i = 0; i < naus; i++) authors[i].cnt1 = 0;	// initialize 1st author count
	for (i = 0; i < nwos; i++) authors[wos[i].author[0]].cnt1++;	
	for (i = 0; i < naus; i++) { authors[i].byear = 3000; authors[i].eyear = 0;}	// initialize byear and eyear
	// find the ending and beginning year of an author's publication
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nau; k++)
		{
			if (authors[wos[i].author[k]].byear > wos[i].year) authors[wos[i].author[k]].byear = wos[i].year;
			if (authors[wos[i].author[k]].eyear < wos[i].year) authors[wos[i].author[k]].eyear = wos[i].year;
		}
	}
	// write out author statistics
	fwprintf(logstream, L"Inventor statistics (total patents, 1st inventors, active years, name):\n");
	for (i = 0; i < naus; i++)
		fwprintf(logstream, L"%d %d %d~%d \"%s\"\n", authors[i].np, authors[i].cnt1, authors[i].byear, authors[i].eyear, authors[i].name);

	ipc_statistics(uspto, nuspto);	// added 2012/04/01

	// sort the data by the patent id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	qsort((void *)uspto, (size_t)nuspto, sizeof(struct USPTO), compare_pid);	// added 2011/06/03

	free(aalias);

	return 0;
}
	
//
// adjust inventor names
// 1. replace a ';' with a ','
// 2. force a blank after the ',' if there is not any
//
int adjust_inventor_names(wchar_t *iname)
{
	int i, after_comma;
	wchar_t *sp, *tp;
	wchar_t vname[MAX_AUTHOR_NAME];
	wchar_t tmps[MAX_AUTHOR_NAME];

	tp = tmps;
	sp = iname;
	after_comma = FALSE;
	while (*sp != L'\0')
	{
		if (*sp == L';')
		{
			*tp++ = L',';
			*tp++ = L' ';	// force a blank after the ';'
			sp++;
			after_comma = TRUE;
		}
		else
		{
			if (after_comma == TRUE)					
			{
				after_comma = FALSE;
				if (*sp == L' ')	// ignore blanks immediately after
					sp++;	
			}
			else
				*tp++ = *sp++;
		}
	}
	*tp = '\0';
	wcscpy_s(iname, MAX_AUTHOR_NAME, tmps);

	return 0;
}

//
// count the numnber of inventors in this string
//
int get_number_inventors(wchar_t *istr)
{
	int cnt;
	wchar_t ch, *sp;

	sp = istr; cnt = 0;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp++; 
		if (ch == L'$') 
		{ 
			cnt++;
			while (*sp == L'$') sp++;	// skip the 2nd '$' delimeter
		}
	}

	cnt = cnt + 1;

	return cnt;
}

//
// count the numnber of assignees in this string
//
int get_number_assignees(wchar_t *istr)
{
	int cnt;
	wchar_t ch, *sp;

	sp = istr; cnt = 0;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp++; 
		if (ch == L'$') 
		{ 
			cnt++;
			while (*sp == L'$') sp++;	// skip the 2nd '$' delimeter
		}
	}

	cnt = cnt + 1;

	return cnt;
}

//
// parse the "IPC" field string, each is delimited by "$$"
//
int parse_ipc(wchar_t *str, int *nipc, wchar_t *ipc_code, int *ipc_code_cnt)
{
	int i, state;
	int cnt;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[BUF_SIZE];

	sp = str; tp = bfr;
	state = 1; cnt = 0;
	while (TRUE)
	{
		ch = *sp;
		switch (state)
		{
		case 1:	// looking for the begining of the delimeter string
			if (ch == L'$'|| ch == L'\n' || ch == L'\r')
			{
				*tp = L'\0';
				tp = bfr; 
				ipc_code_cnt[cnt] = 1;	// 1 for every IPC code, added 2016/05/26
				wcscpy_s(&ipc_code[MAX_IPC_CODE*cnt++], MAX_IPC_CODE*sizeof(wchar_t), bfr);
				//fwprintf(logstream, L"%s ", bfr);
				if (ch == L'\n' || ch == L'\r')
				{
					*nipc = cnt;
					return 0;
				}
				else
				{
					tp = bfr;
					state = 2;
				}
			}
			else
				*tp++ = *sp;
			break;
		case 2:	// expecting the end of the delimeter string
			if (ch != L'$')
			{
				*tp++ = *sp;
				state = 1;
			}
			break;
		default:
			break;
		}
		sp++;
	}

	return 0;
}

//
// split a given inventor string into individual inventors,
// do the correction on the inventor names
//    1. replace a ';' with a ','
//    2. force a blank after the ',' if there is not any
// then find each inventor's index, and then save them to the uspto[] array
//
int parse_inventor_names(wchar_t *istr, int *nvntr, int *vntr, int ninvts, struct AUTHORS *invts)
{
	int i;
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int ndx;
	int tnvntr;

	sp = istr;

	// start parsing
	tp = tmps;
	tnvntr = 0;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == L'$') 
		{ 
			*tp++ = L'\0'; sp++; 
			if (tmps[0] != L'\0')
			{
				adjust_inventor_names(tmps);
				ndx = aname_search(invts, ninvts, tmps);
				vntr[tnvntr] = ndx;
				tnvntr++; 
				if (tnvntr >= MAX_AUTHORS)	// take only MAX_AUTHORS inventors
				{
					*nvntr = tnvntr;
					return 0;
				}
			}
			tp = tmps;
			while (*sp == L'$') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0';
	if (tmps[0] != L'\0')
	{
		adjust_inventor_names(tmps);
		ndx = aname_search(invts, ninvts, tmps);
		vntr[tnvntr] = ndx;
		tnvntr++; 
	}

	*nvntr = tnvntr;

	return 0;
}

//
// split a given inventor string into individual inventors
// and then save them to the global author name array
//
int parse_store_inventors(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau;

	tnau = *nau;
	sp = astr;

	// start parsing
	tp = tmps;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == L'$') 
		{ 
			*tp++ = L'\0'; sp++; 
			if (tmps[0] != L'\0')
				wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
			tp = tmps;
			while (*sp == L'$') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0';
	if (tmps[0] != L'\0')
		wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
	*nau = tnau;

	return 0;
}

//
// split a given assignee string into individual assignees,
// then find each assignee's index, and then save them to the uspto[] array
//
int parse_assignee_names(wchar_t *istr, int *nasgn, int *asgn, int nasgns, struct ASSIGNEES *asgns)
{
	int i;
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int ndx, ndx2;
	int tnasgn;

	sp = istr;

	// start parsing
	tp = tmps;
	tnasgn = 0;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == L'$') 
		{ 
			*tp++ = L'\0'; sp++; 
			if (tmps[0] != L'\0')
			{
				ndx2 = aalias_search(aalias, naalias, tmps);
				if (ndx2 == -1)	// the assignee name may not be in the alias table
					ndx = asgnname_search(asgns, nasgns, tmps);
				else
				{
					ndx = asgnname_search(asgns, nasgns, aalias[ndx2].alias);
					if (ndx == -1)	// assignee name in the alias table, but points to an empty string
						ndx = asgnname_search(asgns, nasgns, tmps);
				}
				asgn[tnasgn] = ndx;
				tnasgn++; 
				if (tnasgn >= MAX_ASSIGNEES)	// take only MAX_ASSIGNEES assignees
				{
					*nasgn = tnasgn;
					return 0;
				}
			}
			tp = tmps;
			while (*sp == L'$') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0';
	if (tmps[0] != L'\0')
	{
		ndx2 = aalias_search(aalias, naalias, tmps);
		if (ndx2 == -1)	// the assignee name may not be in the alias table
			ndx = asgnname_search(asgns, nasgns, tmps);
		else
		{
			ndx = asgnname_search(asgns, nasgns, aalias[ndx2].alias);
			if (ndx == -1)	// assignee name in the alias table, but points to an empty string
				ndx = asgnname_search(asgns, nasgns, tmps);
		}
		asgn[tnasgn] = ndx;
		tnasgn++; 
	}

	*nasgn = tnasgn;

	return 0;
}

//
// split a given assignee string into individual assignees
// and then save them to the global assignee name array
//
int parse_store_assignees(wchar_t *astr, int *nasgns, struct ASSIGNEES *asgn)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnasgns;
	int ndx;

	// replace the assignee names with their alias names if a alias file is given
	tnasgns = *nasgns;
	sp = astr;

	// start parsing
	tp = tmps;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == L'$') 
		{ 
			*tp++ = L'\0'; sp++; 
			if (tmps[0] != L'\0')
			{
				ndx = aalias_search(aalias, naalias, tmps);
				if (ndx == -1)	// the assignee name may not be in the alias table
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
					wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, tmps);
				}
				else
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, aalias[ndx].alias); fflush(logstream);
					if (aalias[ndx].alias[0] == '\0')
						wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, tmps);
					else
						wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, aalias[ndx].alias);
				}
			}
			tp = tmps;
			while (*sp == L'$') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0';
	if (tmps[0] != L'\0')
	{
		ndx = aalias_search(aalias, naalias, tmps);
		if (ndx == -1)	// the assignee name may not be in the alias table
		{
			//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
			wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, tmps);
		}
		else
		{
			//fwprintf(logstream, L"[%d %s]\n", ndx, aalias[ndx].alias); fflush(logstream);
			if (aalias[ndx].alias[0] == '\0')
				wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, tmps);
			else
			wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, aalias[ndx].alias);
		}
	}
	*nasgns = tnasgns;

	return 0;
}

//
// strip the name extension and get the path of a given file name
//
int strip_name_extension(wchar_t *sfname, wchar_t *tfname, wchar_t *fpath)
{
	int i, ind_dot;
	int slen;

	// 1st round, find the name without the extension
	slen = wcslen(sfname);
	ind_dot = -1;
	for (i = 0; i < slen; i++) 
	{
		if (sfname[i] == '.')
			ind_dot = i;	// indicate the position of the last dot in the file name
	}
	if (ind_dot <= 0)	// no '.' in the input file name
		ind_dot = slen;
	for (i = 0; i < ind_dot; i++) 
		tfname[i] = sfname[i];
	tfname[i] = '\0';

	// 2nd round, find the path of the given file name
	slen = wcslen(sfname);
	ind_dot = -1;
	for (i = 0; i < slen; i++) 
	{
		if (sfname[i] == '\\')
			ind_dot = i;	// indicate the position of the last dot in the file name
	}
	if (ind_dot <= 0)	// no '.' in the input file name
		ind_dot = slen;
	for (i = 0; i <= ind_dot; i++) 
		fpath[i] = sfname[i];
	fpath[i] = '\0';

	return 0;
}

//
// parse the "US Reference" string, citations are delimeted by "$$", each patent number is leaded by "US"
// "Foreign Reference" strings are also parsed here
//
int parse_US_reference(wchar_t *str, wchar_t *pid, FILE *stream)
{
	int i, state;
	int issued;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[BUF_SIZE];

	if (!no_wos_citation_file)	// added 2016/09/15
		return 0;

	sp = str; tp = bfr;
	state = 1;
	while (TRUE)
	{
		ch = *sp;
		switch (state)
		{
		case 1:	// looking for the begining of the delimeter string
			if (ch == L'$'|| ch == L'\n' || ch == L'\r')
			{
				*tp = L'\0';
				tp = bfr; issued = TRUE;
				while (*tp != L'\0') 
				{ 
					if (*tp == L'/') { issued = FALSE; break; }
					tp++;
				}
				if (issued)
				{
					if (filetype == EURO_PATENT)	// for EP patent
					{
						remove_end_str(bfr);	
						fwprintf(stream, L"%s %s\n", bfr, pid);
					}
					else	// for US paent
					{
						if (wcsncmp(bfr, L"USUS", 4) == 0)
							fwprintf(stream, L"%s %s\n", &bfr[2], pid);	// skip the leading "US"
						else
							fwprintf(stream, L"%s %s\n", bfr, pid);
					}
				}
				if (ch == L'\n' || ch == L'\r')
					return 0;
				else
				{
					tp = bfr;
					state = 2;
				}
			}
			else
				*tp++ = *sp;
			break;
		case 2:	// expecting the end of the delimeter string
			if (ch != L'$')
			{
				*tp++ = *sp;
				state = 1;
			}
			break;
		default:
			break;
		}
		sp++;
	}

	return 0;
}

//
// parse a given date string, find year and month information
//
int parse_date(wchar_t *str, int *year, int *month)
{
	int i, state;
	int issued;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[BUF_SIZE];

	sp = str; tp = bfr;
	state = 1;
	while (TRUE)
	{
		ch = *sp;
		switch (state)
		{
		case 1:	// looking for the 1st delimeter character '/'
			if (ch == L'/')
			{
				*tp = L'\0';
				*year = _wtoi(bfr);
				tp = bfr; 
				state = 2;
			}
			else
				*tp++ = *sp;
			break;
		case 2:	// expecting the 2nd delimeter character '/'
			if (ch == L'/')
			{
				*tp = L'\0';
				*month = _wtoi(bfr);
				return 0;
			}
			else
				*tp++ = *sp;
			break;
		default:
			break;
		}
		sp++;
	}

	return 0;
}

//
// Remove the ending "A1", "A", "B", "B1", "U", "U1", etc. at the end of "Foreign Reference" string
//
int remove_end_str(wchar_t *str)
{
	wchar_t *sp;

	if (*str == L'\0') return 0;

	sp = str;
	while (*sp != L'\0') sp++;	// move to the end of the string
	sp--;
	// check backward
	if (*sp == L'A' || *sp == L'B' || *sp == L'U' || *sp == L'E')
		*sp = '\0';
	else if (*sp == L'1' || *sp == L'2')
	{
		sp--;
		if (*sp == L'A' || *sp == L'B' || *sp == L'U' || *sp == L'E')
			*sp = L'\0';
	}

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_assignee(const void *n1, const void *n2)
{
	struct ASSIGNEES *t1, *t2;
	
	t1 = (struct ASSIGNEES *)n1;
	t2 = (struct ASSIGNEES *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_aalias(const void *n1, const void *n2)
{
	struct ANAME_ALIAS *t1, *t2;
	
	t1 = (struct ANAME_ALIAS *)n1;
	t2 = (struct ANAME_ALIAS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of an assignee name in an "ASSIGNEE" array
//
int asgnname_search(struct ASSIGNEES d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (num == 0) return -1;
	if (wcscmp(str, d[0].name) < 0) return -1;

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
// use binary search to find the proper position of an assignee name in an "ANAME_ALIAS" array
//
int aalias_search(struct ANAME_ALIAS d[], int num, wchar_t *str)
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

#ifdef FORMAT2	// the FORMAT2 code is not completed yet
int parse_USPTO_line(wchar_t *, int, int, wchar_t *);
int parse_USPTO_1st_line(wchar_t *, int, wchar_t *);
int parse_USPTO_author_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int get_USPTO_number_authors(wchar_t *);
int parse_store_inventors(wchar_t *, int *, struct AUTHORS *);
int compare_str(const void *, const void *);
int str_search(char **tr, int, char *);
int compare_inventor_name(const void *, const void *);
int compare_inventor(const void *, const void *);
int prepare_alias();
int inventor_name_search(struct AUTHORS *, int, wchar_t *);
int compare_patent_id(const void *, const void *);
int docid_search(struct WOS *, int, wchar_t *);
int alias_search(struct WOS *, int, wchar_t *);	
int link_author_USPTO();
int basic_USPTO_statistics();

//
// read WOS data and put it into the wos[] array
//
int read_USPTO(wchar_t *sname)
{
	int i, k, ndx;
	int nsr, cnt;
	int i_H02, i_H04, i_H11;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (_wfopen_s(&sstream, sname, L"r") != 0)
		return MSG_WOSFILE_NOTFOUND;

	// 1st pass, count the number of target records 
	nsr = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		//fwprintf(logstream, L"%s\n", line); fflush(logstream);
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nsr++;	
	}
	nuspto = nsr - 1;	// less the 1st line (format line)

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) 
		return MSG_WOSFILE_FORMAT_ERROR;
	// look for the index of the specified target field name 
	i_H02 = parse_USPTO_1st_line(line, SP_TAB, L"H02");	// patent ID
	i_H04 = parse_USPTO_1st_line(line, SP_TAB, L"H04");	// patent issue date
	i_H11 = parse_USPTO_1st_line(line, SP_TAB, L"H11");	// inventor names
	//if (i_H02 == -1 || i_H04 == -1 || i_H11)
	//	return MSG_WOSFILE_FORMAT_ERROR;

#ifdef XXX
	// 2nd pass, get keyword data and save it
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the author name array
	authors = (struct AUTHORS *)malloc(nwos * 8 * sizeof(struct AUTHORS));	// estimate in average 8 authors per document
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_WOS_line(line, SP_TAB, i_au, tfield);
		parse_store_authors(tfield, &naus, authors);
		//fwprintf(logstream, L"%d: %s\n", i, tfield); fflush(logstream);
		i++;	
	}
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_author);
	// consolidate duplicate author names
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < naus; i++)
	{
		if (wcscmp(authors[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) authors[k-1].np = cnt;
			wcscpy_s(authors[k++].name, MAX_AUTHOR_NAME, authors[i].name); 
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, authors[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	authors[k-1].np = cnt;
	naus = k;
#endif XXX

	// 3rd pass, get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) 
		return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of WOS data
	uspto = (struct USPTO *)malloc(nuspto * sizeof(struct USPTO));
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_USPTO_line(line, SP_TAB, i_H02, tfield);
		wcscpy_s(uspto[i].pid, MAX_DOC_ID, tfield);	
		//parse_WOS_line(line, SP_TAB, i_au, tfield);
		//wos[i].nau = get_number_authors(tfield);
		//parse_author_names(tfield, &wos[i].nau, wos[i].author, naus, authors);
#ifdef DEBUG
		for (k = 0; k < wos[i].nau; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, wos[i].nau, k, wos[i].author[k], authors[wos[i].author[k]].name); fflush(logstream); }
		fwprintf(logstream, L"\n");
#endif DEBUG
#ifdef XXX
		parse_WOS_line(line, SP_TAB, i_py, tfield);
		wos[i].year = _wtoi(tfield);
		//fwprintf(logstream, L"%d:%d %s[%s]\n", i, wos[i].year, wos[i].docid, wos[i].author);
		parse_WOS_line(line, SP_TAB, i_de, tfield);
		parse_keywords(tfield, &wos[i].nde, wos[i].DE, nkwde, kwde);
		parse_WOS_line(line, SP_TAB, i_id, tfield);
		parse_keywords(tfield, &wos[i].nid, wos[i].ID, nkwid, kwid);
		parse_WOS_line(line, SP_TAB, i_so, tfield);
		ndx = jname_search(journals, njrs, tfield);
		wos[i].journal = ndx;
		parse_WOS_line(line, SP_TAB, i_tc, tfield);
		wos[i].tc = _wtoi(tfield);
		parse_WOS_line(line, SP_TAB, i_sc, tfield);
		parse_disciplines(tfield, &wos[i].ndspln, wos[i].dspln, ndsplns, dsplns);
#endif XXX
		//fwprintf(logstream, L"%d:%s", i, wos[i].docid);
		//fwprintf(logstream, L"\n"); fflush(logstream);
		i++;	
	}
	fclose(sstream); 

#ifdef DEBUG
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%s: ", wos[i].docid);
		for (k = 0; k < wos[i].ndspln; k++)
			fwprintf(logstream, L"%s ", dsplns[wos[i].dspln[k]].name);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

#ifdef XXX
	prepare_alias();
	link_author_WOS();	
	link_journal_WOS();	
	coauthor();
	calculate_author_h_index();
	calculate_journal_h_index();
	basic_wos_statistics();

	// sort the data by ISI document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
#endif XXX

	return 0;
}

//
// parse a USPTO record prepared by WebPat
//
int parse_USPTO_line(wchar_t *tline, int separator, int tind, wchar_t *tfield)
{
	wchar_t ch, *sp, *tp, *fp;
	int state;
	int fcnt;

	sp = tline;
	// remove the leading spaces
	while (*sp == ' ') sp++;

	// start parsing
	tp = tmps; fp = tp;
	state = 0; fcnt = 0;
	while (*sp != '\0')
	{
		if (*sp == '\n' || *sp == '\r') 
		{ 
			*tp = '\0'; fcnt++;
			if (fcnt == tind)
			{
				wcscpy_s(tfield, LBUF_SIZE, fp);
				return fcnt;
			}
		}
		ch = *sp; 
		switch (separator)
		{
		case SP_TAB:
			if (ch == '\t') state = 1; break;
		case SP_SPACE:
			if (ch == ' ') state = 1; break;
		case SP_TAB_SPACE:
			if (ch == '\t' || ch == ' ') state = 1; break;
		case SP_SEMIC:
			if (ch == ';') state = 1; break;
		case SP_COLON:
			if (ch == ',') state = 1; break;
		case SP_FSLASH:
			if (ch == '/') state = 1; break;
		case SP_BSLASH:
			if (ch == '\\') state = 1; break;
		default:
			break;
		}
		if (state == 1) 
		{
			*tp++ = '\0'; fcnt++;
			if (fcnt == tind)
			{
				wcscpy_s(tfield, LBUF_SIZE, fp);
				return fcnt;
			}
			state = 0; fp = tp;
			sp++;
		}
		else 
			*tp++ = *sp++;
	}

	return -1;
}

//
// parse the 1st line of the USPTO data file
// the 1st line of USPTO data (provided by WebPat software)
// "H01 H02 ...... H26"
// They numbering is as follows
// H01  H02           H03      H04          H05    H06    H07      H08        H09        H10        H11        H12        H13        H14 H15 H16    H17        H18    H19      H20      H21        H22  H23            H24      H25      H26
// 編號 專利號/公開號 專利名稱 公告(公開)日 申請號 申請日 專利類型 申請人姓名 申請人地址 申請人國家 發明人姓名 發明人地址 發明人國家 IPC UPC 代理人 代理人地址 優先權 優先權號 優先權日 專利權範圍 摘要 美國引證專利號 他人引證 國外引證 Description	
//
int parse_USPTO_1st_line(wchar_t *tline, int separator, wchar_t *tfname)
{
	wchar_t ch, *sp, *tp, *fp;
	int state;
	int fcnt;

	sp = tline;
	// remove the leading spaces
	while (*sp == ' ') sp++;

	// start parsing
	tp = tmps; fp = tp;
	state = 0; fcnt = 0;
	while (*sp != '\0')
	{
		if (*sp == '\n' || *sp == '\r') 
		{ 
			*tp = '\0'; fcnt++;
			if (wcscmp(fp, tfname) == 0)
				return fcnt;
		}
		ch = *sp; 
		if (ch == ' ') break;	// ignore the leading spaces
		switch (separator)
		{
		case SP_TAB:
			if (ch == '\t') state = 1; break;
		case SP_SPACE:
			if (ch == ' ') state = 1; break;
		case SP_TAB_SPACE:
			if (ch == '\t' || ch == ' ') state = 1; break;
		case SP_SEMIC:
			if (ch == ';') state = 1; break;
		case SP_COLON:
			if (ch == ',') state = 1; break;
		case SP_FSLASH:
			if (ch == '/') state = 1; break;
		case SP_BSLASH:
			if (ch == '\\') state = 1; break;
		default:
			break;
		}
		if (state == 1) 
		{
			*tp++ = '\0'; fcnt++;
			if (wcscmp(fp, tfname) == 0)
				return fcnt;
			state = 0; fp = tp;
			sp++;
		}
		else 
			*tp++ = *sp++;
	}

	return -1;
}
#endif FORMAT2