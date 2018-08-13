// 
// TIPdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the Thomson Innovation Patent database_
//
//
// Revision History:
// 2011/04/16 Basic function works
// 2012/09/12 Added function : added codes to read and save assignee names, it also reads "Assignee alias.txt" file 
// 2012/09/12 Modification   : added codes to handle the case of two continuous double quotes in a TIP data sequence
// 2013/07/26 Fixed problems : added to call rewind() in the second pass of processing assignee names
// 2014/04/04 Fixed problems : change the memory allocation size for asignees (from 1.5 times to 2.5 times of record number)
// 2014/07/23 Added function : implemented title and keyword summary function
// 2014/04/08 Modification   : removed the check for "issued" patent, the check is apparently not correct, ignore for now!
// 2016/07/02 Modification   : moved to call prepare_keyword_alias_list() before calling parse_store_keywords()	
// 2016/09/15 Modification   : added codes to handle the situation when a citation file is provided
// 2016/12/17 Added function : added a block of codes to prepare an University-Industry relation file
// 2016/12/18 Added function : added codes to add application year to the patent label
// 2016/12/24 Added function : added to call coassignee() function
// 2017/01/03 Modification   : fixed various problems related university-industry project, mostly on country and area identification
// 2017/01/04 Added function : added TIP_assignee_correction() funcion to correct the problems in the assignee field of the TIP data
// 2017/01/10 Fixed problems : added codes to convert assignee name and its alias to lower case after reading in the assignee alias file
// 2017/02/25 Modification   : moved the codes that call coassignee() to a place in text2Pajek.txt (where the "location" information are set more properly)
// 2017/02/25 Fixed problems : corrected codes that does not handle multi-lingual languages
// 2018/01/19 Added function : added to accept "Inventor First", if the "Inventor" column does not exit
// 2018/01/19 Modification   : brought back parsing keywords and title (under the condition that abstract is provided)
// 2018/01/19 Fixed problem  : changed the usage of stack in the function parse_TIP_2nd_line(), tmps[BUF_SIZE] changed from XXLBUF_SIZE to BUF_SIZE
// 2018/05/02 Fixed problem  : added check for the return of keyword_year_matrix()
// 2018/05/03 Fixed problem  : extend the buffer size inside the function parse_TIP_2nd_line() from BUF_SIZE to LBUF_SIZE
// 2018/05/08 Added function : added codes to read and process 'patent family' data, 
//                                a new column 'FA' needs to be added manually to indicate those patents that are in the same loop (treat them as a family)
// 2018/06/06 Fixed problems : modified the Paper/patent family¡¨memory allocation scheme. This is so that one can assign many families, even thousands of them
// 2018/06/20 Modification   : added the conditional compilation definition "PROCESS_COUNTRY_DATA", turn off for those who do not need country information
//                                   this is because assignee address can cause unknown problems
// 2018/06/23 Fixed problems : added check to avoid errors from strnage assignee data (make sure wos[i].DE[k] >= 0)
// 2018/06/27 Modification   : added "Assignee/Applicant" as the alternative of "Assignee - Original w/address", they seem to contain same information
// 2018/06/27 Modification   : added the check "nuspto < 2000" before processing keywords for abstract
// 2018/08/09 Fixed problems : changed MAX_AUTHORS to MAX_INVENTORS (former is much bigger than the later)
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

int check_UI = 0;
static FILE *sstream;		// for source file
extern FILE *logstream;

extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern int nuspto;
extern struct USPTO *uspto;
extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;
extern int naalias;	// number of items in the assignee alias table
extern struct ANAME_ALIAS *aalias;
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int ntkeywords;
extern int no_wos_citation_file;	// added 2016/09/15

extern int nsds;					// added 2018/05/08
extern struct SERIALDOCS *serialdocs;	// added 2018/05/08

int parse_TIP_2nd_line(wchar_t *, int, wchar_t *);
int parse_TIP_line(wchar_t *, int, wchar_t *);
int parse_Citing_Patents(wchar_t *, wchar_t *, FILE *);
int parse_IPC_Current(wchar_t *, int *, wchar_t *, int *);
int parse_US_Class(wchar_t *, int *, wchar_t *);
int compare_author(const void *, const void *);
int parse_publication_year(wchar_t *);
int compare_pid(const void *, const void *);
int link_assignee_WOS();
int relink_assignee_WOS();
int UI_check(int, struct USPTO *);
int find_significant_country_patent(int, struct USPTO *);
int find_significant_area_patent(int, struct USPTO *);
int TIP_assignee_correction(wchar_t *, int, int *);

//int parse_store_inventor_first(wchar_t *, int *, struct AUTHORS *);
//int parse_inventor_first_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_TIP_store_inventors(wchar_t *, int *, struct AUTHORS *);
int parse_TIP_inventor_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_TIP_store_assignees(wchar_t *, int *, struct ASSIGNEES *);
int parse_TIP_assignee_names(wchar_t *, int *, int *, int, struct ASSIGNEES *);

extern int consolidate_countries_assignee(int, struct ASSIGNEES *);
extern int consolidate_countries_patent_TIP(int, struct USPTO *);
extern int consolidate_areas_patent_TIP(int, struct USPTO *);
extern int prepare_author_alias_list();
extern int prepare_keyword_alias_list();
extern int is_stopwords(wchar_t *);
extern int compare_tkeyword(const void *, const void *);
extern int parse_title(wchar_t *, int, int *, int *, int, struct TKEYWORDS *);	// added 2014/07/23
extern int parse_store_title(int, struct TKEYWORDS *, wchar_t *);	// added 2014/07/23
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int link_author_WOS();
extern int relink_author_WOS();
extern int compare_docid(const void *, const void *);
extern int compare_aalias(const void *, const void *);
extern int parse_aalias(wchar_t *, wchar_t *, wchar_t *);
extern int aalias_search(struct ANAME_ALIAS*, int, wchar_t *);
extern int compare_assignee(const void *, const void *);
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int asgnname_search(struct ASSIGNEES *, int, wchar_t *);
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int is_university(wchar_t *);
extern int parse_countries_TIP(int, wchar_t *, int, struct ASSIGNEES *);
extern int readwrite_UI_TIP(wchar_t *, wchar_t *, struct USPTO *, FILE *);
extern int is_university(wchar_t *);
extern int index_of2(int, wchar_t *, wchar_t *);	// this function does not do the separate word check
extern int compare_serialdocid(const void *, const void *);

#define XXLBUF_SIZE 65536*6	// there are extremely long texts in the "Cited Refs - Non-patent" and "Claims" field in the Thomson Innovatin data
#define BUF_SIZE 1024

//
// read WOS data and put it into the wos[] array
//
int read_TIP(wchar_t *sname)
{
	int i, k, m, ndx;
	int nsr, cnt;
	int ret;
	int i_ut, i_au, i_py, i_pm, i_tc, i_cr, i_ipc, i_usc, i_an, i_ans, i_ti, i_ab, i_c1;
	int i_fa;			// added 2018/05/08
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t tmps[BUF_SIZE], *sp;
	FILE *cstream;		// for citation file
	wchar_t *line;
	wchar_t *tfield;
	FILE *astream;		// for assignee alias file

	// allocate the working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)
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
	nuspto = nsr - 2;	// less the 1st 2 lines (format lines)

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, ignore it
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line
	// look for the index of the specified target field name 
	i_ut = parse_TIP_2nd_line(line, SP_COLON, L"Publication Number");	// document ID
	i_au = parse_TIP_2nd_line(line, SP_COLON, L"Inventor");	// inventor names
	if (i_au == -1)
		i_au = parse_TIP_2nd_line(line, SP_COLON, L"Inventor First");	// 1st inventor name, added 2018/01/19
	i_py = parse_TIP_2nd_line(line, SP_COLON, L"Publication Date");	// date of publication
	i_pm = parse_TIP_2nd_line(line, SP_COLON, L"Application Date");	// date of application
	i_tc = parse_TIP_2nd_line(line, SP_COLON, L"Count of Citing Patents");	// total citation
	i_cr = parse_TIP_2nd_line(line, SP_COLON, L"Citing Patents");	// this field actually contains "cited-by" information
	i_ipc = parse_TIP_2nd_line(line, SP_COLON, L"IPC - Current");	// IPC code
	i_usc = parse_TIP_2nd_line(line, SP_COLON, L"US Class");	// US Class code 
	i_an = parse_TIP_2nd_line(line, SP_COLON, L"Assignee/Applicant");	// Assignee/Applicant
	i_ans = parse_TIP_2nd_line(line, SP_COLON, L"Assignee/Applicant - NTUST");	// Assignee, correctd using the inhouse software
	i_ti = parse_TIP_2nd_line(line, SP_COLON, L"Title - DWPI");		// title rewritten by Thompson, added 2014/07/22
	i_ab = parse_TIP_2nd_line(line, SP_COLON, L"Abstract - DWPI");	// abstract rewritten by Thompson, added 2014/07/22
	i_c1 = parse_TIP_2nd_line(line, SP_COLON, L"Assignee - Original w/address");		//  address, added 2016/12/21
	if (i_c1 == -1)	// added 2018/06/27
		i_c1 = parse_TIP_2nd_line(line, SP_COLON, L"Assignee/Applicant");	// NOTE: "Assignee - Original w/address" and "Assignee/Applicant" seem to contain same info.
	i_fa = parse_TIP_2nd_line(line, SP_COLON, L"FA");		// paper family, added 2018/05/08
	if (i_ut == -1 || i_au == -1 || i_py == -1 || i_pm == -1 || i_tc == -1 || i_cr == -1 || i_ipc == -1 || i_usc == -1 || i_an == -1 || i_c1 == -1)
		return MSG_WOSFILE_FORMAT_ERROR;
	
	prepare_author_alias_list();	// this is so that we can handle various form of an author's name, added 2016/12/20
	prepare_keyword_alias_list();	// this is so that we can handle various form of a keyword, moved up to here 2016/07/02
	// allocate memory for the list of USPTO data
	uspto = (struct USPTO *)malloc(nuspto * sizeof(struct USPTO));
	if (uspto == NULL) return MSG_NOT_ENOUGH_MEMORY;

	strip_name_extension(sname, tname, dpath);
	// open citation file
	if (no_wos_citation_file)	// this check is added 2016/09/15
	{
		if (swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname) == -1)
			return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"w") != 0)
			return MSG_CFILE_CANNOTOPEN;
	}

	// open the "Assignee alias.txt" file, ignore if it does not exist
	if (_wfopen_s(&astream, L"Assignee alias.txt", L"rt, ccs=UTF-8") != 0) // open as text mode for UTF-8 type	
	{
		fwprintf(logstream, L"\nWARNING: Can not find file: Assignee alias.txt.\n");
		naalias = 0;
	}
	else
	{
		fwprintf(logstream, L"\nFound and read file: Assignee alias.txt.\n");
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
		aalias = (struct ANAME_ALIAS *)malloc(naalias * sizeof(struct ANAME_ALIAS));
		if (aalias == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(astream);	// point back to the begining of the assignee alias file
		naalias = 0;		// added 2013/07/26
		while (TRUE)
		{
			if(fgetws(tmps, BUF_SIZE, astream) == NULL)
				break;
			if (tmps[0] == '\n' || tmps[0] == '\r')
				continue;
			parse_aalias(tmps, aalias[naalias].name, aalias[naalias].alias);
			naalias++;		// added 2013/07/26
		}
		for (i = 0; i < naalias; i++)	// change all text to lower case, added 2017/01/10
		{
			sp = aalias[i].name; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
			sp = aalias[i].alias; while (*sp != '\0') { *sp = towlower(*sp); sp++; } 
		}
		// sort the table by the original name
		qsort((void *)aalias, (size_t)naalias, sizeof(struct ANAME_ALIAS), compare_aalias);		
		fwprintf(logstream, L"Assignee alias table\n");	// added 2016/12/17
		for (i = 0; i < naalias; i++)
			fwprintf(logstream, L"[%s]=>[%s]\n", aalias[i].name, aalias[i].alias);
	}

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, ignore it
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line
	// allocate memory for the author name array
	authors = (struct AUTHORS *)malloc(nuspto * 8 * sizeof(struct AUTHORS));	// estimate in average 8 authors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// allocate memory for the assignee name array
	assignees = (struct ASSIGNEES *)malloc((int)(nuspto * 5) * sizeof(struct ASSIGNEES));	// estimate in average 5 assignees per document, increased to 5, 2016/12/17
	if (assignees == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TIP_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		parse_TIP_store_inventors(tfield, &naus, authors);
		//fwprintf(logstream, L"%d: %s\n", i, tfield); fflush(logstream);
		if (i_ans != -1)	//  if a good assignee list is provided, use it
			parse_TIP_line(line, i_ans, tfield);
		else
			parse_TIP_line(line, i_an, tfield);
		//fwprintf(logstream, L"%d: %s\n", i, tfield); fflush(logstream);
		parse_TIP_store_assignees(tfield, &nasgns, assignees);
		i++;	
	}
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_author);
	// consolidate duplicate inventor names
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

	fwprintf(logstream, L"Number of unique assignees: %d\n", nasgns);

#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		fwprintf(logstream, L"#####%d [%s]\n", i+1, authors[i].name);
		fflush(logstream);
	}
	for (i = 0; i < nasgns; i++)
	{
		fwprintf(logstream, L"@@@@@%d [%s]\n", i+1, assignees[i].name);
		fflush(logstream);
	}
#endif DEBUG

// ================================
// following are the codes for the University-Industry Project, added 2016/12/17
// the codes select only the patents that contain universities and industrial companies as assignees or applicants
// it then writes out the collaborating U-I into the "... U-I relations" file
//
#ifdef UNIVERSITY_INDUSRY_PROJECT
	// get required data
	rewind(sstream);	// point back to the begining of the file
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, "Thomson Innovation Patent Export ....", ignore it
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line
	// read source file line by line
	i = 0;
	while (TRUE)
	{	
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;	
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TIP_line(line, i_ut, tfield);
		wcscpy_s(uspto[i].pid, MAX_DOC_ID, tfield);
		//fwprintf(logstream, L"%d: %s\n", i, uspto[i].pid); fflush(logstream);	
		parse_TIP_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		//fwprintf(logstream, L"$$$$$ %d %s\n", i, uspto[i].pid );
		parse_TIP_inventor_names(tfield, &uspto[i].ninventor, uspto[i].inventor, naus, authors);
#ifdef DEBUG
		for (k = 0; k < uspto[i].ninventor; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i+1, uspto[i].ninventor, k, uspto[i].inventor[k], authors[uspto[i].inventor[k]].name); fflush(logstream); }
#endif DEBUG
		parse_TIP_line(line, i_py, tfield);
		uspto[i].year = parse_publication_year(tfield);
		parse_TIP_line(line, i_py, tfield);
		//fwprintf(logstream, L"%d: %d\n", i, uspto[i].year); fflush(logstream);
		parse_TIP_line(line, i_tc, tfield);
		uspto[i].tc = _wtoi(tfield);	// added 2017/03/16
		if (i_ans != -1)	// if a good assignee list is provided, use it
			parse_TIP_line(line, i_ans, tfield);
		else
			parse_TIP_line(line, i_an, tfield);
		//fwprintf(logstream, L"##### %d [%s]\n", i, tfield); fflush(logstream);
		TIP_assignee_correction(tfield, uspto[i].ninventor, uspto[i].inventor);	// this function removes inventors in the assignee string, added 2017/01/04
		//fwprintf(logstream, L"!!!!! %d [%s]\n", i, tfield); fflush(logstream);
		parse_TIP_assignee_names(tfield, &uspto[i].nassignee, uspto[i].assignee, nasgns, assignees);
		i++;	
	}

	// some of the assignee field has duplicated assignee names in the Thomson Innovation database
	// following codes check for duplicate assignees and remove them from the uspto[].assignee[] array
	int tmpasgn[MAX_ASSIGNEES], tmpn, dup;
	for (i = 0; i < nuspto; i++)
	{
		tmpn = uspto[i].nassignee;
		for (k = 0; k < tmpn; k++) tmpasgn[k] = uspto[i].assignee[k];
		uspto[i].nassignee = 1;
		for (k = 1; k < tmpn; k++)
		{
			dup = 0;
			for (m = 0; m < uspto[i].nassignee; m++)
			{
				if (tmpasgn[k] == uspto[i].assignee[m])
					dup = 1;
			}
			if (dup == 0)	// not a duplicate, take it
			{
				uspto[i].assignee[uspto[i].nassignee] = tmpasgn[k];
				uspto[i].nassignee++;
			}
		}

	}
#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"@@@@@ %d\t%s\t%d\t%d\n", i, uspto[i].pid, uspto[i].year, uspto[i].nassignee);
		for (k = 0; k < uspto[i].nassignee; k++)
			fwprintf(logstream, L"\t[%s]", assignees[uspto[i].assignee[k]].name);
		fwprintf(logstream, L"\n"); fflush(logstream);
	}
#endif DEBUG

	wchar_t uiname[FNAME_SIZE];
	wchar_t uiname2[FNAME_SIZE];
	FILE *uistream;		// for output file
	FILE *uistream2;		// for output file
	int j, aj, ak;
	int UI, UU;

	swprintf_s(uiname, FNAME_SIZE, L"%s U-I relations.txt", tname);
	if (_wfopen_s(&uistream, uiname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;
	swprintf_s(uiname2, FNAME_SIZE, L"%s U-I relations 2.txt", tname);
	if (_wfopen_s(&uistream2, uiname2, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nuspto; i++) uspto[i].UI = 0;

	for (i = 0; i < nuspto; i++)
	{
		if (uspto[i].nassignee >= 2)
		{
			//fwprintf(logstream, L"##### %d %s\n", i, uspto[i].pid);
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
						fwprintf(uistream, L"%s\t%s\t1\n", assignees[aj].name, assignees[ak].name);
						fwprintf(uistream2, L"U-I\t%s\t%s\t%s\t1\n", uspto[i].pid, assignees[aj].name, assignees[ak].name);
					}
					else if (is_university(assignees[aj].name) == 0 && is_university(assignees[ak].name) == 1)
					{
						UI = 1;
						fwprintf(uistream, L"%s\t%s\t1\n", assignees[ak].name, assignees[aj].name);
						fwprintf(uistream2, L"U-I\t%s\t%s\t%s\t1\n", uspto[i].pid, assignees[ak].name, assignees[aj].name);
					}
					else if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 1)
					{
						UU = 1;
						fwprintf(uistream, L"%s\t%s\t1\n", assignees[aj].name, assignees[ak].name);
						fwprintf(uistream2, L"U-U\t%s\t%s\t%s\t1\n", uspto[i].pid, assignees[aj].name, assignees[ak].name);
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
	fclose(uistream);

	// prepare a file that contains only patents with university-industry collaboration
	wchar_t oname[FNAME_SIZE];
	swprintf_s(oname, FNAME_SIZE, L"%s U-I.csv", tname);
	// NOTE: a cleaned assignee string is added as the 1st filed in the new data file
	readwrite_UI_TIP(sname, oname, uspto, sstream);

	// the following block of codes is added 2017/03/16
	fwprintf(logstream, L"\n============= Patent Information ================\n");
	fwprintf(logstream, L"\n\tPatent_ID\tPubYear\tUU\tUI\t\UUI\tTC\n");
	for (i = 0; i < nuspto; i++)	
	{
		int UU, UI, UUI;
		UU = UI = UUI = 0;
		if (uspto[i].UI == UNIVERSITY_UNIVERSITY) UU = 1;
		else if (uspto[i].UI == UNIVERSITY_INDUSTRY) UI = 1;
		else if (uspto[i].UI == UNIVERSITY_UNIVERSITY_INDUSTRY) UUI = 1;
		fwprintf(logstream, L"%d\t%s\t%d\t%d\t%d\t%d\t%d\n", 
			i+1, uspto[i].pid, uspto[i].year, UU, UI, UUI, uspto[i].tc);
	}


	fwprintf(logstream, L"\n============= Assignee Information ================\n");
	fwprintf(logstream, 
		L"\nAssignees\tUniversity\tNPatents\n");
	for (i = 0; i < nasgns; i++)	// find the number of collaborating firms and universities
	{	
		int is_univ;
		is_univ = is_university(assignees[i].name);
		fwprintf(logstream, L"%s\t%d\t%d\n", 
			assignees[i].name, is_univ,
			assignees[i].np
			);
	}

	exit(0);
#endif UNIVERSITY_INDUSRY_PROJECT
// ===============================

//#ifdef TESTXXX
	// obtain and process article title information, added 2014/07/22
	struct PTITLES *ptitle;	// paper titles
	rewind(sstream);	// point back to the begining of the file
	// read the 1st two lines of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, ignore it
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line, ignore it
	// allocate memory for the title keyword array
	ptitle = (struct PTITLES *)Jmalloc(nuspto * sizeof(struct PTITLES), L"read_TIP: ptitle");
	if (ptitle == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TIP_line(line, i_ti, tfield);
		wcscpy_s(ptitle[i].name, MAX_TITLES, tfield);
		parse_TIP_line(line, i_py, tfield);
		ptitle[i].year = parse_publication_year(tfield);
		//fwprintf(logstream, L"[%d %d %s]\n", i, ptitle[i].year, ptitle[i].name); fflush(logstream);
		i++;	
	}

	// assume in average each title contains 200 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nuspto * 200 * sizeof(struct TKEYWORDS), L"read_TIP: tkeyword");
	if (tkeyword == NULL) return MSG_NOT_ENOUGH_MEMORY;
	ndx = 0;
	for (i = 0; i < nuspto; i++)
	{
		//fwprintf(logstream, L"[%s]\n", ptitle[i].name);
		ndx = parse_store_title(ndx, tkeyword, ptitle[i].name);
	}
	ntkeywords = ndx;
	fwprintf(logstream, L"***** 1st stage ntkeywords=%d\n", ntkeywords); fflush(logstream);
	qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_tkeyword);
	// consolidate duplicate title keywords names
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < ntkeywords; i++)
	{
		if (!is_stopwords(tkeyword[i].name) && wcscmp(tkeyword[i].name, L"s") != 0 && wcscmp(tkeyword[i].name, L"r") != 0 && wcscmp(tkeyword[i].name, L"d") != 0)	// simply ignore the stopwords and other special cases
		{
			if (wcscmp(tkeyword[i].name, prev_name) != 0)	// hit a new name
			{
				if (k > 0) tkeyword[k-1].cnt = cnt;
				wcscpy_s(tkeyword[k++].name, MAX_TKEYWORDS, tkeyword[i].name); 
				wcscpy_s(prev_name, MAX_TKEYWORDS, tkeyword[i].name); 
				cnt = 1;
			}
			else
				cnt++;
		}
	}
	tkeyword[k-1].cnt = cnt;
	ntkeywords = k;
	fwprintf(logstream, L"***** 2nd stage ntkeywords=%d\n", ntkeywords); fflush(logstream);
	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = i; } 

#ifdef DEBUG
	for (i = 0; i < ntkeywords; i++)
		fwprintf(logstream, L"%d %03d [%s]\n", i, tkeyword[i].cnt, tkeyword[i].name);
#endif DEBUG
	free(ptitle);	// added 2013/08/19
//#endif TESTXXX

	// get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, ignore it
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line

	// read source file line by line
	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TIP_line(line, i_ut, tfield);
		wcscpy_s(uspto[i].pid, MAX_DOC_ID, tfield);	
		parse_TIP_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		parse_TIP_inventor_names(tfield, &uspto[i].ninventor, uspto[i].inventor, naus, authors);
#ifdef DEBUG
		for (k = 0; k < uspto[i].ninventor; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, uspto[i].ninventor, k, uspto[i].inventor[k], authors[uspto[i].inventor[k]].name); fflush(logstream); }
#endif DEBUG
		parse_TIP_line(line, i_py, tfield);
		uspto[i].year = parse_publication_year(tfield);
		//fwprintf(logstream, L"##### %d: %d\n", i, uspto[i].year); fflush(logstream);
		parse_TIP_line(line, i_pm, tfield);
		uspto[i].a_year = parse_publication_year(tfield);
		parse_TIP_line(line, i_tc, tfield);
		uspto[i].tc = _wtoi(tfield);
		parse_TIP_line(line, i_cr, tfield);
		parse_Citing_Patents(tfield, uspto[i].pid, cstream);
		parse_TIP_line(line, i_ipc, tfield);
		parse_IPC_Current(tfield, &uspto[i].nipc, uspto[i].ipc, uspto[i].ipc_cnt);
		parse_TIP_line(line, i_usc, tfield);
		parse_US_Class(tfield, &uspto[i].nusc, uspto[i].usc);
		if (i_ans != -1)	// if a good assignee list is provided, use it
			parse_TIP_line(line, i_ans, tfield);
		else
			parse_TIP_line(line, i_an, tfield);
		parse_TIP_assignee_names(tfield, &uspto[i].nassignee, uspto[i].assignee, nasgns, assignees);
		parse_TIP_line(line, i_ti, tfield);		// added 2014/07/23
		parse_title(tfield, 0, &uspto[i].ntkws, uspto[i].tkws, ntkeywords, tkeyword);	// added 2014/07/23
		if (i_ab != -1 && nuspto < 2000)	// added 2018/01/19, added the condition "nuspto < 2000" 2018/06/27
		{
			parse_TIP_line(line, i_ab, tfield);		// parse abstract, commented out 2016/12/31, because it will need a lot of memory, put it back 2018/01/19
			parse_title(tfield, uspto[i].ntkws, &uspto[i].ntkws, uspto[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array, added 2014/07/23
		}
		i++;	
	}

#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"*****%d %d %s\n", i+1, uspto[i].ntkws, uspto[i].pid); fflush(logstream);
	}
#endif DEBUG

#ifdef UNIVERSITY_INDUSRY_PROJECT
	UI_check(nuspto, uspto);	// set university industry collaboration flag, added 2016/12/28
#endif UNIVERSITY_INDUSRY_PROJECT

#ifdef PROCESS_COUNTRY_DATA	// added 2018/06/20
	// following block that process country data is added 2016/12/21
	// parse country data, do it seperately because it needs complete assignee data
	rewind(sstream);	// point back to the begining of the file
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, ignore it
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line
	for (i = 0; i < nwos; i++) { uspto[i].ncountries = 0; } // initialization
	for (i = 0; i < nasgns; i++) 
	{ 
		assignees[i].ncountries = 0;
		assignees[i].nareas = 0; 
		assignees[i].groupid = 0;		// added 2017/03/25	
		assignees[i].ndx = i;			// added 2017/03/25
	}

	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (i_ans != -1)	//  if a good assignee list is provided, use it
			parse_TIP_line(line, i_ans, tfield);
		else
			parse_TIP_line(line, i_an, tfield);
fwprintf(logstream, L"##### %d %s [%s]\n", i, uspto[i].pid, tfield); fflush(logstream);
		parse_countries_TIP(i, tfield, nasgns, assignees);
		i++;	
	}

#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"!!!!! %d %s\t", i, uspto[i].pid);
		for (k = 0; k < uspto[i].ncountries; k++)
			fwprintf(logstream, L"%d ", uspto[i].countries[k]);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG
#ifdef DEBUG
	for (i = 0; i < nasgns; i++)
	{
		fwprintf(logstream, L"$$$$$ %d %s\t", i, assignees[i].name);
		for (k = 0; k < assignees[i].nareas; k++)
			fwprintf(logstream, L"%d ", assignees[i].areas[k]);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	consolidate_countries_patent_TIP(nuspto, uspto);
	find_significant_country_patent(nuspto, uspto);
	consolidate_areas_patent_TIP(nuspto, uspto);
	find_significant_area_patent(nuspto, uspto);
#endif PROCESS_COUNTRY_DATA

	// following code block that process 'patent family' (serialdoc) data is added 2018/05/08
	// yet another pass, get 'patent family' data, the column 'FA' needs to be added manually to indicate those data that has loop
	rewind(sstream);	// point back to the begining of the file
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line, ignore it (Derwent identification)
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line, ignore it again (column label)
	// allocate memory for the list of serial document data
	//int mest = 1000;
	//if ((nuspto / 10) < 1000) mest = 1000; 
	serialdocs = (struct SERIALDOCS *)Jmalloc(nuspto * sizeof(struct SERIALDOCS), L"read_TIP: serialdocs");	// modified 2018/06/06
	if (serialdocs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	i = 0; nsds = 0; 
	wchar_t *xret;
	while (TRUE)
	{		
		xret = fgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TIP_line(line, i_fa, tfield);
		if (_wtoi(tfield) != 0)	// it is a member of one of the serial document set
		{
			//fwprintf(logstream, L"Paper family: %s [%s]\n", tfield, uspto[i].pid);
			wcscpy(serialdocs[nsds].sdocid, tfield);
			wcscpy(serialdocs[nsds].docname[0], uspto[i].pid);
			serialdocs[nsds].ndx[0] = i;	// this will changed later to pointing to nw[] (the codes is in text2Pajek.cpp)
			nsds++;
		}
		i++;	
		if (xret == NULL)
			break;
	}
	qsort((void *)serialdocs, (size_t)nsds, sizeof(struct SERIALDOCS), compare_serialdocid);

#ifdef DEBUG
	for (i = 0; i < nsds; i++)
	{
		fwprintf(logstream, L"@@@@@ [%s] %d %s\n", serialdocs[i].sdocid, serialdocs[i].ndx[0], serialdocs[i].docname[0]);
	}
#endif DEBUG

	// consolidate the same patent family into one serialdocs structure
	if (nsds > 0)	
	{
		prev_name[0] = '\0';
		k = 0; cnt = 1;
		for (i = 0; i < nsds; i++)
		{
			if (wcscmp(serialdocs[i].sdocid, prev_name) != 0)	// hit a new name
			{
				//fwprintf(logstream, L"++ i=%d, k=%d, cnt=%d [%s]\n", i, k, cnt, serialdocs[i].sdocid);
				if (k > 0) 
				{
					serialdocs[k-1].nd = cnt;
					wcscpy(serialdocs[k-1].docname[cnt-1], serialdocs[i-1].docname[0]);
					serialdocs[k-1].ndx[cnt-1] = serialdocs[i-1].ndx[0];
				}
				wcscpy_s(serialdocs[k++].sdocid, MAX_LAWSNOTE_ID, serialdocs[i].sdocid); 
				wcscpy_s(prev_name, MAX_LAWSNOTE_ID, serialdocs[i].sdocid); 
				cnt = 1;
			}
			else
			{
				//fwprintf(logstream, L"@@ i=%d, k=%d, cnt=%d\n", i, k, cnt);
				wcscpy(serialdocs[k-1].docname[cnt-1], serialdocs[i-1].docname[0]);
				serialdocs[k-1].ndx[cnt-1] = serialdocs[i-1].ndx[0];
				cnt++;
			}
		}
		serialdocs[k-1].nd = cnt;
		wcscpy(serialdocs[k-1].docname[cnt-1], serialdocs[i-1].docname[0]);
		serialdocs[k-1].ndx[cnt-1] = serialdocs[i-1].ndx[0];
		nsds = k;
		// going further to discard the patent family that contains only 1 patent (does not make sense!)
		for (i = 0, k = 0; i < nsds; i++)
		{
			if (serialdocs[i].nd > 1)
				serialdocs[k++] = serialdocs[i];
		}
		nsds = k;
		// reallocate a smaller block of memory so that we can reduce the usage of memory, added 2018/06/06
		struct SERIALDOCS *serialdocs_temp, *temp;	// added 2018/05/08
		serialdocs_temp = (struct SERIALDOCS *)Jmalloc(nsds * sizeof(struct SERIALDOCS), L"read_TIP: serialdocs_temp");
			if (serialdocs_temp == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (i = 0; i <nsds; i++)
			serialdocs_temp[i] = serialdocs[i];
		temp = serialdocs;	// swap the pointer addresses
		serialdocs = serialdocs_temp;
		serialdocs_temp = temp;
		Jfree(serialdocs_temp, L"read_TIP: serialdocs_temp");
		// write out the results
		for (i = 0; i < nsds; i++)
		{
			fwprintf(logstream, L"Patent family [%d] %s: %d documents\n", i+1, serialdocs[i].sdocid, serialdocs[i].nd);
			for (k = 0; k < serialdocs[i].nd; k++)
				fwprintf(logstream, L"{%d %s} ", serialdocs[i].ndx[k], uspto[serialdocs[i].ndx[k]].pid);
			fwprintf(logstream, L"\n");
		}
	}
	// code for patent family ends here

	fclose(sstream);	// close full record file
	if (no_wos_citation_file) fclose(cstream);	// close citation file (relationship file) if it is created

#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"%d: ", i);
		for (k = 0; k < uspto[i].ntkws; k++)
			fwprintf(logstream, L"%d [%s] ", uspto[i].tkws[k], tkeyword[uspto[i].tkws[k]].name);
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
		wos[i].nde = uspto[i].nassignee;	// use the space for authro keyword to save assignee names
		for (k = 0; k < uspto[i].nassignee; k++)	// added 2012/09/11
			wos[i].DE[k] = uspto[i].assignee[k];
		wos[i].year = uspto[i].year;
		wos[i].month = uspto[i].month;		// added 2012/09/11
		wos[i].a_year = uspto[i].a_year;	// added 2012/09/11
		wos[i].a_month = uspto[i].a_month;	// added 2012/09/11
		wos[i].tc = uspto[i].tc;
		wos[i].country[0] = uspto[i].country[0]; wos[i].country[1] = uspto[i].country[1]; wos[i].country[2] = '\0';	// added 2013/07/27
		wos[i].nipc = uspto[i].nipc;												// added 2013/07/27
		for (m = 0; m < MAX_IPC*MAX_IPC_CODE; m++) wos[i].ipc[m] = uspto[i].ipc[m];	// added 2013/07/27
		wos[i].ntkws = uspto[i].ntkws;												// added 2014/07/23
		for (m = 0; m < MAX_TKEYWORDS; m++) wos[i].tkws[m] = uspto[i].tkws[m];	// added 2014/07/23
	}

	// append the year information to the patent number as alias
	for (i = 0; i < nwos; i++)
		swprintf_s(wos[i].alias, MAX_ALIAS, L"%s_%d_%d", wos[i].docid, wos[i].a_year, wos[i].year);

	link_author_WOS();
	link_assignee_WOS();

	// following co-assignee codes are added 2016/12/24, but then moved to "text2Pajek.cpp" 2017/02/25
	// count the number of times each assignee as the 1st assignee
	//for (i = 0; i < nasgns; i++) assignees[i].cnt1 = 0;	// initialize 1st assignee count
	//for (i = 0; i < nwos; i++) assignees[wos[i].DE[0]].cnt1++;	
	//ret = coassignee(sname, 0); if (ret != 0) return ret;	// output coassignee network that contains only assignees that have been the 1st assignees
	//ret = coassignee(sname, 1); if (ret != 0) return ret;	// output coassignee network that contains all assignees

	keyword_year_matrix(ntkeywords, tkeyword);	// added 2014/07/23
	if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;	// added 2018/05/02

	// author statistics
	// count the number of times each author as the 1st author
	//
	for (i = 0; i < naus; i++) authors[i].cnt1 = 0;	// initialize 1st author count
	for (i = 0; i < nwos; i++) authors[wos[i].author[0]].cnt1++;	
	for (i = 0; i < naus; i++) { authors[i].byear = 3000; authors[i].eyear = 0;}	// initialize byear and eyear
	for (i = 0; i < nasgns; i++) { assignees[i].byear = 3000; assignees[i].eyear = 0;}	// initialize byear and eyear
	// find the ending and beginning year of an inventor's patents
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nau; k++)
		{
			if (authors[wos[i].author[k]].byear > wos[i].year) authors[wos[i].author[k]].byear = wos[i].year;
			if (authors[wos[i].author[k]].eyear < wos[i].year) authors[wos[i].author[k]].eyear = wos[i].year;
		}
	}
	// find the ending and beginning year of an assignee's patents
	int j;
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nde; k++)
		{
			j = wos[i].DE[k];
			if (j < 0) continue;	// added 2018/06/23
			if (assignees[j].byear > wos[i].year) assignees[j].byear = wos[i].year;
			if (assignees[j].eyear < wos[i].year) assignees[j].eyear = wos[i].year;
		}
	}

	// write out author statistics
	fwprintf(logstream, L"Inventor statistics (total patents, 1st inventors, active years, name):\n");
	for (i = 0; i < naus; i++)
		fwprintf(logstream, L"%d %d %d~%d \"%s\"\n", authors[i].np, authors[i].cnt1, authors[i].byear, authors[i].eyear, authors[i].name);

	// sort the data by the patent id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	qsort((void *)uspto, (size_t)nuspto, sizeof(struct USPTO), compare_pid);	// added 2011/06/03
	relink_author_WOS();	// this is necessary after the wos[] arrary is sorted, added 2014/07/23
	relink_assignee_WOS();

	free(line); free(tfield);

	return 0;
}


//
// parse the 1st line of the Thomson Innovation patent file
//
int parse_TIP_2nd_line(wchar_t *line, int separator, wchar_t *tfname)
{
	wchar_t ch, *sp, *tp, *fp;
	wchar_t tmps[LBUF_SIZE];	// changed from XXLBUF_SIZE to BUF_SIZE, 2018/01/19, to LBUF_SIZE, 2018/05/03
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
// this function is re-written 2011/07/06 to cope with the situation that some text are enclosed with quotes, some are not
//
int parse_TIP_line(wchar_t *line, int tind, wchar_t *tfield)
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
			if (ch == ',')	// this field is empty
			{
				*tp = '\0'; fcnt++;
				if (fcnt == tind)
					return fcnt;
				tp = tfield;
				sp++;	// skip this ','
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
		case 1:	// waiting for the delimiter ','
			if (ch == ',') 
			{		
				*tp = '\0'; fcnt++;
				if (fcnt == tind)
					return fcnt;
				tp = tfield;
				sp++;	// skip this ','
				state = 0; 
			}
			else
				*tp++ = *sp++;
			break;
		case 2:	// waiting for the closing quote '"'
			if (ch == '"') 
			{	
				sp++;
				if (*sp == '"')	// special case: "xxx""yyy" (two double quotes in a row), added 2012/09/12
				{
					sp++; break;	// sip the second double quotes and continue to search for the ending double quote
				}
				*tp = '\0'; fcnt++;	// normal case: "xxx","yyy" (a double quote is followed by a comma)
				if (fcnt == tind)
					return fcnt;
				tp = tfield;
				sp++; // skip the ',' after the double quote
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
// parse the "Citing Patents" field string, citations are delimeted by "|", each patent number is leaded by "US"
//
int parse_Citing_Patents(wchar_t *str, wchar_t *pid, FILE *stream)
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
			if (ch == L'|'|| ch == L'\n' || ch == L'\r' || ch == L'\0')
			{
				*tp = L'\0';
				tp = bfr; issued = TRUE;
				cnt = 0;
				while (*tp != L'\0') // check if the patent number is in the format of an issued patent
				{ tp++; cnt++; }
				// the following line is commented out on 2016/04/08, the check is apparently not correct, ignore for now!
				//if (cnt >= 14) issued = FALSE; 	// total 10-11 characters for standard US/JP/WO/EP patents in this database
				if (issued && cnt != 0)
				{
					fwprintf(stream, L"%s %s\n", pid, bfr);	// write the results
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
// parse the "IPC - Current" field string, each is delimited by "|"
//
int parse_IPC_Current(wchar_t *str, int *nipc, wchar_t *ipc_code, int *ipc_code_cnt)
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
			if (ch == L'|'|| ch == L'\n' || ch == L'\r' || ch == L'\0')
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
// parse the "US Class" field string, each is delimited by "|"
//
int parse_US_Class(wchar_t *str, int *nusc, wchar_t *usc_code)
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

//
// split a given inventor string into individual inventors,
// then find each inventor's index, and then save them to the uspto[] array
//
int parse_TIP_inventor_names(wchar_t *istr, int *ninvs, int *invs, int nau, struct AUTHORS *au)
{
	int i;
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int ndx, ndx2;
	int tnau;

	sp = istr;

	// start parsing
	tp = tmps;
	tnau = 0;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == L'|') 
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
			if (tmps[0] != L'\0')
			{
				ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
				if (ndx2 == -1)	// the assignee name may not be in the alias table
					ndx = aname_search(au, nau, tmps);
				else
					ndx = aname_search(au, nau, authoralias[ndx2].alias);
				invs[tnau] = ndx;
				tnau++; 
				if (tnau >= MAX_INVENTORS)	// take only MAX_AUTHORS inventors, changed to MAX_INVENTORS, 2018/08/09
				{
					*ninvs = tnau;
					return 0;
				}
			}
			tp = tmps;
			while (*sp == L'$') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
	if (tmps[0] != L'\0')
	{
		ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
		if (ndx2 == -1)	// the assignee name may not be in the alias table
			ndx = aname_search(au, nau, tmps);
		else
			ndx = aname_search(au, nau, authoralias[ndx2].alias);
		invs[tnau] = ndx;
		tnau++; 
	}

	*ninvs = tnau;

	return 0;
}

//
// split a given inventor string into individual inventors
// and then save them to the global author (inventor) name array
// replace the author (inventor) names with their alias names if a alias file is given
//
int parse_TIP_store_inventors(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau;
	int ndx;

	tnau = *nau;
	sp = astr;

	// start parsing
	tp = tmps;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == L'|')		// the delimiter is '|'
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
			if (tmps[0] != L'\0')
			{
				ndx = authoralias_search(authoralias, nauthoralias, tmps);
				if (ndx == -1)	// the author name may not be in the alias table
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
				}
				else
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, authoralias[ndx].alias); fflush(logstream);
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx].alias);
				}
			}
			tp = tmps;
			while (*sp == L'|') sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
	if (tmps[0] != L'\0')
	{
		ndx = authoralias_search(authoralias, nauthoralias, tmps);
		if (ndx == -1)	// the author name may not be in the alias table
		{
			//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
			wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
		}
		else
		{
			//fwprintf(logstream, L"[%d %s]\n", ndx, aalias[ndx].alias); fflush(logstream);
			wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx].alias);
		}
	}
	*nau = tnau;

	return 0;
}

#ifdef OBSOLETE	// 2016/12/20
//
// parse the "Inventor First" string
// and then save them to the global author name array
//
int parse_store_inventor_first(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau;

	tnau = *nau;
	sp = astr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == '\"') { sp++; continue; }
		if (ch == ';') 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
				wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
	if (tmps[0] != '\0')
		wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
	*nau = tnau;

	return 0;
}

//
// parse a string of "Inventor First" names
// save the results into the wos[] array
//
int parse_inventor_first_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau;
	int ndx;

	sp = astr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	tnau = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == '\"') { sp++; continue; }
		if (ch == ';') 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				ndx = aname_search(athrs, nathrs, tmps);
				au[tnau] = ndx;
				tnau++; 
				if (tnau >= MAX_AUTHORS)	// take only MAX_AUTHORS authors
				{
					*nau = tnau;
					return 0;
				}
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		ndx = aname_search(athrs, nathrs, tmps);
		au[tnau] = ndx;
		tnau++;
	}
	*nau = tnau;

	return 0;
}
#endif OBSOLETE	// 2016/12/20

//
// split a given assignee string into individual assignees,
// then find each assignee's index, and then save them to the uspto[] array
//
int parse_TIP_assignee_names(wchar_t *istr, int *nasgn, int *asgn, int nasgns, struct ASSIGNEES *asgns)
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
		if (ch == L'|') 
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
			if (tmps[0] != L'\0')
			{
				ndx2 = aalias_search(aalias, naalias, tmps);
				if (ndx2 == -1)	// the assignee name may not be in the alias table
					ndx = asgnname_search(asgns, nasgns, tmps);
				else
					ndx = asgnname_search(asgns, nasgns, aalias[ndx2].alias);
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
	*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
	if (tmps[0] != L'\0')
	{
		ndx2 = aalias_search(aalias, naalias, tmps);
		if (ndx2 == -1)	// the assignee name may not be in the alias table
			ndx = asgnname_search(asgns, nasgns, tmps);
		else
			ndx = asgnname_search(asgns, nasgns, aalias[ndx2].alias);
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
int parse_TIP_store_assignees(wchar_t *astr, int *nasgns, struct ASSIGNEES *asgn)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnasgns;
	int ndx;

	tnasgns = *nasgns;
	sp = astr;
	// remove the leading double quote if there is one
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == '|')		// the delimiter is '|'
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
			if (tmps[0] != L'\0')
			{
				// replace the assignee names with their alias names if a alias file is given
				ndx = aalias_search(aalias, naalias, tmps);
				if (ndx == -1)	// the assignee name may not be in the alias table
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
					wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, tmps);
				}
				else
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, aalias[ndx].alias); fflush(logstream);
					wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, aalias[ndx].alias);
				}
			}
			tp = tmps;
			while (*sp == L'|') sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
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
			wcscpy_s(asgn[tnasgns++].name, MAX_ASSIGNEE_NAME, aalias[ndx].alias);
		}
	}
	*nasgns = tnasgns;

	return 0;
}

//
//
//
int parse_publication_year(wchar_t *tfield)
{
	int pyear;
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];

	sp = tfield;
	tp = tmps;

	while (*sp != '\0')
	{
		*tp++ = *sp++;
		if (*sp == '/' || *sp == '-')
			break;
	}
	*tp = '\0';
	pyear = _wtoi(tmps);

		if (pyear == 0)
			pyear = 0;

	return pyear;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_pid(const void *n1, const void *n2)
{
	struct USPTO *t1, *t2;
	
	t1 = (struct USPTO *)n1;
	t2 = (struct USPTO *)n2;
	if (wcscmp(t2->pid, t1->pid) < 0)
		return 1;
	else if (wcscmp(t2->pid, t1->pid) == 0)
		return 0;
	else return -1;
}


//
// establish the relationships between the tables wos[] and authors[]
//
int link_assignee_WOS()
{
	int i, k, j;

	// allocate memory for each assignee's paper index table
	for (i = 0; i < nasgns; i++)
	{
		assignees[i].paper = (int *)Jmalloc(assignees[i].np * sizeof(int), L"link_assignee_WOS: assignees[i].paper");
		if (assignees[i].paper == NULL) return MSG_NOT_ENOUGH_MEMORY;
	}

	for (i = 0; i < nasgns; i++) assignees[i].np = 0;	// want to count np again
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nde; k++)
		{
			j = wos[i].DE[k];
			if (j < 0) continue;	// added 2018/06/23, avoide errors from strnage assignee data
			assignees[j].paper[assignees[j].np++] = i;
		}
	}

#ifdef DEBUG
	for (i = 0; i < nasgns; i++)
	{
		fwprintf(logstream, L"%d %s: ", assignees[i].np, assignees[i].name);
		for (k = 0; k < assignees[i].np; k++)
			fwprintf(logstream, L"[%d:%s] ", assignees[i].paper[k], wos[assignees[i].paper[k]].alias);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// re-establish the relationships between the tables wos[] and assignees[]
// this is necessary after the wos[] arrary is sorted
//
int relink_assignee_WOS()
{
	int i, k, j;

	for (i = 0; i < nasgns; i++) assignees[i].np = 0;	// want to count np again
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nde; k++)
		{
			j = wos[i].DE[k];
			if (j < 0) continue;	// added 2018/06/23, avoide errors from strnage assignee data
			assignees[j].paper[assignees[j].np++] = i;
		}
	}

	return 0;
}

//
// university-industry collaboration check
// set the UI, UU, UUI flag in the USPTO structure
//
int UI_check(int nuspto, struct USPTO *uspto)
{
	int i, j, k, aj, ak;
	int UI, UU;

	check_UI = 1;
	for (i = 0; i < nuspto; i++) uspto[i].UI = 0;
	for (i = 0; i < nuspto; i++)
	{
		if (uspto[i].nassignee >= 2)
		{
			UI = UU = 0;
			for (j = 0; j < uspto[i].nassignee; j++)
			{
				for (k = j + 1; k < uspto[i].nassignee; k++)
				{
					aj = uspto[i].assignee[j];
					ak = uspto[i].assignee[k];
					if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 0)
						UI = 1;
					else if (is_university(assignees[aj].name) == 0 && is_university(assignees[ak].name) == 1)
						UI = 1;
					else if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 1)
						UU = 1;
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

	return 0;
}

//
// Two strange problems occur in some "Assignee/Applicant" and "Assignee - Original w/address" fields in the Thomson Innovation database 
//   1. mixing inventors and "legal representative" with assignees 
//   2. duplicate assignee name
// The 2nd problem is not addressed here and it is resolved after the uspto[].assignee[] data is established
// This function removes the inventors and "legal representationve" from the given "Assignee/Applicant" data string
// This function also replace assignee names with their alias names
// This function is added 2017/01/04, modified to handle "legal representative" 2017/04/11
//
int TIP_assignee_correction(wchar_t *aastring, int ninventors, int *inventor)
{
	wchar_t astr[LBUF_SIZE];
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnasgns;
	int ndx, k, found;
	wchar_t *sp1, *sp2;

	wcscpy(astr, aastring);
	sp = astr;	aastring[0] = '\0';
	// remove the leading double quote if there is oe
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == '|')		// the delimiter is '|'
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
			if (tmps[0] != L'\0')
			{
				// replace the assignee names with their alias names if a alias file is given
				ndx = aalias_search(aalias, naalias, tmps);
				if (ndx != -1)	// the assignee name is in the alias table
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, aalias[ndx].alias); fflush(logstream);
					wcscpy_s(tmps, MAX_ASSIGNEE_NAME, aalias[ndx].alias);
				}
				found = 0;
				for (k = 0; k < ninventors; k++)
				{
					sp1 = authors[inventor[k]].name;
					sp2 = tmps;
					while (*sp1 != '\0') // compare an inventor name with the obtained assignee name
					{
						if (*sp1 == ',') sp1++;	// skip the comma in the inventor name
						if (tolower(*sp1) == tolower(*sp2)) { sp1++; sp2++; }
						else break;
					}
					if (*sp1 == '\0') 
					{ 
						found = 1; 
						break; 
					}
				}
				if (index_of2(0, tmps, L"legal representative") != -1)
					found = 1;
				if (!found) // copy over if it is not an inventor, ignore it otherwise
				{
					wcscat(aastring, tmps);	
					wcscat(aastring, L" | ");
				}
			}
			tp = tmps;
			while (*sp == L'|') sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
	if (tmps[0] != L'\0')
	{
		// replace the assignee names with their alias names if a alias file is given
		ndx = aalias_search(aalias, naalias, tmps);
		if (ndx != -1)	// the assignee name is in the alias table
		{
			//fwprintf(logstream, L"[%d %s]\n", ndx, aalias[ndx].alias); fflush(logstream);
			wcscpy_s(tmps, MAX_ASSIGNEE_NAME, aalias[ndx].alias);
		}
		found = 0;
		for (k = 0; k < ninventors; k++)
		{
			sp1 = authors[inventor[k]].name;
			sp2 = tmps;
			while (*sp1 != '\0') // compare an inventor name with the obtained assignee name
			{
				if (*sp1 == ',') sp1++;	// skip the comma in the inventor name
				if (tolower(*sp1) == tolower(*sp2)) { sp1++; sp2++; }
				else break;
			}
			if (*sp1 == '\0') 
			{ 
				found = 1; 
				break; 
			}
		}
		if (index_of2(0, tmps, L"legal representative") != -1)
			found = 1;
		if (!found) // copy over if it is not an inventor, ignore it otherwise
			wcscat(aastring, tmps);	
	}

	// get rid of " | " at the end of the string
	sp = aastring;
	while (*sp != '\0') sp++;
	sp--; 
	if (*sp == ' ') sp--;
	if (*sp == '|') sp--;
	if (*sp == ' ') *sp = '\0';

	return 0;
}