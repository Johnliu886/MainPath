//
// select.cpp
//
// this file is created 2014/05/15.
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "network.h"

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1024
#define FNAME_SIZE 260

//
// function types
//
int compare_str(const void *, const void *);
int str_search2(wchar_t **tr, int, wchar_t *);

extern int index_of(wchar_t *, wchar_t *);
extern int WOS_or_Others(wchar_t *);
extern int read_WOS(wchar_t *sname);
extern int docid_search_nw(struct PN *, int, wchar_t *);
extern int parse_WOS_line(wchar_t *, int, int, wchar_t *);
extern int parse_WOS_1st_line(wchar_t *, int, wchar_t *);
extern int parse_Scopus_1st_line(wchar_t *, wchar_t *);
extern int parse_Scopus_line(wchar_t *, int, wchar_t *);
extern int create_Scopus_docid(wchar_t *, wchar_t, int, int, int);

//
// global variables
//
extern FILE *logstream;
extern int full_record_type;	// WOS, patent, etc.
extern int text_type;	// ASCII, UTFxx, etc.

//extern int nwos;
//extern struct WOS *wos;
extern int nnodes;
extern struct PN *nw;
extern int naus;
extern struct AUTHORS *authors;	// author name array

//
// read WOS files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
//
int readwrite_WOS(wchar_t *sname, wchar_t *oname, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield, *tf2;
	int ut_ind;
	int nw_ndx;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"w") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
		return -1;
	fwprintf(ostream, L"%s", line);		// write out the 1st line

	if (text_type == FT_UTF_8)
		ln = &line[3];	// that's 3 bytes
	else if (text_type == FT_UTF_16_BE || text_type == FT_UTF_16_LE)
		ln = &line[1];	// that's 2 bytes
	else if (text_type == FT_UTF_32_BE || text_type == FT_UTF_32_LE)
		ln = &line[1];	// that's 4 bytes
	else
		ln = line;

	ut_ind = parse_WOS_1st_line(ln, SP_TAB, L"UT");
	// read source file line by line
	// if the target record matches, write to the output file
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_WOS_line(line, SP_TAB, ut_ind, tfield);
		if (wcsncmp(tfield, L"ISI:", 4) == 0 || wcsncmp(tfield, L"WOS:", 4) == 0)	// remove "ISI:" or "WOS:"
			tf2 = &tfield[4];		
		else
			tf2 = tfield;
		nw_ndx = docid_search_nw(nw, nnodes, tf2);
		if (nw_ndx >= 0)
		{
			if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
			{
				fwprintf(ostream, L"%s", line); 
				nw[nw_ndx].flag = 1;	// raise flag
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read Scopus files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
//
int readwrite_Scopus(wchar_t *sname, wchar_t *oname, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield;
	int nw_ndx;
	int i_au, i_py, i_vl, i_bp;
	int year, volume, bpage;
	wchar_t docid[MAX_DOC_ID];

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"w") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read the 1st line of the source file, it contains the field names
	if (text_type == FT_ASCII)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = line;
	}
	else if (text_type == FT_UTF_8)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = &line[3];	// the fourth byte
		if (line[3] == 0xef && line[4] == 0xbb && line[5] == 0xbf)	// check if the UTF-8 mark repeat, this happens for Scopus csv data
			ln = &line[6];	// start from the 7th byte
	}
	else	// Unicode type
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = &line[1];	// actually the 3rd byte
	}

	fwprintf(ostream, L"%s", line);		// write out the 1st line

	i_au = parse_Scopus_1st_line(ln, L"Authors");	// author names
	i_py = parse_Scopus_1st_line(ln, L"Year");			// year of publication
	i_vl = parse_Scopus_1st_line(ln, L"Volume");	// volume
	i_bp = parse_Scopus_1st_line(ln, L"Page start");	// beginning page

	// read source file line by line
	// if the target record matches, write to the output file
	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_Scopus_line(line, i_py, tfield);	year = _wtoi(tfield);
		parse_Scopus_line(line, i_vl, tfield);	volume = _wtoi(tfield);
		parse_Scopus_line(line, i_bp, tfield);	bpage = _wtoi(tfield);
		if (iswdigit(tfield[0]) == 0 && tfield[0] != '\0')	// 1st character is not a digit
			bpage = _wtoi(tfield+1);	// skip the 1st character whether it's a 'A' or 'B', etc	// 2011/08/22
		else
			bpage = _wtoi(tfield);
		parse_Scopus_line(line, i_au, tfield);	
		if (wcsncmp(tfield, L"Penney K", 8) == 0)
		{
			i = i+1;
			fwprintf(logstream, L"herehere\n");
			nw_ndx = i;
		}
		create_Scopus_docid(docid, towlower(tfield[0]), year, volume, bpage);
		nw_ndx = docid_search_nw(nw, nnodes, docid);
		if (nw_ndx >= 0)
		{
			if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
			{
				fwprintf(ostream, L"%s", line); 
				nw[nw_ndx].flag = 1;	// raise flag
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// use binary search to find the proper position of a document id in a nw[] array
//
int docid_search_nw(struct PN d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].name) < 0)
		return -1;
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
