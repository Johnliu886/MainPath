// 
// LAWSNOTEdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the Lawsnote database
//
//
// Revision History:
// 2017/05/02 Basic function works
// 2017/05/20 Added codes to prepare alias, which adds court name to the document name
//
	
#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

int nsds;
struct SERIALDOCS *serialdocs;

extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;

static FILE *sstream;		// for source file
extern FILE *logstream;

extern int text_type;
extern int nnodes;
extern struct PN *nw;
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
extern int no_wos_citation_file;
extern struct TTD_TITLE *ttd_title;

int parse_LAWSNOTE_1st_line(wchar_t *, wchar_t *);
int parse_LAWSNOTE_line(wchar_t *, int, wchar_t *);
int parse_store_LAWSNOTE_authors(wchar_t *, int *, struct AUTHORS *);
int parse_store_LAWSNOTE_authors2(wchar_t *, int *, struct AUTHORS *);
int get_LAWSNOTE_number_authors(wchar_t *);
int parse_LAWSNOTE_author_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_LAWSNOTE_author_names2(wchar_t *, int *, int *, int, struct AUTHORS *);
int check_author_name_format_type(wchar_t *);
int compare_serialdocid(const void *, const void *);

extern int prepare_author_alias_list();

extern int prepare_keyword_alias_list();
extern int parse_keywords(wchar_t *, int *, int *, int, struct KWORDS *);
extern int parse_store_keywords(wchar_t *, int *, struct KWORDS *);
extern int parse_store_title(int, struct TKEYWORDS *, wchar_t *);	
extern int parse_title(wchar_t *, int, int *, int *, int, struct TKEYWORDS *);
extern int compare_kname(const void *, const void *);
extern int compare_author(const void *, const void *);
extern int compare_tkeyword(const void *, const void *);
extern int compare_wordcount(const void *, const void *);
extern int is_stopwords(wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int prepare_alias(int);
extern int parse_countries(int, wchar_t *, int, struct AUTHORS *);
extern int parse_countries_rp(int, wchar_t *, int, struct AUTHORS *);
extern int prepare_for_CR_search();
extern int clear_preparation_for_CR_search();
extern int parse_LAWSNOTE_citation_info(wchar_t *, int, wchar_t *, FILE *);
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int link_author_WOS();
extern int coauthor(wchar_t *, int);		
extern int calculate_author_h_index();
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int basic_wos_statistics();
extern int compare_docid(const void *, const void *);
extern int relink_author_WOS();
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int compare_ttd_title(const void *, const void *);

#define XXLBUF_SIZE 65536*4	// expecting extremely long lines
#define BUF_SIZE 1024

static wchar_t *xline;
static int n_fields = 0;
static int exit_at_next_line = 0;
int init_lnfgetws(FILE *sstream)
{
	xline = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"init_lnfgetws: xline");

	return 0;
}

int done_lnfgetws()
{
	Jfree(xline, L"done_lnfgetws: xline");

	return 0;
}

//
//
//
wchar_t *lnfgetws(wchar_t *line, int bufsize, FILE *sstream)
{
	if (fgetws(line, XXLBUF_SIZE, sstream) == NULL) 
		return NULL;
	else 
		return line;
}

#ifdef XXX
//
// this function collects text strings until all fields are collected 
// the reason why this is necessary is that LAWSNOTE data (.csv file) embed many line feeds in a record and end a record with "\r\n"
// what's more, fgetws() function changes "\r\n" sequence to '\n' such that we cannot rely on the sequence to find a record
//
wchar_t *lnfgetws(wchar_t *line, int bufsize, FILE *sstream)
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
		if (exit_at_next_line == 0) // don't read for the 1st time, because the line is already read in the previous call
		{
			retstat = fgetws(tline, XXLBUF_SIZE, sstream);
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
			if (*sp == ',')	// the line begins with digits follows by a ','
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
#endif XXX

//
// read LAWSNOTE data and put it into the wos[] array
//
int read_LAWSNOTE(wchar_t *sname)
{
	int i, k, ndx, ndx2;
	int nsr, cnt, ret;
	int i_ut, i_au, i_de, i_py, i_sc, i_cr, i_ab, i_sd;
	wchar_t *line;
	wchar_t *nline;
	wchar_t *tfield;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t *sp, *tp;
	wchar_t *xret;
	FILE *cstream;		// for citation file

	// allocate the working memory
	line = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_LAWSNOTE: line");
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_LAWSNOTE: tfield");
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	text_type = FT_UTF_8;	// enforce the text type for LAWSNOTE file
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

	// find the number of fields from the data in the 1st line
	fgetws(line, XXLBUF_SIZE, sstream);
	sp = line;
	while (*sp != '\n') 
	{ 
		if (*sp == '\t') 
			n_fields++; 
		sp++;	
	}	// Lawsnote insist on using tab as delimiter
	n_fields++;	// but the number of delimiter (tab) is n_fields-1
	//fwprintf(logstream, L"#####n_fields = %d\n", n_fields);

	// 1st pass, count the number of target records 
	init_lnfgetws(sstream); // prepare for lnfgetws() to work
	nsr = 0;
	while (TRUE)
	{	
		if (lnfgetws(line, XXLBUF_SIZE, sstream) == NULL)
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
	fwprintf(logstream, L"Number of documents: %d\n", nwos);

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	nline = line;
	//if (line[0] == 0xfe && line[1] == 0xff)	// UTF-16 (big-endian) stream leading code: "FE FF" (in hex)?????
	//	nline += 2;
	// look for the index of the specified target field name 
	i_ut = 1; // temp code
	//i_ut = parse_LAWSNOTE_1st_line(nline, L"no");	// decision id
	i_au = parse_LAWSNOTE_1st_line(nline, L"court");	// use "court" for now
	i_de = parse_LAWSNOTE_1st_line(nline, L"reason");	// use "reason" for now
	i_py = parse_LAWSNOTE_1st_line(nline, L"date");			// date
	i_cr = parse_LAWSNOTE_1st_line(nline, L"noInContent");		// documents this decision references
	i_ab = parse_LAWSNOTE_1st_line(nline, L"mainText");	// abstract
	i_sd = parse_LAWSNOTE_1st_line(nline, L"histories");	// serial documents

	if (i_ut == -1 || i_au == -1 || i_de == -1 || i_py == -1 || i_cr == -1 || i_ab == -1 || i_sd == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	prepare_keyword_alias_list();	// this is so that we can handle various form of a keyword
	// 2nd pass, get keyword data and save it
	// allocate memory for the author keyword data
	kwde = (struct KWORDS *)Jmalloc(nwos * 12 * sizeof(struct KWORDS), L"read_LAWSNOTE: kwde");	// estimate in average 12 keywords per document
	if (kwde == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; nkwde = 0;
	while (TRUE)
	{		
		xret = lnfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_LAWSNOTE_line(line, i_de, tfield);
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
	if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the author name array
	authors = (struct AUTHORS *)Jmalloc(nwos * 5 * sizeof(struct AUTHORS), L"read_LAWSNOTE: authors");	// estimate in average 5 authors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		xret = lnfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_LAWSNOTE_line(line, i_au, tfield);
		parse_store_LAWSNOTE_authors(tfield, &naus, authors);
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

#ifdef XXX
	// obtain and process article title information
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the title keyword array
	ttd_title = (struct TTD_TITLE *)Jmalloc(nwos * sizeof(struct TTD_TITLE), L"read_LAWSNOTE: ttd_title");
	if (ttd_title == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		xret = lnfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_LAWSNOTE_line(line, i_ti, tfield);
		wcscpy_s(ttd_title[i].name, MAX_TITLES, tfield);
		ttd_title[i].ndx = i;
		parse_LAWSNOTE_line(line, i_py, tfield);
		ttd_title[i].year = _wtoi(tfield);
		i++;	
		if (xret == NULL)
			break;		
	}
	// sort the article titles, this is to be used in checking references
	qsort((void *)ttd_title, (size_t)nwos, sizeof(struct TTD_TITLE), compare_ttd_title);
#endif XXX

	//for (i = 0; i < nwos; i++)
	//	fwprintf(logstream, L"%d %d[%s]\n", i+1, ttd_title[i].year, ttd_title[i].name); 

#ifdef XXX
	// assume in average each title contains 100 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nwos * 100 * sizeof(struct TKEYWORDS), L"read_LAWSNOTE: tkeyword");
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
#endif XXX

	for (i = 0; i < naus; i++) // initialization
	{
		authors[i].groupid = 0;		
		authors[i].ndx = i;	
	}

	// 3rd pass, get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of WOS data
	wos = (struct WOS *)Jmalloc(nwos * sizeof(struct WOS), L"read_LAWSNOTE: wos");
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;	
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		xret = lnfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		wos[i].ndx = i;	
		parse_LAWSNOTE_line(line, i_ut, tfield);
		wcscpy(wos[i].docid, tfield);
		parse_LAWSNOTE_line(line, i_au, tfield);
		wos[i].nau = get_LAWSNOTE_number_authors(tfield);
		parse_LAWSNOTE_author_names(tfield, &wos[i].nau, wos[i].author, naus, authors);
#ifdef DEBUG
		for (k = 0; k < wos[i].nau; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, wos[i].nau, k, wos[i].author[k], authors[wos[i].author[k]].name); fflush(logstream); }
		fwprintf(logstream, L"\n");
#endif DEBUG
		parse_LAWSNOTE_line(line, i_py, tfield);
		wos[i].year = _wtoi(tfield);
		//fwprintf(logstream, L"%d:%d %s[%s]\n", i, wos[i].year, wos[i].docid, wos[i].author);
		parse_LAWSNOTE_line(line, i_de, tfield);
		parse_keywords(tfield, &wos[i].nde, wos[i].DE, nkwde, kwde);
#ifdef XXX
#ifdef USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS
		wos[i].ntkws = wos[i].nde;
		for (k = 0; k < wos[i].ntkws; k++)
			wos[i].tkws[k] = wos[i].DE[k];
#else
		//parse_LAWSNOTE_line(line, i_ti, tfield);
		//parse_title(tfield, 0, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);
		//parse_LAWSNOTE_line(line, i_ab, tfield);		
		//parse_title(tfield, wos[i].ntkws, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array
#endif USE_AUTHOR_KEYWORDS_FOR_TITLE_KEYWRODS		
#endif XXX
		i++;	
		if (xret == NULL)
			break;
	}

	// yet another pass, get serial document data (歷審資料)
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of serial document data
	int mest = 1000;
	if ((nwos / 10) < 1000) mest = 1000; 
	serialdocs = (struct SERIALDOCS *)Jmalloc(mest * sizeof(struct SERIALDOCS), L"read_LAWSNOTE: wserialdocs");
	if (serialdocs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	i = 0; nsds = 0;
	while (TRUE)
	{		
		xret = lnfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_LAWSNOTE_line(line, i_sd, tfield);
		fwprintf(logstream, L"[%s]\n", tfield);
		if (tfield[0] != '\0')	// it is a member of one of the serial document set
		{
			wcscpy(serialdocs[nsds].sdocid, tfield);
			wcscpy(serialdocs[nsds].docname[0], wos[i].docid);
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

	// consolidate duplicate serial documents names
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
#ifdef DEBUG
	for (i = 0; i < nsds; i++)
	{
		fwprintf(logstream, L"##### %d [%s]\n", serialdocs[i].nd, serialdocs[i].sdocid);
		for (k = 0; k < serialdocs[i].nd; k++)
			fwprintf(logstream, L"{%d %s} ", serialdocs[i].ndx[k], serialdocs[i].docname[k]);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	// prepare the alias
	for (i = 0; i < nwos; i++)
	{
		wchar_t court[100];
		int tstate;
		sp = authors[wos[i].author[0]].name;
		tp = court;
		tstate = 0;
		while (*sp != '\0')
		{
			switch (tstate)
			{
			case 0:
				if (*sp == L'臺')
				{
					sp++; tstate = 1;
					break;
				}
				else if (*sp == L'智')	// 智慧財產法院
				{
					wcscpy(tp, L"智財法院");
					sp += 6; tp += 4; tstate = 2;
					break;
				}
				else
				{					
					*tp++ = *sp++; tstate = 2;
				}
			case 1:
				if (*sp == L'灣')
				{
					sp++; tstate = 2;
					break;
				}
				else
					*tp++ = *sp++;
				break;
			case 2:
				if (*sp == ' ')
				{
					sp++;
					break;
				}
				else if (*sp == L'分')	// ignore "分院"
				{
					sp += 2;
					break;
				}
				else if (*sp == L'地')	// 地方法院
				{
					wcscpy(tp, L"地院");
					sp += 4; tp += 2; tstate = 2;
					break;
				}
				else
					*tp++ = *sp++;
				break;
			default:
				break;
			}
		}
		*tp = '\0';
		swprintf(wos[i].alias, L"%s_%s", wos[i].docid, court);
	}

	if (no_wos_citation_file)
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
		if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		// read source file line by line
		i = 0; 
		while (TRUE)
		{		
			xret = lnfgetws(line, XXLBUF_SIZE, sstream);
			if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
				continue;
			parse_LAWSNOTE_line(line, i_au, tfield); 
			//fwprintf(logstream, L"\n***** %s %s [%s]\n", wos[i].docid, wos[i].alias, tfield);
			parse_LAWSNOTE_line(line, i_cr, tfield);	
			parse_LAWSNOTE_citation_info(tfield, i, wos[i].docid, cstream);
			i++;
			if (xret == NULL) break;
		} 
		clear_preparation_for_CR_search();
		fclose(cstream); 
	}

	fclose(sstream); 

	ret = link_author_WOS();   if (ret != 0) return ret;

	// count the number of times each author as the 1st author
	for (i = 0; i < naus; i++) authors[i].cnt1 = 0;	// initialize 1st author count
	for (i = 0; i < nwos; i++) authors[wos[i].author[0]].cnt1++;	
	ret = coauthor(sname, 0); if (ret != 0) return ret;	// output coauthor network that contains all authors
	ret = coauthor(sname, 1); if (ret != 0) return ret;	// output coauthor network that contains only authors that have been 1st author
	
#ifdef XXX
	ret = keyword_year_matrix(ntkeywords, tkeyword);
	if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;

	basic_wos_statistics();
#endif XXX

	// sort the data by ISI document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	relink_author_WOS();	// this is necessary after the wos[] arrary is sorted

	Jfree(line, L"read_LAWSNOTE: line"); 
	Jfree(tfield, L"read_LAWSNOTE: tfield");
	done_lnfgetws();

	return 0;
}

//
// parse the 1st line of the LAWSNOTE data file
// the 1st line looks like this:
// ",題名,題名(外文),作者,作者(外文),書刊名,系所名稱,學位類別,出版年民國,卷期,卷,頁次,開始頁,結束頁,出版日期,主題關鍵詞,主題關鍵詞(外文),中文摘要,英文摘要,論文參考文獻"
//
int parse_LAWSNOTE_1st_line(wchar_t *line, wchar_t *tfname)
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
		if (ch == '\t') 
			state = 1;
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
// parse a data record prepared by LAWSNOTE
// this function works with the situation that some text are enclosed with double quotes, some are not
//
int parse_LAWSNOTE_line(wchar_t *line, int tind, wchar_t *tfield)
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
			if (ch == '\t')	// this field is empty
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
		case 1:	// waiting for the delimiter '\t'
			if (ch == '\t') 
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
// parse the "noInContent" field string, citations are delimeted by ";"
//
int parse_LAWSNOTE_citation_info(wchar_t *str, int ind, wchar_t *pid, FILE *stream)
{
	int i;
	int cnt;
	wchar_t *sp, *tp, ch;
	wchar_t bfr[LBUF_SIZE];

	if (!no_wos_citation_file)
		return 0;

	sp = str; tp = bfr;
	while (*sp != '\0')
	{
		ch = *sp;
		if (ch == L';'|| ch == L'\n' || ch == L'\r' || ch == L'\0')
		{
			*tp = L'\0';
			if (wcscmp(pid, bfr) != 0)	// avoid those pointing to themselves
				fwprintf(stream, L"%s %s\n", bfr, pid);	// write the results
			tp = bfr; 
		}
		else
			*tp++ = *sp;
		sp++;
	}
	*tp = L'\0';
	if (wcscmp(pid, bfr) != 0)	// avoid those pointing to themselves
		fwprintf(stream, L"%s %s\n", bfr, pid);	// write the results

	return 0;
}

//
// split a given autor string into individual authors and then save them to the global author name array
// individual name is delimited by a '\n', names can be enclosed by double quotes
//
int parse_store_LAWSNOTE_authors(wchar_t *astr, int *nau, struct AUTHORS *au)
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
int get_LAWSNOTE_number_authors(wchar_t *allnames)
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
int parse_LAWSNOTE_author_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
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
// this fucntion is to be called by qsort() only
// 
int compare_serialdocid(const void *n1, const void *n2)
{
	struct SERIALDOCS *t1, *t2;
	
	t1 = (struct SERIALDOCS *)n1;
	t2 = (struct SERIALDOCS *)n2;
	if (wcscmp(t2->sdocid, t1->sdocid) < 0)
		return 1;
	else if (wcscmp(t2->sdocid, t1->sdocid) == 0)
		return 0;
	else return -1;
}