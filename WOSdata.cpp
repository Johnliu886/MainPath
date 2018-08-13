// 
// WOSdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles Web of Science (WOS) data
//
// NOTE: assume that the  1st line of the data source file consists of field names
//
//

//
// Revision History:
// 2012/01/12 Modification  : added new function find_author_total_paths(), which replaces find_author_total_spx()
// 2012/01/15 Modification  : added the line "authors[i].total_cites = ssum;" in the function that calculates the h-index
// 2012/02/07 Modification  : forced wos[i].ndx2nw to -1 in the function link_WOS_nw()
// 2012/02/07 Add Function  : added function find_author_outward_nodes()
// 2012/06/22 Modification  : added code to handle Unicode data
// 2012/06/23 Modification  : cleaned code to make sure the support for all formats
//                            (WOS ASCII, WOS UTF-8, WOS UTF-16, Patent PGUIDER, Patent Webpat, Patent TIP, Westlaw) are working 
// 2012/10/18 Modification  : added a new entry "ndx" into the WOS structure to keep track of the original data order
//                            sort the data back to the original order (as read) in the function prepare_alias();
// 2012/12/09 Modification  : in self_citation_check(), added codes to handle special cases when a paper have authors with the same name
// 2012/12/13 Modification  : increased the definition of MAX_AUTHOR (defined in network.h) from 20 to 100
// 2013/01/03 Add fumction  : added codes to print the information of papers not inlucde in the final network (link_WOS_nw())
// 2013/02/04 Modification  : added codes to return FALSE immediately for Westlaw data upon calling the function self_citation_check()
// 2013/02/28 Added function: added codes to handle WUHAN_DATA, a new patent data format used in data from 武漢大學　王燕玲
// 2013/03/17 Fixed problems: fixed a problem in self_citation_check(), the problem occurs when "All Nodes" in Node Scope is selected, 
//                                  some of the nodes are not in full record data in this situation
// 2013/04/10 Added function: added codes to handle nantionality of authors (most of the codes are in the file "countries.cpp"
// 2013/04/19 Modification  : change the allocation of memory [athr] from using stack to allocate dynamically (in the function self_citation_check())
// 2013/04/24 Fixed problems: added the 4th pass in parsing WOS data. It parses country data. It is done seperately because it needs complete author data.
// 2013/04/30 Fixed problems: added code to handle the cases when journal name is not given in the WOS data
// 2013/06/11 Added function: added an argument to the function parse_citation_info()
// 2013/07/14 Added function: added codes to obtain keywords in article titles
// 2014/05/17 Modification  : removed the codes that assign tkeyword[].ranking, do it now in the function keyword_year_matrix()
// 2014/05/27 Added function: added codes to handle author alias (authoralias_search()), for example, "cook, w" => "cook, wd"
// 2014/05/28 Modification  : changed to print tabs (instead of spaces) between author/journal summary data
// 2014/07/07 Added function: changed to present more information for the journal statistics. 
//                            New information includes "Years active" and "Total citations"
// 2014/10/29 Added function: changed to call coauthor() twice, i.e. write out two type of coauthor networks
// 2014/12/12 Added function: added to call consolidate_countries() after read countries data for authors
// 2015/03/31 Added function: added to handle journal alias (most of the codes added are in the file "journal alias.txt")
//                            added to show "Total number of journals" in the log file
// 2015/07/10 Added function: added to call consolidate_keywords().
// 2015/09/29 Modification  : changed to adjust author last name earlier, added a new function adjust_author_last_name()
// 2015/10/08 Added function: added codes to calculate total paths (m-index) for a research group (in the function move_deeper())
// 2015/10/31 Added function: added codes to report m-index calculation progress
// 2015/11/03 Added function: added codes to write 1st author only information to "Author SPX" file
// 2015/11/19 Added function: added code to calculate hm-index, which take multiple authorship into consideration
// 2016/01/19 Modification  : changed the file opening to Unicode mode (ccs=UTF-8)
// 2016/04/06 Modification  : for Scoupus data, added codes to handle a new 1st line format ["Authors","Title","Year","Source title","Volume","Issue","Art. No."]
// 2016/04/13 Modification  : added to allow '\n' as delimiter in the function  parse_store_keywords() and parse_keywords()
// 2016/04/14 Modification  : added a format parameter to the prepare_alias() function, this is a modificat for TCI
// 2016/05/13 Added function: added code to check for WEBPAT ".csv" data
// 2016/07/01 Added function: added alias replacement for author keywords (ID and DE field), modified in the functions parse_store_keywords() and parse_keywords()
// 2016/07/02 Modification  : moved to call prepare_keyword_alias_list() before calling parse_store_keywords()	
// 2016/08/05 Added function: added additional identification for Thomson Innovation format
// 2016/10/13 Added function: added codes to support WEBPAT3 data
// 2016/10/29 Added codes to re-allocate memory when "kwde" or "kwid" is full	
// 2016/11/19 Added the function relink_journal_WOS()
// 2017/03/01 Extended the estimated number of authors from 8 to 12 when allocating memeory
// 2017/03/07 Modified the check for WEBPAT3 patent title
// 2017/04/07 Added initialization for "hm_cn" and "hm" (in the functions calculate_author_h_index() and calculate_author_h_cn_index())
// 2017/06/16 Modification  : changed to use "AF" rather than "AU" for author names 
// 2017/06/25 Added function: added codes to check for multiple WOS data 
// 2017/11/02 Added function: added three functions find_assignee_total_paths(), find_assignee_outward_nodes(), 
//                               and find_assignee_total_spx() to support the calculation of assignee m-index
// 2017/12/19 Fixed problem : recovered the code block that parses country data [calls parse_countries(), parse_countries_rp()]
//                            the block was unintentionally deleted in an unkonwn time.
// 2018/01/03 Added funciton: added codes to recognize the leading identifier in English for WEBPAT3 data
// 2018/01/03 Added function: added codes to check for multiple WEBPAT3 data 
// 2018/01/19 Added function: added codes to accept "Derwent Innovation Patent", new indicator at the 1st line of Thomson Innovation Patent
// 2018/01/31 Added function: added codes to read and process 'paper family' data, 
//                                a new column 'FA' is added manually to indicate those papers in the same loop (treat them as a family)
// 2018/03/14 Fixed problem : avoid consolidating the same paper family into one serialdocs structure if 'paper family' does not exist 
// 2018/05/09 Modification  : reduced the length for the leading string for Scopus data
// 2018/06/14 Fixed problem : changed to take author data from "AU" (rather than "AF") if using "CR" (no citation file) to get citations.
//                                  This is because “CR” in the WOS data use abbreviated author name
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
#include "clustering.h"	// added 2015/10/08

//
// function types
//
int parse_WOS_line(wchar_t *, int, int, wchar_t *);
int parse_WOS_1st_line(wchar_t *, int, wchar_t *);
int parse_author_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int get_number_authors(wchar_t *);
int parse_keywords(wchar_t *, int *, int *, int, struct KWORDS *);
int parse_store_keywords(wchar_t *, int *, struct KWORDS *);
int parse_store_authors(wchar_t *, int *, struct AUTHORS *);
int parse_store_disciplines(wchar_t *, int *, struct DISCIPLINES *);
int parse_disciplines(wchar_t *, int *, int *, int, struct DISCIPLINES *);
int compare_str(const void *, const void *);
int str_search(char **tr, int, char *);
int compare_aname(const void *, const void *);
int compare_kname(const void *, const void *);
int compare_author(const void *, const void *);
int compare_dpname(const void *, const void *);
int compare_countryname(const void *, const void *);
int prepare_alias(int);
int kname_search(struct KWORDS *, int, wchar_t *);
int aname_search(struct AUTHORS *, int, wchar_t *);
int jname_search(struct JOURNALS *, int, wchar_t *);
int dpname_search(struct DISCIPLINES *, int, wchar_t *);
int compare_docid(const void *, const void *);
int compare_1st_author(const void *, const void *);
int compare_journal(const void *, const void *);
int compare_tkeyword(const void *, const void *);
int compare_year(const void *, const void *);
int compare_tc(const void *, const void *);
int compare_wordcount(const void *, const void *);
int compare_aid(const void *, const void *);
int compare_gid(const void *, const void *);
int compare_index(const void *, const void *);
int docid_search(struct WOS *, int, wchar_t *);
int alias_search(struct WOS *, int, wchar_t *);	
int link_author_WOS();
int relink_author_WOS();
int link_journal_WOS();
int relink_journal_WOS();
int calculate_author_h_index();
int calculate_journal_h_index();
int basic_wos_statistics();
int coauthor(wchar_t *, int);		
int prepare_for_CR_search();
int clear_preparation_for_CR_search();
int move_deeper(int, struct PN *);
int move_deeper2(int, struct PN *);
int move_deeper_assignee(int, struct PN *);
int move_deeper2_assignee(int, struct PN *);
int parse_countries(int, wchar_t *, int, struct AUTHORS *);
int parse_countries_rp(int, wchar_t *, int, struct AUTHORS *);
int journal_abbreviations(struct JOURNALS *);
int adjust_author_last_name(wchar_t *);

extern int prepare_author_alias_list();
extern int prepare_keyword_alias_list();
extern int prepare_journal_alias_list();
extern int parse_store_title(int, struct TKEYWORDS *, wchar_t *);	// added 2013/07/12
extern int parse_title(wchar_t *, int, int *, int *, int, struct TKEYWORDS *);	// added 2013/10/26
extern int parse_line(wchar_t *, int, wchar_t *, wchar_t *, double *);
extern int parse_citation_info(wchar_t *, int, wchar_t *, FILE *);
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int is_stopwords(wchar_t *);
extern int title_stat(struct TKEYWORDS *, int, struct PTITLES *, int);
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int journalalias_search(struct JOURNAL_ALIAS *, int, wchar_t *);
extern int consolidate_countries(int, struct AUTHORS *);
extern int consolidate_keywords();
extern int keywordalias_search(struct KEYWORD_ALIAS *, int, wchar_t *);
extern int compare_serialdocid(const void *, const void *);

//
// global variables
//
FILE *sstream;		// for source file
extern FILE *logstream;
extern int no_wos_citation_file;
extern int nsources;
extern int *sources;	// memory to save index of all sources
extern int full_record_type;
extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;
extern int njournalalias;	// number of items in the journal alias table, 2015/03/31
extern struct JOURNAL_ALIAS *journalalias;	// 2015/03/31
extern int nrgroups;
extern struct RESEARCH_GROUP *rgroups;
extern int nkeywordalias;	// number of items in the keyword alias table
extern struct KEYWORD_ALIAS *keywordalias;

extern int nnodes;
extern struct PN *nw;

extern int nsds;
extern struct SERIALDOCS *serialdocs;

int text_type;
int nwos;
struct WOS *wos;
int nkwde;
struct KWORDS *kwde;	// author keywords
int nkwid;
struct KWORDS *kwid;	// Keyword Plus keywords
struct TKEYWORDS *tkeyword;	// title keywords
int ntkeywords;
int naus;
struct AUTHORS *authors;	// author name array
int njrs;
struct JOURNALS *journals;	// journal name array
int ndsplns;
struct DISCIPLINES *dsplns;	// disciplines
int nasgns;
struct ASSIGNEES *assignees;	// for patent data only
//int ncountries;				// added 2013/04/10
//struct COUNTRIES *countries;// added 2013/04/10

#define BUF_SIZE 1024

// 
// check the type of the full record file 
//
int check_file_type(wchar_t *sname, int *text_type)
{
	wchar_t *line;

	// allocate the working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// check the file type, ASCII or UNICODE
	if (_wfopen_s(&sstream, sname, L"r") != 0)	// open in text mode to check
		return MSG_WOSFILE_NOTFOUND;
	// get the first line 
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return UNKNOWN_DATA;
	if (line[0] == '\n' || line[0] == '\r')        return UNKNOWN_DATA;

	if (line[0] == 0xef && line[1] == 0xbb && line[2] == 0xbf)	// UTF-8 stream leading code: "EF BB BF" (in hex)
		*text_type = FT_UTF_8;
	else if (line[0] == 0xfe && line[1] == 0xff)	// UTF-16 (big-endian) stream leading code: "FE FF" (in hex)
		*text_type = FT_UTF_16_BE;
	else if (line[0] == 0xff && line[1] == 0xfe)	// UTF-16 (little-endian) stream leading code: "FF FE" (in hex)
		*text_type = FT_UTF_16_LE;
	else if (line[0] == 0x00 && line[2] == 0xfe)	// UTF-32 (big-endian) stream leading code: "00 00 FE FF" (in hex)
		*text_type = FT_UTF_32_BE;
	else if (line[0] == 0xfe && line[2] == 0xff)	// UTF-32 (little-endian) stream leading code: "00 00 FF FE" (in hex)
		*text_type = FT_UTF_32_LE;
	else
		*text_type = FT_ASCII;

	fclose(sstream);
	free(line);

	return 0;
}

// 
// check the type of full record file 
// the 1st line of WOS data
// "PT AU BA ED GP AF CA TI SO SE LA DT CT CY CL SP HO DE ID AB C1 RP EM FU FX CR NR TC PU PI PA SN BN DI J9 JI PD PY VL IS PN SU SI BP EP AR DI PG SC GA UT"
// the 1st 18 characters in the 1st line of USPTO data is
// "PatentNumber     :"
// NOTE: this function is called by Form1.h, therefore is not proper to use Jmalloc() and Jfree()
int WOS_or_Others(wchar_t *sname)
{
	int ret;	
	int i_ut, i_au, i_de, i_id, i_py, i_so, i_tc, i_sc, i_ti;
	int i_c1, i_rp;		// added 2013/04/10
	wchar_t *nline;
	wchar_t *line;
	wchar_t *tfield;

	// allocate the working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

	check_file_type(sname, &text_type);

	wchar_t webpat_kw[8] = {0xa8, 0xd3, 0xb7, 0xbd, 0xaf, 0xb8, 0xa5, 0x78};	// 檔案之起頭四個字 "來源站台", for PGUIDER data

	// open again according to the file type, get the 1st line afterwards
	if (text_type == FT_ASCII)
	{
		_wfopen_s(&sstream, sname, L"r");	// open as text mode for ASCII
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return UNKNOWN_DATA;
		nline = line;
	}
	else if (text_type == FT_UTF_8)
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8");	// open as text mode for UTF-8 type	
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return UNKNOWN_DATA;
		nline = &line[0];
		//if (line[3] == 0xef && line[4] == 0xbb && line[5] == 0xbf)	// check if the UTF-8 mark repeat, this happens for Scopus csv data
		//	nline = &line[6];	// start from the 7th byte
	}
	else	// Unicode type
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE");	// for Unicode data	
		if(fgetws(line, XXLBUF_SIZE/2, sstream) == NULL) return UNKNOWN_DATA;
		nline = &line[0];	
	}
	
	if (wcsncmp(nline, L"Taiwan T&D", 10) == 0)	
		ret = TAIWAN_TD_DATA;
	else if (wcsncmp(nline, L",題名,題名(外文),作者,", 14) == 0)	// added 2016/04/12	
		ret = TCI_DATA;
	else if (wcsncmp(nline, L"Thomson Innovation Patent", 25) == 0)	
		ret = THOMSON_INNOVATION_DATA;
	else if (wcsncmp(nline, L"\"Thomson Innovation Patent", 26) == 0)	// added 2016/08/05
		ret = THOMSON_INNOVATION_DATA;
	else if (wcsncmp(nline, L"Derwent Innovation Patent", 25) == 0)	// added 2018/01/19
		ret = THOMSON_INNOVATION_DATA;
	else if (wcsncmp(nline, L"\"Derwent Innovation Patent", 26) == 0)	// added 2018/01/19
		ret = THOMSON_INNOVATION_DATA;
	else if (wcsncmp(nline, L"No.\tPatent No.", 14) == 0)	
		ret = WUHAN_DATA;
	else if (wcsncmp(nline, L"PatentNumber     :", 18) == 0)	
		ret = USPTO_DATA;
	else if (wcsncmp(nline, webpat_kw, 8) == 0)	// leading characters of PGUIDER data
		ret = PGUIDER_DATA;
	else if (wcsncmp(nline, L"WESTLAW DATA2", 13) == 0)	// Leading string for new Westlaw data, added 2016/05/17
		ret = WESTLAW_DATA2;
	else if (wcsncmp(nline, L"WESTLAW DATA", 12) == 0)	// Leading string for Westlaw data
		ret = WESTLAW_DATA;
	else if (wcsncmp(nline, L"Authors,Title,Year,", 19) == 0)	// Leading string for Scopus data, length reduced, 2018/05/09
		ret = SCOPUS_DATA;
	else if (wcsncmp(nline, L"\"Authors\",\"Title\",\"Year\",", 25) == 0)	// Leading string for Scopus data, length reduced, 2018/05/09
		ret = SCOPUS_DATA;
	else if (wcsncmp(nline, L",,,,,,,,,,,,,,,,,,,,", 20) == 0)	// added 2016/05/13
		ret = WEBPAT2_DATA;
	else if (wcsncmp(nline, L"公告(開)號,", 7) == 0)	// modified 2017/03/07
		ret = WEBPAT3_DATA;
	else if (wcsncmp(nline, L"Publication Number,Publication Date", 35) == 0)	// added 2018/01/03
		ret = WEBPAT3_DATA;
	else if (wcsncmp(nline, L"\"no\"\t\"date\"\t\"reason\"", 20) == 0)	// added 2017/04/28
		ret = LAWSNOTE_DATA;
	else if (wcsncmp(nline, L"no\tdate\treason\tcourt", 20) == 0)	// added 2017/06/03
		ret = LAWSNOTE_DATA;
	else if (wcsncmp(nline, L"WOS DATA", 8) == 0)	// Leading string for mutiple WOS, added 2017/06/26
		ret = WOS_MULTIPLE_DATA;
	else if (wcsncmp(nline, L"TCI DATA", 8) == 0)	// Leading string for mutiple TCI, added 2017/06/26
		ret = TCI_MULTIPLE_DATA;
	else if (wcsncmp(nline, L"WEBPAT3 DATA", 12) == 0)	// Leading string for mutiple WEBPAT3, added 2018/01/03
		ret = WEBPAT3_MULTIPLE_DATA;
	else
	{
		i_ut = parse_WOS_1st_line(nline, SP_TAB, L"UT");	// document ID
#ifdef RESEARCH_MODALITY_PROJECT
		i_au = parse_WOS_1st_line(nline, SP_TAB, L"AF");	// author names
#else
		i_au = parse_WOS_1st_line(nline, SP_TAB, L"AU");	// author names
#endif RESEARCH_MODALITY_PROJECT
		i_de = parse_WOS_1st_line(nline, SP_TAB, L"DE");	// autor keywords
		i_id = parse_WOS_1st_line(nline, SP_TAB, L"ID");	// ID plus keyword
		i_py = parse_WOS_1st_line(nline, SP_TAB, L"PY");	// year of publication
		i_so = parse_WOS_1st_line(nline, SP_TAB, L"SO");	// journal name
		i_tc = parse_WOS_1st_line(nline, SP_TAB, L"TC");	// total citation
		i_sc = parse_WOS_1st_line(nline, SP_TAB, L"SC");	// discipline
		i_c1 = parse_WOS_1st_line(line, SP_TAB, L"C1");		// author address, added 2013/04/10
		i_rp = parse_WOS_1st_line(line, SP_TAB, L"RP");		// reprint address, added 2013/04/10
		i_ti = parse_WOS_1st_line(line, SP_TAB, L"TI");		// article title, added 2013/07/12
#ifdef DEA_APPLICATIONS
		int i_in;
		i_in = parse_WOS_1st_line(nline, SP_TAB, L"IN");	// type of DEA applications
#endif DEA_APPLICATIONS
		if (i_ut == -1 || i_au == -1 || i_de == -1 || i_id == -1 || i_py == -1 || i_so == -1 || i_tc == -1 || i_sc == -1 || i_c1 == -1 || i_rp == -1 || i_ti == -1)
			return MSG_WOSFILE_FORMAT_ERROR;
		else
			ret = WOS_DATA;
	}

	fclose(sstream);
	free(line); free(tfield);

	return ret;
}

//
// read WOS data and put it into the wos[] array
//
int read_WOS(wchar_t *sname)
{
	int i, k, ndx, ndx2;
	int nsr, cnt, ret;
	int i_ut, i_au, i_de, i_id, i_py, i_so, i_tc, i_sc, i_cr, i_di, i_vl, i_bp, i_ab;
	int i_c1, i_rp;		// added 2013/04/10
	int i_ti;			// added 2013/07/12
	int i_fa;			// added 2018/01/31
	wchar_t *line;
	wchar_t *tfield;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t *sp;
	FILE *cstream;		// for citation file

	// allocate the working memory
	line = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_WOS: line");
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"read_WOS: tfield");
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
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)	// for Unicode data	
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
		nsr++;	
	}
	nwos = nsr - 1;	// less the 1st line (format line)

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// look for the index of the specified target field name 
	i_ut = parse_WOS_1st_line(line, SP_TAB, L"UT");	// document ID
	if (no_wos_citation_file)	// added 2016/06/15
		i_au = parse_WOS_1st_line(line, SP_TAB, L"AU");	// use "AU" (1st name abbriviated) if no citation file
	else
		i_au = parse_WOS_1st_line(line, SP_TAB, L"AF");	// use "AF" (1st name full) otherwise
//#ifdef RESEARCH_MODALITY_PROJECT
//	i_au = parse_WOS_1st_line(line, SP_TAB, L"AF");	// author names, changed to use "AF", 2017/06/16
//#else
//	i_au = parse_WOS_1st_line(line, SP_TAB, L"AU");	// author names
//#endif RESEARCH_MODALITY_PROJECT
	i_de = parse_WOS_1st_line(line, SP_TAB, L"DE");	// autor keywords
	i_id = parse_WOS_1st_line(line, SP_TAB, L"ID");	// ID plus keyword
	i_py = parse_WOS_1st_line(line, SP_TAB, L"PY");	// year of publication
	i_so = parse_WOS_1st_line(line, SP_TAB, L"SO");	// journal name
	i_tc = parse_WOS_1st_line(line, SP_TAB, L"TC");	// total citation
	i_sc = parse_WOS_1st_line(line, SP_TAB, L"SC");	// discipline
	i_cr = parse_WOS_1st_line(line, SP_TAB, L"CR");	// "citing" information
	i_di = parse_WOS_1st_line(line, SP_TAB, L"DI");	// DOI information
	i_vl = parse_WOS_1st_line(line, SP_TAB, L"VL");	// volume
	i_bp = parse_WOS_1st_line(line, SP_TAB, L"BP");	// beginning page
	i_ab = parse_WOS_1st_line(line, SP_TAB, L"AB");	// abstract
	i_c1 = parse_WOS_1st_line(line, SP_TAB, L"C1");		// author address, added 2013/04/10
	i_rp = parse_WOS_1st_line(line, SP_TAB, L"RP");		// reprint address, added 2013/04/10
	i_ti = parse_WOS_1st_line(line, SP_TAB, L"TI");		// reprint address, added 2013/07/12
	i_fa = parse_WOS_1st_line(line, SP_TAB, L"FA");		// paper family, added 2018/01/31
#ifdef DEA_APPLICATIONS
	int i_in;
	i_in = parse_WOS_1st_line(line, SP_TAB, L"IN");	// type of DEA applications
#endif DEA_APPLICATIONS
	if (i_ut == -1 || i_au == -1 || i_de == -1 || i_id == -1 || i_py == -1 || i_so == -1 || i_tc == -1 || i_sc == -1 || i_cr == -1 || i_di == -1 || i_c1 == -1 || i_rp == -1 || i_ti == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	prepare_keyword_alias_list();	// this is so that we can handle variations of keywords, moved up to here, 2016/07/02
	// 2nd pass, get keyword data and save it
	// allocate memory for the author keyword data
	int size_kwde;
	size_kwde = nwos * 10;	// estimate in average 10 keywords per document
	kwde = (struct KWORDS *)Jmalloc(size_kwde * sizeof(struct KWORDS), L"read_WOS: kwde");	
	if (kwde == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; nkwde = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_de, tfield);
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
	kwid = (struct KWORDS *)Jmalloc(size_kwid * sizeof(struct KWORDS), L"read_WOS: kwid");	// estimate in average 12 keywords per document
	if (kwid == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; nkwid = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_id, tfield);
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
	// allocate memory for the author name array
	authors = (struct AUTHORS *)Jmalloc(nwos * 12 * sizeof(struct AUTHORS), L"read_WOS: authors");	// estimate in average 12 authors per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
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
#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		fwprintf(logstream, L"%d, [%s]\n", i, authors[i].name);
	}
#endif DEBUG

	prepare_journal_alias_list();	// this is so that we can handle the situation when journal change titles, added 2015/03/31
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the journal name array
	journals = (struct JOURNALS *)Jmalloc(nwos * sizeof(struct JOURNALS), L"read_WOS: journals");
	if (journals == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_so, tfield);
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

	// obtain and process article title information, added 2013/07/12
	struct PTITLES *ptitle;	// paper titles
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the title keyword array
	ptitle = (struct PTITLES *)Jmalloc(nwos * sizeof(struct PTITLES), L"read_WOS: ptitle");
	if (ptitle == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_ti, tfield);
		wcscpy_s(ptitle[i].name, MAX_TITLES, tfield);
		parse_WOS_line(line, SP_TAB, i_py, tfield);
		ptitle[i].year = _wtoi(tfield);
		i++;	
	}

	// assume in average each title contains 60 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nwos * 60 * sizeof(struct TKEYWORDS), L"read_WOS: tkeyword");
	if (tkeyword == NULL) return MSG_NOT_ENOUGH_MEMORY;
	ndx = 0;
	for (i = 0; i < nwos; i++)
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
	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = -1; } // initialization, added 2014/05/17
	// NOTE: "tkeyword[i].ndx = i" is changed to "tkeyword[i].ndx = -1" on 2015/03/04, this is used in the function critical_transition() 

	// sort to the order of counts
	//qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_wordcount);
	//for (i = 0; i < ntkeywords; i++) tkeyword[i].ranking = i;	// rank begins from 0
#ifdef DEBUG
	for (i = 0; i < ntkeywords; i++)
		fwprintf(logstream, L"%d %03d [%s]\n", i, tkeyword[i].cnt, tkeyword[i].name);
#endif DEBUG
	//fwprintf(logstream, L"\n"); 
	//title_stat(tkeyword, ntkeywords, ptitle, nwos);	// create a keyword-year matrix
	free(ptitle);	// added 2013/08/19
	//qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_tkeyword); // sort again, back to the alphabetical order
	//free(tkeyword);	// added 2013/08/19

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the discipline data
	dsplns = (struct DISCIPLINES *)Jmalloc(nwos * 5 * sizeof(struct DISCIPLINES), L"read_WOS: dsplns");	// estimate in average 5 disciplines per document
	if (dsplns == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// read source file line by line
	i = 0; ndsplns = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_sc, tfield);
		parse_store_disciplines(tfield, &ndsplns, dsplns);
		//fwprintf(logstream, L"%d:%d: %s\n", i, ndsplns, tfield); fflush(logstream);
		i++;	
	}
	qsort((void *)dsplns, (size_t)ndsplns, sizeof(struct DISCIPLINES), compare_dpname);
	// consolidate duplicate keywords
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < ndsplns; i++)
	{
		if (wcscmp(dsplns[i].name, prev_name) != 0)	// hit new name
		{
			wcscpy_s(dsplns[k++].name, MAX_DISCIPLINE_NAME, dsplns[i].name); 
			wcscpy_s(prev_name, MAX_DISCIPLINE_NAME, dsplns[i].name); 
		}
	}
	ndsplns = k;

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
	wos = (struct WOS *)Jmalloc(nwos * sizeof(struct WOS), L"read_WOS: wos");
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;	
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		wos[i].ndx = i;	// added 2012/10/18
		parse_WOS_line(line, SP_TAB, i_ut, tfield);
		wcscpy_s(wos[i].docid, MAX_DOC_ID, &tfield[4]);	// ignore the 4 leading characters -- "ISI:" or "WOS:"
		parse_WOS_line(line, SP_TAB, i_au, tfield);
		wos[i].nau = get_number_authors(tfield);
		parse_author_names(tfield, &wos[i].nau, wos[i].author, naus, authors);
#ifdef DEBUG
		for (k = 0; k < wos[i].nau; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, wos[i].nau, k, wos[i].author[k], authors[wos[i].author[k]].name); fflush(logstream); }
		fwprintf(logstream, L"\n");
#endif DEBUG
		parse_WOS_line(line, SP_TAB, i_py, tfield);
		wos[i].year = _wtoi(tfield);
		//fwprintf(logstream, L"%d:%d %s[%s]\n", i, wos[i].year, wos[i].docid, wos[i].author);
		parse_WOS_line(line, SP_TAB, i_de, tfield);
		parse_keywords(tfield, &wos[i].nde, wos[i].DE, nkwde, kwde);
		parse_WOS_line(line, SP_TAB, i_id, tfield);
		parse_keywords(tfield, &wos[i].nid, wos[i].ID, nkwid, kwid);
		parse_WOS_line(line, SP_TAB, i_so, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }		// added 2015/03/31		
		ndx2 = journalalias_search(journalalias, njournalalias, tfield);	// added 2015/03/31
		if (ndx2 >= 0)	// found that this is a variation of the journal title
			ndx = jname_search(journals, njrs, journalalias[ndx2].alias);
		else
			ndx = jname_search(journals, njrs, tfield);
		wos[i].journal = ndx;
		parse_WOS_line(line, SP_TAB, i_tc, tfield);
		wos[i].tc = _wtoi(tfield);
		parse_WOS_line(line, SP_TAB, i_sc, tfield);
		parse_disciplines(tfield, &wos[i].ndspln, wos[i].dspln, ndsplns, dsplns);
		parse_WOS_line(line, SP_TAB, i_ti, tfield);
		parse_title(tfield, 0, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);
		parse_WOS_line(line, SP_TAB, i_ab, tfield);		// parse abstract, added 2013/11/06
		parse_title(tfield, wos[i].ntkws, &wos[i].ntkws, wos[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array, added 2013/11/06
#ifdef DEA_APPLICATIONS
		parse_WOS_line(line, SP_TAB, i_in, tfield);	// the content of this field is the type of applications, or empty
		if (tfield[0] == '\0' || tfield[0] == ' ')
			wos[i].app_type = 0;	// theoretical
		else
			wos[i].app_type = 1;	// non-theoretical
#endif DEA_APPLICATIONS
		parse_WOS_line(line, SP_TAB, i_di, tfield);
		sp = tfield; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	// turn into lower case
		wcscpy_s(wos[i].doi, MAX_DOI_LEN, tfield);	
		//fwprintf(logstream, L"%d:%s", i, wos[i].doi);
		//fwprintf(logstream, L"\n"); fflush(logstream);
		parse_WOS_line(line, SP_TAB, i_vl, tfield);
		wos[i].volume = _wtoi(tfield);
		parse_WOS_line(line, SP_TAB, i_bp, tfield);
		if (iswdigit(tfield[0]) == 0)	// 1st character is not a digit
			wos[i].bpage = _wtoi(tfield+1);	// skip the 1st character whether it's a 'A' or 'B', etc	// 2011/08/22
		else
			wos[i].bpage = _wtoi(tfield);
		i++;	
	}
	//adjust_author_names();	// this is because that CR field removed spaces in last names, we have to do the same thing in the author name field 

	prepare_alias(1);

#ifdef DEBUG  // for tkeyword array
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%d %s\t%d: ", i, wos[i].alias, wos[i].ntkws); fflush(logstream);
		for (k = 0; k < wos[i].ntkws; k++)
			fwprintf(logstream, L"[%d:%s] ", k, tkeyword[wos[i].tkws[k]].name);
		fwprintf(logstream, L"\n----------------------------\n");
		fflush(logstream);
	}
#endif DEBUG

	consolidate_keywords();		// added 2015/07/10

#ifdef DEBUG  // for tkeyword array
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%d %s\t%d: ", i, wos[i].alias, wos[i].ntkws); fflush(logstream);
		for (k = 0; k < wos[i].ntkws; k++)
			fwprintf(logstream, L"[%d:%s %d] ", k, tkeyword[wos[i].tkws[k]].name, wos[i].tkws_cnt[k]);
		fwprintf(logstream, L"\n=============================\n");
		fflush(logstream);
	}
#endif DEBUG

	// the following code block (up to consolidated_countries()) was recovered 2017/12/19
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
		parse_WOS_line(line, SP_TAB, i_c1, tfield);
		parse_countries(i, tfield, naus, authors);
		parse_WOS_line(line, SP_TAB, i_rp, tfield);
		parse_countries_rp(i, tfield, naus, authors);	
		i++;	
	}
	consolidate_countries(naus, authors); // added 2014/12/12

	// following code block that process 'paper family' (serialdoc) data is added 2018/01/31
	// yet another pass, get 'paper family' data, the column 'FA' is added manually to indicate those data that has loop
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of serial document data
	int mest = 1000;
	if ((nwos / 10) < 1000) mest = 1000; 
	serialdocs = (struct SERIALDOCS *)Jmalloc(mest * sizeof(struct SERIALDOCS), L"read_WOS: serialdocs");
	if (serialdocs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	i = 0; nsds = 0; 
	wchar_t *xret;
	while (TRUE)
	{		
		xret = fgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_fa, tfield);
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
	if (nsds > 0)	// added 2018/03/14
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
		if (_wfopen_s(&cstream, cname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/19
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
			parse_WOS_line(line, SP_TAB, i_au, tfield); 
			fwprintf(logstream, L"%s %s [%s]\n", wos[i].docid, wos[i].alias, tfield);
			//parse_WOS_line(line, SP_TAB, i_py, tfield); fwprintf(logstream, L"%s\n", tfield);
			parse_WOS_line(line, SP_TAB, i_cr, tfield);	
			parse_citation_info(tfield, i, wos[i].docid, cstream);
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
	ret = coauthor(sname, 1); if (ret != 0) return ret;	// output coauthor network that contains only authors that have been the 1st author 
#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		if (authors[i].ndx1 == -1) continue;	// check only 1st authors
		//fwprintf(logstream, L"###%d %d ==> degree=%d\n", i+1, authors[i].ndx1+1, authors[i].degree);
		for (k = 0; k < authors[i].degree; k++)
		{
			if (authors[i].ndx1+1 < authors[authors[i].nbrs[k]].ndx1)	// display only one direction
			fwprintf(logstream, L"###\t%05d\t%05d\t%.2f\n", authors[i].ndx1+1, authors[authors[i].nbrs[k]].ndx1+1, authors[i].weight[k]);
		}
	}
#endif DEBUG

	ret = keyword_year_matrix(ntkeywords, tkeyword);
	if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;

	basic_wos_statistics();

	// sort the data by ISI document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
	relink_author_WOS();	// this is necessary after the wos[] arrary is sorted
	relink_journal_WOS();	// added 2016/11/19

	Jfree(line, L"read_WOS: line"); Jfree(tfield, L"read_WOS: tfield");

	return 0;
}

//
// given a name string, remove spaces in the last name
// this is because that CR field removed spaces in last names, we have to do the same thing in the author name field 
//
int adjust_author_last_name(wchar_t *aname)
{
	wchar_t *sp, *tp;

	sp = tp = aname;
	while (*sp != ',' && *sp != '\0')	// before ',' is the last name
	{
		if (*sp != ' ') *tp++ = *sp++;
		else sp++;
	}
	while (*sp != '\0') *tp++ = *sp++;
	*tp = *sp;

	return 0;
}

#ifdef NOT_USED_AFTER_20150929
//
// remove spaces in the last name
// this is because that CR field removed spaces in last names, we have to do the same thing in the author name field 
//
int adjust_author_names()
{
	int i;
	wchar_t *sp, *tp;

	for (i = 0; i < naus; i++)
	{
		sp = tp = authors[i].name;
		while (*sp != ',' && *sp != '\0')	// before ',' is the last name
		{
			if (*sp != ' ') *tp++ = *sp++;
			else sp++;
		}
		while (*sp != '\0') *tp++ = *sp++;
		*tp = *sp;
	}

	return 0;
}
#endif NOT_USED_AFTER_20150929

//
// basic year, author and journal statistics
//
int basic_wos_statistics()
{
	int i, k, cnt, acnt;
	int prev_year;
	wchar_t prev_journal[MAX_JOURNAL_NAME];

	// paper growth statistics
	// count the number of documents for each year
	//
	fwprintf(logstream, L"Paper growth statistics:\n");
	// sort by years
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_year);
	prev_year = 0;
	cnt = acnt = 0;
	for (i = 0; i < nwos; i++)
	{
		if (wos[i].year != prev_year)
		{
			if (cnt != 0)
				fwprintf(logstream, L"%d\t%d\t%d\n", prev_year, cnt, acnt);
			prev_year = wos[i].year;
			cnt = 1; acnt++;
		}
		else	// same year
		{
			cnt++; acnt++;
		}
	}
	fwprintf(logstream, L"%d\t%d\t%d\n", wos[i-1].year, cnt, acnt);
	fwprintf(logstream, L"\n");

	// journla statistics
	// count the number of articles for each journal
	//
	for (i = 0; i < njrs; i++) { journals[i].byear = 3000; journals[i].eyear = 0;}	// initialize byear and eyear
	// find the ending and beginning year of a journal's publication
	for (i = 0; i < nwos; i++)
	{
		if (journals[wos[i].journal].byear > wos[i].year) journals[wos[i].journal].byear = wos[i].year;
		if (journals[wos[i].journal].eyear  < wos[i].year) journals[wos[i].journal].eyear = wos[i].year;
	}
	for (i = 0; i < njrs; i++) journals[i].cnt1 = 0;
	for (i = 0; i < nwos; i++)
	{
		if (wos[i].year >= 2000)
			journals[wos[i].journal].cnt1++;
	}

	// assemble journal abbreviations	// added 2014/07/07
	for (i = 0; i < njrs; i++) 
		journal_abbreviations(&journals[i]);

	fwprintf(logstream, L"Journal statistics (Total number of unique journals: %d)\n", njrs);	// modified 2015/03/31
	fwprintf(logstream, L"\tYears active\tTotal papers\tPapers after 2000\tg-index\th-index\tTotal citations\tActive years\tAbbr\tName\n");
	for (i = 0; i < njrs; i++)
		fwprintf(logstream, L"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d~%d\t%s\t\"%s\"\n", i+1, journals[i].eyear-journals[i].byear+1, journals[i].np, journals[i].cnt1, journals[i].g, journals[i].h, journals[i].sum_tc, journals[i].byear, journals[i].eyear, journals[i].abbr, journals[i].name);

	// author statistics
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

	// author growth statistics
	//
	fwprintf(logstream, L"\nAuthor growth statistics:\n");
	#define NYEARS 4000
	int newathrs[50000];	// changed from 4000 to 50000, 2015/03/31
	int atotal;
	for (i = 0; i < NYEARS; i++) newathrs[i] = 0;
	for (i = 0; i < naus; i++)	// a bug here is fixed on 2011/01/05
		newathrs[authors[i].byear]++;
	atotal = 0;
	for (i = 0; i < NYEARS; i++)
	{
		atotal += newathrs[i];
		if (newathrs[i] != 0)
			fwprintf(logstream, L"%d\t%d\t%d\n", i, newathrs[i], atotal);
	}
	fwprintf(logstream, L"\n\n");

#ifdef XXX
	fwprintf(logstream, L"\nFirst author statistics:\n");
	for (i = 0; i < naus; i++)
	{
		if (authors[i].cnt1 != 0)
			fwprintf(logstream, L"%d \"%s\"\n", authors[i].cnt1, authors[i].name);
	}
#endif

	fwprintf(logstream, L"Author statistics (Total number of unique authors: %d)\n", naus);
	fwprintf(logstream, L"Count\tTotal papers\t1st authors\tG-index\tH-index\tHm-index\tActive years\tName\n");
	for (i = 0; i < naus; i++)
		fwprintf(logstream, L"%d\t%d\t%d\t%d\t%d\t%.2f\t%d~%d\t\"%s\"\n", i+1, authors[i].np, authors[i].cnt1, authors[i].g, authors[i].h, authors[i].hm, authors[i].byear, authors[i].eyear, authors[i].name);

#ifdef ARTICLE_AUTHOR_FILE
	//
	// create the article-author relationship file
	//
	FILE *pstream;
	wchar_t *sp, *tp;
	wchar_t nstr[MAX_AUTHOR_NAME];
	_wfopen_s(&pstream, L"Paper-Author.txt", L"w");
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nau; k++)
		{
			tp = nstr;
			sp = authors[wos[i].author[k]].name;
			while (*sp != '\0')
			{
				if (*sp == ',') sp++;
				else *tp++ = *sp++;
			}
			*tp = '\0';
			fwprintf(pstream, L"%s\t%s\n", wos[i].alias, nstr);
		}
	}
	fclose(pstream);
#endif ARTICLE_AUTHOR_FILE

	return 0;
}

//
// assemble abbreviation for each journal
// added 2014/07/07
//
int journal_abbreviations(struct JOURNALS *jnl)
{
	wchar_t *sp, *tp;

	// skip the beginning spaces
	sp = jnl->name;
	while (*sp == ' ') sp++;
	// check if it is a one-word name
	while (*sp != ' ' && *sp != '\0') sp++;
	if (*sp == '\0')	// no space in name, one-word name
	{
		wcscpy_s(jnl->abbr, MAX_JOURNAL_ABBR, jnl->name);
		return 0;
	}
	else if (*++sp == '(')	// for the case "Online (Wilton, Connecticut)"
	{
		sp = jnl->name;	tp = jnl->abbr;
		while (*sp != ' ')
			*tp++ = *sp++;
		*tp = '\0';
		return 0;
	}
	// start again from the beginning	
	sp = jnl->name; while (*sp == ' ') sp++;
	tp = jnl->abbr;
	*tp++ = towupper(*sp);
	while (*sp != '\0')	
	{ 
		if (*sp == ' ') 
		{
			sp++; 
			if (*sp == '&')
			{
				*tp++ = 'A';
				sp++;
			}
			else if (wcsncmp(sp, L"of ", 3) == 0 || wcsncmp(sp, L"OF ", 3) == 0)
				sp += 2;
			else if (wcsncmp(sp, L"the ", 4) == 0 || wcsncmp(sp, L"THE ", 4) == 0)
				sp += 3;
			else if (wcsncmp(sp, L"for ", 4) == 0 || wcsncmp(sp, L"FOR ", 4) == 0)
				sp += 3;
			else if (*sp == '(')
				break;	// ignore the information after '('
			else
				*tp++ = towupper(*sp++);
		}
		else if (*sp == ':')
			break;	// ignore the information after ':'
		else 
			sp++;
	}
	*tp = '\0';

	return 0;
}

//
//
//
int prepare_alias(int format)
{
	int i, k, nnd;
	wchar_t suffix;
	wchar_t n2str[MAX_AUTHOR_NAME];
	wchar_t n1str[MAX_AUTHOR_NAME];
	wchar_t *sp, *tp;

	for (i = 0; i < nwos; i++)
	{
		for (k = 1; k < wos[i].nau; k++)
			n2str[k-1] = towupper(authors[wos[i].author[k]].name[0]);
		n2str[k-1] = '\0';
		sp = authors[wos[i].author[0]].name;
		tp = n1str;
		while (*sp != ',' && *sp != '\0') 
		{
			if (*sp == ' ') { *tp++ = '_'; sp++; }
			else *tp++ = *sp++;
		}
		*tp = '\0';
		n1str[0] = towupper(n1str[0]);
		if (format == 1)	// for WOS, Scopus, etc.
			swprintf_s(wos[i].alias, MAX_ALIAS, L"%s%s%d", n1str, n2str, wos[i].year);
		else	// for TCI (Chinese data)
		{
			if (n2str[0] == '\0')
				swprintf_s(wos[i].alias, MAX_ALIAS, L"%s%d", n1str, wos[i].year);
			else
				swprintf_s(wos[i].alias, MAX_ALIAS, L"%s&%s%d", n1str, n2str, wos[i].year);
		}
	}

	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_aname);

	// consolidate duplicate author names
	wchar_t prev_name[MAX_ALIAS];
	prev_name[0] = '\0';
	nnd = 0; suffix = 'a';
	for (i = 0; i < nwos; i++)
	{
		if (wcscmp(wos[i].alias, prev_name) != 0)
		{
			wcscpy_s(prev_name, MAX_ALIAS, wos[i].alias); 
			suffix = 'a';
		}
		else	// same name
		{
			if (suffix == 'a')
			{
				swprintf_s(wos[i-1].alias, MAX_ALIAS, L"%s%c", wos[i-1].alias, suffix);
				suffix += (wchar_t)1;
			}
			swprintf_s(wos[i].alias, MAX_ALIAS, L"%s%c", wos[i].alias, suffix);
			suffix += (wchar_t)1;
		}
	}

	// sort the data back to the original order
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_index);

	return 0;
}

//
// given a relationship-file and an year
// create a new file that cut out the data before and after the specified year
// 
int create_new_relationship_list(wchar_t *iname, wchar_t *oname, int separator, int byear, int eyear)
{
	int ndx1, ndx2, nlines;
	double weight;
	wchar_t line[LBUF_SIZE+1];
	wchar_t docid1[MAX_NODE_NAME+1], docid2[MAX_NODE_NAME+1];
	FILE *istream, *ostream;

// 
// Open the given relationship-list file (will fail if the file does not exist)
//	
	if (_wfopen_s(&istream, iname, L"rt, ccs=UTF-8") != 0)
		return MSG_IFILE_NOTFOUND;

	_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8");

	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nlines++;
#ifdef CUT_OFF_YEAR_DATA
		if (byear == 0 && eyear == 0)	// years are not given, simply duplicate the data
			fwprintf(ostream, L"%s", line);
		else if (byear == 0 && eyear != 0)	// byear not given, eyear given
		{
			parse_line(line, separator, docid1, docid2, &weight);
			ndx1 = docid_search(wos, nwos, docid1);
			ndx2 = docid_search(wos, nwos, docid2);
			if (ndx1 >= 0 && ndx2 >= 0)
			{
				if (wos[ndx1].year > eyear || wos[ndx2].year > eyear) continue;	// ignore the line
				else fwprintf(ostream, L"%s", line);
			}
		}
		else if (byear != 0 && eyear == 0)	// byear given, eyear not given
		{
			parse_line(line, separator, docid1, docid2, &weight);
			ndx1 = docid_search(wos, nwos, docid1);
			ndx2 = docid_search(wos, nwos, docid2);
			if (ndx1 >= 0 && ndx2 >= 0)
			{
				//if (wos[ndx2].year < byear) continue;	// ignore the line
				if (wos[ndx1].year < byear || wos[ndx2].year < byear) continue;	// ignore the line
				else fwprintf(ostream, L"%s", line);
			}
		}
		else
		{
			parse_line(line, separator, docid1, docid2, &weight);
			ndx1 = docid_search(wos, nwos, docid1);
			ndx2 = docid_search(wos, nwos, docid2);
			if (ndx1 >= 0 && ndx2 >= 0)
			{
				// take citation to old papers (outside the time window) into consideration? two possibilities!
				//if (wos[ndx1].year > eyear || (wos[ndx2].year < byear || wos[ndx2].year > eyear)) continue;		// ignore only the new papers
				if ((wos[ndx1].year < byear || wos[ndx1].year > eyear) || (wos[ndx2].year < byear || wos[ndx2].year > eyear)) continue;	// ignore the old papers
				else fwprintf(ostream, L"%s", line);
			}
		}
#else 
		fwprintf(ostream, L"%s", line);	// simply duplicate the data
#endif CUT_OFF_YEAR_DATA
	}

	fclose(istream);
	fclose(ostream);

	return 0;
}

//
// create the alias table file
//
int create_alias_table_file(wchar_t *fnheader)
{
	int i;
	FILE *astream;
	wchar_t atablefname[FNAME_SIZE];

	swprintf(atablefname, L"%sAliasTable.txt", fnheader);
	_wfopen_s(&astream, atablefname, L"wt, ccs=UTF-8");	// modified 2016/01/18
	for (i = 0; i < nwos; i++)
		fwprintf(astream, L"%s\t%s\n", wos[i].docid, wos[i].alias);

	fclose(astream);

	return 0;
}

//
// calculate H-index for each author
// NOTE: the algorithm follows the paper "An improvement of the H-index: the G-index" by L. Egghe
// NOTE2: added to calculate hm-index, 2015/11/19, 
//        according to M. Schreiber, "A modification for the h-index: the hm-index accounts for multi-authored manuscriptes", Journal of Informetrics
//
struct CDATA	
{
	int tc;
	int rank;
	int sum_tc;
	int rank_square;
	int nau;	// number of authors
	double rank_eff;	// effective rank for hm calculation, added 2015/11/19
};
#define MAX_PAPERS_AUTHOR 2000
#define MAX_PAPERS_JOURNAL 20000
int calculate_author_h_index()
{
	int i, j, k, m;
	int ssum;
	double rnkf;
	int ccnt;
	struct CDATA cites[MAX_PAPERS_AUTHOR];

	for (i = 0; i < naus; i++)
	{	
		authors[i].g = 0; authors[i].h = 0; authors[i].hm = 0.0; // initialization of "hm" added 2017/04/07
		// obtain total citation data for this author
		for (k = 0; k < authors[i].np; k++)
		{
			cites[k].tc = wos[authors[i].paper[k]].tc;
			cites[k].sum_tc = 0; 
			cites[k].rank_eff = 0.0;
		}
		qsort((void *)cites, (size_t)authors[i].np, sizeof(struct CDATA), compare_tc);
		ssum = 0; rnkf = 0.0;
		for (k = 0; k < authors[i].np; k++)
		{
			cites[k].rank = k + 1;
			cites[k].rank_square = (k + 1) * (k + 1);
			ssum += cites[k].tc;
			cites[k].sum_tc = ssum;
			rnkf += (double)1/wos[authors[i].paper[k]].nau;
			cites[k].rank_eff = rnkf;
			cites[k].nau = wos[authors[i].paper[k]].nau;
		}
		authors[i].total_cites = ssum;	// added 2012/01/15
#ifdef DEBUG
		fwprintf(logstream, L"***** %s [%d]\n", authors[i].name, authors[i].np);
		for (k = 0; k < authors[i].np; k++)
			fwprintf(logstream, L"%d\t%d\t%.4f\t%d\t%d\t%d\n", cites[k].tc, cites[k].rank, cites[k].rank_eff, cites[k].nau, cites[k].sum_tc, cites[k].rank_square);
#endif DEBUG
		for (k = 0; k < authors[i].np; k++)
		{
			if (cites[k].tc < cites[k].rank) break;
			authors[i].h = cites[k].rank;
		}
		for (k = 0; k < authors[i].np; k++)
		{
			if (cites[k].sum_tc < cites[k].rank_square) break;
			authors[i].g = cites[k].rank;
		}
		for (k = 0; k < authors[i].np; k++)	// added 2015/11/19
		{
			if ((double)cites[k].tc < cites[k].rank_eff) break;
			authors[i].hm = cites[k].rank_eff;
		}
	}

#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		fwprintf(logstream, L"%d %d %d [%s]:", authors[i].np, authors[i].g, authors[i].h, authors[i].name);
		for (k = 0; k < authors[i].np; k++)
			fwprintf(logstream, L"%d ", wos[authors[i].paper[k]].tc);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// calculate h_cn-index for each author, it is the h-index within the citation network
// NOTE: the algorithm follows the paper "An improvement of the H-index: the G-index" by L. Egghe
//
int calculate_author_h_cn_index()
{
	int i, j, k, m, q;
	int ssum;
	double rnkf;
	int ccnt;
	struct CDATA cites[MAX_PAPERS_AUTHOR];

	for (i = 0; i < naus; i++)
	{	
		authors[i].g_cn = 0; authors[i].h_cn = 0; authors[i].hm_cn = 0.0;	// initialization of "hm_cn" added 2017/04/07 
		// obtain total citation data for this author
		for (k = 0; k < authors[i].np; k++)
		{
			q = wos[authors[i].paper[k]].ndx2nw;
			if (q == -1)
				cites[k].tc = 0;
			else
				cites[k].tc = nw[q].out_deg;
			cites[k].sum_tc = 0; 
			cites[k].rank_eff = 0.0;
		}
		qsort((void *)cites, (size_t)authors[i].np, sizeof(struct CDATA), compare_tc);
		ssum = 0; rnkf = 0.0;
		for (k = 0; k < authors[i].np; k++)
		{
			cites[k].rank = k + 1;
			cites[k].rank_square = (k + 1) * (k + 1);
			ssum += cites[k].tc;
			cites[k].sum_tc = ssum;
			rnkf += (double)1/wos[authors[i].paper[k]].nau;
			cites[k].rank_eff = rnkf;
			cites[k].nau = wos[authors[i].paper[k]].nau;
		}
		authors[i].total_cites = ssum;

		for (k = 0; k < authors[i].np; k++)
		{
			if (cites[k].tc < cites[k].rank) break;
			authors[i].h_cn = cites[k].rank;
		}
		for (k = 0; k < authors[i].np; k++)
		{
			if (cites[k].sum_tc < cites[k].rank_square) break;
			authors[i].g_cn = cites[k].rank;
		}
		for (k = 0; k < authors[i].np; k++)
		{
			if ((double)cites[k].tc < cites[k].rank_eff) break;
			authors[i].hm_cn = cites[k].rank_eff;
		}
	}

	return 0;
}

//
// calculate H-index for each journal
// NOTE: the algorithm follows the paper "An improvement of the H-index: the G-index" by L. Egghe
//
int calculate_journal_h_index()
{
	int i, j, k, m;
	int ssum;
	int ccnt;
	struct CDATA cites[MAX_PAPERS_JOURNAL];

	for (i = 0; i < njrs; i++)
	{	
		journals[i].g = 0; journals[i].h = 0;
		journals[i].sum_tc = 0;
		// obtain total citation data for this journal
		for (k = 0; k < journals[i].np; k++)
		{
			cites[k].tc = wos[journals[i].paper[k]].tc;
			cites[k].sum_tc = 0; 
			journals[i].sum_tc += wos[journals[i].paper[k]].tc;	// added 2014/07/07, to get total citation count
		}
		qsort((void *)cites, (size_t)journals[i].np, sizeof(struct CDATA), compare_tc);
		ssum = 0;
		for (k = 0; k < journals[i].np; k++)
		{
			cites[k].rank = k + 1;
			cites[k].rank_square = (k + 1) * (k + 1);
			ssum += cites[k].tc;
			cites[k].sum_tc = ssum;
		}
#ifdef DEBUG
		fwprintf(logstream, L"##### %d\t%d\t[%s]\n", i, journals[i].np, journals[i].name);
		for (k = 0; k < journals[i].np; k++)
			fwprintf(logstream, L"%d %d %d %d\n", cites[k].tc, cites[k].rank, cites[k].sum_tc, cites[k].rank_square);
#endif DEBUG
		for (k = 0; k < journals[i].np; k++)
		{
			if (cites[k].tc < cites[k].rank) break;
			journals[i].h = cites[k].rank;
		}
		for (k = 0; k < journals[i].np; k++)
		{
			if (cites[k].sum_tc < cites[k].rank_square) break;
			journals[i].g = cites[k].rank;
		}
	}

#ifdef DEBUG
	for (i = 0; i < njrs; i++)
	{
		fwprintf(logstream, L"$$$$$ %d\t%d\t%d\t%d\t[%s]:", i, journals[i].np, journals[i].g, journals[i].h, journals[i].name);
		for (k = 0; k < journals[i].np; k++)
			fwprintf(logstream, L"%d ", wos[journals[i].paper[k]].tc);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// calculate h_cn-index for each journal, it is the h-index within the citation network
// NOTE: the algorithm follows the paper "An improvement of the H-index: the G-index" by L. Egghe
//
int calculate_journal_h_cn_index()
{
	int i, j, k, m, q;
	int ssum;
	int ccnt;
	struct CDATA cites[MAX_PAPERS_JOURNAL];

	for (i = 0; i < njrs; i++)
	{	
		journals[i].g_cn = 0; journals[i].h_cn = 0;
		// obtain total number of citations (within the network) for this journal
		for (k = 0; k < journals[i].np; k++)
		{
			q = wos[journals[i].paper[k]].ndx2nw;
			if (q == -1)
				cites[k].tc = 0;
			else
				cites[k].tc = nw[q].out_deg;
			cites[k].sum_tc = 0; 
		}
		qsort((void *)cites, (size_t)journals[i].np, sizeof(struct CDATA), compare_tc);
		ssum = 0;
		for (k = 0; k < journals[i].np; k++)
		{
			cites[k].rank = k + 1;
			cites[k].rank_square = (k + 1) * (k + 1);
			ssum += cites[k].tc;
			cites[k].sum_tc = ssum;
		}
#ifdef DEBUG
		fwprintf(logstream, L"%s\n", journals[i].name);
		for (k = 0; k < journals[i].np; k++)
			fwprintf(logstream, L"%d %d %d %d\n", cites[k].tc, cites[k].rank, cites[k].sum_tc, cites[k].rank_square);
#endif DEBUG
		for (k = 0; k < journals[i].np; k++)
		{
			if (cites[k].tc < cites[k].rank) break;
			journals[i].h_cn = cites[k].rank;
		}
		for (k = 0; k < journals[i].np; k++)
		{
			if (cites[k].sum_tc < cites[k].rank_square) break;
			journals[i].g_cn = cites[k].rank;
		}
	}

#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		fwprintf(logstream, L"%d %d %d [%s]:", journals[i].np, journals[i].g, journals[i].h, journals[i].name);
		for (k = 0; k < journals[i].np; k++)
			fwprintf(logstream, L"%d ", wos[journals[i].paper[k]].tc);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// establish the relationships between the tables wos[] and authors[]
//
int link_author_WOS()
{
	int i, k, j;

	// allocate memory for each author's paper index table
	for (i = 0; i < naus; i++)
	{
		authors[i].paper = (int *)Jmalloc(authors[i].np * sizeof(int), L"link_author_WOS: authors[i].paper");
		if (authors[i].paper == NULL) return MSG_NOT_ENOUGH_MEMORY;
	}

	for (i = 0; i < naus; i++) authors[i].np = 0;	// want to count np again
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nau; k++)
		{
			j = wos[i].author[k];
			authors[j].paper[authors[j].np++] = i;
		}
	}

#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		fwprintf(logstream, L"%d %s: ", authors[i].np, authors[i].name);
		for (k = 0; k < authors[i].np; k++)
			fwprintf(logstream, L"[%d:%s] ", authors[i].paper[k], wos[authors[i].paper[k]].alias);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// re-establish the relationships between the tables wos[] and authors[]
// this is necessary after the wos[] arrary is sorted
//
int relink_author_WOS()
{
	int i, k, j;

	for (i = 0; i < naus; i++) authors[i].np = 0;	// want to count np again
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nau; k++)
		{
			j = wos[i].author[k];
			authors[j].paper[authors[j].np++] = i;
		}
	}

	return 0;
}

//
// establish the relationships between the tables wos[] and journals[]
//
int link_journal_WOS()
{
	int i, k, j;

	// allocate memory for each journal's paper index table
	for (i = 0; i < njrs; i++)
	{
		journals[i].paper = (int *)Jmalloc(journals[i].np * sizeof(int), L"link_journal_WOS: journals[i].paper");
		if (journals[i].paper == NULL) return MSG_NOT_ENOUGH_MEMORY;
	}

	for (i = 0; i < njrs; i++) journals[i].np = 0;	// want to count np again
	for (i = 0; i < nwos; i++)
	{
		j = wos[i].journal;
		if (j < 0)	// added 2013/04/30
		{
			fwprintf(logstream, L"\nEORROR: Journal not found for %s.\n", wos[i].alias); fflush(logstream);
			exit(-1);
		}
		else
			journals[j].paper[journals[j].np++] = i;
	}

#ifdef DEBUG
	for (i = 0; i < njrs; i++)
	{
		fwprintf(logstream, L"%d %s: ", journals[i].np, journals[i].name);
		for (k = 0; k < journals[i].np; k++)
			fwprintf(logstream, L"[%d:%s] ", journals[i].paper[k], wos[journals[i].paper[k]].alias);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// establish the relationships between the tables wos[] and journals[]
// this is necessary after the wos[] arrary is sorted
//
int relink_journal_WOS()
{
	int i, k, j;

	for (i = 0; i < njrs; i++) journals[i].np = 0;	// want to count np again
	for (i = 0; i < nwos; i++)
	{
		j = wos[i].journal;
		if (j < 0)
		{
			fwprintf(logstream, L"\nEORROR: Journal not found for %s.\n", wos[i].alias); fflush(logstream);
			exit(-1);
		}
		else
			journals[j].paper[journals[j].np++] = i;
	}

	return 0;
}

//
// establish the relationships between the tables wos[] and nw[]
//
// NOTE: nodes[] and nw[] array has the same order
extern int nnodes;
extern struct NODE *nodes;
extern struct PN *nw;
extern int name_search(struct NODE *, int, wchar_t *);
int link_WOS_nw()
{
	int i;
	int ndx;

	fwprintf(logstream, L"\nDocuments not included in the final network:\n");

	for (i = 0; i < nwos; i++)
	{
		ndx = name_search(nodes, nnodes, wos[i].docid);
		if (ndx >= 0)
		{
			wos[i].ndx2nw = ndx;
			nw[ndx].ndx2wos = i;
		}
		else // added 2012/02/07
		{
			wos[i].ndx2nw = -1;	// the document is not include in the network
			fwprintf(logstream, L"%s %s\n", wos[i].docid, wos[i].alias);	// added 2013/01/03
		}
	}

	fwprintf(logstream, L"\n");

	return 0;
}

//
// split a given keyword string into several keywords
// and then save them to the global keyword array
//
int parse_store_keywords(wchar_t *kstr, int *nkw, struct KWORDS *kw)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnkw;
	int ndx2;

	tnkw = *nkw;
	sp = kstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == ';' || ch == ',' || ch == '\n')	// 2016/04/13 added to allow '\n' as delimiter
		{ 
			*tp++ = '\0'; sp++; 
#ifdef OLD
			if (tmps[0] != '\0')
				wcscpy_s(kw[tnkw++].name, MAX_KEYWORD_NAME, tmps);
#endif OLD
			if (tmps[0] != '\0')	// added alias replacement codes on 2016/07/01
			{
				ndx2 = keywordalias_search(keywordalias, nkeywordalias, tmps);
				if (ndx2 >= 0)	// found that this is a variation of the keyword
					wcscpy_s(kw[tnkw++].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
				else
					wcscpy_s(kw[tnkw++].name, MAX_KEYWORD_NAME, tmps);
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
#ifdef OLD
	if (tmps[0] != '\0')
		wcscpy_s(kw[tnkw++].name, MAX_KEYWORD_NAME, tmps);
#endif OLD
	if (tmps[0] != '\0')	// added alias replacement codes on 2016/07/01
	{
		ndx2 = keywordalias_search(keywordalias, nkeywordalias, tmps);
		if (ndx2 >= 0)	// found that this is a variation of the keyword
			wcscpy_s(kw[tnkw++].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
		else
			wcscpy_s(kw[tnkw++].name, MAX_KEYWORD_NAME, tmps);
	}
	*nkw = tnkw;

	return 0;
}

//
// split a given keyword string into several keywords
//
int parse_keywords(wchar_t *kstr, int *nk, int *kw, int nkwords, struct KWORDS *kwords)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnk;
	int ndx, ndx2;

	sp = kstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	tnk = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == ';' || ch == ',' || ch == '\n') // 2016/04/13 added to allow '\n' as delimiter
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				//ndx = kname_search(kwords, nkwords, tmps);
				// added alias replacement codes on 2016/07/01
				ndx2 = keywordalias_search(keywordalias, nkeywordalias, tmps);	// replace if there is a alias keyword name
				if (ndx2 >= 0)	// found that this is a variation of the keyword
				{
					ndx = kname_search(kwords, nkwords, keywordalias[ndx2].alias);		
					if (ndx == -1) 
					{ 
						fwprintf(logstream, L"ERROR: undefined keyword [%s]. Please take only existing keywords as alias.\n", keywordalias[ndx2].alias);
						exit(-1); 
					}
				}
				else
					ndx = kname_search(kwords, nkwords, tmps);			
				kw[tnk] = ndx;
				tnk++; 
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
		//ndx = kname_search(kwords, nkwords, tmps);
		// added alias replacement codes on 2016/07/01
		ndx2 = keywordalias_search(keywordalias, nkeywordalias, tmps);	// replace if there is a alias keyword name
		if (ndx2 >= 0)	// found that this is a variation of the keyword
		{
			ndx = kname_search(kwords, nkwords, keywordalias[ndx2].alias);	if (ndx == -1) 
			{ 
				fwprintf(logstream, L"ERROR: undefined keyword [%s]. Please take only existing keywords as alias.\n", keywordalias[ndx2].alias); 
				exit(-1); 
			}
		}
		else
			ndx = kname_search(kwords, nkwords, tmps);		
		kw[tnk] = ndx;
		tnk++;
	}
	*nk = tnk;

	return 0;
}

//
// split a given autor string into individual authors
// and then save them to the global author name array
//
int parse_store_authors(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tnau, ndx2;

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
			{
				adjust_author_last_name(tmps);	// added 2015/09/29
				ndx2 = authoralias_search(authoralias, nauthoralias, tmps);	// added 2014/05/27
				if (ndx2 >= 0)	// found that this is a variation of the author name
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
				else
					wcscpy_s(au[tnau++].name, MAX_AUTHOR_NAME, tmps);
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
		adjust_author_last_name(tmps);	// added 2015/09/29
		ndx2 = authoralias_search(authoralias, nauthoralias, tmps); // added 2014/05/27
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
int get_number_authors(wchar_t *allnames)
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
		if (ch == ';') 
			cnt++;
	}
	cnt++;

	//fwprintf(logstream, L"%d %s\n", cnt, allnames);
	return cnt;
}

//
// parse a sting of author names
// save the results into the wos[] array
//
int parse_author_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
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
				adjust_author_last_name(tmps);	// added 2015/09/29
				ndx2 = authoralias_search(authoralias, nauthoralias, tmps);	// added 2014/05/27
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
		adjust_author_last_name(tmps);	// added 2015/09/29
		ndx2 = authoralias_search(authoralias, nauthoralias, tmps); // added 2014/05/27
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
// split a given discipline string into several discipline
// and then save them to the global discipline array
//
int parse_store_disciplines(wchar_t *dpstr, int *ndp, struct DISCIPLINES *dp)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tndp;

	tndp = *ndp;
	sp = dpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == ';') 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
				wcscpy_s(dp[tndp++].name, MAX_DISCIPLINE_NAME, tmps);
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
	if (tmps[0] != '\0')
		wcscpy_s(dp[tndp++].name, MAX_DISCIPLINE_NAME, tmps);
	*ndp = tndp;

	return 0;
}

//
// split a given discipline string into several disciplines
//
int parse_disciplines(wchar_t *dpstr, int *ndp, int *dp, int ndspns, struct DISCIPLINES *dspns)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[BUF_SIZE];
	int tndp;
	int ndx;

	sp = dpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	tndp = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (ch == ';') 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				ndx = dpname_search(dspns, ndspns, tmps);
				dp[tndp] = ndx;
				tndp++; 
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
		ndx = dpname_search(dspns, ndspns, tmps);
		dp[tndp] = ndx;
		tndp++;
	}
	*ndp = tndp;

	return 0;
}

//
// parse a data record prepared by Web of Science
//
int parse_WOS_line(wchar_t *line, int separator, int tind, wchar_t *tfield)
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
	*tp = '\0'; fcnt++;	// following 5 lines are added 2011/02/23, fixed the problem of no carriage return or line feed at the end of a file
	if (fcnt == tind)		
	{
		wcscpy_s(tfield, LBUF_SIZE, fp);
		return fcnt;
	}

	return -1;
}

//
// parse the 1st line of the source file
// the 1st line looks like this:
// "PT	AU	BA	ED	GP	AF	CA	TI	SO	SE	LA	DT	CT	CY	CL	SP	HO	DE	ID	AB	C1	RP	EM	FU	FX	CR	NR	TC	PU	PI	PA	SN	BN	DI	J9	JI	PD	PY	VL	IS	PN	SU	SI	BP	EP	AR	DI	PG	SC	GA	UT"
//
int parse_WOS_1st_line(wchar_t *line, int separator, wchar_t *tfname)
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

//
// this fucntion is to be called by qsort() only
// 
int compare_str(const void *n1, const void *n2)
{
	char **t1, **t2;
	
	t1 = (char **)n1;
	t2 = (char **)n2;
	if (strcmp(*t2, *t1) < 0)
		return 1;
	else if (strcmp(*t2, *t1) == 0)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of a "name" in a "NODE" array
//
int str_search(char *tr[], int num, char *str)
{
	int low, high, cur;
	int ret;

	ret = strcmp(str, tr[0]);
	if (ret < 0)
		return -1;
	else if (ret == 0)
		return 1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (strcmp(str, tr[cur]) == 0)
			return 1;
		if (cur == low)
			return -1;
		else if (strcmp(str, tr[cur]) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// this fucntion is to be called by qsort() only
// 
int compare_author(const void *n1, const void *n2)
{
	struct AUTHORS *t1, *t2;
	
	t1 = (struct AUTHORS *)n1;
	t2 = (struct AUTHORS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_kname(const void *n1, const void *n2)
{
	struct KWORDS *t1, *t2;
	
	t1 = (struct KWORDS *)n1;
	t2 = (struct KWORDS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_dpname(const void *n1, const void *n2)
{
	struct DISCIPLINES *t1, *t2;
	
	t1 = (struct DISCIPLINES *)n1;
	t2 = (struct DISCIPLINES *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_aname(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (wcscmp(t2->alias, t1->alias) < 0)
		return 1;
	else if (wcscmp(t2->alias, t1->alias) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_docid(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (wcscmp(t2->docid, t1->docid) < 0)
		return 1;
	else if (wcscmp(t2->docid, t1->docid) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_alias(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (wcscmp(t2->alias, t1->alias) < 0)
		return 1;
	else if (wcscmp(t2->alias, t1->alias) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_journal(const void *n1, const void *n2)
{
	struct JOURNALS *t1, *t2;
	
	t1 = (struct JOURNALS *)n1;
	t2 = (struct JOURNALS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_tkeyword(const void *n1, const void *n2)
{
	struct TKEYWORDS *t1, *t2;
	
	t1 = (struct TKEYWORDS *)n1;
	t2 = (struct TKEYWORDS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_1st_author(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (wcscmp(authors[t2->author[0]].name, authors[t1->author[0]].name) < 0)
		return 1;
	else if (wcscmp(authors[t2->author[0]].name, authors[t1->author[0]].name) == 0)
		return 0;
	else return -1;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_year(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (t2->year < t1->year)
		return 1;
	else if (t2->year == t1->year)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_a_year(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (t2->a_year < t1->a_year)
		return 1;
	else if (t2->a_year == t1->a_year)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// NOTE: order from large to small
// 
int compare_wordcount(const void *n1, const void *n2)
{
	struct TKEYWORDS *t1, *t2;
	
	t1 = (struct TKEYWORDS *)n1;
	t2 = (struct TKEYWORDS *)n2;
	if (t2->cnt > t1->cnt)
		return 1;
	else if (t2->cnt == t1->cnt)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// NOTE: order from large to small
// 
int compare_tc(const void *n1, const void *n2)
{
	struct CDATA *t1, *t2;
	
	t1 = (struct CDATA *)n1;
	t2 = (struct CDATA *)n2;
	if (t2->tc > t1->tc)
		return 1;
	else if (t2->tc == t1->tc)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of a "name" in a "KWORDS" array
//
int kname_search(struct KWORDS d[], int num, wchar_t *str)
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

//
// use binary search to find the proper position of an author name in an "AUTHOR" array
//
int aname_search(struct AUTHORS d[], int num, wchar_t *str)
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

//
// use binary search to find the proper position of a journal name in a "JOURNAL" array
//
int jname_search(struct JOURNALS d[], int num, wchar_t *str)
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

//
// use binary search to find the proper position of a discipline in a DISCIPLINES array
//
int dpname_search(struct DISCIPLINES d[], int num, wchar_t *str)
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

//
// use binary search to find the proper position of a document id in a WOS array
//
int docid_search(struct WOS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].docid) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].docid) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].docid) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].docid) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search to find the proper position of an alias name in a WOS array
//
int alias_search(struct WOS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].alias) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].alias) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].alias) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].alias) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// given two document ids, find if there are same authors
// NOTE: the method returns a fals TRUE when there are same author names in the same document (they are actually different, but same abbreviations)
// return TRUE or FALSE
//
int self_citation_check(wchar_t *docid1, wchar_t *docid2)
{
	int same_author;

	//struct AUTHORS athr[200];	// removed 2013/04/19
	struct AUTHORS *athr;		// added 2013/04/19
	int ndx1, ndx2;
	int i, nau;
	wchar_t prev_name[MAX_AUTHOR_NAME];
	int prev_paper;

	same_author = FALSE;
	if (full_record_type == WESTLAW_DATA || full_record_type == WESTLAW_DATA2)	// for Westlaw data, this is always FALSE
		return same_author;

	ndx1 = docid_search(wos, nwos, docid1);
	ndx2 = docid_search(wos, nwos, docid2);
	if (ndx1 == -1 || ndx2 == -1)			// this may happen if "All Nodes" in Node Scope is selected, some of the nodes are not in the full record,2013/03/17
		return same_author;

	athr = (struct AUTHORS *)Jmalloc(1000 * sizeof(struct AUTHORS), L"self_citation_check: athr");	// added 2013/04/19, assume the largest possible number of authors per paper is 1000	
	if (athr == NULL) return FALSE;

	nau = 0;
	for (i = 0; i < wos[ndx1].nau; i++)
	{
		athr[nau] = authors[wos[ndx1].author[i]];
		athr[nau++].cflag = ndx1;
	}
	for (i = 0; i < wos[ndx2].nau; i++)
	{
		athr[nau] = authors[wos[ndx2].author[i]];
		athr[nau++].cflag = ndx2;
	}

	qsort((void *)athr, (size_t)nau, sizeof(struct AUTHORS), compare_author);
	// check if there are same author
	prev_name[0] = '\0'; prev_paper = -1;
	for (i = 0; i < nau; i++)
	{
		if (wcscmp(athr[i].name, L"[anon]") == 0)	// ignore name noted "anonymous"
			continue;
		if (wcscmp(athr[i].name, prev_name) == 0)	// hit a same name
		{
			if (athr[i].cflag == prev_paper) // they are in the same paper, ignore, added 2012/12/09
				continue;
			else
			{
				same_author = TRUE;
				break;
			}
		}
		else
		{
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, athr[i].name); 
			prev_paper = athr[i].cflag;	// added 2012/12/09
		}
	}

#ifdef DEBUG
	if (same_author)
	{
		fwprintf(logstream, L"%s (TC=%d): [", wos[ndx1].alias, wos[ndx1].tc);
		for (i = 0; i < wos[ndx1].nau; i++)
			fwprintf(logstream, L"%s; ", authors[wos[ndx1].author[i]].name);
		fwprintf(logstream, L"] ");
		fwprintf(logstream, L"==> %s (TC=%d): [", wos[ndx2].alias, wos[ndx2].tc);
		for (i = 0; i < wos[ndx2].nau; i++)
			fwprintf(logstream, L"%s; ", authors[wos[ndx2].author[i]].name);
		fwprintf(logstream, L"]\n");
	}
#endif DEBUG

	Jfree(athr, L"self_citation_check: athr");

	return same_author;
}

//
// find an author's total SPx value (accumulated over all his publications)
// this function is added 2010/12/30
// NOTE (2012/01/12): the results of this function (total SPx) counts some of paths multiple times,
//                    it is therefore not a good indicator for total number of paths for an author.
//                    a new funciton find_author_total_paths() counts the correct number of paths
// 2012/02/06 ==> changed to assign values to in_spx of souces and out_spx of sinks, they were originally zero,
//                this is under the assumption that there is a super sink after all the sinks, and a super source before all the sources.
//
extern int nnodes;
extern struct PN *nw;
int find_author_total_spx()	
{
	int i, j, k, wi, wk, ak;
	double cur_spx;

	// initialize total spx values
	for (ak = 0; ak < naus; ak++) { authors[ak].total_in_spx = 0.0;  authors[ak].total_out_spx = 0.0; }

	for (i = 0; i < nnodes; i++)	// for all papers
	{
		wi = nw[i].ndx2wos;
		if (nw[i].out_deg != 0)	// not sinks
		{
			for (k = 0; k < nw[i].out_deg; k++)	// for all of this paper's outward links
			{
				cur_spx = nw[i].out_spx[k];
				for (j = 0; j < wos[wi].nau; j++)	// for all authors of this paper
				{
					ak = wos[wi].author[j];
					authors[ak].total_out_spx += cur_spx;
				}
			}
		}
		else // for sinks, take their in_spx as the out_spx
		{
			for (k = 0; k < nw[i].in_deg; k++)	// for all of this paper's inward links
			{
				cur_spx = nw[i].in_spx[k];
				for (j = 0; j < wos[wi].nau; j++)	// for all authors of this papers
				{
					ak = wos[wi].author[j];
					authors[ak].total_out_spx += cur_spx;
				}
			}
		}
	}

	for (i = 0; i < nnodes; i++)	// for all papers
	{
		wi = nw[i].ndx2wos;
		if (nw[i].in_deg != 0)	// not sources
		{
			for (k = 0; k < nw[i].in_deg; k++)	// for all of this paper's inward links
			{
				cur_spx = nw[i].in_spx[k];
				for (j = 0; j < wos[wi].nau; j++)	// for all authors of this papers
				{
					ak = wos[wi].author[j];
					authors[ak].total_in_spx += cur_spx;
				}
			}
		}
		else // for sources, take their out_spx as the in_spx
		{
			for (k = 0; k < nw[i].out_deg; k++)	// for all of this paper's outward links
			{
				cur_spx = nw[i].out_spx[k];
				for (j = 0; j < wos[wi].nau; j++)	// for all authors of this papers
				{
					ak = wos[wi].author[j];
					authors[ak].total_in_spx += cur_spx;
				}
			}
		}
	}

#ifdef XXX
	// output author spx values to a file
	FILE *ostream;
	_wfopen_s(&ostream, L"Author SPX.txt", L"w");

	for (j = 0; j < naus; j++)	
	{
		fwprintf(ostream, L"%04d\t%s\t%.0f\t%.0f\n", j+1, authors[j].name, authors[j].total_in_spx, authors[j].total_out_spx);
		fflush(ostream);
	}
	fclose(ostream);
#endif XXX

	return 0;
}
//
// find an assignee's total SPx value (accumulated over all his publications)
// this function is added 2017/11/02
// NOTE: the results of this function (total SPx) counts some of paths multiple times,
//       it is therefore not a good indicator for total number of paths for an assignee.
//       the funciton find_assignee_total_paths() counts the correct number of paths
// NOTE: assign values to in_spx of souces and out_spx of sinks, they were originally zero,
//                this is under the assumption that there is a super sink after all the sinks, and a super source before all the sources.
//
int find_assignee_total_spx()	
{
	int i, j, k, wi, wk, ak;
	double cur_spx;

	// initialize total spx values
	for (ak = 0; ak < nasgns; ak++) { assignees[ak].total_in_spx = 0.0;  assignees[ak].total_out_spx = 0.0; }

	for (i = 0; i < nnodes; i++)	// for all patents
	{
		wi = nw[i].ndx2wos;
		if (nw[i].out_deg != 0)	// not sinks
		{
			for (k = 0; k < nw[i].out_deg; k++)	// for all of this patent's outward links
			{
				cur_spx = nw[i].out_spx[k];
				for (j = 0; j < wos[wi].nde; j++)	// for all assignees of this patent
				{
					ak = wos[wi].DE[j];
					assignees[ak].total_out_spx += cur_spx;
				}
			}
		}
		else // for sinks, take their in_spx as the out_spx
		{
			for (k = 0; k < nw[i].in_deg; k++)	// for all of this paper's inward links
			{
				cur_spx = nw[i].in_spx[k];
				for (j = 0; j < wos[wi].nde; j++)	// for all authors of this papers
				{
					ak = wos[wi].DE[j];
					assignees[ak].total_out_spx += cur_spx;
				}
			}
		}
	}

	for (i = 0; i < nnodes; i++)	// for all patents
	{
		wi = nw[i].ndx2wos;
		if (nw[i].in_deg != 0)	// not sources
		{
			for (k = 0; k < nw[i].in_deg; k++)	// for all of this paper's inward links
			{
				cur_spx = nw[i].in_spx[k];
				for (j = 0; j < wos[wi].nde; j++)	// for all authors of this papers
				{
					ak = wos[wi].DE[j];
					assignees[ak].total_in_spx += cur_spx;
				}
			}
		}
		else // for sources, take their out_spx as the in_spx
		{
			for (k = 0; k < nw[i].out_deg; k++)	// for all of this paper's outward links
			{
				cur_spx = nw[i].out_spx[k];
				for (j = 0; j < wos[wi].nde; j++)	// for all authors of this papers
				{
					ak = wos[wi].DE[j];
					assignees[ak].total_in_spx += cur_spx;
				}
			}
		}
	}

	return 0;
}

//
// author m-index (main stream index) calculation function
// use depth first algorithm to count the total numnber paths that traverse an author's papers
// this function is added 2012/01/12 to replace find_author_total_spx()
// NOTE: total_paths + multiple_work_paths = sum of out_spx or sum of in_spx
// NOTE: 2015/10/08 added codes to also calculate total paths (m-index) for a research group (codes are in the function move_deeper())
//
#define ASTACK_SIZE 2000
struct ATHRID { int id; };
struct RGRPID { int id; };	// added 2015/10/08
static int slevel;
static double tpaths = 0.0;	// totoa number of paths for this network
static double nodeseq[ASTACK_SIZE];	// a stack to store the node sequence in a path
static struct ATHRID aarray[ASTACK_SIZE];		// an array to store authors in a path
static struct RGRPID garray[ASTACK_SIZE];		// an array to store research groups in a path, added 2015/10/08
static int nau_path;	// number of authors in a path
static int nrg_path;	// number of research groups in a path, added 2015/10/08
static int clusteringoptions;
static FILE *status_stream;
static wchar_t status_name[FNAME_SIZE];
int find_author_total_paths(wchar_t *pname, int clusteringcoauthornetworkoptions)
{
	int i, j, k, m, ak, gk;

	clusteringoptions = clusteringcoauthornetworkoptions;

	swprintf(status_name, L"%s m-index status.txt", pname);
	_wfopen_s(&status_stream, status_name, L"wt, ccs=UTF-8");	// modified 2016/01/19
	fwprintf(status_stream, L"m-index calculation progress:\n");
	fclose(status_stream);


	// initialization
	for (ak = 0; ak < naus; ak++) { authors[ak].total_paths = 0.0; authors[ak].multiple_work_paths = 0.0; }
	if (clusteringcoauthornetworkoptions == CCoauthor_UNWEIGHTED)
	{
		for (gk = 0; gk < nrgroups; gk++) { rgroups[gk].total_paths = 0.0; rgroups[gk].multiple_work_paths = 0.0; }
	}

	// search begins from all sources
	for (m = 0; m < nsources; m++)	
	{
		for (k = 0; k < ASTACK_SIZE; k++) nodeseq[k] = 0;
		i = sources[m];
		//fwprintf(ostream, L"Source %d, NW %d, %s\n", m, i, nw[i].alias); fflush(ostream);
		_wfopen_s(&status_stream, status_name, L"at, ccs=UTF-8");	// added 2015/10/31, modified for Unicode 2016/01/19
		fwprintf(status_stream, L"%d/%d (%d) %s==>\n", m+1, nsources, nw[i].out_deg, nw[i].alias);	// added 2015/10/31
		fclose(status_stream);
		slevel = 0;	
		move_deeper(i, nw);
	}
		
	// find m-index for each group, added 2015/10/08
	if (clusteringcoauthornetworkoptions == CCoauthor_UNWEIGHTED)
	{
		for (i = 0; i < nrgroups; i++)
			rgroups[i].mindex = rgroups[i].total_paths / tpaths;
	}

	FILE *ostream;
	wchar_t tmpname[FNAME_SIZE];
	swprintf(tmpname, L"%s Author SPX.txt", pname);
	_wfopen_s(&ostream, tmpname, L"wt, ccs=UTF-8");	// modified 2016/01/19

	// output author total_paths values to a file (along with their number of citations and h-indices)
	fwprintf(ostream, L"Total number of papers (WOS/network)= %d/%d\nTotal paths in the citation network = %.0f\n", nwos, nnodes, tpaths);


	fwprintf(ostream, L"\n============= Author Information (all authors) ================\n");
	fwprintf(ostream, L"Index\tName\tNPapers\tTotalDescendants\tNTotalDescendants\tTotalPaths\tm-index\th-index\tTotalCites\tTotalOutDegree\tDuplicate\tOut_SPX\tIn_SPX\n");
	for (j = 0; j < naus; j++)	
	{
		fwprintf(ostream, L"%04d\t%s\t%d\t%d\t%f\t%.0f\t%f\t%d\t%d\t%d\t%.0f\t%.0f\t%.0f\n", j+1, 
			authors[j].name, authors[j].np, authors[j].total_descendants, (double)authors[j].total_descendants/nwos, 
			authors[j].total_paths, authors[j].total_paths/tpaths, authors[j].h, authors[j].total_cites, 
			authors[j].total_outdeg, authors[j].multiple_work_paths, authors[j].total_out_spx, authors[j].total_in_spx);
		fflush(ostream);
	}
			
	fwprintf(ostream, L"\n============= Author Information (1st authors only) ================\n");	// added 2015/11/03
	fwprintf(ostream, L"Index1\tIndex2\tName\tNPapers\tTotalDescendants\tNTotalDescendants\tTotalPaths\tm-index\th-index\tTotalCites\tTotalOutDegree\tDuplicate\tOut_SPX\tIn_SPX\n");
	int cnt = 0;
	for (j = 0; j < naus; j++)	
	{
		if (authors[j].cnt1 > 0)
		{
			fwprintf(ostream, L"%04d\t%04d\t%s\t%d\t%d\t%f\t%.0f\t%f\t%d\t%d\t%d\t%.0f\t%.0f\t%.0f\n", cnt+1, j+1, 
				authors[j].name, authors[j].np, authors[j].total_descendants, (double)authors[j].total_descendants/nwos, 
				authors[j].total_paths, authors[j].total_paths/tpaths, authors[j].h, authors[j].total_cites, 
				authors[j].total_outdeg, authors[j].multiple_work_paths, authors[j].total_out_spx, authors[j].total_in_spx);
			fflush(ostream);
			cnt++;
		}
	}
		
	fclose(ostream);

	return 0;
}

//
// given a node, mark the node
// recursive, depth-first algorithm
//
// NOTE: total_paths + multiple_work_paths = sum of out_spx or sum of in_spx
// NOTE: 2015/10/08 added to also calculate total paths (m-index) for a research group
//
int ccnt = 0;
int move_deeper(int nid, struct PN *nwk)
{
	int j, k;

	nodeseq[slevel] = nid;
	if (nwk[nid].type == T_SINK) 
	{ 		
		int i, id, wi, ak, gk, m;
		nau_path = 0;
		nrg_path = 0; // added 2015/10/08
		for (m = 0; m <= slevel; m++)
		{
			id = nodeseq[m];
			// mark for all the authors of this paper
			wi = nwk[id].ndx2wos;
			for (j = 0; j < wos[wi].nau; j++)	// for all authors of this paper
			{	
				ak = wos[wi].author[j];
				aarray[nau_path++].id = ak;	// save temporarily to an array
				if (authors[ak].groupid != 999999)	// added 2015/10/08, only for non-1st-authors
					garray[nrg_path++].id = authors[ak].groupid;	// added 2015/10/08
			}
		}
		// work on author array
		qsort((void *)aarray, (size_t)nau_path, sizeof(struct ATHRID), compare_aid);
#ifdef DEBUG
		for (i = 0; i < nau_path; i++)
		{
			ak = aarray[i].id;
			if (wcscmp(authors[ak].name, L"10muller") == 0)
			{
				ccnt++;
				fwprintf(logstream, L"%d: ", ccnt);
				for (j = 0; j < nau_path; j++)
				{
					ak = aarray[j].id;
					fwprintf(logstream, L"%s=>", authors[ak].name);
				}
				fwprintf(logstream, L"\n");
				break;
			}
		}
#endif DEBUG
		int prev = -1; 
		for (i = 0; i < nau_path; i++)
		{
			ak = aarray[i].id;
			if (ak != prev)	// hit a different author
			{
				authors[ak].total_paths++;
				prev = ak; 
			}
			else	// each replication of the author indicate a duplicate path, 
			{
				authors[ak].multiple_work_paths++;	// this path contains multiple works for author ak
			}
		}
		// work on group array, added 2015/10/08
		if (clusteringoptions == CCoauthor_UNWEIGHTED)
		{
			qsort((void *)garray, (size_t)nrg_path, sizeof(struct RGRPID), compare_gid);
			prev = -1; 
			for (i = 0; i < nrg_path; i++)
			{
				gk = garray[i].id;
				if (gk != prev)	// hit a different group
				{
					rgroups[gk].total_paths++;
					prev = gk; 
				}
				else	// each replication of the group indicate a duplicate path, 
				{
					rgroups[gk].multiple_work_paths++;	// this path contains multiple works for research groups gk
				}
			}
		}

		tpaths++;	// hit a sink, count as one path
		return 0; 
	}	

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{	
		if (slevel == 1)	// added 2015/10/31
		{
			// write out the progress status to the status file
			_wfopen_s(&status_stream, status_name, L"at, ccs=UTF-8");	// added 2015/10/31, modified for Unicode 2016/01/19
			fwprintf(status_stream, L"  %.0f\t%s\n", tpaths, nwk[nwk[nid].out_nbrs[k]].alias);	// added 2015/10/31
			fclose(status_stream);
		}
		j = nwk[nid].out_nbrs[k];
		move_deeper(j, nwk);
	}
	slevel--;

	return 0;
}

//
// assignee m-index (main stream index) calculation function
// use depth first algorithm to count the total numnber paths that traverse an author's papers
// this function is added 2017/11/02
//
extern int nasgns;
extern struct ASSIGNEES *assignees;
int find_assignee_total_paths(wchar_t *pname)
{
	int i, j, k, m, ak, gk;

	swprintf(status_name, L"%s m-index status.txt", pname);
	_wfopen_s(&status_stream, status_name, L"wt, ccs=UTF-8");	// modified 2016/01/19
	fwprintf(status_stream, L"m-index calculation progress:\n");
	fclose(status_stream);

	// initialization
	for (ak = 0; ak < nasgns; ak++) { assignees[ak].total_paths = 0.0; assignees[ak].multiple_work_paths = 0.0; }

	// search begins from all sources
	for (m = 0; m < nsources; m++)	
	{
		for (k = 0; k < ASTACK_SIZE; k++) nodeseq[k] = 0;
		i = sources[m];
		//fwprintf(ostream, L"Source %d, NW %d, %s\n", m, i, nw[i].alias); fflush(ostream);
		_wfopen_s(&status_stream, status_name, L"at, ccs=UTF-8");	
		fwprintf(status_stream, L"%d/%d (%d) %s==>\n", m+1, nsources, nw[i].out_deg, nw[i].alias);	
		fclose(status_stream);
		slevel = 0;	
		move_deeper_assignee(i, nw);
	}
		
	FILE *ostream;
	wchar_t tmpname[FNAME_SIZE];
	swprintf(tmpname, L"%s Assignee SPX.txt", pname);
	_wfopen_s(&ostream, tmpname, L"wt, ccs=UTF-8");	

	// output assignee total_paths values to a file (along with other information)
	fwprintf(ostream, L"Total number of patents (original/network)= %d/%d\nTotal paths in the citation network = %.0f\n", nwos, nnodes, tpaths);

	fwprintf(ostream, L"\n============= Assignee Information ================\n");
	fwprintf(ostream, L"Index\tName\tNPatents\tTotalDescendants\tTotalPaths\tm-index\tTotalOutDegree\tDuplicate\tOut_SPX\tIn_SPX\n");
	for (j = 0; j < nasgns; j++)	
	{
		fwprintf(ostream, L"%04d\t%s\t%d\t%d\t%.0f\t%f\t%d\t%.0f\t%.0f\t%.0f\n", j+1, 
			assignees[j].name, assignees[j].np, assignees[j].total_descendants, assignees[j].total_paths, assignees[j].total_paths/tpaths, 
			assignees[j].total_outdeg, assignees[j].multiple_work_paths, assignees[j].total_out_spx, assignees[j].total_in_spx);
		fflush(ostream);
	}

	fclose(ostream);

	return 0;
}

//
// given a node, mark the node
// recursive, depth-first algorithm
// NOTE: total_paths + multiple_work_paths = sum of out_spx or sum of in_spx
// NOTE: wos[].nde has the number of assignee information
//       wos[].DE[] has the individual assignee index
// this function is added 2017/11/02
//
int move_deeper_assignee(int nid, struct PN *nwk)
{
	int j, k;

	nodeseq[slevel] = nid;
	if (nwk[nid].type == T_SINK) 
	{ 		
		int i, id, wi, ak, gk, m;
		nau_path = 0;
		nrg_path = 0; // added 2015/10/08
		for (m = 0; m <= slevel; m++)
		{
			id = nodeseq[m];
			// mark for all the authors of this paper
			wi = nwk[id].ndx2wos;
			for (j = 0; j < wos[wi].nde; j++)	// for all assignees of this patent
			{	
				ak = wos[wi].DE[j];
				aarray[nau_path++].id = ak;	// save temporarily to an array
			}
		}
		// work on assignee array
		qsort((void *)aarray, (size_t)nau_path, sizeof(struct ATHRID), compare_aid);
#ifdef DEBUG
		for (i = 0; i < nau_path; i++)
		{
			ak = aarray[i].id;
			if (wcscmp(assignees[ak].name, L"10muller") == 0)
			{
				ccnt++;
				fwprintf(logstream, L"%d: ", ccnt);
				for (j = 0; j < nau_path; j++)
				{
					ak = aarray[j].id;
					fwprintf(logstream, L"%s=>", assignees[ak].name);
				}
				fwprintf(logstream, L"\n");
				break;
			}
		}
#endif DEBUG
		int prev = -1; 
		for (i = 0; i < nau_path; i++)
		{
			ak = aarray[i].id;
			if (ak != prev)	// hit a different author
			{
				assignees[ak].total_paths++;
				prev = ak; 
			}
			else	// each replication of the assignee indicate a duplicate path, 
			{
				assignees[ak].multiple_work_paths++;	// this path contains multiple works for assignee ak
			}
		}
		tpaths++;	// hit a sink, count as one path
		return 0; 
	}	

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{	
		if (slevel == 1)	
		{
			// write out the progress status to the status file
			_wfopen_s(&status_stream, status_name, L"at, ccs=UTF-8");	
			fwprintf(status_stream, L"  %.0f\t%s\n", tpaths, nwk[nwk[nid].out_nbrs[k]].alias);	
			fclose(status_stream);
		}
		j = nwk[nid].out_nbrs[k];
		move_deeper_assignee(j, nwk);
	}
	slevel--;

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_index(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (t2->ndx < t1->ndx)
		return 1;
	else if (t2->ndx == t1->ndx)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_aid(const void *n1, const void *n2)
{
	struct ATHRID *t1, *t2;
	
	t1 = (struct ATHRID *)n1;
	t2 = (struct ATHRID *)n2;
	if (t2->id < t1->id)
		return 1;
	else if (t2->id == t1->id)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_gid(const void *n1, const void *n2)
{
	struct RGRPID *t1, *t2;
	
	t1 = (struct RGRPID *)n1;
	t2 = (struct RGRPID *)n2;
	if (t2->id < t1->id)
		return 1;
	else if (t2->id == t1->id)
		return 0;
	else return -1;
}

//
// find the number of outward nodes eminating from the works of an author, "emnating from the works of an author" is the key point here
// that is, find all the "descendants" of all author's works
// this function is added 2012/02/07
//
int total_descendants = 0;	// the count includes the node itself and all its descendants
int find_author_outward_nodes()
{	int i, j, k, m, ak;

	// initialization
	for (ak = 0; ak < naus; ak++) { authors[ak].total_descendants = 0; authors[ak].total_outdeg = 0; }
	for (i = 0; i < nnodes; i++) { nw[i].was_here = FALSE; nw[i].flag = FALSE; }	// clear the "was_here" and "flag" 

	// search through each paper of an author for all authors
	for (m = 0; m < naus; m++)	
	{
		for (i = 0; i < nnodes; i++) nw[i].flag = FALSE; // clear "flag" which indicates if the node has been counted as one of the descendants
		total_descendants = 0;
		//fwprintf(logstream, L"%s %d: ", authors[m].name, authors[m].np);
		for (k = 0; k < authors[m].np; k++)
		{
			for (i = 0; i < nnodes; i++) nw[i].was_here = FALSE; // clear "was_here" which indicates if the current search has touch on the node
			j = wos[authors[m].paper[k]].ndx2nw;	// index in the nw[] array
			if (j < 0) continue;	// the paper is not included in the network
			authors[m].total_outdeg += nw[j].out_deg;
			//fwprintf(logstream, L"%s(%d) ", nw[j].alias, nw[j].out_deg);
			slevel = 0;	
			// begin search from this node (which have this author as one of the co-author)
			move_deeper2(j, nw);
		}
		//fwprintf(logstream, L"\n");
		authors[m].total_descendants = total_descendants;	
	}

#ifdef OBSOLETE	// this is done in the function find_author_total_paths()
	FILE *ostream;
	_wfopen_s(&ostream, L"Author SPX.txt", L"w");

	// output author total_paths values to a file (along with their number of citations and h-indices)
	fwprintf(ostream, L"\tName\tNPapers\tTotalDescendants\tNTotalDescendants\tTotalPaths\tNTotalPaths\th-index\tTotalCites\tTotalOutDegree\tDuplicate\tOut_SPX\tIn-SPX\n");
	for (j = 0; j < naus; j++)	
	{
		fwprintf(ostream, L"%04d\t%s\t%d\t%d\t%f\t%.0f\t%f\t%d\t%d\t%d\t%.0f\t%.0f\t%.0f\n", j+1, 
			authors[j].name, authors[j].np, authors[j].total_descendants, (double)authors[j].total_descendants/nwos, 
			authors[j].total_paths, authors[j].total_paths/tpaths, authors[j].h, authors[j].total_cites, 
			authors[j].total_outdeg, authors[j].multiple_work_paths, authors[j].total_out_spx, authors[j].total_in_spx);
		fflush(ostream);
	}
	
	fwprintf(ostream, L"Total number of papers (WOS/network)= %d/%d\nTotal paths in the citation network = %f\n", nwos, nnodes, tpaths);

	fclose(ostream);
#endif OBSOLETE

	return 0;
}

//
// given a node, mark the node
// recursive, depth-first algorithm
// this function is called inside the function find_author_outward_nodes()
//
int move_deeper2(int nid, struct PN *nwk)
{
	int i, j, k, wi, ak, m;
	int id;

	if (nwk[nid].was_here == TRUE) return 0;
	// mark the node
	nwk[nid].was_here = TRUE;
	if (nwk[nid].flag == FALSE)
	{
		nwk[nid].flag = TRUE;	// mark as one of the descendants
		total_descendants++;
	}

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		j = nwk[nid].out_nbrs[k];
		move_deeper2(j, nwk);
	}
	slevel--;

	return 0;
}

//
// find the number of outward nodes eminating from the works of an assigneer, "emnating from the works of an assignee" is the key point here
// that is, find all the "descendants" of all assignee's works
// this function is added 2017/11/02
//
int find_assignee_outward_nodes()
{	int i, j, k, m, ak;

	// initialization
	for (ak = 0; ak < nasgns; ak++) { assignees[ak].total_descendants = 0; assignees[ak].total_outdeg = 0; }
	for (i = 0; i < nnodes; i++) { nw[i].was_here = FALSE; nw[i].flag = FALSE; }	// clear the "was_here" and "flag" 

	// search through each paper of an assignee for all assignees
	for (m = 0; m < nasgns; m++)	
	{
		for (i = 0; i < nnodes; i++) nw[i].flag = FALSE; // clear "flag" which indicates if the node has been counted as one of the descendants
		total_descendants = 0;
		//fwprintf(logstream, L"%s %d: ", assignees[m].name, assignees[m].np);
		for (k = 0; k < assignees[m].np; k++)
		{
			for (i = 0; i < nnodes; i++) nw[i].was_here = FALSE; // clear "was_here" which indicates if the current search has touch on the node
			j = wos[assignees[m].paper[k]].ndx2nw;	// index in the nw[] array
			if (j < 0) continue;	// the paper is not included in the network
			assignees[m].total_outdeg += nw[j].out_deg;
			//fwprintf(logstream, L"##### %s(%d) ", nw[j].alias, nw[j].out_deg);
			slevel = 0;	
			// begin search from this node (which have this author as one of the co-author)
			move_deeper2_assignee(j, nw);
		}
		//fwprintf(logstream, L"\n");
		assignees[m].total_descendants = total_descendants;	
	}

	return 0;
}

//
// given a node, mark the node
// recursive, depth-first algorithm
// this function is called inside the function find_author_outward_nodes()
//
int move_deeper2_assignee(int nid, struct PN *nwk)
{
	int i, j, k, wi, ak, m;
	int id;

	if (nwk[nid].was_here == TRUE) return 0;
	// mark the node
	nwk[nid].was_here = TRUE;
	if (nwk[nid].flag == FALSE)
	{
		nwk[nid].flag = TRUE;	// mark as one of the descendants
		total_descendants++;
	}

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		j = nwk[nid].out_nbrs[k];
		move_deeper2_assignee(j, nwk);
	}
	slevel--;

	return 0;
}

//
// find the number of outward paths eminating from the works of an author, "emnating from the works of an author" is the key point here
// this function is added 2012/02/05 to replace find_author_total_paths() and the even older find_author_total_spx()
//    find_author_total_paths() count the paths that taverse through the works of an author, no duplicate paths
//    find_author_total_spx()   count the paths that taverse through the works of an author, but with duplicate paths
//
int find_author_outward_paths()
{
	// to be implemented

	return 0;
}

