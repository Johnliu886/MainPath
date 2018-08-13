// 
// Scopusdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the Scopus database_
//
//
// Revision History:
// 2014/05/16 Basic function works
// 2014/05/17 Removed the codes that assign tkeyword[].ranking, do it now in the function keyword_year_matrix()
// 2014/07/06 Changed the estimation of the number of average "Index Keywords" from 12 to 20, a data set provided by Louis has this over 12
// 2014/10/11 Added code to take page end data (i_ep, epage)
// 2014/10/29 Fixed a problem in the function parse_store_Scopus_authors() by limiting the number of authors if there are too many authors
// 2014/10/29 Changed to allocate more memory for tkeyword (changed from aerage 60 per article to 100)
// 2014/10/29 Changed to call coauthor() twice, i.e. write out two type of coauthor networks
// 2014/10/31 Changed to use EID (similar to UT in WOS) if it exist
// 2014/10/31 Added check to the return of prepare_for_CR_search()
// 2015/02/02 Increased the memory allocation size for "kwid"
// 2015/03/31 Added codes to handle journal alias (most of the codes added are in the file "journal alias.txt")
// 2015/09/29 Changed to adjust author last name earlier, added a new function adjust_author_last_name()
// 2015/10/29 Added codes to automatically differenciate between two formats of author names "Chen J.S." and "Chen, J.S.", the later is seen after October, 2015
// 2016/04/06 added codes to handle a new 1st line format ["Authors","Title","Year","Source title","Volume","Issue","Art. No."]
//                this is done by ignoring the double quotes closing each column identifier
// 2016/07/02 Moved to call prepare_keyword_alias_list() before calling parse_store_keywords()	
// 2016/07/05 Added to call consolidate_keywords() in the function read_Scopus(). This is a necessary step but was missed in the previous release.
// 2016/10/29 Added codes to re-allocate memory when "kwde" or "kwid" is full
// 2016/10/29 Added codes to ignore lines begin with ",," for Scopus data
// 2016/11/19 Added to call relink_journal_WOS()
// 2017/02/12 Extended the length of journal abbreviation from 50 to 80 (modification is in the network.h file)
// 2017/02/28 Removed all calls to adjust_author_last_name(), these calls were added 2015/10/29 but they were mistakes (not necessary for Scopus data)
// 2017/02/28 Changed to open source file in UTF-8 mode if the file is in UTF-8 format
// 2017/03/01 Extended the estimated number of authors from 8 to 12 when allocating memeory
// 2018/05/09 Added codes to read and process 'paper family' data, 
//                 a new column 'FA' needs to be added manually to indicate those patents that are in the same loop (treat them as a family)
//	

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

int Scopus_EID = 0;	// indicate if Scopus EID column exists in data
int Scopus_author_name_format = 0;

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

extern int nsds;					// added 2018/05/09
extern struct SERIALDOCS *serialdocs;	// added 2018/05/09

int parse_Scopus_1st_line(wchar_t *, wchar_t *);
int parse_Scopus_line(wchar_t *, int, wchar_t *);
int parse_store_Scopus_authors(wchar_t *, int *, struct AUTHORS *);
int parse_store_Scopus_authors2(wchar_t *, int *, struct AUTHORS *);
int get_Scopus_number_authors(wchar_t *);
int parse_Scopus_author_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_Scopus_author_names2(wchar_t *, int *, int *, int, struct AUTHORS *);
int create_Scopus_docid(wchar_t *, wchar_t, int, int, int);
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
extern int parse_countries_Scopus(int, wchar_t *, int, struct AUTHORS *);
extern int parse_countries_rp_Scopus(int, wchar_t *, int, struct AUTHORS *);
extern int prepare_for_CR_search();
extern int clear_preparation_for_CR_search();
extern int parse_Scopus_citation_info(wchar_t *, int, wchar_t *, FILE *);
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
extern int adjust_author_last_name(wchar_t *);
extern int consolidate_countries(int, struct AUTHORS *);
extern int consolidate_keywords();
extern int compare_serialdocid(const void *, const void *);

#define AUTHOR_NAME_FORMAT1	1	// "Chen J.S."
#define AUTHOR_NAME_FORMAT2	2	// "Chen, J.S.", after Oct, 2015
#define AUTHOR_NAME_FORMAT_UNKNOWN -1

#define XXLBUF_SIZE 65536*4	// expecting extremely long lines
#define BUF_SIZE 1024

//
// read Scopus data and put it into the wos[] array
//
int read_Scopus(wchar_t *sname)
{
	int i, k, ndx, ndx2;
	int nsr, cnt, ret;
	int i_ut, i_au, i_de, i_id, i_py, i_so, i_tc, i_sc, i_cr, i_di, i_vl, i_bp, i_ep, i_ab;
	int i_c1, i_rp;		// added 2013/04/10
	int i_ti;			// added 2013/07/12
	int i_fa;			// added 2018/05/09
	wchar_t *line;
	wchar_t *nline;
	wchar_t *tfield;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t *sp;
	FILE *cstream;		// for citation file

	// allocate the working memory
	line = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_Scopus: line");
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_Scopus: tfield");
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else if (text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)	// open as text mode for UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	// 1st pass, count the number of target records 
	nsr = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		nsr++;	
	}
	nwos = nsr - 1;	// less the 1st line (format line)

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if (text_type == FT_ASCII)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		nline = line;
	}
	else if (text_type == FT_UTF_8)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		nline = &line[1];	// this works, but don't know the reason
	}
	else	// Unicode type
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		nline = &line[1];	// actually the 3rd byte
	}

	// look for the index of the specified target field name 
	i_au = parse_Scopus_1st_line(nline, L"Authors");	// author names
	i_de = parse_Scopus_1st_line(nline, L"Author Keywords");	// autor keywords
	i_id = parse_Scopus_1st_line(nline, L"Index Keywords");	// index keywords
	i_py = parse_Scopus_1st_line(nline, L"Year");			// year of publication
	i_so = parse_Scopus_1st_line(nline, L"Source title");	// journal name
	i_tc = parse_Scopus_1st_line(nline, L"Cited by");		// total citation
	i_cr = parse_Scopus_1st_line(nline, L"References");		// documents this article references
	i_di = parse_Scopus_1st_line(nline, L"DOI");	// DOI information
	i_vl = parse_Scopus_1st_line(nline, L"Volume");	// volume
	i_bp = parse_Scopus_1st_line(nline, L"Page start");	// beginning page
	i_ep = parse_Scopus_1st_line(nline, L"Page end");	// ending page
	i_ab = parse_Scopus_1st_line(nline, L"Abstract");	// abstract
	i_c1 = parse_Scopus_1st_line(nline, L"Authors with affiliations");	// author address, added 2013/04/10
	i_rp = parse_Scopus_1st_line(nline, L"Correspondence Address");		// correspondence address
	i_ti = parse_Scopus_1st_line(nline, L"Title");		// title
	i_ut = parse_Scopus_1st_line(nline, L"EID");		// EID, the unique ID for an article
	if (i_ut != -1) Scopus_EID = 1;
	i_fa = parse_Scopus_1st_line(nline, L"FA");		// paper family, added 2018/05/09

	if (i_au == -1 || i_de == -1 || i_id == -1 || i_py == -1 || i_so == -1 || i_tc == -1 || i_cr == -1 || 
		i_di == -1 || i_vl == -1 || i_bp == -1 || i_ep == -1 || i_ab == -1 || i_c1 == -1 || i_rp == -1 || i_ti == -1 || i_ut == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	prepare_keyword_alias_list();	// this is so that we can handle various form of a keyword, moved up to here 2016/07/02
	// 2nd pass, get keyword data and save it
	// allocate memory for the author keyword data
	int size_kwde;
	size_kwde = nwos * 10;	// estimate in average 10 keywords per document
	kwde = (struct KWORDS *)Jmalloc(size_kwde * sizeof(struct KWORDS), L"read_Scopus: kwde");	// estimate in average 10 keywords per document
	if (kwde == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; nkwde = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_de, tfield);
		parse_store_keywords(tfield, &nkwde, kwde);
		//fwprintf(logstream, L"%d:%d: %s\n", i, nkwde, tfield); fflush(logstream);
		if ((nkwde + 300) > size_kwde)	// memory is running out, added 2016/10/29
		{
			size_kwde = size_kwde + 10 * nwos;	// increase the memory size
			kwde = (struct KWORDS *)realloc(kwde, size_kwde * sizeof(struct KWORDS));	// re-alloac memory of larger size
			if (kwde == NULL) return MSG_NOT_ENOUGH_MEMORY;
		}
		i++;	
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

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the Keyword Plus keyword data	
	int size_kwid;
	size_kwid = nwos * 20;	// estimate in average 20 keywords per document, but added re-allocation scheme, 2016/10/29
	kwid = (struct KWORDS *)Jmalloc(size_kwid * sizeof(struct KWORDS), L"read_Scopus: kwid");
	if (kwid == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; nkwid = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_id, tfield);
		parse_store_keywords(tfield, &nkwid, kwid);
		//fwprintf(logstream, L"%d:%d: %s\n", i, nkwid, tfield); fflush(logstream);
		if ((nkwid + 300) > size_kwid)	// memory is running out, added 2016/10/29
		{
			size_kwid = size_kwid + 10 * nwos;	// increase the memory size
			kwid = (struct KWORDS *)realloc(kwid, size_kwid * sizeof(struct KWORDS));	// re-alloac memory of larger size
			if (kwid == NULL) return MSG_NOT_ENOUGH_MEMORY;
		}
		i++;	
	}
	qsort((void *)kwid, (size_t)nkwid, sizeof(struct KWORDS), compare_kname);
	// consolidate duplicate keywords
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < nkwid; i++)
	{
		if (wcscmp(kwid[i].name, prev_name) != 0)	// hit new name
		{
			wcscpy_s(kwid[k++].name, MAX_KEYWORD_NAME, kwid[i].name); 
			wcscpy_s(prev_name, MAX_KEYWORD_NAME, kwid[i].name); 
		}
	}
	nkwid = k;

	prepare_author_alias_list();	// this is so that we can handle various form of an author's name
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// check the format of the author names, added 2015/10/29
	int format1 = 0; int format2 = 0;
	for (i = 0; i < 5; i++)	// check only the 1st 5 lines
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_Scopus_line(line, i_au, tfield);
		if (check_author_name_format_type(tfield) == AUTHOR_NAME_FORMAT1)
			format1++;
		if (check_author_name_format_type(tfield) == AUTHOR_NAME_FORMAT2)
			format2++;
	}
	if (format2 >= format1)
	{
		Scopus_author_name_format = AUTHOR_NAME_FORMAT2;
		fwprintf(logstream, L"Scopus author name format type: \"Chen, J.S.\"");
	}
	else
	{
		Scopus_author_name_format = AUTHOR_NAME_FORMAT1;
		fwprintf(logstream, L"Scopus author name format type: \"Chen J.S.\"");
	}

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the author name array
	authors = (struct AUTHORS *)Jmalloc(nwos * 12 * sizeof(struct AUTHORS), L"read_Scopus: authors");	// estimate in average 12 authors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_au, tfield);
		if (Scopus_author_name_format == AUTHOR_NAME_FORMAT1)
			parse_store_Scopus_authors(tfield, &naus, authors);	// "Chen J.S."
		else
			parse_store_Scopus_authors2(tfield, &naus, authors);	// "Chen, J.S.", added 2015/10/28
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

	prepare_journal_alias_list();	// this is so that we can handle the situation when journal change titles, added 2015/03/31
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the journal name array
	journals = (struct JOURNALS *)Jmalloc(nwos * sizeof(struct JOURNALS), L"read_Scopus: journals");
	if (journals == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_so, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }		// added 2015/03/31
		ndx2 = journalalias_search(journalalias, njournalalias, tfield);	// added 2015/03/31
		if (ndx2 >= 0)	// found that this is a variation of the journal title
			wcscpy_s(journals[i].name, MAX_JOURNAL_NAME, journalalias[ndx2].alias);
		else
			wcscpy_s(journals[i].name, MAX_JOURNAL_NAME, tfield);
		i++;	
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
	struct PTITLES *ptitle;	// paper titles
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the title keyword array
	ptitle = (struct PTITLES *)Jmalloc(nwos * sizeof(struct PTITLES), L"read_Scopus: ptitle");
	if (ptitle == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_ti, tfield);
		wcscpy_s(ptitle[i].name, MAX_TITLES, tfield);
		parse_Scopus_line(line, i_py, tfield);
		ptitle[i].year = _wtoi(tfield);
		i++;	
	}

	// assume in average each title contains 100 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nwos * 100 * sizeof(struct TKEYWORDS), L"read_Scopus: tkeyword");	// changed from 60 to 100, 2014/10/29
	if (tkeyword == NULL) return MSG_NOT_ENOUGH_MEMORY;
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
	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = i; } // initialization, added 2014/05/17
	
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

	for (i = 0; i < naus; i++) // initialization
	{
		authors[i].ncountries = 0;	
		authors[i].ncountries_rp = 0;	
		authors[i].groupid = 0;		// added 2015/10/07	
		authors[i].ndx = i;			// added 2015/10/08
	}

	// 3rd pass, get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of WOS data
	wos = (struct WOS *)Jmalloc(nwos * sizeof(struct WOS), L"read_Scopus: wos");
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;	
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		wos[i].ndx = i;	
		parse_Scopus_line(line, i_au, tfield);
		wos[i].nau = get_Scopus_number_authors(tfield);
		if (Scopus_author_name_format == AUTHOR_NAME_FORMAT1)
			parse_Scopus_author_names(tfield, &wos[i].nau, wos[i].author, naus, authors);	// "Chen J.S."
		else
			parse_Scopus_author_names2(tfield, &wos[i].nau, wos[i].author, naus, authors);	// "Chen, J.S.", added 2015/1028
		
#ifdef DEBUG
		for (k = 0; k < wos[i].nau; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, wos[i].nau, k, wos[i].author[k], authors[wos[i].author[k]].name); fflush(logstream); }
		fwprintf(logstream, L"\n");
#endif DEBUG
		parse_Scopus_line(line, i_py, tfield);
		wos[i].year = _wtoi(tfield);
		//fwprintf(logstream, L"%d:%d %s[%s]\n", i, wos[i].year, wos[i].docid, wos[i].author);
		parse_Scopus_line(line, i_de, tfield);
		parse_keywords(tfield, &wos[i].nde, wos[i].DE, nkwde, kwde);
		parse_Scopus_line(line, i_id, tfield);
		parse_keywords(tfield, &wos[i].nid, wos[i].ID, nkwid, kwid);
		parse_Scopus_line(line, i_so, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }		// added 2015/03/31		
		ndx2 = journalalias_search(journalalias, njournalalias, tfield);	// added 2015/03/31
		if (ndx2 >= 0)	// found that this is a variation of the journal title
			ndx = jname_search(journals, njrs, journalalias[ndx2].alias);
		else
			ndx = jname_search(journals, njrs, tfield);
		wos[i].journal = ndx;
		parse_Scopus_line(line, i_tc, tfield);
		wos[i].tc = _wtoi(tfield);
		parse_Scopus_line(line, i_ti, tfield);
		parse_title(tfield, 0, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);
		parse_Scopus_line(line, i_ab, tfield);		
		parse_title(tfield, wos[i].ntkws, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array
#ifdef DEA_APPLICATIONS
		parse_WOS_line(line, SP_TAB, i_in, tfield);	// the content of this field is the type of applications, or empty
		if (tfield[0] == '\0' || tfield[0] == ' ')
			wos[i].app_type = 0;	// theoretical
		else
			wos[i].app_type = 1;	// non-theoretical
#endif DEA_APPLICATIONS
		parse_Scopus_line(line, i_di, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	// turn into lower case
		wcscpy_s(wos[i].doi, MAX_DOI_LEN, tfield);	
		//fwprintf(logstream, L"%d:%s", i, wos[i].doi);
		//fwprintf(logstream, L"\n"); fflush(logstream);
		parse_Scopus_line(line, i_vl, tfield);
		wos[i].volume = _wtoi(tfield);
		parse_Scopus_line(line, i_bp, tfield);
		if (iswdigit(tfield[0]) == 0 && tfield[0] != '\0')	// 1st character is not a digit
			wos[i].bpage = _wtoi(tfield+1);	// skip the 1st character whether it's a 'A' or 'B', etc	// 2011/08/22
		else
			wos[i].bpage = _wtoi(tfield);
		parse_Scopus_line(line, i_ep, tfield);	// added 2014/10/11
		if (iswdigit(tfield[0]) == 0 && tfield[0] != '\0')	// 1st character is not a digit
			wos[i].epage = _wtoi(tfield+1);	// skip the 1st character whether it's a 'A' or 'B', etc
		else
			wos[i].epage = _wtoi(tfield);
		if (Scopus_EID == 1)
		{
			parse_Scopus_line(line, i_ut, tfield);	// added 2014/10/31
			wcscpy_s(wos[i].docid, MAX_DOC_ID, tfield);	
		}
		i++;	
	}

	//adjust_author_names();	// this is because that CR field removed spaces in last names, we have to do the same thing in the author name field 
	prepare_alias(1);

	if (Scopus_EID == 0)
	{
		// create a docid for each paper, this is because Scopus does not assign ID to each document
		for (i = 0; i < nwos; i++)
			create_Scopus_docid(wos[i].docid, authors[wos[i].author[0]].name[0], wos[i].year, wos[i].volume, wos[i].bpage);
	}

	consolidate_keywords();		// added 2016/07/05

#ifdef DEBUG  // for tkeyword array
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%d %s\t%d: ", i, wos[i].alias, wos[i].ntkws); fflush(logstream);
		for (k = 0; k < wos[i].ntkws; k++)
			fwprintf(logstream, L"[%d:%s] ", k, tkeyword[wos[i].tkws[k]].name);
		fwprintf(logstream, L"\n");
		fflush(logstream);
	}
#endif DEBUG

	// the following codes that process country data are added 2017/02/26
	// 4th pass, parse country data, do it seperately because it needs complete author data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;

	for (i = 0; i < nwos; i++) { wos[i].ncountries = 0;	wos[i].ncountries_rp = 0; } // initialization

	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_Scopus_line(line, i_c1, tfield);
		parse_countries_Scopus(i, tfield, naus, authors);
		parse_Scopus_line(line, i_rp, tfield);
		parse_countries_rp_Scopus(i, tfield, naus, authors);		
		i++;	
	}

	consolidate_countries(naus, authors); // added 2017/02/26

	// following code block that process 'paper family' (serialdoc) data is added 2018/05/09
	// yet another pass, get 'paper family' data, the column 'FA' needs to be added manually to indicate those data that has loop
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of serial document data
	int mest = 1000;
	if ((nwos / 10) < 1000) mest = 1000; 
	serialdocs = (struct SERIALDOCS *)Jmalloc(mest * sizeof(struct SERIALDOCS), L"read_Scopus: serialdocs");
	if (serialdocs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	i = 0; nsds = 0; 
	wchar_t *xret;
	while (TRUE)
	{		
		xret = fgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_Scopus_line(line, i_fa, tfield);
		if (_wtoi(tfield) != 0)	// it is a member of one of the serial document set
		{
			//fwprintf(logstream, L"Paper family: %s [%s]\n", tfield, wos[i].docid);
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

	// consolidate the same paper family into one serialdocs structure
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
		for (i = 0; i < nsds; i++)
		{
			fwprintf(logstream, L"Paper family %s: %d documents\n", serialdocs[i].sdocid, serialdocs[i].nd);
			for (k = 0; k < serialdocs[i].nd; k++)
				fwprintf(logstream, L"{%d %s} ", serialdocs[i].ndx[k], wos[serialdocs[i].ndx[k]].alias);
			fwprintf(logstream, L"\n");
		}
	}
	// code for paper family ends here

	if (no_wos_citation_file)
	{	
		// open the citation file	
		strip_name_extension(sname, tname, dpath);
		if (swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname) == -1)
			return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"w") != 0)
			return MSG_CFILE_CANNOTOPEN;
		if (prepare_for_CR_search() == MSG_NOT_ENOUGH_MEMORY)	// this check is added 2014/10/31
			return MSG_NOT_ENOUGH_MEMORY;
		// 4th pass, get CR information
		rewind(sstream);	// point back to the begining of the file
		// read the 1st line of the source file, it contains the field names
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		// read source file line by line
		i = 0; 
		while (TRUE)
		{		
			if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
				break;
			if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
				continue;
			if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
				continue;
			parse_Scopus_line(line, i_au, tfield); 
			fwprintf(logstream, L"\n*** %s %s [%s]\n", wos[i].docid, wos[i].alias, tfield);
			parse_Scopus_line(line, i_cr, tfield);	
			parse_Scopus_citation_info(tfield, i, wos[i].docid, cstream);
			i++;
		} 
		clear_preparation_for_CR_search();
		fclose(cstream); 
	}

	fclose(sstream); 

#ifdef DEBUG
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%s: ", wos[i].docid);
		//for (k = 0; k < wos[i].ndspln; k++)
		//	fwprintf(logstream, L"%s ", dsplns[wos[i].dspln[k]].name);
		fwprintf(logstream, L"%d: %s\n", i+1, journals[wos[i].journal].name);
		fwprintf(logstream, L"\n");
		fflush(logstream);
	}
#endif DEBUG

	ret = link_author_WOS();   if (ret != 0) return ret;
	ret = link_journal_WOS();  if (ret != 0) return ret; 
	calculate_author_h_index();
	calculate_journal_h_index();	

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
	relink_journal_WOS();	// added 2016/11/19

	Jfree(line, L"read_Scopus: line"); Jfree(tfield, L"read_Scopus: tfield");

	return 0;
}


//
// parse the 1st line of the Scopus data file
// the 1st line looks like this:
// "Authors,Title,Year,Source title,Volume,Issue,Art. No.,Page start,Page end,Page count,Cited by,Link,Affiliations,Authors with affiliations,Abstract,Author Keywords,Index Keywords,Molecular Sequence Numbers,Chemicals/CAS,Tradenames,Manufacturers,Funding Details,References,Correspondence Address,Editors,Sponsors,Publisher,Conference name,Conference date,Conference location,Conference code,ISSN,ISBN,CODEN,DOI,PubMed ID,Language of Original Document,Abbreviated Source Title,Document Type,Source"
// or
// ""Authors","Title","Year","Source title","Volume","Issue","Art. No.", ....."
//
int parse_Scopus_1st_line(wchar_t *line, wchar_t *tfname)
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
		if (*sp == '\"') { sp++; continue; }	// added 2016/04/06, ignore the double quotes, allow the format with double quotes closing each column identifier
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
// parse a data record prepared by Scopus
// this function works with the situation that some text are enclosed with quotes, some are not
//
int parse_Scopus_line(wchar_t *line, int tind, wchar_t *tfield)
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
// split a given autor string into individual authors, assume the input is in the form "Chen J.S., Wang A.B."
// and then save them to the global author name array
// NOTE: this function also changes the author name from Scopus format to WOS format
//
int parse_store_Scopus_authors(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau, ndx2;
	int nau_this_article = 0;	// added 2014/10/29

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
		if (ch == ',') 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				//adjust_author_last_name(tmps);	// added 2015/09/29
				ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
				if (ndx2 >= 0)	// found that this is a variation of the author name
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
				else
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
				nau_this_article++;
				if (nau_this_article >= MAX_AUTHORS)	// take only MAX_AUTHORS authors, added 2014/10/29
				{
					*nau = tnau;
					return 0;
				}
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
		{
			if (*sp == ' ' && sp[2] == '.')		// the new check "sp[2] == '.'" is to avoid "De Marzo A.M." been recognized as "De, Marzo, A.M.", added 2017/02/28,
			{ *tp++ = ','; *tp++ = ' '; sp++; }	// replace a space with a comma plus a space
			else if (*sp == '.') sp++;	// ignore the periods after the first initial
			else *tp++ = towlower(*sp++); 
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		//adjust_author_last_name(tmps);	// added 2015/09/29
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
// split a given author string into individual authors, assume the input is in the form "Chen, J.S., Wang, A.B.""
// and then save them to the global author name array
// NOTE: this function also changes the author name from Scopus format to WOS format
// this function is added 2015/10/29
//
int parse_store_Scopus_authors2(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau, ndx2;
	int nau_this_article = 0;
	int state;

	tnau = *nau;
	sp = astr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps; state = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == '\"') { sp++; continue; }
		if (ch == ',') 
		{ 
			if (state == 0)
			{
				*tp++ = *sp++; // copy over the ','
				state = 1;	// changed to wait for the name delimiter
			}
			else
			{
				*tp++ = '\0'; sp++; 
				if (tmps[0] != '\0')
				{
					//adjust_author_last_name(tmps);	// added 2015/09/29
					ndx2 = authoralias_search(authoralias, nauthoralias, tmps);
					if (ndx2 >= 0)	// found that this is a variation of the author name
						wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
					else
						wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
					nau_this_article++;
					if (nau_this_article >= MAX_AUTHORS)	// take only MAX_AUTHORS authors, added 2014/10/29
					{
						*nau = tnau;
						return 0;
					}
				}
				tp = tmps;
				while (*sp == ' ') sp++;
				state = 0;	// back to the state of waiting for the ',' after the surname
			}
		}
		else 
		{
			if (*sp == '.') sp++;	// ignore the periods after the first initial
			else *tp++ = towlower(*sp++); 
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		//adjust_author_last_name(tmps);	// added 2015/09/29
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
int get_Scopus_number_authors(wchar_t *allnames)
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
		if (ch == ',') 
			cnt++;
	}
	cnt++;

	//fwprintf(logstream, L"%d %s\n", cnt, allnames);
	return cnt;
}

//
// parse a sting of author names, assume the input is in the form "Chen J.S., Wang A.B."
// save the results into the wos[] array
// NOTE: this function also changes the author name from Scopus format to WOS format
//
int parse_Scopus_author_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
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
		if (ch == ',') 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				//adjust_author_last_name(tmps);	// added 2015/09/29
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
		{
			if (*sp == ' ' && sp[2] == '.')		// the new check "sp[2] == '.'" is to avoid "De Marzo A.M." been recognized as "De, Marzo, A.M.", added 2017/02/28,
			{ *tp++ = ','; *tp++ = ' '; sp++; }	// replace a space with a comma plus a space
			else if (*sp == '.') sp++;	// ignore the periods after the first initial
			else *tp++ = towlower(*sp++); 
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		//adjust_author_last_name(tmps);	// added 2015/09/29
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
// parse a sting of author names, assume the input is in the form "Chen, J.S., Wang, A.B.",
// save the results into the wos[] array
// NOTE: this function also changes the author name from Scopus format to WOS format
// This function is added 2015/10/29
//
int parse_Scopus_author_names2(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau;
	int ndx, ndx2;
	int state;

	sp = astr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	tnau = 0; state = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == '\"') { sp++; continue; }
		if (ch == ',') 
		{ 
			if (state == 0)
			{
				*tp++ = *sp++; // copy over the ','
				state = 1;		// changed to wait for the name delimiter
			}
			else
			{
				*tp++ = '\0'; sp++; 
				if (tmps[0] != '\0')
				{
					//adjust_author_last_name(tmps);	// added 2015/09/29
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
				state = 0;	// back to the state of waiting for the ',' after the surname
			}
		}
		else 
		{
			if (*sp == '.') sp++;	// ignore the periods after the first initial
			else *tp++ = towlower(*sp++); 
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		//adjust_author_last_name(tmps);	// added 2015/09/29
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
// give a string, check whether it has the format "Chen J.S." (old format before around Oct. 2015) or "Chen, J.S." (new ater Oct. 2015)
// this function is added 2015/10/29
//
int check_author_name_format_type(wchar_t *aname)
{
	wchar_t *sp;

	sp = aname;
	while (*sp != '\0')
	{
		if (*sp == ' ') 
			return AUTHOR_NAME_FORMAT1;		// see space first, "Chen J.S.", here it can also be form 2 but with double surname
		else if (*sp == ',')
			return AUTHOR_NAME_FORMAT2;		// see comma first, "Chen, J.S."
		else 
			sp++;
	}

	return AUTHOR_NAME_FORMAT_UNKNOWN;
}

//
// create Scopus docid (in parallel with WOS doc ID)
//
int create_Scopus_docid(wchar_t *docid, wchar_t iau, int year, int volume, int bpage)
{
	swprintf(docid, L"%c%04d%06d%06d", iau, year, volume, bpage);

	return 0;
}

#ifdef OBSOLETE
//
// create Scopus docid (in parallel with WOS doc ID)
//
int create_Scopus_docid()
{
	int i;
	wchar_t tmps[MAX_DOC_ID], *tp; 

	// create a docid for each paper, this is because Scopus does not assign ID to each document
	for (i = 0; i < nwos; i++)
	{		
		sp = wos[i].alias; tp = tmps;
		for (cnt = 0; cnt < 10; cnt++)
		{
			if (*sp == '\0') break;
			*tp++ = *sp++;
		}
		if (cnt < 10) { for (; cnt < 10; cnt++) *tp++ = '0'; }
		*tp = '\0';
		swprintf(wos[i].docid, L"%04d%s%07d", wos[i].year, tmps, wos[i].bpage);
		//fwprintf(logstream, L"%s\n", wos[i].docid); 
	}

	return 0;
}
#endif OBSOLETE