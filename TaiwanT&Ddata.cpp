// 
// TaiwanT&Ddata.cpp
//
// Author: John Liu
// 
// this file contains function that handles Taiwan thesis and disseatation data (台灣碩博士論文資料)
//
//

//
// Revision History:
// 2016/01/25 Basic function works
// NOTES: Files that has “Taiwan T&D”in the 1st line is treated as 台灣碩博士資網 data.
//        The support for TaiwanT&D citation links are verified using thesis title, and title only. 
//        Title keywords, for now, are taken from author keywords. That means the statistics for title keywords and author keywords are the same, 
//        There is now the support for the new file “author report.txt”.
//        All four files "keyword alias.txt",“keyword report.txt", "author alias.txt", and "author report.txt" needs to be in the format "UTF-8"
//             and without BOM (byte order mark in the beginning of a file). The format can be controlled using Notepad++ editor.
// 2016/02/01 Added codes to check if there is byte-order-mark, this is necessary after the modification of openning file in UTF-8 type. Previous  
//              limitations that some files need to be without BOM is gone.
// 2016/03/09 Added codes to display the "missing 研究生 label" message in the MainPath log file
// 2016/03/19 Added a new TDID label "URI "
// 2016/03/30 Fixed a problem in parsing "論文名稱：", spaces within the title are now allowed
// 2016/03/30 Change the method of assigning docid, the counting method used previously cannot work after clustering, changed to use author name 
// 2016/05/12 Fixed a problem caused by non-initialization in .acnt and .acnt2 of the tkeyword array
// 2016/07/02 Moved to call prepare_keyword_alias_list() before calling parse_store_keywords()	
// 2016/09/15 added codes to handle the situation when a citation file is provided
// 2017/05/03 added a new delimiter ',' when parsing author names
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include "resource.h"
#include "network.h"

#define XXLBUF_SIZE 65536
#define BUF_SIZE 1024

//
// function types
//
int store_tdauthors(wchar_t *, int *, struct AUTHORS *);
int tdauthor_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int store_tddepartment(wchar_t *, int *, struct DEPARTMENTS *);
int tddepartment_names(wchar_t *, int *, int, struct DEPARTMENTS *);
int parse_references(wchar_t *, wchar_t *, FILE *, int, int);
int compare_author(const void *, const void *);
int compare_ttd_title(const void *, const void *);
int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
int store_tdkwde(wchar_t *, int *, struct KWORDS *);
int tdkwde_names(wchar_t *, int *, int *, int, struct KWORDS *);
int read_author_report(int, struct AUTHORS *);
int ttdtitle_search(struct TTD_TITLE *, int, wchar_t *);
int dname_search(struct DEPARTMENTS *, int, wchar_t *);
int compare_deptname(const void *, const void *);

extern int link_author_WOS();
extern int compare_kname(const void *, const void *);
extern int compare_docid(const void *, const void *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int kname_search(struct KWORDS *, int, wchar_t *);
extern int prepare_author_alias_list();
extern int prepare_keyword_alias_list();
extern int prepare_department_alias_list();
extern int keywordalias_search(struct KEYWORD_ALIAS *, int, wchar_t *);
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int departmentalias_search(struct DEPARTMENT_ALIAS *, int, wchar_t *);
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int coauthor(wchar_t *, int);	

//
// global variables
//
int nareport;
struct AUTHOR_REPORT *areport;
struct TTD_TITLE *ttd_title;
int ndeps;
struct DEPARTMENTS *departments;	// author name array, inventors are stored in the same array

static FILE *sstream;		// for source file

extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern int nkwde;
extern struct KWORDS *kwde;	// author keywords
extern int ntkeywords;
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;
extern int nkeywordalias;	// number of items in the keyword alias table
extern struct KEYWORD_ALIAS *keywordalias;
extern int ndepartmentalias;	// number of items in the author alias table
extern struct DEPARTMENT_ALIAS *departmentalias;
extern int text_type;
extern FILE *logstream;
extern int no_wos_citation_file;	// added 2016/09/15


struct TDID tdid[27] = {
L"論文名稱：", 5, 1,
L"論文名稱(外文)：", 9, 2,
L"論文出版年：", 6, 3,
L"研究生：", 4, 4,
L"研究生(外文)：", 8, 5,
L"指導教授：", 5, 6,
L"指導教授(外文)：", 9, 7,
L"口試委員：", 5, 8,
L"口試委員(外文)：", 9, 9,
L"學位類別：", 5, 10,
L"校院名稱：", 5, 11,
L"系所名稱：", 5, 12,
L"畢業學年度：", 6, 13,
L"語文別：", 4, 14,
L"論文頁數：", 5, 15,
L"中文關鍵詞：", 6, 16,
L"外文關鍵詞：", 6, 17,
L"論文摘要：", 5, 18,
L"論文外文摘要：", 7, 19,
L"論文目次：", 5, 20,
L"論文參考文獻：", 7, 21,
L"被引用:", 4, 22,
L"點閱:", 3, 23,
L"評分:", 3, 24,
L"下載:", 3, 25,
L"書目收藏:", 5, 26,
L"URI ", 4, 27,	// added 2016/0319
};


//
// read Taiwan T&D data and put it into the wos[] array
//
int read_TaiwanTD(wchar_t *sname)
{
	int i, k, ret;
	int state, lcnt, pcnt, cnt;
	int slen;
	wchar_t tmps[BUF_SIZE];
	wchar_t cur_author[MAX_AUTHOR_NAME];
	wchar_t lptr[XXLBUF_SIZE], *line, *dp, *tp;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t student[200];
	FILE *cstream;		// for citation file
	FILE *astream;		// for assignee alias file

// 
// Open the source file (will fail if the file does not exist)
//		
	if (text_type == FT_ASCII)
	{
		_wfopen_s(&sstream, sname, L"r");	// open as text mode for ASCII
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

	srand((unsigned)clock());	// only for the purpose of generating random citation links

	// 1st pass, count the number of papers in the file
	state = 1; lcnt = 0; pcnt = 0;
	while (TRUE)
	{		
		line = lptr;
		if(fgetws(lptr, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (lptr[0] == '\n' || line[0] == '\r')
			continue;
		if (wcsncmp(line, L"論文名稱：", 5) == 0 && state != 1)		// synchronization check
		{ 
			fwprintf(logstream, L"WARNING: Synchronization error encountered [state=%d] on %s", state, line); 
			//state = 1;	// enfore to synchronize
		} 
		switch (state)
		{
		case 1:	// "論文名稱：", it may take several lines to find this label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{	
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0' && *tp != '\n') tp++; tp--;
				while (*tp == L' ') tp--; tp++; *tp = L'\0';	// remove spaces and line feed at the end
				wcscpy(tmps, dp);
				//fwprintf(logstream, L"#####%d [%s]\n", pcnt+1, tmps); fflush(logstream);
				state = 2;
			}
			break;		
		case 2:	// 論文名稱(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 3; break; } else state = 3; 
		case 3:	// 論文出版年：, this label can be missing
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 4; break; } else state = 4; 
		case 4:	// 研究生：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
				state = 5; 
			else 
			{
				fwprintf(logstream, L"Missing \"研究生\" label in the paper [%s]", tmps);	// added 2016/03/09
				return MSG_WOSFILE_FORMAT_ERROR;
			}
			break;
		case 5:	// 研究生(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 6; break; } else state = 6; 
		case 6:	// 指導教授：, there can be multiple advisers
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 6;
				break;
			}
			else 
				state = 7;
		case 7:	// 指導教授(外文), optional and multiple
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 7;
				break;
			}
			else 
				state = 8; 
		case 8:	// 口試委員：, there can be multiple oral committee members
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 8;
				break;
			}
			else 
				state = 9; 
		case 9:	// 口試委員(外文)：, optional and multipe
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 9;
				break;
			}
			else 
				state = 10; 
		case 10:	// 學位類別：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 11; break; } else state = 11; 
		case 11:	// 校院名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 12; 
			break; 
		case 12:// 系所名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 13;	else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// 畢業學年度：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 14; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// 語文別：, optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 15; break; } else state = 15; 
		case 15:// 論文頁數： , optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 16; break; } else state = 16; 
		case 16:// 中文關鍵詞：, there can be repeated "中文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 16;
				break;
			}
			else 
				state = 17;
		case 17:// 外文關鍵詞：, there can be repeated "外文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 17;
				break;
			}
			else 
				state = 18;
		case 18:// 論文摘要：	// optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 19; break; } else state = 19; 
		case 19:// 論文外文摘要： // optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 20; break; } else state = 20; 
		case 20:// 論文目次：, optonal label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 21; break; } else state = 21; 
		case 21:// 論文參考文獻： , optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 22; break; } else state = 22;
		case 22:// 被引用:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
				state = 23; 
			break;
		case 23:// 點閱:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 24; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 24:// 評分:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 25; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 25:// 下載: 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 26; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 26:// 書目收藏:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				pcnt++;
				state = 1;
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}
	nwos = pcnt;
	// allocate memory for the list of Taiwan T&D data
	wos = (struct WOS *)Jmalloc(nwos * sizeof(struct WOS), L"read_TaiwanTD: wos");
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nwos; i++)	// initialization
	{
		wos[i].nau = 0; wos[i].nde = 0; wos[i].nid = 0; wos[i].ndspln = 0;
	}

	// allocate memory for the titles of Taiwan T&D data
	ttd_title = (struct TTD_TITLE *)Jmalloc(nwos * sizeof(struct TTD_TITLE), L"read_TaiwanTD: ttd_title");	
	if (ttd_title == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// open citation file
	if (no_wos_citation_file)	// this check is added 2016/09/15
	{
		strip_name_extension(sname, tname, dpath);
		ret = swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname);
			if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"wt, ccs=UTF-8") != 0)
			return MSG_CFILE_CANNOTOPEN;
	}

	// 2nd pass, get the year, degree, author data, and establish the author array, keyword array
	rewind(sstream);	// point back to the begining of the file
	// allocate memory for the author name array
	authors = (struct AUTHORS *)Jmalloc(nwos * 3 * sizeof(struct AUTHORS), L"read_TaiwanTD: authors");	// estimate in average 3 authors (students+advisors) per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// allocate memory for the author keyword data
	kwde = (struct KWORDS *)Jmalloc(nwos * 10 * sizeof(struct KWORDS), L"read_TaiwanTD: kwde");	// estimate in average 10 keywords per document
	if (kwde == NULL) return MSG_NOT_ENOUGH_MEMORY;
	departments = (struct DEPARTMENTS *)Jmalloc(nwos * sizeof(struct DEPARTMENTS), L"read_TaiwanTD: departments");	
	if (departments == NULL) return MSG_NOT_ENOUGH_MEMORY;

	prepare_author_alias_list();	// this is so that we can handle various form of an author's name
	prepare_keyword_alias_list();	// this is so that we can handle variations of keywords
	prepare_department_alias_list();	// this is so that we can handle variations of department names

	state = 1; lcnt = 0; pcnt = 0; naus = 0; nkwde = 0;
	while (TRUE)
	{		
		line = lptr;
		if(fgetws(lptr, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (lptr[0] == '\n' || line[0] == '\r')
			continue;
		switch (state)
		{
		case 1:	// "論文名稱：", it may take several lines to find this label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{				
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0' && *tp != '\n') tp++; tp--;
				while (*tp == L' ') tp--; tp++; *tp = L'\0';	// remove spaces and line feed at the end
				wcscpy(ttd_title[pcnt].name, dp);
				ttd_title[pcnt].ndx = pcnt;
				wos[pcnt].ndx2title = pcnt;	// will be used later, after sorting, added 2017/0422
				//fwprintf(logstream, L"#####%d [%s]\n", pcnt, ttd_title[pcnt].name); fflush(logstream);
				state = 2;
			}
			break;		
		case 2:	// 論文名稱(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 3; break; } else state = 3; 
		case 3:	// 論文出版年：, this label can be missing
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 4; break; } else state = 4; 
		case 4:	// 研究生：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces and line feed at the end
				store_tdauthors(&line[tdid[state-1].len], &naus, authors);
				wcscpy(cur_author, dp);
				state = 5; 
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 5:	// 研究生(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 6; break; } else state = 6; 
		case 6:	// 指導教授：, there can be multiple advisers
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces/comma and line feed at the end
				store_tdauthors(&line[tdid[state-1].len], &naus, authors);
				state = 6;
				break;
			}
			else 
				state = 7;
		case 7:	// 指導教授(外文), optional and multiple 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 7;
				break;
			}
			else 
				state = 8; 
		case 8:	// 口試委員：, there can be multiple oral committee members
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 8;
				break;
			}
			else 
				state = 9; 
		case 9:	// 口試委員(外文)：, optional and multiple 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 9;
				break;
			}
			else 
				state = 10; 
		case 10:	// 學位類別：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = &line[tdid[state-1].len];
				if (wcsncmp(dp, L"碩士", 2) == 0)
					wos[pcnt].info |= TAIWANTD_MASTER;
				else if (wcsncmp(dp, L"博士", 2) == 0)
					wos[pcnt].info |= TAIWANTD_PHD;
				state = 11; 
				break;
			}
			else
			{
				wos[pcnt].info |= TAIWANTD_DEGREE_UNKNOWN;
				state = 11; 
			}
		case 11:	// 校院名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 12; 
			break; 
		case 12:// 系所名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces and line feed at the end
				store_tddepartment(&line[tdid[state-1].len], &ndeps, departments);
				state = 13;	
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// 畢業學年度：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = &line[tdid[state-1].len];
				wos[pcnt].year = _wtoi(dp) + 1911;
				//fwprintf(logstream, L"[%d] \n", wos[pcnt].year);
				// make up the paper ID (wos[].docid)
				if ((wos[pcnt].info &= 0xff) == TAIWANTD_MASTER)
					swprintf(wos[pcnt].docid, L"%d_1_%s", wos[pcnt].year, cur_author);	// modified 2016/03/30
				else if ((wos[pcnt].info &= 0xff) == TAIWANTD_PHD)
					swprintf(wos[pcnt].docid, L"%d_2_%s", wos[pcnt].year, cur_author);	// modified 2016/03/30
				else
					swprintf(wos[pcnt].docid, L"%d_?_%s", wos[pcnt].year, cur_author);	// modified 2016/03/30
				state = 14; 
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// 語文別：, optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 15; break; } else state = 15; 
		case 15:// 論文頁數： , optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 16; break; } else state = 16; 
		case 16:// 中文關鍵詞：, there can be repeated "中文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0' && *tp != L' ') tp++; tp--; *tp = L'\0';	// remove the spaces and line feed at the end
				store_tdkwde(dp, &nkwde, kwde);
				state = 16;
				break;
			}
			else 
				state = 17;
		case 17:// 外文關鍵詞：, there can be repeated "外文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 17;
				break;
			}
			else 
				state = 18;
		case 18:// 論文摘要：	// optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 19; break; } else state = 19; 
		case 19:// 論文外文摘要： // optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 20; break; } else state = 20; 
		case 20:// 論文目次：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 21; break; } else state = 21;
		case 21:// 論文參考文獻： , optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 22; break; } else state = 22;
		case 22:// 被引用:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
				state = 23; 
			break;
		case 23:// 點閱:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 24; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 24:// 評分:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 25; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 25:// 下載: 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 26; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 26:// 書目收藏:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				pcnt++;
				state = 1;
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}

#ifdef DEBUG
	for (i = 0; i < naus; i++)
		fwprintf(logstream, L"%d %s\r\n", i+1, authors[i].name);
#endif DEBUG

	// sort the article titles, this is to be used in checking references
	qsort((void *)ttd_title, (size_t)nwos, sizeof(struct TTD_TITLE), compare_ttd_title);
	for (i = 0; i < nwos; i++)
		wos[ttd_title[i].ndx].ndx2title = i;	// added 2017/04/22

	// sort and consolidate duplicate author names
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

	// consolidate duplicate keywords
	qsort((void *)kwde, (size_t)nkwde, sizeof(struct KWORDS), compare_kname);
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nkwde; i++)
	{
		if (wcscmp(kwde[i].name, prev_name) != 0)	// hit new name
		{
			if (k > 0) kwde[k-1].cnt = cnt;
			wcscpy_s(kwde[k++].name, MAX_KEYWORD_NAME, kwde[i].name); 
			wcscpy_s(prev_name, MAX_KEYWORD_NAME, kwde[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	kwde[k-1].cnt = cnt;
	nkwde = k;

	// sort and consolidate duplicate author names
	qsort((void *)departments, (size_t)ndeps, sizeof(struct DEPARTMENTS), compare_deptname);
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < ndeps; i++)
	{
		if (wcscmp(departments[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) departments[k-1].cnt = cnt;
			wcscpy_s(departments[k++].name, MAX_DEPARTMENT_NAME, departments[i].name); 
			wcscpy_s(prev_name, MAX_DEPARTMENT_NAME, departments[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	departments[k-1].cnt = cnt;
	ndeps = k;

#ifdef DEBUG
	//for (i = 0; i < naus; i++)
	//	fwprintf(logstream, L"%d %s [%d]\n", i+1, authors[i].name, authors[i].np);
	for (i = 0; i < nkwde; i++)
		fwprintf(logstream, L"%d %s [%d]\n", i+1, kwde[i].name, kwde[i].cnt);
	for (i = 0; i < ndeps; i++)
		fwprintf(logstream, L"%d %s [%d]\n", i+1, departments[i].name, departments[i].cnt);
#endif DEBUG

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
		case 1:	// "論文名稱：", it may take several lines to find this label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				//fwprintf(logstream, L"%d %s", pcnt+1, line); fflush(logstream);
				state = 2;
			}
			break;		
		case 2:	// 論文名稱(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 3; break; } else state = 3; 
		case 3:	// 論文出版年：, this label can be missing
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 4; break; } else state = 4; 
		case 4:	// 研究生：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces and line feed at the end
				// save name to the wos[] array
				wos[pcnt].nau = 0;	// start here, advisors will be regarded as  2nd, 3rd, ... authors
				tdauthor_names(dp, &wos[pcnt].nau, wos[pcnt].author, naus, authors);
				wcscpy(student, dp);
				state = 5; 
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 5:	// 研究生(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 6; break; } else state = 6; 
		case 6:	// 指導教授：, there can be multiple advisers
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces and line feed at the end
				tdauthor_names(dp, &wos[pcnt].nau, wos[pcnt].author, naus, authors);	// advisors are regarded as 2nd, 3rd ... authors
				// make up the paper alias (wos[].alias)
				if (wos[pcnt].nau >= 2)	// assign alias name only once
				{
					if ((wos[pcnt].info &= 0xff) == TAIWANTD_MASTER)
						swprintf(wos[pcnt].alias, L"%d碩%s_%s", wos[pcnt].year, student, authors[wos[pcnt].author[1]].name);
					else if ((wos[pcnt].info &= 0xff) == TAIWANTD_PHD)
						swprintf(wos[pcnt].alias, L"%d博%s_%s", wos[pcnt].year, student, authors[wos[pcnt].author[1]].name);
					else
						swprintf(wos[pcnt].alias, L"%d??%s_%s", wos[pcnt].year, student, authors[wos[pcnt].author[1]].name);
					fwprintf(logstream, L"[%s]\n", wos[pcnt].alias);
				}
				state = 6;
				break;
			}
			else 
				state = 7;
		case 7:	// 指導教授(外文), optional and multiple
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 7;
				break;
			}
			else 
				state = 8; 
		case 8:	// 口試委員：, there can be multiple oral committee members
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 8;
				break;
			}
			else 
				state = 9; 
		case 9:	// 口試委員(外文)：, otinal and multiple 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 9;
				break;
			}
			else 
				state = 10; 
		case 10:	// 學位類別：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 11; break; } else state = 11; 
		case 11:	// 校院名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 12; 
			break; 
		case 12:// 系所名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces and line feed at the end
				// save name to the wos[] array
				tddepartment_names(dp, &wos[pcnt].department, ndeps, departments);
				state = 13; 
			}
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// 畢業學年度：, data is collected in the 2nd pass
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 14;	else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// 語文別：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 15; break; } else state = 15; 
		case 15:// 論文頁數： , optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 16; break; } else state = 16; 
		case 16:// 中文關鍵詞：, there can be repeated "中文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0' && *tp != L' ') tp++; tp--; *tp = L'\0';	// remove the spaces and line feed at the end
				tdkwde_names(dp, &wos[pcnt].nde, wos[pcnt].DE, nkwde, kwde);
				state = 16;
				break;
			}
			else 
				state = 17;
		case 17:// 外文關鍵詞：, there can be repeated "外文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 17;
				break;
			}
			else 
				state = 18;
		case 18:// 論文摘要：, optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 19; break; } else state = 19; 
		case 19:// 論文外文摘要：, optional 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 20; break; } else state = 20; 
		case 20:// 論文目次：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 21; break; } else state = 21;
		case 21:// 論文參考文獻：, reference information is to be obtained in the next state, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; *tp = L'\0';	// remove line feed at the end
				parse_references(dp, wos[pcnt].docid, cstream, wos[pcnt].year, pcnt);	// catch the reference right after the label "論文參考文獻："
				state = 22; 
			}
			else
				state = 22;
		case 22:// 被引用:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
				state = 23; 
			else	// obtaining the remaining reference information while waiting for the label "被引用:"
			{
				dp = tp = line;
				while (*tp != L'\0') tp++; tp--; *tp = L'\0';	// remove the line feed at the end
				while (*dp == ' ' || *dp == '\t') dp++;	// skip the leading spaces and tabs
				parse_references(dp, wos[pcnt].docid, cstream, wos[pcnt].year, pcnt);
			}
			break;
		case 23:// 點閱:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 24; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 24:// 評分:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 25; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 25:// 下載: 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 26; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 26:// 書目收藏:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				pcnt++;
				state = 1;
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}

	fclose(sstream);	// close full record file
	if (no_wos_citation_file) fclose(cstream);	// close citation file (relationship file) if it is created

#ifdef DEBUG
	//for (i = 0; i < nwos; i++)
	//{
	//	fwprintf(logstream, L"==>%d %d ", i+1, wos[i].nau);
	//	for (k = 0; k < wos[i].nau; k++)
	//		fwprintf(logstream, L"{%d %s}\n", wos[i].author[k], authors[wos[i].author[k]].name);
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%d {%s}\n", wos[i].department, departments[wos[i].department].name);
	}
#endif DEBUG

	link_author_WOS();	// actually, this fuction recalculate authors[].np

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
	fwprintf(logstream, L"Author statistics:\n");
	fwprintf(logstream, L"Total papers\t1st authors\tBYear\tEYear\tName\n");
	for (i = 0; i < naus; i++)
		fwprintf(logstream, L"%d\t%d\t%d\t%d\t%s\n", authors[i].np, authors[i].cnt1, authors[i].byear, authors[i].eyear, authors[i].name);

	// count the number of times each author as the 1st author
	ret = coauthor(sname, 0); if (ret != 0) return ret;	// output coauthor network that contains all authors
	ret = coauthor(sname, 1); if (ret != 0) return ret;	// output coauthor network that contains only authors that have been the 1st author 


	read_author_report(naus, authors);	// if "Author report.txt" does not exist, nareport = 0;

	// write out department statistics
	fwprintf(logstream, L"\nDepartment statistics:\n");
	fwprintf(logstream, L"\tDepartment\tCount\n");
	for (i = 0; i < ndeps; i++)
		fwprintf(logstream, L"%d\t%s\t%d\n", i, departments[i].name, departments[i].cnt);



	// NOTE!!!! following codes use the author keyword as the title keyword, for now !!!, 2016/01/21
	//=========
	ntkeywords = nkwde;
	tkeyword = (struct TKEYWORDS *)Jmalloc(ntkeywords * sizeof(struct TKEYWORDS), L"read_TaiwanTD: tkeyword");
	for (i = 0; i < ntkeywords; i++)
	{
		wcscpy(tkeyword[i].name, kwde[i].name);
		tkeyword[i].cnt = kwde[i].cnt;
	}
	for (i = 0; i < nwos; i++)
	{
		wos[i].ntkws = wos[i].nde;
		for (k = 0; k < wos[i].ntkws; k++)
			wos[i].tkws[k] = wos[i].DE[k];
	}

	for (i = 0; i < ntkeywords; i++) { tkeyword[i].acnt = 0; tkeyword[i].acnt2 = 0; tkeyword[i].ndx = i; } // initialization, added 2016/05/12

	//=========

	ret = keyword_year_matrix(ntkeywords, tkeyword);
	if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;

	// sort the data by the document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);

	return 0;
}

//
// find each author's index, and then save them to the wos[] array
//
int tdauthor_names(wchar_t *astr, int *nathrs, int *athr, int naus, struct AUTHORS *authors)
{
	int i;;
	int ndx, ndx2;
	wchar_t aname[MAX_AUTHOR_NAME];
	wchar_t tname[MAX_AUTHOR_NAME];
	wchar_t lname[MAX_AUTHOR_NAME];
	wchar_t *tp, *sp;
	int tnathrs;

	tnathrs = *nathrs;

	sp = astr; tp = lname; while (*sp !='\0') *tp++ = towlower(*sp++); *tp = '\0';	
	ndx2 = authoralias_search(authoralias, nauthoralias, lname);	
	if (ndx2 >= 0)	// found that this is a variation of the author name
		wcscpy_s(aname, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
	else
		wcscpy_s(aname, MAX_AUTHOR_NAME, lname);
	sp = aname; tp = tname;
	while (*sp != '\0')
	{
		if (*sp == ' ' || *sp == ';' || *sp == L'、' || *sp == ',')	// hit a delimiter, added a new delimiter ',', 2017/05/03 
		{
			*tp = '\0';
			ndx = aname_search(authors, naus, tname);
			athr[tnathrs++] = ndx;
			if (tnathrs >= MAX_AUTHORS)	// take only MAX_AUTHORS authors
			{
				*nathrs = tnathrs; return 0;
			}
			tp = tname; sp++; 
		}
		else
			*tp++ = *sp++;
	}
	*tp = '\0';
	ndx = aname_search(authors, naus, tname);
	athr[tnathrs++] = ndx;

	*nathrs = tnathrs;

	return 0;
}

//
// save the author names to the global author name array
//
int store_tdauthors(wchar_t *astr, int *nau, struct AUTHORS *au)
{
	int tnau;
	int ndx2;
	wchar_t aname[MAX_AUTHOR_NAME];
	wchar_t tname[MAX_AUTHOR_NAME];
	wchar_t lname[MAX_AUTHOR_NAME];
	wchar_t *tp, *sp;

	tnau = *nau;

	sp = astr; tp = lname; while (*sp !='\0') *tp++ = towlower(*sp++); *tp = '\0';	
	ndx2 = authoralias_search(authoralias, nauthoralias, lname);	
	if (ndx2 >= 0)	// found that this is a variation of the author name
		wcscpy_s(aname, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
	else
		wcscpy_s(aname, MAX_AUTHOR_NAME, lname);
	sp = aname; tp = tname;
	while (*sp != '\0')
	{
		if (*sp == ' ' || *sp == ';' || *sp == L'、' || *sp == ',')	// hit a delimiter, added a new delimiter ',', 2017/05/03
		{
			*tp = '\0';
			wcscpy(au[tnau++].name, tname);
			tp = tname; sp++;
		}
		else
			*tp++ = *sp++;
	}
	*tp = '\0';
	wcscpy(au[tnau++].name, tname);

	*nau = tnau;

	return 0;
}

//
// save the author keywords to the global keyword array
//
int store_tdkwde(wchar_t *str, int *nkwde, struct KWORDS *kwde)
{
	int tnkwde;
	int ndx2;

	tnkwde = *nkwde;
	ndx2 = keywordalias_search(keywordalias, nkeywordalias, str);
	if (ndx2 >= 0)	// found that this is a variation of the keyword
		wcscpy_s(kwde[tnkwde++].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
	else
		wcscpy_s(kwde[tnkwde++].name, MAX_KEYWORD_NAME, str);
	*nkwde = tnkwde;

	return 0;
}

//
// find each DE keyword's index, and then save them to the wos[] array
//
int tdkwde_names(wchar_t *kstr, int *nkeyws, int *keyw, int nkwde, struct KWORDS *kwde)
{
	int i;
	int ndx, ndx2;
	int tnkeyws;

	tnkeyws = *nkeyws;
	ndx2 = keywordalias_search(keywordalias, nkeywordalias, kstr);	// replace if there is a alias keyword name
	if (ndx2 >= 0)	// found that this is a variation of the keyword
		ndx = kname_search(kwde, nkwde, keywordalias[ndx2].alias);
	else
		ndx = kname_search(kwde, nkwde, kstr);
	//fwprintf(logstream, L"[%s] %d %d\n", kstr, ndx, tnkeyws); fflush(logstream);
	keyw[tnkeyws] = ndx;
	tnkeyws++; 
	if (tnkeyws >= MAX_KEYWORDS)	// take only MAX_KEYWORDS keywords
	{
		*nkeyws = tnkeyws;
		return 0;
	}
	*nkeyws = tnkeyws;

	return 0;
}

//
// save the department names to the global department name array
//
int store_tddepartment(wchar_t *str, int *ndept, struct DEPARTMENTS *dept)
{
	int tndept;
	int ndx2;

	tndept = *ndept;
	ndx2 = departmentalias_search(departmentalias, ndepartmentalias, str);
	if (ndx2 >= 0)	// found that this is a variation of the keyword
		wcscpy_s(dept[tndept++].name, MAX_DEPARTMENT_NAME, departmentalias[ndx2].alias);
	else
		wcscpy_s(dept[tndept++].name, MAX_DEPARTMENT_NAME, str);
	*ndept = tndept;

	return 0;
}

//
// find each department name's index, and then save them to the wos[] array
//
int tddepartment_names(wchar_t *dstr, int *dps, int ndept, struct DEPARTMENTS *depts)
{
	int i;
	int ndx, ndx2;
	int tndepts;

	ndx2 = departmentalias_search(departmentalias, ndepartmentalias, dstr);	// replace if there is a alias keyword name
	if (ndx2 >= 0)	// found that this is a variation of the keyword
		ndx = dname_search(depts, ndept, departmentalias[ndx2].alias);
	else
		ndx = dname_search(depts, ndept, dstr);
	//fwprintf(logstream, L"[%s] %d %d\n", kstr, ndx, tnkeyws); fflush(logstream);
	*dps = ndx;

	return 0;
}

//
// parse information in the "論文參考文獻：" section
// NOTE: for now, simply return random connections
//
#define TRUE_LINKS
int parse_references(wchar_t *str, wchar_t *docid, FILE *stream, int year, int ndx)
{
	int i, k, cnt, index;
	int degree;
	wchar_t buf[BUF_SIZE];
	wchar_t *sp, *tp;

	if (!no_wos_citation_file)	// added 2016/09/15
		return 0;

#ifdef TRUE_LINKS
	if (iswascii((int)str[0]) && !(iswdigit((int)str[0]) || str[0] == '['))
		return 0;	// ignore references that start in ASCII characters (except those that starts in digit and left bracket)
	else 
	{
		if (iswdigit((int)str[0]) || str[0] == '[') // for those start in digit and left bracket
		{
			if (iswascii((int)str[4]) && iswascii((int)str[5]))	
				return 0;	// ignore references that start in digit and most of characters are in English
		}
		// parse the remaining reference strings
		//fwprintf(logstream, L"==>[%s]\n", str);
		sp = str; tp = buf; cnt = 0;
		while (*sp != '\0')
		{
			if (*sp == L'。' || *sp == L'，'|| *sp == ',' || *sp == '.' || *sp == ' ') 
			{
				*tp = '\0';
				//fwprintf(logstream, L"@@@[%s]\n", buf);
				index = ttdtitle_search(ttd_title, nwos, buf);
				if (index != -1)
				{
					k = ttd_title[index].ndx;
					if (k != ndx)	// rule out the situation of citing self
					{
						if (wos[ndx].year >= wos[k].year)	// only if citing backward
						{
							fwprintf(logstream, L"     Cites: %d [%s]\n", wos[k].year, buf);
							fwprintf(stream, L"%s %s\n", wos[k].docid, docid);
							fflush(logstream);
						}
					}
				}
				tp = buf; sp++;
			}
			else
			{
				*tp++ = *sp++;
				cnt++;
				if (cnt >= BUF_SIZE)
					break;	// string too long, forget about the remaining characters
			}
		}
	}


#endif TRUE_LINKS

#ifdef RANDOM_LINKS
	degree = 2 + (int)(((double)nwos * rand() / (RAND_MAX + 1)) / 50);	// determine the number of connections
	for (i = 0, cnt = 0; i < degree;)
	{
		k = (int)((double)nwos * rand() / (RAND_MAX + 1));
		if (wos[k].year > year)	// link only forwared references
		{
			fwprintf(stream, L"%s %s\n", docid, wos[k].docid, docid); fflush(logstream);
			i++;
		}
		cnt++; if (cnt > (degree * 3)) break;
	}
#endif RANDOM_LINKS

	return 0;
}

//
// read the "Author report.txt" file
// save the content the array areport[]
//
int read_author_report(int naus, struct AUTHORS *authors)
{
	FILE *astream;
	int i, na, index;
	wchar_t line2[BUF_SIZE], *line;
	wchar_t *sp, *tp;

	for (i = 0; i < naus; i++) authors[i].ndx2 = -1;	// initialize the cross reference to the authors in the keyword report file

	if (_wfopen_s(&astream, L"Author report.txt", L"rt, ccs=UTF-8") == 0)	// open as UTF-8 file	
	{
		// count the number of authors in the file
		na = 0;
		while (TRUE)
		{		
			if(fgetws(line2, BUF_SIZE, astream) == NULL)
				break;		
			if (line2[0] == '\n' || line2[0] == '\r' || line2[0] == ' ' || line2[0] == '\t')
				continue;
			na++;
		}
		nareport = na;
	}
	else
	{
		nareport = 0;
		return 0;
	}

	areport = (struct AUTHOR_REPORT*)Jmalloc(nareport * sizeof(struct AUTHOR_REPORT), L"read_author_report: areport");
	if (areport == NULL) return MSG_NOT_ENOUGH_MEMORY;

	rewind(astream);
	na = 0;
	while (TRUE)
	{		
		if(fgetws(line2, BUF_SIZE, astream) == NULL)
			break;		
		if (line2[0] == '\n' || line2[0] == '\r' || line2[0] == ' ' || line2[0] == '\t')
			continue;	
		line = line2;
		if (na == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line2[0] == 0xfeff || line2[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				line = &line2[1];	// skip the BOM
		}
		sp = tp = line; while (*sp !='\0' && *sp != '\n') *tp++ = towlower(*sp++); *tp = '\0';	
		wcscpy(areport[na].name, line);
		index = aname_search(authors, naus, line);
		if (index != -1)
		{
			areport[na].ndx = index;
			authors[index].ndx2 = na;
		}
		else
			areport[na].ndx = -1;
		na++;
	}

#ifdef DEBUG
	for (na = 0; na < nareport; na++)
	{
		fwprintf(logstream, L"==>[%s] %d\n", areport[na].name, areport[na].ndx);
	}
#endif DEBUG

	fclose(astream);

	return 0;
}

//
// use binary search to find the proper position of a journal name in a "JOURNAL" array
//
int ttdtitle_search(struct TTD_TITLE d[], int num, wchar_t *str)
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
// this fucntion is to be called by qsort() only
// 
int compare_ttd_title(const void *n1, const void *n2)
{
	struct TTD_TITLE *t1, *t2;
	
	t1 = (struct TTD_TITLE *)n1;
	t2 = (struct TTD_TITLE *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of a department name in an "DEPARTMENTS" array
//
int dname_search(struct DEPARTMENTS d[], int num, wchar_t *str)
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
// this fucntion is to be called by qsort() only
// 
int compare_deptname(const void *n1, const void *n2)
{
	struct DEPARTMENTS *t1, *t2;
	
	t1 = (struct DEPARTMENTS *)n1;
	t2 = (struct DEPARTMENTS *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}
