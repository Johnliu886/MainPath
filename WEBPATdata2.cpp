// 
// WEBPATdata2.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the Webpat database
//
//
// Revision History:
// 2016/05/xx Basic function works
// 2016/06/03 Fixed the problem of not copying ipc_cnt[] from uspto[] to wos[]
// 2016/07/02 Moved to call prepare_keyword_alias_list() before calling parse_store_keywords()	
// 2016/09/15 Modification   : added codes to handle the situation when a citation file is provided
// 2018/08/09 Fixed problems : changed MAX_AUTHORS to MAX_INVENTORS (former is much bigger than the later)
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

static FILE *sstream;		// for source file
extern FILE *logstream;

extern int text_type;
extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern int nuspto;
extern struct USPTO *uspto;
extern int naalias;	// number of items in the assignee alias table
extern struct ANAME_ALIAS *aalias;
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int ntkeywords;
extern int no_wos_citation_file;	// added 2016/09/15

extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;

int parse_WEBPAT_2nd_line(wchar_t *, int, wchar_t *);
int parse_WEBPAT_line(wchar_t *, int, wchar_t *);
int parse_WEBPAT_Citing_Patents(wchar_t *, wchar_t *, FILE *);
int parse_store_WEBPAT_inventor_first(wchar_t *, int *, struct AUTHORS *);
int parse_WEBPAT_inventor_first_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_WEBPAT_CPC_Current(wchar_t *, int *, wchar_t *, int *);
int parse_WEBPAT_US_Class(wchar_t *, int *, wchar_t *);
int compare_author(const void *, const void *);
int parse_WEBPAT_year(wchar_t *, int *year, int *month);

int correct_WEBPAT_pid(wchar_t *, wchar_t *);
int parse_WEBPAT_store_assignees(wchar_t *, int *, struct ASSIGNEES *);
int parse_WEBPAT_assignee_names(wchar_t *, int *, int *, int, struct ASSIGNEES *);

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
extern int asgnname_search(struct ASSIGNEES *, int, wchar_t *);
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int compare_pid(const void *, const void *);
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);

#define XXLBUF_SIZE 65536*4	// there are extremely long texts in the "Cited Refs - Non-patent" field in the Thomson Innovatin data
#define BUF_SIZE 1024

wchar_t *pline;
int just_exit_at_next_line = 1;	// set it as if the 1st data line is read

int init_pfgetws(FILE *sstream)
{
	pline = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"init_pfgetws: pline");

	return 0;
}

int done_pfgetws()
{
	Jfree(pline, L"done_pfgetws: pline");

	return 0;
}
//
// this function collects text strings until all fields are collected 
// the reason why this is necessary is that WEBPAT data (.csv file) embed many line feeds in a record and end a record with "\r\n", which is preceded with many commas
// what's more, fgetws() function changes "\r\n" sequence to '\n' such that we cannot rely on the sequence to find a record
//
wchar_t *pfgetws(wchar_t *line, int bufsize, FILE *sstream)
{
	wchar_t *sp, *tp;
	wchar_t *retstat;
	wchar_t tline[XXLBUF_SIZE];
	int nf;
	int in_quote;
	int i;
	int ccnt;
	
	tp = line;
	nf = 0; in_quote = 0;
	while (TRUE)	
	{		
		if (just_exit_at_next_line == 0) // don't read for the 1st time, because the line is already read in the previous call
		{
			retstat = fgetws(tline, XXLBUF_SIZE, sstream);
			if (retstat == NULL) 
				break;		// end of file
		}
		else
			wcscpy(tline, pline);	// copy back from the buffer
		sp = tline; 
		if (iswdigit(*sp) && !just_exit_at_next_line)	// if the line begins with a digit
		{
			while (iswdigit(*sp)) sp++;
			if (*sp == ',')	// the line begins with digits follows by a ','
			{
				sp++; 
				if ((sp[0] == 'R' && sp[1] == 'E') || (iswdigit(sp[0]) && iswdigit(sp[1])) || (sp[0] == 'H' && iswdigit(sp[1])))	// some US patent ID begins with 'RE' or "H"
				{
					sp += 2; ccnt = 0; 
					for (i = 0; i < 6; i++, sp++)
					{
						if (*sp == '\0' || *sp == ',')	break;
						else if (iswdigit(*sp)) ccnt++;
					}
					if (*sp == ',' && ccnt == 6)	// find the end of previous record, because this new line begins with digits followed by a ',' plus 8 digits of US patent
					{
						just_exit_at_next_line = 1;	
						wcscpy(pline, tline);	// save the content that is already read
						break;
					}
				}
			}
		}
		just_exit_at_next_line = 0;
		sp = tline;
		while (*sp != '\n')
		{
			if (*sp == '\"')
			{
				if (in_quote == 0) in_quote = 1;
				else if (in_quote == 1) in_quote = 0;
			}
			if (*sp == ',' && in_quote == 0)
				nf++;
			*tp++ = *sp++;	// copy over character by character
		}
		*tp++ = *sp++;	// copy over the '\n'
	}
	*tp = '\0'; 

	if (retstat == NULL)	// end of file
		return NULL;
	else
		return line;
}

//
// read WOS data and put it into the wos[] array
//
int read_WEBPAT2(wchar_t *sname)
{
	int i, k, m, ndx;
	int nsr, cnt;
	int i_ut, i_au, i_py, i_pm, i_cr, i_ipc, i_usc, i_an, i_ti, i_ab;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t tmps[BUF_SIZE], *gret, *sp;
	FILE *cstream;		// for citation file
	FILE *astream;		// for assignee alias file
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
	if (text_type == FT_ASCII)
	{
		_wfopen_s(&sstream, sname, L"rt");	// open as text mode for ASCII
		//if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return UNKNOWN_DATA;
	}
	else if (text_type == FT_UTF_8)
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8");	// open as text mode for UTF-8 type	
		//if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return UNKNOWN_DATA;
	}
	else	// Unicode type
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE");	// for Unicode data	
		//if(fgetws(line, XXLBUF_SIZE/2, sstream) == NULL) return UNKNOWN_DATA;
	}

	// 1st pass, count the number of target records 
	init_pfgetws(sstream); // prepare for pfgetws() to work
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 2nd line ==> 編號,專利號/公開號,專利名稱,公告(公開)日,申請號,申請日,專利類型, .....
	fgetws(line, XXLBUF_SIZE, sstream); // read the 3rd line, copy it to the buffer (pline), this set the 1st call to pfgetws() to a proper setting 
	wcscpy(pline, line); just_exit_at_next_line = 1;
	nsr = 0;
	while (TRUE)
	{		
		gret = pfgetws(line, XXLBUF_SIZE, sstream);
		//fwprintf(logstream, L"#####[%s]\n", line); fflush(logstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nsr++;	
		if (gret == NULL)
			break;
	}
	//fwprintf(logstream, L"#####[%s]\n", line); fflush(logstream);
	nuspto = nsr;

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	fgetws(line, XXLBUF_SIZE, sstream);	// get the 2nd line ==> 編號,專利號/公開號,專利名稱,公告(公開)日,申請號,申請日,專利類型, .....
	// look for the index of the specified target field name 
	i_ut = parse_WEBPAT_2nd_line(line, SP_COLON, L"專利號/公開號");	// document ID
	i_au = parse_WEBPAT_2nd_line(line, SP_COLON, L"發明人姓名");	// author names
	i_py = parse_WEBPAT_2nd_line(line, SP_COLON, L"公告(公開)日");	// year of publication
	i_pm = parse_WEBPAT_2nd_line(line, SP_COLON, L"申請日");	// month of publication
	i_cr = parse_WEBPAT_2nd_line(line, SP_COLON, L"本國引證專利號");	// this field actually contains "cited-by" information
	i_ipc = parse_WEBPAT_2nd_line(line, SP_COLON, L"合作分類號");	// CPC code, was IPC code
	i_an = parse_WEBPAT_2nd_line(line, SP_COLON, L"專利權人姓名");	// Assignee/Applicant
	i_ti = parse_WEBPAT_2nd_line(line, SP_COLON, L"專利名稱");		// title 
	i_ab = parse_WEBPAT_2nd_line(line, SP_COLON, L"摘要");	// abstract 
	if (i_ut == -1 || i_au == -1 || i_py == -1 || i_pm == -1 || i_cr == -1 || i_ipc == -1 || i_an == -1 || i_ti == -1 || i_ab == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	prepare_keyword_alias_list();	// this is so that we can handle various form of a keyword, moved up to here 2016/07/02
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

	// open the "Assignee alis.txt" file, ignore if it does not exist
	if (_wfopen_s(&astream, L"Assignee alias.txt", L"r") != 0)
	{
		fwprintf(logstream, L"WARNING: Can not find file: Assignee alias.txt.\n");
		naalias = 0;
	}
	else
	{
		fwprintf(logstream, L"Found and read file: Assignee alias.txt.\n");
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
		fwprintf(logstream, L"Assignee alias table\n");	// added 2017/01/10
		for (i = 0; i < naalias; i++)
			fwprintf(logstream, L"[%s]=>[%s]\n", aalias[i].name, aalias[i].alias);
	}
	fclose(astream);

	// allocate memory for the author name array
	authors = (struct AUTHORS *)malloc(nuspto * 8 * sizeof(struct AUTHORS));	// estimate in average 8 inventors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// allocate memory for the assignee name array
	assignees = (struct ASSIGNEES *)malloc((int)(nuspto * 2.5) * sizeof(struct ASSIGNEES));	// estimate in average 2.5 assignees per document, changed from 1.5 to 2.5, 20014/04/04
	if (assignees == NULL) return MSG_NOT_ENOUGH_MEMORY;

	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 2nd line ==> 編號,專利號/公開號,專利名稱,公告(公開)日,申請號,申請日,專利類型, .....
	fgetws(line, XXLBUF_SIZE, sstream); // read the 3rd line, copy it to the buffer (pline), this set the 1st call to pfgetws() to a proper setting 
	wcscpy(pline, line); just_exit_at_next_line = 1;
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		gret = pfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		parse_store_WEBPAT_inventor_first(tfield, &naus, authors);
		//fwprintf(logstream, L"%d: %d [%s]\n", i, naus, tfield); fflush(logstream);
		parse_WEBPAT_line(line, i_an, tfield);
		parse_WEBPAT_store_assignees(tfield, &nasgns, assignees);
		//fwprintf(logstream, L"%d: %d [%s]\n", i, nasgns, tfield); fflush(logstream);
		i++;	
		if (gret == NULL)
			break;
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

#ifdef DEBUG
	//for (i = 0; i < naus; i++)
	//{
	//	fwprintf(logstream, L"#####%d [%s]\n", i+1, authors[i].name);
	//	fflush(logstream);
	//}
	for (i = 0; i < nasgns; i++)
	{
		fwprintf(logstream, L"@@@@@%d [%s]\n", i+1, assignees[i].name);
		fflush(logstream);
	}
#endif DEBUG

	// obtain and process article title information
	// allocate memory for the title keyword array
	struct PTITLES *ptitle;	// paper titles
	ptitle = (struct PTITLES *)Jmalloc(nuspto * sizeof(struct PTITLES), L"read_WEBPAT: ptitle");
	if (ptitle == NULL) return MSG_NOT_ENOUGH_MEMORY;
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 2nd line ==> 編號,專利號/公開號,專利名稱,公告(公開)日,申請號,申請日,專利類型, .....
	fgetws(line, XXLBUF_SIZE, sstream); // read the 3rd line, copy it to the buffer (pline), this set the 1st call to pfgetws() to a proper setting 
	wcscpy(pline, line); just_exit_at_next_line = 1;
	// read source file line by line
	int ttmp;
	i = 0; 
	while (TRUE)
	{		
		gret = pfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT_line(line, i_ti, tfield);
		wcscpy_s(ptitle[i].name, MAX_TITLES, tfield);
		parse_WEBPAT_line(line, i_py, tfield);
		parse_WEBPAT_year(tfield, &ptitle[i].year, &ttmp);
		//fwprintf(logstream, L"[%d %d %s]\n", i+1, ptitle[i].year, ptitle[i].name); fflush(logstream);
		i++;	
		if (gret == NULL)
			break;
	}

	// assume in average each title contains 200 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nuspto * 200 * sizeof(struct TKEYWORDS), L"read_WEBPAT: tkeyword");
	if (tkeyword == NULL) return MSG_NOT_ENOUGH_MEMORY;
	ndx = 0;
	for (i = 0; i < nuspto; i++)
	{
		//fwprintf(logstream, L"[%s]\n", ptitle[i].name);
		ndx = parse_store_title(ndx, tkeyword, ptitle[i].name);
	}
	ntkeywords = ndx;
	//fwprintf(logstream, L"*****ntkeywords=%d\n", ntkeywords);
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
	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = i; } 
	
#ifdef DEBUG
	for (i = 0; i < ntkeywords; i++)
		fwprintf(logstream, L"%d %03d [%s]\n", i, tkeyword[i].cnt, tkeyword[i].name);
#endif DEBUG
	free(ptitle);

	// get all other required data
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 2nd line ==> 編號,專利號/公開號,專利名稱,公告(公開)日,申請號,申請日,專利類型, .....
	fgetws(line, XXLBUF_SIZE, sstream); // read the 3rd line, copy it to the buffer (pline), this set the 1st call to pfgetws() to a proper setting 
	wcscpy(pline, line); just_exit_at_next_line = 1;
	// read source file line by line
	i = 0;
	while (TRUE)
	{		
		gret = pfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT_line(line, i_ut, tfield);
		correct_WEBPAT_pid(uspto[i].pid, tfield);	// write to .pid after correction
		parse_WEBPAT_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		//fwprintf(logstream, L"$$$$$ %d %s\n", i, uspto[i].pid );
		parse_WEBPAT_inventor_first_names(tfield, &uspto[i].ninventor, uspto[i].inventor, naus, authors);
#ifdef DEBUG
		for (k = 0; k < uspto[i].ninventor; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i+1, uspto[i].ninventor, k, uspto[i].inventor[k], authors[uspto[i].inventor[k]].name); fflush(logstream); }
#endif DEBUG
		parse_WEBPAT_line(line, i_py, tfield);
		parse_WEBPAT_year(tfield, &uspto[i].year, &uspto[i].month);
		//fwprintf(logstream, L"%d: %d\n", i, uspto[i].year); fflush(logstream);
		parse_WEBPAT_line(line, i_pm, tfield);
		parse_WEBPAT_year(tfield, &uspto[i].a_year, &uspto[i].a_month);
		uspto[i].tc = 0;
		parse_WEBPAT_line(line, i_cr, tfield);
		parse_WEBPAT_Citing_Patents(tfield, uspto[i].pid, cstream);
		parse_WEBPAT_line(line, i_ipc, tfield);
		parse_WEBPAT_CPC_Current(tfield, &uspto[i].nipc, uspto[i].ipc, uspto[i].ipc_cnt);
		parse_WEBPAT_line(line, i_an, tfield);
		parse_WEBPAT_assignee_names(tfield, &uspto[i].nassignee, uspto[i].assignee, nasgns, assignees);
		parse_WEBPAT_line(line, i_ti, tfield);		
		parse_title(tfield, 0, &uspto[i].ntkws, uspto[i].tkws, ntkeywords, tkeyword);	
		parse_WEBPAT_line(line, i_ab, tfield);		// parse abstract
		parse_title(tfield, uspto[i].ntkws, &uspto[i].ntkws, uspto[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array
		i++;	
		if (gret == NULL)
			break;
	}

	fclose(sstream);	// close full record file
	if (no_wos_citation_file) fclose(cstream);	// close citation file (relationship file) if it is created

#ifdef DEBUG
	
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"%d: ", i);
		//for (k = 0; k < uspto[i].nipc; k++) 
		//	fwprintf(logstream, L"%d:%d:%d [%s]\n", i+1, uspto[i].nipc, k, &uspto[i].ipc[MAX_IPC_CODE*k]); fflush(logstream); 
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
		for (k = 0; k < uspto[i].nassignee; k++)	
			wos[i].DE[k] = uspto[i].assignee[k];
		wos[i].year = uspto[i].year;
		wos[i].month = uspto[i].month;	
		wos[i].a_year = uspto[i].a_year;
		wos[i].a_month = uspto[i].a_month;
		//wos[i].country[0] = uspto[i].country[0]; wos[i].country[1] = uspto[i].country[1]; wos[i].country[2] = '\0';	
		wos[i].nipc = uspto[i].nipc;												
		for (m = 0; m < MAX_IPC*MAX_IPC_CODE; m++) wos[i].ipc[m] = uspto[i].ipc[m];											
		for (m = 0; m < MAX_IPC; m++) wos[i].ipc_cnt[m] = uspto[i].ipc_cnt[m];		// added 2016/06/03	
		wos[i].ntkws = uspto[i].ntkws;												
		for (m = 0; m < MAX_TKEYWORDS; m++) wos[i].tkws[m] = uspto[i].tkws[m];	
	}

	// append the year information to the patent number as alias
	for (i = 0; i < nwos; i++)
		swprintf_s(wos[i].alias, MAX_ALIAS, L"%s_%d_%d", wos[i].docid, wos[i].a_year, wos[i].year);

	link_author_WOS();
	keyword_year_matrix(ntkeywords, tkeyword);	

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

	// sort the data by the patent id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	qsort((void *)uspto, (size_t)nuspto, sizeof(struct USPTO), compare_pid);
	relink_author_WOS();	// this is necessary after the wos[] arrary is sorted

	Jfree(line, L"read_WEBPAT: line"); 
	Jfree(tfield, L"read_WEBPAT: tfield");
	done_pfgetws();

	return 0;
}


//
// parse the field name line of the WebPat file
//
int parse_WEBPAT_2nd_line(wchar_t *line, int separator, wchar_t *tfname)
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
// parse a data record prepared by WebPat
//
int parse_WEBPAT_line(wchar_t *line, int tind, wchar_t *tfield)
{
	wchar_t ch, *sp, *tp, *fp;
	int state;
	int fcnt;

	sp = line;

	// start parsing
	tp = tfield; fcnt = 0;
	// Note: some fields are enclosed with double quotes, some are not
	state = 0;	// neutral state
	while (*sp != '\0')
	{
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
// parse the "本國引證專利號" field string, citations are delimeted by "\n"
//
int parse_WEBPAT_Citing_Patents(wchar_t *str, wchar_t *pid, FILE *stream)
{
	int i, state;
	int cnt;
	int issued;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[LBUF_SIZE];

	if (!no_wos_citation_file)	// this check is added 2016/09/15
		return 0;

	sp = str; tp = bfr;
	state = 1;
	while (TRUE)
	{
		ch = *sp;
		if (ch == '\0') break;
		switch (state)
		{
		case 1:	// looking for the delimeter string
			if (ch == L'\n')
			{
				*tp = L'\0';
				if (bfr[0] == L'\0')
					return 0;
				tp = bfr; issued = TRUE;
				cnt = 0;
				while (*tp != L'\0') // check if the patent number is in the format of an issued patent
				{ tp++; cnt++; }
				if (cnt >= 12) issued = FALSE; 	// non-issued patent is in the form "2009/1234567"
				if (issued && cnt != 0)
				{
					fwprintf(stream, L"%s %s\n", bfr, pid);	// write the results
					fflush(stream);
				}
				tp = bfr;
				state = 2;
			}
			else
			{
				if (ch != L' ')	// skip the spaces
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
// parse the "合作分類號" field string, each is delimited by a "\n"
//
int parse_WEBPAT_CPC_Current(wchar_t *str, int *nipc, wchar_t *ipc_code, int *ipc_code_cnt)
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
		if (ch == '\0') break;
		switch (state)
		{
		case 1:	// looking for the delimeter string
			if (ch == L'\n')
			{
				*tp = L'\0';
				tp = bfr;
				ipc_code_cnt[cnt] = 1;	// 1 for every IPC code, added 2016/05/26
				wcscpy_s(&ipc_code[MAX_IPC_CODE*cnt], MAX_IPC_CODE*sizeof(wchar_t), bfr);
				cnt++;
				*nipc = cnt;
				tp = bfr;
				state = 2;
			}
			else
			{
					//if (ch != L' ')	// skip the space
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
// parse the "發明人姓名" string
// and then save them to the global author name array
//
int parse_store_WEBPAT_inventor_first(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau, ndx2;
	int nau_this_article = 0;	

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
		if (ch == ';') // author names are separated by a ';'
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
				if (ndx2 >= 0)	// found that this is a variation of the author name
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
				else
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
				nau_this_article++;
				if (nau_this_article >= MAX_INVENTORS)	// take only MAX_INVENTORS authors, modified 2018/08/09
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
		ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
		if (ndx2 >= 0)	// found that this is a variation of the author name
			wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
		else
			wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
	}
	*nau = tnau;

	return 0;
}

//
// parse a string of "發明人姓名" names
// save the results into the wos[] array
//
int parse_WEBPAT_inventor_first_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau;
	int ndx, ndx2;

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
				ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
				if (ndx2 >= 0)	// found that this is a variation of the author name
					ndx = aname_search(athrs, nathrs, authoralias[ndx2].alias);
				else
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
		ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
		if (ndx2 >= 0)	// found that this is a variation of the author name
			ndx = aname_search(athrs, nathrs, authoralias[ndx2].alias);
		else
			ndx = aname_search(athrs, nathrs, tmps);
		au[tnau] = ndx;
		tnau++;
	}
	*nau = tnau;

	return 0;
}

//
// split a given assignee string into individual assignees,
// then find each assignee's index, and then save them to the uspto[] array
//
int parse_WEBPAT_assignee_names(wchar_t *istr, int *nasgn, int *asgn, int nasgns, struct ASSIGNEES *asgns)
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
		if (ch == L'\n')	// assignee names are seperated by a line feed
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
int parse_WEBPAT_store_assignees(wchar_t *astr, int *nasgns, struct ASSIGNEES *asgn)
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
	while (*sp != L'\0')
	{
		ch = *sp; 
		if (ch == L'\n')		// the delimiter is '\n'
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
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
int parse_WEBPAT_year(wchar_t *tfield, int *year, int *month)
{
	int i;
	wchar_t *sp, *tp;
	wchar_t tmps[BUF_SIZE];

	sp = tfield;
	tp = tmps;
	for (i = 0; i < 4; i++)
		*tp++ = *sp++;
	*tp = '\0';
	*year = _wtoi(tmps);
	tp = tmps;
	for (i = 0; i < 2; i++)
		*tp++ = *sp++;
	*month = _wtoi(tmps);

	return 0;
}

//
// correct pid in WEBPAT data and then write to the given address
// WEBPAT data has a leading '0'
//
int correct_WEBPAT_pid(wchar_t *pid, wchar_t *original)
{
	wchar_t *sp, *tp;

	if (original[0] == '0')
		wcscpy(pid, &original[1]);
	else if (original[0] == 'R' && original[1] == 'E')
	{
		sp = original + 2; tp = pid;
		tp[0] = 'R'; tp[1] = 'E'; tp += 2;
		while (*sp == '0') sp++;
		while (*sp != '\0') *tp++ = *sp++;
	}
	else if (original[0] == 'H')
	{
		sp = original + 1; tp = pid;
		tp[0] = 'H'; tp += 1;
		while (*sp == '0') sp++;
		while (*sp != '\0') *tp++ = *sp++;
	}
	else
		wcscpy(pid, original);

	return 0;
}
