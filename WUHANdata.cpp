// 
// WUHANdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from ªZº~¤j¾Ç¡@¤ý¿P¬Â
//
//
// Revision History:
// 2013/02/28 Basic function works
// 2016/09/15 Modification   : added codes to handle the situation when a citation file is provided
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

static FILE *sstream;		// for source file
extern FILE *logstream;

extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern int nuspto;
extern struct USPTO *uspto;
extern int no_wos_citation_file;	// added 2016/09/15

int parse_WUHAN_1st_line(wchar_t *, int, wchar_t *);
int parse_WUHAN_line(wchar_t *, int, wchar_t *);
int parse_WUHAN_Patent_Ref(wchar_t *, wchar_t *, FILE *);
int parse_WUHAN_IPC(wchar_t *, int *, wchar_t *, int *);

extern int compare_pid(const void *, const void *);
extern int parse_publication_year(wchar_t *);
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int link_author_WOS();
extern int compare_docid(const void *, const void *);
extern int ipc_statistics(struct USPTO *, int);

#define XXLBUF_SIZE 65536*4	// there are extremely long texts in the "Cited Refs - Non-patent" field in the Thomson Innovatin data
#define BUF_SIZE 1024

//
// read WOS data and put it into the wos[] array
//
int read_WUHAN(wchar_t *sname)
{
	int i, k;
	int nsr;
	int i_ut, i_py, i_cr, i_ipc, i_an;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	FILE *cstream;		// for citation file
	wchar_t *line;
	wchar_t *tfield;

	// allocate the working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
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
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nsr++;	
	}
	nuspto = nsr - 1;	// less the 1st line (format lines)

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	

	// look for the index of the specified target field name 
	i_ut = parse_WUHAN_1st_line(line, SP_TAB, L"Patent No.");	// document ID
	i_py = parse_WUHAN_1st_line(line, SP_TAB, L"Issued Date");	// year of publication
	i_cr = parse_WUHAN_1st_line(line, SP_TAB, L"Direct Backward Pat.");	// this field actually contains "citing" information
	i_an = parse_WUHAN_1st_line(line, SP_TAB, L"Assignee");	// Assignee/Applicant
	i_ipc = parse_WUHAN_1st_line(line, SP_TAB, L"Intl. Class");	// internatioal class
	if (i_ut == -1 || i_py == -1 || i_cr == -1 || i_an == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	// allocate memory for the list of USPTO data
	uspto = (struct USPTO *)malloc(nuspto * sizeof(struct USPTO));
	if (uspto == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// open citation file
	if (no_wos_citation_file)	// this check is added 2016/09/15
	{
		strip_name_extension(sname, tname, dpath);
		if (swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname) == -1)
			return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"w") != 0)
			return MSG_CFILE_CANNOTOPEN;
	}

	// allocate memory for the author name array
	authors = (struct AUTHORS *)malloc(nuspto * 8 * sizeof(struct AUTHORS));	// estimate in average 8 authors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	for (i = 0; i < nuspto; i++)
	{
		swprintf_s(authors[i].name, MAX_AUTHOR_NAME, L"Anonymous%04d", i);
		authors[i].np = 1;
	}
	naus = nuspto;

	// get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line

	// read source file line by line
	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WUHAN_line(line, i_ut, tfield);
		wcscpy_s(uspto[i].pid, MAX_DOC_ID, tfield);
		uspto[i].ninventor = 1;
		uspto[i].inventor[0] = i;
#ifdef DEBUG
		for (k = 0; k < uspto[i].ninventor; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, uspto[i].ninventor, k, uspto[i].inventor[k], authors[uspto[i].inventor[k]].name); fflush(logstream); }
#endif DEBUG
		parse_WUHAN_line(line, i_py, tfield);
		if (*tfield == '\0')
			uspto[i].year = 1970;	// for those patent that does not have issue date information
		else
			uspto[i].year = parse_publication_year(tfield);
		//fwprintf(logstream, L"%d: %s %d\n", i, uspto[i].pid, uspto[i].year); fflush(logstream);
		parse_WUHAN_line(line, i_cr, tfield);
		parse_WUHAN_Patent_Ref(tfield, uspto[i].pid, cstream);
		parse_WUHAN_line(line, i_ipc, tfield);
		parse_WUHAN_IPC(tfield, &uspto[i].nipc, uspto[i].ipc, uspto[i].ipc_cnt);
		i++;	
	}

	fclose(sstream);	// close full record file
	if (no_wos_citation_file) fclose(cstream);	// close citation file (relationship file) if it is created


#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"%d: ", i);
		for (k = 0; k < uspto[i].nusc; k++)
			fwprintf(logstream, L"%s ", &uspto[i].usc[MAX_USC_CODE*k]);
		fwprintf(logstream, L"\n"); fflush(logstream);
	}
#endif DEBUG

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
		wos[i].year = uspto[i].year;
		wos[i].tc = 0;	// no such information in the database
	}

	// append the year information to the patent number as alias
	for (i = 0; i < nwos; i++)
		swprintf_s(wos[i].alias, MAX_ALIAS, L"%s_%d", wos[i].docid, wos[i].year);

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

	// ipc_statistics(uspto, nuspto);

	// sort the data by the patent id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	qsort((void *)uspto, (size_t)nuspto, sizeof(struct USPTO), compare_pid);	// added 2011/06/03

	free(line); free(tfield);

	return 0;
}

//
// parse the 1st line of the Thomson Innovation patent file
//
int parse_WUHAN_1st_line(wchar_t *line, int separator, wchar_t *tfname)
{
	wchar_t ch, *sp, *tp, *fp;
	wchar_t tmps[XXLBUF_SIZE];
	int state;
	int fcnt;

	sp = line;
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

//
// parse a data record prepared by Thomson Innovation
// this function handles the situation that some text are enclosed with quotes, some are not
//
int parse_WUHAN_line(wchar_t *line, int tind, wchar_t *tfield)
{
	wchar_t ch, *sp, *tp, *fp;
	int state;
	int fcnt;

	sp = line;

	// start parsing
	tp = tfield; fcnt = 0;
	// Note: some fields are enclosed with quotes, some are not
	state = 0;	// neutral state
	while (*sp != '\0')
	{
		if (*sp == '\n' || *sp == '\r') 
		{ 
			*tp = '\0'; fcnt++;
			if (fcnt == tind)
				return fcnt;			
			tp = tfield;
		}
		ch = *sp; 
		switch (state)
		{
		case 0: // neutral
			if (ch == '\t')	// this field is empty
			{
				*tp = '\0'; fcnt++;
				if (fcnt == tind)
					return fcnt;
				tp = tfield;
				sp++;	// skip this '\t'
			}
			else if (ch == '"')	// enclosed by quotes
			{
				sp++;
				state = 2;
			}
			else
			{
				*tp++ = *sp++;
				state = 1;
			}
			break;
		case 1:	// waiting for the delimiter, TAB
			if (ch == '\t') 
			{		
				*tp = '\0'; fcnt++;
				if (fcnt == tind)
					return fcnt;				
				tp = tfield;
				sp++;	// skip this '\t'
				state = 0; 
			}
			else
				*tp++ = *sp++;
			break;
		case 2:	// waiting for the closing quote '"'
			if (ch == '"') 
			{	
				*tp = '\0'; fcnt++;
				if (fcnt == tind)
					return fcnt; 				
				tp = tfield;
				sp++; sp++;	// skip the '"\t' 
				state = 0; 
			}
			else
				*tp++ = *sp++;
			break;
		default:
			break;
		}
	}
	*tp = '\0'; fcnt++;	// for the case of no carriage return or line feed at the end of a file
	if (fcnt == tind)	
		return fcnt;	

	return -1;
}

//
// parse the "Direct Backward Pat." field string, citations are delimeted by space
//
int parse_WUHAN_Patent_Ref(wchar_t *str, wchar_t *pid, FILE *stream)
{
	int i, state;
	int cnt;
	int issued;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[LBUF_SIZE];

	if (!no_wos_citation_file)	// added 2016/09/15
		return 0;

	sp = str; tp = bfr;
	state = 1;
	while (TRUE)
	{
		ch = *sp;
		switch (state)
		{
		case 1:	// looking for the delimeter string
			if (ch == L' '|| ch == L'\n' || ch == L'\r' || ch == L'\0')
			{
				*tp = L'\0';
				tp = bfr; issued = TRUE;
				cnt = 0;
				while (*tp != L'\0') // check if the patent number is in the format of an issued patent
				{ tp++; cnt++; }
				if (cnt > 7) issued = FALSE; 	// total up to 7 characters for standard US patents in this database
				if (issued && cnt != 0)
				{
					fwprintf(stream, L"%s %s\n", bfr, pid);	// write the results
					fflush(stream);
				}
				if (ch == L'\n' || ch == L'\r' || ch == L'\0')
					return 0;
				else
				{
					tp = bfr;
					state = 2;
				}
			}
			else
			{
				if (ch != L' ')	// skip the space at the end of the patent number
					*tp++ = *sp;
			}
			break;
		case 2:	// expecting a space after the delimeter 
			if (ch != L' ')
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
// parse the "Intl. Class" field string, each is delimited by " "
//
int parse_WUHAN_IPC(wchar_t *str, int *nipc, wchar_t *ipc_code, int *ipc_code_cnt)
{
	int i, state;
	int cnt;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[LBUF_SIZE];

	if (*str == L'\0')
	{
		*nipc = 0;
		return 0;
	}
	sp = str; tp = bfr;
	state = 1; cnt = 0;
	while (TRUE)
	{
		ch = *sp;
		switch (state)
		{
		case 1:	// looking for the delimeter string
			if (ch == L' '|| ch == L'\n' || ch == L'\r' || ch == L'\0')
			{
				*tp = L'\0';
				tp = bfr;
				ipc_code_cnt[cnt] = 1;	// 1 for every IPC code, added 2016/05/26
				wcscpy_s(&ipc_code[MAX_IPC_CODE*cnt++], MAX_IPC_CODE*sizeof(wchar_t), bfr);
				if (ch == L'\n' || ch == L'\r' || ch == L'\0')
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
			{
				if (ch != L' ')	// skip the space at the end of the patent number
					*tp++ = *sp;
			}
			break;
		case 2:	// expecting a space after the delimeter 
			if (ch != L' ')
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
// parse the "UPC" field string, each is delimited by "|"
//
int parse_WUHAN_UPC(wchar_t *str, int *nusc, wchar_t *usc_code)
{
	int i, state;
	int cnt;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[LBUF_SIZE];

	if (*str == L'\0')
	{
		*nusc = 0;
		return 0;
	}
	sp = str; tp = bfr;
	state = 1; cnt = 0;
	while (TRUE)
	{
		ch = *sp;
		switch (state)
		{
		case 1:	// looking for the delimeter string
			if (ch == L'|'|| ch == L'\n' || ch == L'\r' || ch == L'\0')
			{
				*tp = L'\0';
				tp = bfr;
				wcscpy_s(&usc_code[MAX_USC_CODE*cnt++], MAX_USC_CODE*sizeof(wchar_t), bfr);
				if (ch == L'\n' || ch == L'\r' || ch == L'\0')
				{
					*nusc = cnt;
					return 0;
				}
				else
				{
					tp = bfr;
					state = 2;
				}
			}
			else
			{
				if (ch != L' ')	// skip the space at the end of the patent number
					*tp++ = *sp;
			}
			break;
		case 2:	// expecting a space after the delimeter 
			if (ch != L' ')
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

