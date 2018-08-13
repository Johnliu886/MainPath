// 
// TICdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the TCI (Taiwan Citation Index) database
//
//
// Revision History:
// 2016/04/20 Basic function works
// 2016/04/23 Fixed a problem in determining the beginning of a record
// 2016/07/02 Moved to call prepare_keyword_alias_list() before calling parse_store_keywords()	
// 2016/11/19 Added to call relink_journal_WOS()
// 2017/06/27 Initialized n_fileds (n_fields = 0) before counting it
// 2017/06/28 Added a new argument "secondary_data", which indicates that the reading is for the secondary data
// 2017/07/07 Modified code to avoide taking such line "3,000 xxxxxxx" as the beginning of a record
// 2017/07/12 Modified code to avoide taking another pattern "26, rue de xxx" as the beginning of a record
// 2017/08/15 Modified code to cope with another two special patterns
// 2017/08/21 Modified code to cope with the legal special pattern "1234,850 nm雷射"
//
	
#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;
extern int njournalalias;	// number of items in the journal alias table, 2015/03/31
extern struct JOURNAL_ALIAS *journalalias;	// 2015/03/31

static FILE *sstream;		// for source file

extern FILE *logstream;

extern int text_type;
extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern int nkwde;
extern struct KWORDS *kwde;	// author keywords
extern int nkwid;
extern struct KWORDS *kwid;	// Keyword Plus keywords
extern int ntkeywords;
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int njrs;
extern struct JOURNALS *journals;	// author name array
extern int no_wos_citation_file;
extern struct TTD_TITLE *ttd_title;

int parse_TCI_1st_line(wchar_t *, wchar_t *);
int parse_TCI_line(wchar_t *, int, wchar_t *);
int parse_store_TCI_authors(wchar_t *, int *, struct AUTHORS *);
int parse_store_TCI_authors2(wchar_t *, int *, struct AUTHORS *);
int get_TCI_number_authors(wchar_t *);
int parse_TCI_author_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_TCI_author_names2(wchar_t *, int *, int *, int, struct AUTHORS *);
int create_TCI_docid(wchar_t *, wchar_t *, int, int, int, int);
int check_author_name_format_type(wchar_t *);

extern int prepare_author_alias_list();

extern int prepare_keyword_alias_list();
extern int prepare_journal_alias_list();
extern int parse_keywords(wchar_t *, int *, int *, int, struct KWORDS *);
extern int parse_store_keywords(wchar_t *, int *, struct KWORDS *);
extern int parse_store_title(int, struct TKEYWORDS *, wchar_t *);	
extern int parse_title(wchar_t *, int, int *, int *, int, struct TKEYWORDS *);
extern int compare_kname(const void *, const void *);
extern int compare_author(const void *, const void *);
extern int compare_journal(const void *, const void *);
extern int compare_tkeyword(const void *, const void *);
extern int compare_wordcount(const void *, const void *);
extern int is_stopwords(wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int jname_search(struct JOURNALS *, int, wchar_t *);
extern int prepare_alias(int);
extern int parse_countries(int, wchar_t *, int, struct AUTHORS *);
extern int parse_countries_rp(int, wchar_t *, int, struct AUTHORS *);
extern int prepare_for_CR_search();
extern int clear_preparation_for_CR_search();
extern int parse_TCI_citation_info(wchar_t *, int, wchar_t *, FILE *);
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int link_author_WOS();
extern int link_journal_WOS();
extern int relink_journal_WOS();
extern int coauthor(wchar_t *, int);		
extern int calculate_author_h_index();
extern int calculate_journal_h_index();
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int basic_wos_statistics();
extern int compare_docid(const void *, const void *);
extern int relink_author_WOS();
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int journalalias_search(struct JOURNAL_ALIAS *, int, wchar_t *);
extern int compare_ttd_title(const void *, const void *);

#define AUTHOR_NAME_FORMAT1	1	// "Chen J.S."
#define AUTHOR_NAME_FORMAT2	2	// "Chen, J.S.", after Oct, 2015
#define AUTHOR_NAME_FORMAT_UNKNOWN -1

#define XXLBUF_SIZE 65536*4	// expecting extremely long lines
#define BUF_SIZE 1024

//
//
//
int n_fields = 0;
static wchar_t *xline;
static int exit_at_next_line = 0;
int init_xfgetws(FILE *sstream)
{
	xline = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"init_xfgetws: xline");

	return 0;
}

int done_xfgetws()
{
	Jfree(xline, L"done_xfgetws: xline");

	return 0;
}

//
// this function collects text strings until all fields are collected 
// the reason why this is necessary is that TCI data (.csv file) embed many line feeds in a record and end a record with "\r\n"
// what's more, fgetws() function changes "\r\n" sequence to '\n' such that we cannot rely on the sequence to find a record
//
wchar_t *xfgetws(wchar_t *line, int bufsize, FILE *sstream)
{
	wchar_t *sp, *tp;
	wchar_t *retstat;
	wchar_t tline[XXLBUF_SIZE];
	int nf;
	int in_quote;
	int i;
	int ccnt;
	int special_pattern;
	
	tp = line;
	nf = 0; in_quote = 0;
	while (TRUE)	
	{		
		if (exit_at_next_line == 0) // don't read for the 1st time, because the line is already read in the previous call
		{
			retstat = fgetws(tline, XXLBUF_SIZE, sstream);
			//fwprintf(logstream, L"[%s]", tline); fflush(logstream);
			if (retstat == NULL) 
				break;		// end of file
		}
		else
			wcscpy(tline, xline);	// copy back from the buffer
		exit_at_next_line = 0;
		sp = tline; 
		if (iswdigit(*sp))	// if the line begins with a digit
		{
			while (iswdigit(*sp)) sp++;
			if (*sp == ',')	
			{
				special_pattern = 0;
				if (isdigit(*(sp+1)) && isdigit(*(sp+2)) && isdigit(*(sp+3)) && *(sp+4) == ' ') // check for the special pattern "3,000 xxx", added 2017/07/07
				{
					if (!(*(sp+5) == 'n' && *(sp+6) == 'm'))	// but the string "830 nm" is legal
						special_pattern = 1;
				}
				else if (*(sp+1) == ' ' && iswalpha(*(sp+2))) // check for the special pattern "26, rue de xxx", added 2017/07/12
					special_pattern = 1;
				else if (*(sp+1) == ' ' && iswdigit(*(sp+2)) && *(sp+3) == ',' && *(sp+4) == ' ')	// check for the special pattern "1, 2, 3", added 2017/08/15
					special_pattern = 1;
				else if (*(sp+1) == ',' && *(sp+2) == ' ')	// check for the special pattern "2004,, ", added 2017/08/15
					special_pattern = 1;
			}
			if (*sp == ',' && !special_pattern)	// the line begins with digits follows by a ',', but there should be no special patterns, modified 2017/07/07
			//if (*sp == ',')	// the line begins with digits follows by a ','
			{
				sp = tline; ccnt = 0; 
				while (*sp != '\0')
				{
					if (*sp == ',') 
					{
						ccnt++;
						if (ccnt >= 3)	// hit at least 3 ','
							break;
					}
					sp++;
				}
				if (*sp == ',')	// find the end of previous record, because this new line begins with digits followed by at least 3 ',' (for example "35,")
				{
					//fwprintf(logstream, L"#####nf=%d [%s]\n", nf, tline); fflush(logstream);
					if (nf == (n_fields - 1))
					{
						exit_at_next_line = 1;	
						wcscpy(xline, tline);	// save the content that is already read
						break;
					}
				}
			}
		}
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
// read TCI data and put it into the wos[] array
//
int read_TCI(wchar_t *sname, int secondary_data)
{
	int i, k, ndx, ndx2;
	int nsr, cnt, ret;
	int i_ut, i_au, i_de, i_py, i_so, i_sc, i_cr, i_vl, i_pg, i_bp, i_ep, i_ab;
	int i_c1, i_rp;		// added 2013/04/10
	int i_ti;			// added 2013/07/12
	wchar_t *line;
	wchar_t *nline;
	wchar_t *tfield;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t *sp, *tp;
	wchar_t *xret;
	FILE *cstream;		// for citation file
	int xerr;

	// allocate the working memory
	line = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_TCI: line");
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_TCI: tfield");
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
		xerr = 0;
		if ((xerr = _wfopen_s(&sstream, sname, L"rt, ccs=UTF-8")) != 0)	// open as text mode for UTF-8 type	
		{
			fwprintf(logstream, L"FILE OPEN ERROR: %d\n", xerr);
			return MSG_IFILE_NOTFOUND;
		}
		//if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return UNKNOWN_DATA;
	}
	else	// Unicode type
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE");	// for Unicode data	
		//if(fgetws(line, XXLBUF_SIZE/2, sstream) == NULL) return UNKNOWN_DATA;
	}

	// find the number of fields from the data in the 1st line
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) 
		return UNKNOWN_DATA;
	sp = line; n_fields = 0;
	while (*sp != '\n') { if (*sp == ',') n_fields++; sp++;	}
	n_fields++;	// but the number of delimiter (',') is n_fields-1
	//fwprintf(logstream, L"#####n_fields = %d\n", n_fields);

	// 1st pass, count the number of target records 
	init_xfgetws(sstream); // prepare for xfgetws() to work
	nsr = 0;
	while (TRUE)
	{	
		if (xfgetws(line, XXLBUF_SIZE, sstream) == NULL)
		{
			nsr++;
			break;
		}
		//fwprintf(logstream, L"#####[%s]\n", line); fflush(logstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nsr++;	
	}
	nwos = nsr;	
	fwprintf(logstream, L"Number of papers: %d\n", nwos);

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	nline = line;
	// look for the index of the specified target field name 
	i_au = parse_TCI_1st_line(nline, L"作者");	// author names
	i_de = parse_TCI_1st_line(nline, L"主題關鍵詞");	// autor keywords
	i_py = parse_TCI_1st_line(nline, L"出版日期");			// year of publication
	i_so = parse_TCI_1st_line(nline, L"書刊名");	// journal name
	i_cr = parse_TCI_1st_line(nline, L"論文參考文獻");		// documents this article references
	i_vl = parse_TCI_1st_line(nline, L"卷期");	// volume
	i_pg = parse_TCI_1st_line(nline, L"頁次");	// page
	i_bp = parse_TCI_1st_line(nline, L"開始頁");	// beginning page
	i_ep = parse_TCI_1st_line(nline, L"結束頁");	// ending page
	i_ab = parse_TCI_1st_line(nline, L"中文摘要");	// abstract
	i_ti = parse_TCI_1st_line(nline, L"題名");		// title

	if (i_au == -1 || i_de == -1 || i_py == -1 || i_so == -1 || i_cr == -1 || i_vl == -1 || i_pg == -1 || i_ab == -1 || i_ti == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	prepare_keyword_alias_list();	// this is so that we can handle various form of a keyword, moved up to here 2016/07/02
	// 2nd pass, get keyword data and save it
	// allocate memory for the author keyword data
	kwde = (struct KWORDS *)Jmalloc(nwos * 12 * sizeof(struct KWORDS), L"read_TCI: kwde");	// estimate in average 12 keywords per document
	if (kwde == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; nkwde = 0;
	while (TRUE)
	{		
		xret = xfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		//fwprintf(logstream, L"@@@@@ %d [%s]\n", i , line); fflush(logstream);
		parse_TCI_line(line, i_de, tfield);
		//fwprintf(logstream, L"##### %d [%s]\n", i , tfield); fflush(logstream);
		parse_store_keywords(tfield, &nkwde, kwde);
		//fwprintf(logstream, L"%d:%d: [%s]\n", i+1, nkwde, tfield); fflush(logstream);
		i++;	
		if (xret== NULL) break;
	}

	qsort((void *)kwde, (size_t)nkwde, sizeof(struct KWORDS), compare_kname);
	// consolidate duplicate keywords
	wchar_t prev_name[MAX_JOURNAL_NAME];
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < nkwde; i++)
	{
		if (wcscmp(kwde[i].name, prev_name) != 0)	// hit new name
		{
			wcscpy_s(kwde[k++].name, MAX_KEYWORD_NAME, kwde[i].name); 
			wcscpy_s(prev_name, MAX_KEYWORD_NAME, kwde[i].name); 
		}
	}
	nkwde = k;
	//for (i = 0; i < nkwde; i++)
	//	fwprintf(logstream, L"=====%d [%s]\n", i+1, kwde[i].name);

	prepare_author_alias_list();	// this is so that we can handle various form of an author's name
	
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the author name array
	authors = (struct AUTHORS *)Jmalloc(nwos * 5 * sizeof(struct AUTHORS), L"read_TCI: authors");	// estimate in average 5 authors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		xret = xfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TCI_line(line, i_au, tfield);
		parse_store_TCI_authors(tfield, &naus, authors);
		//fwprintf(logstream, L"%d: %s\n", i+1, tfield); fflush(logstream);
		i++;
		if (xret== NULL) break;	
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

	prepare_journal_alias_list();	// this is so that we can handle the situation when journal change titles
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the journal name array
	journals = (struct JOURNALS *)Jmalloc(nwos * sizeof(struct JOURNALS), L"read_TCI: journals");
	if (journals == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		xret = xfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TCI_line(line, i_so, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
		if (tfield[0] != '\0')
		{
			ndx2 = journalalias_search(journalalias, njournalalias, tfield);
			if (ndx2 >= 0)	// found that this is a variation of the journal title
				wcscpy_s(journals[i].name, MAX_JOURNAL_NAME, journalalias[ndx2].alias);
			else
				wcscpy_s(journals[i].name, MAX_JOURNAL_NAME, tfield);
		}
		else
			wcscpy_s(journals[i].name, MAX_JOURNAL_NAME, L"Unknown");
		i++;
		if (xret== NULL) break;		
	}
	njrs = i;
	qsort((void *)journals, (size_t)njrs, sizeof(struct JOURNALS), compare_journal);
	// consolidate duplicate journal names
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < njrs; i++)
	{
		if (wcscmp(journals[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) journals[k-1].np = cnt;
			wcscpy_s(journals[k++].name, MAX_JOURNAL_NAME, journals[i].name); 
			wcscpy_s(prev_name, MAX_JOURNAL_NAME, journals[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	journals[k-1].np = cnt;
	njrs = k;
	//for (i = 0; i < njrs; i++)
	//	fwprintf(logstream, L"%s\n", journals[i].name);

	// obtain and process article title information
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the title keyword array
	ttd_title = (struct TTD_TITLE *)Jmalloc(nwos * sizeof(struct TTD_TITLE), L"read_TCI: ttd_title");
	if (ttd_title == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		xret = xfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TCI_line(line, i_ti, tfield);
		//fwprintf(logstream, L"##### %d [%s]\n", i+1, tfield);
		wcscpy_s(ttd_title[i].name, MAX_TITLES, tfield);
		ttd_title[i].ndx = i;
		parse_TCI_line(line, i_py, tfield);
		ttd_title[i].year = _wtoi(tfield);
		i++;	
		if (xret == NULL)
			break;		
	}
	// sort the article titles, this is to be used in checking references
	qsort((void *)ttd_title, (size_t)nwos, sizeof(struct TTD_TITLE), compare_ttd_title);

	//for (i = 0; i < nwos; i++)
	//	fwprintf(logstream, L"%d %d[%s]\n", i+1, ttd_title[i].year, ttd_title[i].name); 

	// assume in average each title contains 100 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nwos * 100 * sizeof(struct TKEYWORDS), L"read_TCI: tkeyword");
	if (tkeyword == NULL) return MSG_NOT_ENOUGH_MEMORY;

#define USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS
#ifdef USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS
	ntkeywords = nkwde;
	for (i = 0; i < ntkeywords; i++)
	{
		wcscpy(tkeyword[i].name, kwde[i].name);
		tkeyword[i].cnt = kwde[i].cnt;
	}
	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = i; } // initialization
#else
	ndx = 0;
	for (i = 0; i < nwos; i++)
	{
		//fwprintf(logstream, L"[%s]\n", ptitle[i].name); fflush(logstream);
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
	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = i; } // initialization
	
	// sort to the order of counts
	//qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_wordcount);
	//for (i = 0; i < ntkeywords; i++) tkeyword[i].ranking = i;	// rank begins from 0
#ifdef DEBUG
	for (i = 0; i < ntkeywords; i++)
		fwprintf(logstream, L"%d %03d [%s]\n", i, tkeyword[i].cnt, tkeyword[i].name);
#endif DEBUG
	//fwprintf(logstream, L"\n"); 
	free(ptitle);	
	//qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_tkeyword); // sort again, back to the alphabetical order
#endif USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS

	for (i = 0; i < naus; i++) // initialization
	{
		authors[i].ncountries = 0;	
		authors[i].ncountries_rp = 0;	
		authors[i].groupid = 0;		
		authors[i].ndx = i;	
	}

	// 3rd pass, get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of WOS data
	wos = (struct WOS *)Jmalloc(nwos * sizeof(struct WOS), L"read_TCI: wos");
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;	
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		xret = xfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		wos[i].ndx = i;	
		parse_TCI_line(line, i_au, tfield);
		wos[i].nau = get_TCI_number_authors(tfield);
		parse_TCI_author_names(tfield, &wos[i].nau, wos[i].author, naus, authors);
#ifdef DEBUG
		for (k = 0; k < wos[i].nau; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, wos[i].nau, k, wos[i].author[k], authors[wos[i].author[k]].name); fflush(logstream); }
		fwprintf(logstream, L"\n");
#endif DEBUG
		parse_TCI_line(line, i_py, tfield);
		wos[i].year = _wtoi(tfield);
		//fwprintf(logstream, L"%d:%d %s[%s]\n", i, wos[i].year, wos[i].docid, wos[i].author);
		parse_TCI_line(line, i_de, tfield);
		parse_keywords(tfield, &wos[i].nde, wos[i].DE, nkwde, kwde);
		parse_TCI_line(line, i_so, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	
		if (tfield[0] == '\0') wcscpy(tfield, L"Unknown");	// journal name field is empty
		ndx2 = journalalias_search(journalalias, njournalalias, tfield);
		if (ndx2 >= 0)	// found that this is a variation of the journal title
			ndx = jname_search(journals, njrs, journalalias[ndx2].alias);
		else
			ndx = jname_search(journals, njrs, tfield);
		wos[i].journal = ndx;
#ifdef USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS
		wos[i].ntkws = wos[i].nde;
		for (k = 0; k < wos[i].ntkws; k++)
			wos[i].tkws[k] = wos[i].DE[k];
#else
		//parse_TCI_line(line, i_ti, tfield);
		//parse_title(tfield, 0, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);
		//parse_TCI_line(line, i_ab, tfield);		
		//parse_title(tfield, wos[i].ntkws, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array
#endif USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS		
		parse_TCI_line(line, i_vl, tfield);
		wchar_t tbuf[20];
		sp = tfield; tp = tbuf;
		while (*sp != ':' && *sp != '\0') *tp++ = *sp++; *tp = '\0';
		wos[i].volume = _wtoi(tbuf);
		if (*sp == ':')
		{
			sp++; tp = tbuf;
			while (*sp != ':' && *sp != '\0' && *sp != '=') *tp++ = *sp++; *tp = '\0';
			wos[i].issue = _wtoi(tbuf);
		}
		else 
			wos[i].issue = 0;
		parse_TCI_line(line, i_pg, tfield);
		sp = tfield; tp = tbuf;
		if (iswdigit(*sp) == 0 && *sp != '\0')	// 1st character is not a digit
			sp++;	
		while (*sp != '-' && *sp != '\0') *tp++ = *sp++; *tp = '\0';
		wos[i].bpage = _wtoi(tbuf);
		if (*sp == '-')
		{
			sp++; tp = tbuf;
			while (*sp != '\0') *tp++ = *sp++; *tp = '\0';
			wos[i].epage = _wtoi(tbuf);
		}
		else 
			wos[i].epage = wos[i].bpage;
		i++;	
		if (xret == NULL)
			break;
	}

	prepare_alias(2);	// the parameter indicates a different format than WOS data
	//for (i = 0; i < nwos; i++)
	//	fwprintf(logstream, L"%d [%s]\n", i+1, wos[i].alias); fflush(logstream);

	// create a docid for each paper, this is because TCI does not assign ID to each document
	for (i = 0; i < nwos; i++)
		create_TCI_docid(wos[i].docid, authors[wos[i].author[0]].name, wos[i].year, wos[i].volume, wos[i].issue, wos[i].bpage);

	if (no_wos_citation_file && !secondary_data)	// do not mass on citation if it is a secondary data (like in the MOST modality project)
	{	
		// open the citation file	
		strip_name_extension(sname, tname, dpath);
		if (swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname) == -1)
			return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"wt, ccs=UTF-8") != 0)
			return MSG_CFILE_CANNOTOPEN;

		// 4th pass, get reference information
		rewind(sstream);	// point back to the begining of the file
		// read the 1st line of the source file, it contains the field names
		if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		// read source file line by line
		i = 0; 
		while (TRUE)
		{		
			xret = xfgetws(line, XXLBUF_SIZE, sstream);
			if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
				continue;
			parse_TCI_line(line, i_au, tfield); 
			//fwprintf(logstream, L"\n***** %s %s [%s]\n", wos[i].docid, wos[i].alias, tfield);
			parse_TCI_line(line, i_cr, tfield);	
			parse_TCI_citation_info(tfield, i, wos[i].docid, cstream);
			i++;
			if (xret == NULL) break;
		} 
		clear_preparation_for_CR_search();
		fclose(cstream); 
	}

	fclose(sstream); 

	ret = link_author_WOS();   if (ret != 0) return ret;
	//ret = link_journal_WOS();  if (ret != 0) return ret; 
	//calculate_author_h_index();
	//calculate_journal_h_index();	

	// count the number of times each author as the 1st author
	for (i = 0; i < naus; i++) authors[i].cnt1 = 0;	// initialize 1st author count
	for (i = 0; i < nwos; i++) authors[wos[i].author[0]].cnt1++;	
	ret = coauthor(sname, 0); if (ret != 0) return ret;	// output coauthor network that contains all authors
	ret = coauthor(sname, 1); if (ret != 0) return ret;	// output coauthor network that contains only authors that have been 1st author
	
	ret = keyword_year_matrix(ntkeywords, tkeyword);
	if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;

	basic_wos_statistics();

	// sort the data by ISI document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	relink_author_WOS();	// this is necessary after the wos[] arrary is sorted
	//relink_journal_WOS();	// added 2016/11/19

	Jfree(line, L"read_TCI: line"); 
	Jfree(tfield, L"read_TCI: tfield");
	done_xfgetws();

	return 0;
}

//
// parse the 1st line of the TCI data file
// the 1st line looks like this:
// ",題名,題名(外文),作者,作者(外文),書刊名,系所名稱,學位類別,出版年民國,卷期,卷,頁次,開始頁,結束頁,出版日期,主題關鍵詞,主題關鍵詞(外文),中文摘要,英文摘要,論文參考文獻"
//
int parse_TCI_1st_line(wchar_t *line, wchar_t *tfname)
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
		if (*sp == '\"') { sp++; continue; }	// ignore the double quotes, allow the format with double quotes closing each column identifier
		if (*sp == '\n' || *sp == '\r') 
		{ 
			*tp = '\0'; fcnt++;
			if (wcscmp(fp, tfname) == 0)
				return fcnt;
		}
		ch = *sp; 
		if (ch == ',') state = 1;
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
// parse a data record prepared by TCI
// this function works with the situation that some text are enclosed with double quotes, some are not
//
int parse_TCI_line(wchar_t *line, int tind, wchar_t *tfield)
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
// split a given autor string into individual authors and then save them to the global author name array
// individual name is delimited by a '\n', names can be enclosed by double quotes
//
int parse_store_TCI_authors(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau, ndx2;
	int nau_this_article = 0;	

	tnau = *nau;
	sp = astr;
	// remove the leading double quotes if it is there
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == '\"') { sp++; continue; }
		if (ch == '\n') // author names are separated by a '\n'
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
				if (nau_this_article >= MAX_AUTHORS)	// take only MAX_AUTHORS authors
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
// parse a sting of author names and return the number of authors
//
int get_TCI_number_authors(wchar_t *allnames)
{
	wchar_t ch, *sp;
	int cnt;

	sp = allnames; 
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	cnt = 0;
	while (*sp != '\0')
	{
		ch = *sp++; 
		if (ch == '\n') 
			cnt++;
	}
	cnt++;

	//fwprintf(logstream, L"%d %s\n", cnt, allnames);
	return cnt;
}

//
// parse a sting of author names,
// individual name is delimited by a '\n', names can be enclosed by double quotes
// save the results into the wos[] array
//
int parse_TCI_author_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
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
		if (ch == '\n') 
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
// create TCI docid (in parallel with WOS doc ID)
//
int create_TCI_docid(wchar_t *docid, wchar_t *iau, int year, int volume, int issue, int bpage)
{
	int i;
	wchar_t aus[BUF_SIZE];
	wchar_t *sp, *tp;

	tp = aus; sp = iau;
	for (i = 0; i < 4; i++)
	{
		if (*sp == '\0') break;
		*tp++ = *sp++;
	}
	*tp = '\0';
	swprintf(docid, L"%s%04d%03d%02d%04d", aus, year, volume, issue, bpage);

	return 0;
}

//
// this function collects text strings until it sees "\r\n"
// it is useful in the situation of ".csv" file where each line is ended with "\r\n" and there are many "\n" within each line
//
//static wchar_t *cur_pos;
//static wchar_t *end_pos;
#ifdef XXX
int x_fgetws()
//wchar_t *xfgetws(wchar_t *line, int xxlbuf_size, FILE *sstream)
{
	wchar_t *sp, *tp;
	int nb_read;
	wchar_t *line;

	fwprintf(logstream, L"Dadada\n"); fflush(logstream);

	line = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_TCI: line");
	nb_read = 10;
	sp = line;
	tp = line + 100;
	*tp++ = *sp++;
	*tp++ = *sp++;
	*tp++ = *sp++;

	//tp = line;
	//sp = cur_pos;
#ifdef XXX
	while (*sp != '\0x0d')	// look for carriage return, each string is ended with 0x0D, 0x0A
	{		
		*tp++ = *sp++;	// copy over one byte
		cur_pos++;
		//if (cur_pos >= xxlbuf_size)
		//{
		//	nb_read = fread(xline, sizeof(unsigned char), xxlbuf_size, sstream);
		//	if (nb_read < xxlbuf_size)
		//		end_pos = nb_read;
		//	sp = cur_pos;
		//}
	}
	*tp = '\0'; 
	cur_pos += 2; // skip the sequence 0D,0A

	if (end_pos >= cur_pos)
		return NULL;
#endif 

	return 0;
	//return line;	
}
#endif XXX

