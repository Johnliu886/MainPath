// 
// WEBPATdata3.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the Webpat database
//
// NOTE:
// Starting from October 2016, WEBPAT provided a new type of data format (in Excel).
// The codes in this file handles the new format. 
// The codes in WEBPATdata2.cpp handles another WEBPAT format (also in Excel).
// The codes in USPTOdata.cpp handles WEBPAT data in text format.
//
// Revision History:
// 2016/10/14 Basic function works
// 2016/11/24 Replace function pfgetws() with wfgetws(). This is because the format of WEBPATdata2 and WEBPATdata3 are different.
// 2016/12/07 Added a block of codes to prepare an University-Industry relation file
// 2016/12/24 Added to call coassignee() function
// 2017/01/22 Webpat made another unnoticed change (again!) in the output data. "專利名稱" shorted to "名稱"
//            Modified the codes in t reading data to accept both the "名稱" and "專利名稱" as the label for patent title
// 2017/01/25 Added a block of code to get the delimiter (for assignee, CPC, references, etc.)
//               This is due a change in the Webpat format, again!!! (was using ',', changed to ';' around 2017/01/23)
// 2017/04/21 Added to take care of cases that the patent id begins with country ID "US"
// 2017/06/20 Added codes to handle different date format (yes, Webpat changed date format recently! was: 2017/12/30, now: 12/30/2017)
// 2017/08/24 Added codes to handle "現專利權人國家" information
// 2017/10/28 Added to accept patent id of the format "US06167391", was able to accept the formats "US06167391B1" and "06167391"
//            Fixed a problem in correct_WEBPAT3_pid(), to include the ending 'B1' in patent id
// 2018/01/03 Added codes to recognize the leading identifiers in English
// 2018/02/10 Added codes to take the patent in the form "USRE046158" (in valid_patent_id())
// 2018/02/10 Changed to not removing the leading '0' in the patent id 
//            [previously, WEBPAT citations contains both the id with and without the leading '0', but now it has only the id with the leading '0']
// 2018/02/10 Added two new variables: WEBPAT3_delimiter_CPC and WEBPAT3_delimiter_inventor
//            [this is because the new WEBPAT data contains different delimiters across fields]
// 2018/05/02 Added check for the return of keyword_year_matrix()
// 2018/06/27 Added code to include published application of patents as valid patents, in addition to issued patents
// 2018/08/09 Changed MAX_AUTHORS to MAX_INVENTORS (former is much bigger than the later)
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

static int date_format = 0; // 0: not yet determined, 1: 2017/12/30, 2: 12/30/2017
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
extern struct LOCATION location2[];

extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;

int parse_WEBPAT3_1st_line(wchar_t *, int, wchar_t *);
int parse_WEBPAT3_line(wchar_t *, int, wchar_t *);
int parse_WEBPAT3_Citing_Patents(wchar_t *, wchar_t *, FILE *);
int parse_store_WEBPAT3_inventor_first(wchar_t *, int *, struct AUTHORS *);
int parse_WEBPAT3_inventor_first_names(wchar_t *, int *, int *, int, struct AUTHORS *);
int parse_WEBPAT3_CPC_Current(wchar_t *, int *, wchar_t *, int *);
int parse_WEBPAT3_US_Class(wchar_t *, int *, wchar_t *);
int compare_author(const void *, const void *);
int parse_WEBPAT3_year(wchar_t *, int *year, int *month);
int valid_patent_id(wchar_t *);
int valid_patent_id_US_issued(wchar_t *);
int valid_patent_id_US_published_application(wchar_t *);

int correct_WEBPAT3_pid(wchar_t *, wchar_t *);
int parse_WEBPAT3_store_assignees(wchar_t *, int *, struct ASSIGNEES *);
int parse_WEBPAT3_assignee_names(wchar_t *, int *, int *, int, struct ASSIGNEES *);
int is_university(wchar_t *);

extern int prepare_keyword_alias_list();
extern int is_stopwords(wchar_t *);
extern int compare_tkeyword(const void *, const void *);
extern int parse_title(wchar_t *, int, int *, int *, int, struct TKEYWORDS *);	// added 2014/07/23
extern int parse_store_title(int, struct TKEYWORDS *, wchar_t *);	// added 2014/07/23
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int link_author_WOS();
extern int relink_author_WOS();
extern int link_assignee_WOS();
extern int relink_assignee_WOS();
extern int compare_docid(const void *, const void *);
extern int compare_aalias(const void *, const void *);
extern int parse_aalias(wchar_t *, wchar_t *, wchar_t *);
extern int aalias_search(struct ANAME_ALIAS*, int, wchar_t *);
extern int compare_assignee(const void *, const void *);
extern int asgnname_search(struct ASSIGNEES *, int, wchar_t *);
extern int keyword_year_matrix(int, struct TKEYWORDS *);
extern int compare_pid(const void *, const void *);
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int index_of(int, wchar_t *, wchar_t *);
extern int coassignee(wchar_t *, int);
extern int parse_countries_WEBPAT3(int, wchar_t *, int, struct ASSIGNEES *);

#define XXLBUF_SIZE 65536*4	// there are extremely long texts in the "Cited Refs - Non-patent" field in the Thomson Innovatin data
#define BUF_SIZE 1024

#define DMTR_COMMA ','
#define DMTR_SEMICOLON ';'
wchar_t WEBPAT3_delimiter;
wchar_t WEBPAT3_delimiter_CPC;	// added 2018/02/10
wchar_t WEBPAT3_delimiter_inventor;	// added 2018/02/10
wchar_t *wline;
extern int just_exit_at_next_line;	// set it as if the 1st data line is read

int init_wfgetws(FILE *sstream)
{
	wline = (wchar_t *)Jmalloc(XXLBUF_SIZE * sizeof(wchar_t), L"init_wfgetws: wline");

	return 0;
}

int done_wfgetws()
{
	Jfree(wline, L"done_wfgetws: wline");

	return 0;
}

//
// this function collects text strings until all fields are collected 
// the reason why this is necessary is that WEBPAT data (.csv file) embed many line feeds in a record and end a record with "\r\n", which is preceded with many commas
// what's more, fgetws() function changes "\r\n" sequence to '\n' such that we cannot rely on the sequence to find a record
//
wchar_t *wfgetws(wchar_t *line, int bufsize, FILE *sstream)
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
			//fwprintf(logstream, L"$$$$$%s", tline); fflush(logstream);
		}
		else
			wcscpy(tline, wline);	// copy back from the buffer
		sp = tline; 
		if (!just_exit_at_next_line)	// check if the line begins with a digit, some US patent ID begins with 'RE', 'H', or 'D'
		{
			if (valid_patent_id(tline))
			{
				just_exit_at_next_line = 1;	
				wcscpy(wline, tline);	// save the content that is already read
				break;
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
// given a line, check if the leading column is a valid patent id (for US patent)
//
int valid_patent_id(wchar_t *line)
{
	int validity;

	validity = valid_patent_id_US_issued(line);
	validity |= valid_patent_id_US_published_application(line);	// added 2018/06/27

	return validity;
}

//
// given a line, check if the leading column is a valid patent id (for US patent)
// added to take cases that the patent id begins with country ID "US", but takes no other countries, for now, 2017/04/20
// format: US06622142B1 or 06622142
//
int valid_patent_id_US_issued(wchar_t *line)
{
	wchar_t *sp;
	int i;
	int ccnt;

	sp = line;
	if (sp[0] == '\n' || sp[0] == '\r' || sp[0] == ' ' || sp[0] == '\t') return 0;
	if (sp[0] == 'U' && sp[1] == 'S' && (iswdigit(sp[2]) || (sp[2] == 'R' && sp[3] == 'E')))	// the patent id begins with 'US', improved 2018/02/10
	{
		sp = line + 2;
		// the patent id is ended with B1 or B2
		if ((sp[0] == 'R' && sp[1] == 'E') || (iswdigit(sp[0]) && iswdigit(sp[1])) || (sp[0] == 'H' && iswdigit(sp[1])) || (sp[0] == 'D' && iswdigit(sp[1])))
		{
			sp += 2; ccnt = 0; 
			for (i = 0; i < 8; i++, sp++)
			{
				if (*sp == '\0' || *sp == ',')	break;
				else if (iswdigit(*sp) || isascii(*sp)) ccnt++;
			}
			if (*sp == ',' && ccnt == 8)	// the new line begins with 'US' plus 10 codes followed by a ',' 
				return 1;
		}
		// the patent id does not ended with B1 or B2, added 2017/10/28
		sp = line + 2;
		if ((sp[0] == 'R' && sp[1] == 'E') || (iswdigit(sp[0]) && iswdigit(sp[1])) || (sp[0] == 'H' && iswdigit(sp[1])) || (sp[0] == 'D' && iswdigit(sp[1])))
		{
			sp = line + 2;
			sp += 2; ccnt = 0; 
			for (i = 0; i < 6; i++, sp++)
			{
				if (*sp == '\0' || *sp == ',')	break;
				else if (iswdigit(*sp)) ccnt++;
			}
			if (*sp == ',' && ccnt == 6)	// the new line begins with 'US' plus 8 digits followed by a ',' 
				return 1;
		}
	}
	//else if (iswupper(sp[0]) && iswupper(sp[1]) && iswdigit(sp[2]))	// for country other than US, for now
	//	return 1;
	else if ((sp[0] == 'R' && sp[1] == 'E') || (iswdigit(sp[0]) && iswdigit(sp[1])) || (sp[0] == 'H' && iswdigit(sp[1])) || (sp[0] == 'D' && iswdigit(sp[1]))) // the patent id does not begins with 'US'
	{
		sp += 2; ccnt = 0; 
		for (i = 0; i < 6; i++, sp++)
		{
			if (*sp == '\0' || *sp == ',')	break;
			else if (iswdigit(*sp)) ccnt++;
		}
		if (*sp == ',' && ccnt == 6)	// the new line begins with 8 digits followed by a ',' 
			return 1;
	}
	else 
		return 0;

	return 0;
}

//
// given a line, check if the leading column is a valid published application id (for US patent)
// format: US20070239794A1 or 20070239794
//
int valid_patent_id_US_published_application(wchar_t *line)
{
	wchar_t *sp;
	wchar_t tyear[30];
	int year;
	int i;
	int ccnt;

	sp = line;
	if (sp[0] == '\n' || sp[0] == '\r' || sp[0] == ' ' || sp[0] == '\t') return 0;
	if (sp[0] == 'U' && sp[1] == 'S')	// the patent id begins with 'US'
		sp = line + 2;
	// 1st check, has to contain 11 continuous digit
	for (i = 0; i < 11; i++)	
	{
		tyear[i] = sp[i];
		if (!iswdigit(sp[i]))
			return 0;
	}
	// 2nd check, the leading four digits represent year
	tyear[4] = '\0';
	year = _wtoi(tyear);
	if (year > 1970 && year < 2100)
		return 1;
	else
		return 0;
}

//
// read WOS data and put it into the wos[] array
//
int read_WEBPAT3(wchar_t *sname)
{
	int i, k, m, ndx;
	int nsr, cnt;
	int ret;
	int i_ut, i_au, i_py, i_pm, i_cr, i_ipc, i_usc, i_an, i_ti, i_ab, i_nt, i_nto, i_ano;
	wchar_t cname[FNAME_SIZE], tname[FNAME_SIZE], dpath[FNAME_SIZE];
	wchar_t tmps[BUF_SIZE], *gret, *sp;
	FILE *cstream;		// for citation file
	FILE *astream;		// for assignee alias file
	wchar_t *line;
	wchar_t *tfield;
	int in_English;

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
	init_wfgetws(sstream); // prepare for xfgetws() to work
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;
	nsr = 0;
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		//fwprintf(logstream, L"#####[%s]\n", line); fflush(logstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nsr++;	
		if (gret == NULL) 
			break;
	}
	//fwprintf(logstream, L"#####[%s]\n", line); fflush(logstream);
	nuspto = nsr;
	fclose(sstream);

	// open again, because if use rewind(), the 1st fgetws() is abnormal
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

	//rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	fgetws(line, XXLBUF_SIZE, sstream);	// get the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	// look for the index of the specified target field name 
	in_English = 0;	// default is false
	i_ut = parse_WEBPAT3_1st_line(line, SP_COLON, L"公告(開)號");	// document ID
	if (i_ut == -1)
	{
		i_ut = parse_WEBPAT3_1st_line(line, SP_COLON, L"Publication Number");	// document ID
		if (i_ut != -1) in_English = 1; else return MSG_WOSFILE_FORMAT_ERROR;
	}
	if (in_English)	// added 2018/01/03
	{
		i_au = parse_WEBPAT3_1st_line(line, SP_COLON, L"Inventors");	// author names
		i_py = parse_WEBPAT3_1st_line(line, SP_COLON, L"Publication Date");	// date of publication
		i_pm = parse_WEBPAT3_1st_line(line, SP_COLON, L"Application Date");	// application date
		i_cr = parse_WEBPAT3_1st_line(line, SP_COLON, L"Domestic References");	// this field actually contains "citing" information
		i_ipc = parse_WEBPAT3_1st_line(line, SP_COLON, L"Cooperative Patnet Classification");	// CPC code, was IPC code
		i_an = parse_WEBPAT3_1st_line(line, SP_COLON, L"Assignees");	// Assignee/Applicant
		i_ano = parse_WEBPAT3_1st_line(line, SP_COLON, L"Current Assignees");	
		if (i_ano != -1) i_an = i_ano;	// if there is "現專利權人" informaiton, use it rather than using "專利權人"
		i_nt = parse_WEBPAT3_1st_line(line, SP_COLON, L"Assignee Country");	// nation
		i_nto = parse_WEBPAT3_1st_line(line, SP_COLON, L"Current Assignee Country");
		if (i_nto != -1) i_nt = i_nto;	// if there is "現專利權人國家" informaiton, use it rather than using "專利權人國家". 
		i_ti = parse_WEBPAT3_1st_line(line, SP_COLON, L"Title");		// title 
		i_ab = parse_WEBPAT3_1st_line(line, SP_COLON, L"Abstract");	// abstract 
	}
	else
	{
		i_au = parse_WEBPAT3_1st_line(line, SP_COLON, L"發明人");	// author names
		i_py = parse_WEBPAT3_1st_line(line, SP_COLON, L"公告(開)日");	// date of publication
		i_pm = parse_WEBPAT3_1st_line(line, SP_COLON, L"申請日");	// application date
		i_cr = parse_WEBPAT3_1st_line(line, SP_COLON, L"國內引證資料");	// this field actually contains "citing" information
		i_ipc = parse_WEBPAT3_1st_line(line, SP_COLON, L"CPC");	// CPC code, was IPC code
		i_an = parse_WEBPAT3_1st_line(line, SP_COLON, L"專利權人");	// Assignee/Applicant
		i_ano = parse_WEBPAT3_1st_line(line, SP_COLON, L"現專利權人");	// added 2018/01/03
		if (i_ano != -1) i_an = i_ano;	// if there is "現專利權人" informaiton, use it rather than using "專利權人", added 2018/01/03
		i_nt = parse_WEBPAT3_1st_line(line, SP_COLON, L"專利權人國家");	// nation
		i_nto = parse_WEBPAT3_1st_line(line, SP_COLON, L"現專利權人國家");	// added 2017/08/24
		if (i_nto != -1) i_nt = i_nto;	// if there is "現專利權人國家" informaiton, use it rather than using "專利權人國家", added 2017/08/24
		i_ti = parse_WEBPAT3_1st_line(line, SP_COLON, L"專利名稱");		// title 
		if (i_ti == -1)	// try a different label
			i_ti = parse_WEBPAT3_1st_line(line, SP_COLON, L"名稱");		// added 2017/01/22
		i_ab = parse_WEBPAT3_1st_line(line, SP_COLON, L"摘要");	// abstract 
	}
	if (i_ut == -1 || i_au == -1 || i_py == -1 || i_pm == -1 || i_cr == -1 || i_ipc == -1 || i_an == -1 || i_nt == -1 || i_ti == -1 || i_ab == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	// get the delimiter, following code block is added 2017/01/25, due a change in the Webpat format, again!!! (was using ',', changed to ';' around 2017/01/23)
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;	// read source file line by line
	i = 0;
	int comma_cnt, semicolon_cnt;
	comma_cnt = semicolon_cnt = 0;
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_cr, tfield);
		sp = tfield;
		while (*sp != '\0')
		{
			if (*sp == ',') comma_cnt++;
			else if (*sp == ';') semicolon_cnt++;
			sp++;
		}
		i++;
		if (i > 10) break;	// 10 records is enough to know the delimiter
	}
	if (semicolon_cnt < comma_cnt)
		WEBPAT3_delimiter = DMTR_COMMA;
	else
		WEBPAT3_delimiter = DMTR_SEMICOLON;

	// get the delimiter for the inventor field, following code block is added 2018/02/10, due a change in the Webpat format, again!!! (they now have inconsistent delimits acorss fields)
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;	// read source file line by line
	i = 0;
	comma_cnt = semicolon_cnt = 0;
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_au, tfield);
		sp = tfield;
		while (*sp != '\0')
		{
			if (*sp == ',') comma_cnt++;
			else if (*sp == ';') semicolon_cnt++;
			sp++;
		}
		i++;
		if (i > 10) break;	// 10 records is enough to know the delimiter
	}
	if (semicolon_cnt < comma_cnt)
		WEBPAT3_delimiter_inventor = DMTR_COMMA;
	else
		WEBPAT3_delimiter_inventor = DMTR_SEMICOLON;

	// get the delimiter for the CPC field, following code block is added 2018/02/10, due a change in the Webpat format, again!!! (they now have inconsistent delimits acorss fields)
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;	// read source file line by line
	i = 0;
	comma_cnt = semicolon_cnt = 0;
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_ipc, tfield);
		sp = tfield;
		while (*sp != '\0')
		{
			if (*sp == ',') comma_cnt++;
			else if (*sp == ';') semicolon_cnt++;
			sp++;
		}
		i++;
		if (i > 10) break;	// 10 records is enough to know the delimiter
	}
	if (semicolon_cnt < comma_cnt)
		WEBPAT3_delimiter_CPC = DMTR_COMMA;
	else
		WEBPAT3_delimiter_CPC = DMTR_SEMICOLON;

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
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		//fwprintf(logstream, L"!!!!! %d: [%c%c%c%c%c%c%c%c%c%c]\n", i, line[0], line[1], line[2], line[3], line[4],
		//	line[5], line[6], line[7], line[8], line[9]); fflush(logstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		//if (i == 507)
		//	fwprintf(logstream, L"@@@@@ [%s]\n", line);
		parse_WEBPAT3_line(line, i_au, tfield);
		//fwprintf(logstream, L"***** %d: [%s]\n", i, tfield); fflush(logstream);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		parse_store_WEBPAT3_inventor_first(tfield, &naus, authors);
		//fwprintf(logstream, L"##### %d: %d [%s]\n", i, naus, tfield); fflush(logstream);
		parse_WEBPAT3_line(line, i_an, tfield);
		parse_WEBPAT3_store_assignees(tfield, &nasgns, assignees);
		//fwprintf(logstream, L"@@@@@ %d: %d [%s]\n", i, nasgns, tfield); fflush(logstream);
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
	for (i = 0; i < naus; i++)
	{
		fwprintf(logstream, L"#####%d [%s] %d\n", i+1, authors[i].name, authors[i].np);
		fflush(logstream);
	}
	for (i = 0; i < nasgns; i++)
	{
		fwprintf(logstream, L"@@@@@%d [%s] %d\n", i+1, assignees[i].name, assignees[i].np);
		fflush(logstream);
	}
#endif DEBUG

// ================================
// following are the codes for the University-Industry Project
// the codes select only the patents that contain universities and industrial companies as assignees or applicants
// it then writes out the collaborating U-I into the "... U-I relations" file
//
#ifdef UNIVERSITY_INDUSRY_PROJECT
	// get required data
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;
	// read source file line by line
	i = 0;
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_ut, tfield);
		correct_WEBPAT3_pid(uspto[i].pid, tfield);	// write to .pid after correction
		parse_WEBPAT3_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		//fwprintf(logstream, L"$$$$$ %d %s\n", i, uspto[i].pid );
		parse_WEBPAT3_inventor_first_names(tfield, &uspto[i].ninventor, uspto[i].inventor, naus, authors);
#ifdef DEBUG
		for (k = 0; k < uspto[i].ninventor; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i+1, uspto[i].ninventor, k, uspto[i].inventor[k], authors[uspto[i].inventor[k]].name); fflush(logstream); }
#endif DEBUG
		parse_WEBPAT3_line(line, i_py, tfield);
		parse_WEBPAT3_year(tfield, &uspto[i].year, &uspto[i].month);
		//fwprintf(logstream, L"%d: %d\n", i, uspto[i].year); fflush(logstream);
		parse_WEBPAT3_line(line, i_pm, tfield);
		parse_WEBPAT3_year(tfield, &uspto[i].a_year, &uspto[i].a_month);
		parse_WEBPAT3_line(line, i_an, tfield);
		parse_WEBPAT3_assignee_names(tfield, &uspto[i].nassignee, uspto[i].assignee, nasgns, assignees);
		i++;	
		if (gret == NULL)
			break;
	}
	fclose(sstream);	// close full record file

#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(logstream, L"%d\t%s\t%d\t%d\n", i, uspto[i].pid, uspto[i].year, uspto[i].nassignee);
		for (k = 0; k < uspto[i].nassignee; k++)
			fwprintf(logstream, L"\t%s", assignees[uspto[i].assignee[k]].name);
		fwprintf(logstream, L"\n"); fflush(logstream);
	}
#endif DEBUG

	wchar_t uiname[FNAME_SIZE];
	FILE *uistream;		// for output file
	int j, aj, ak;
	int UU, UI;

	swprintf_s(uiname, FNAME_SIZE, L"%s U-I relations.txt", tname);
	// open the output file
	if (_wfopen_s(&uistream, uiname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

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
					// write out relations when only one of them is an university
					if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 0)
					{
						UI = 1;
						fwprintf(uistream, L"%s\t%s\n", assignees[aj].name, assignees[ak].name);
						fwprintf(uistream2, L"U-I\t%s\t%s\t%s\n", uspto[i].pid, assignees[aj].name, assignees[ak].name);
					}
					else if (is_university(assignees[aj].name) == 0 && is_university(assignees[ak].name) == 1)
					{
						UI = 1;
						fwprintf(uistream, L"%s\t%s\n", assignees[ak].name, assignees[aj].name);
						fwprintf(uistream2, L"U-I\t%s\t%s\t%s\n", uspto[i].pid, assignees[ak].name, assignees[aj].name);
					}
					else if (is_university(assignees[aj].name) == 1 && is_university(assignees[ak].name) == 1)
					{
						UU = 1;
						fwprintf(uistream, L"%s\t%s\n", assignees[aj].name, assignees[ak].name);
						fwprintf(uistream2, L"U-U\t%s\t%s\t%s\n", uspto[i].pid, assignees[aj].name, assignees[ak].name);
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

	exit(0);
#endif UNIVERSITY_INDUSRY_PROJECT
// ===============================

	// obtain and process article title information
	// allocate memory for the title keyword array
	struct PTITLES *ptitle;	// paper titles
	ptitle = (struct PTITLES *)Jmalloc(nuspto * sizeof(struct PTITLES), L"read_WEBPAT3: ptitle");
	if (ptitle == NULL) return MSG_NOT_ENOUGH_MEMORY;
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;	// read source file line by line
	int ttmp;
	i = 0; 
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_ti, tfield);
		wcscpy_s(ptitle[i].name, MAX_TITLES, tfield);
		parse_WEBPAT3_line(line, i_py, tfield);
		parse_WEBPAT3_year(tfield, &ptitle[i].year, &ttmp);
		//fwprintf(logstream, L"[%d %d %s]\n", i+1, ptitle[i].year, ptitle[i].name); fflush(logstream);
		i++;	
		if (gret == NULL)
			break;
	}

	// assume in average each title contains 200 words (including single, double, triple, and quadruple word phrases)
	tkeyword = (struct TKEYWORDS *)Jmalloc(nuspto * 200 * sizeof(struct TKEYWORDS), L"read_WEBPAT3: tkeyword");
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
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;
	// read source file line by line
	i = 0;
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_ut, tfield);
		correct_WEBPAT3_pid(uspto[i].pid, tfield);	// write to .pid after correction
		parse_WEBPAT3_line(line, i_au, tfield);
		if (tfield[0] == '\0')	// empty author name, assign an artificial name to it
			swprintf(tfield, LBUF_SIZE, L"Anonymous %d", i);
		//fwprintf(logstream, L"$$$$$ %d %s\n", i, uspto[i].pid ); fflush(logstream);
		parse_WEBPAT3_inventor_first_names(tfield, &uspto[i].ninventor, uspto[i].inventor, naus, authors);
#ifdef DEBUG
		for (k = 0; k < uspto[i].ninventor; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i+1, uspto[i].ninventor, k, uspto[i].inventor[k], authors[uspto[i].inventor[k]].name); fflush(logstream); }
#endif DEBUG
		parse_WEBPAT3_line(line, i_py, tfield);
		parse_WEBPAT3_year(tfield, &uspto[i].year, &uspto[i].month);
		//fwprintf(logstream, L"%d: %d\n", i, uspto[i].year); fflush(logstream);
		parse_WEBPAT3_line(line, i_pm, tfield);
		parse_WEBPAT3_year(tfield, &uspto[i].a_year, &uspto[i].a_month);
		uspto[i].tc = 0;
		parse_WEBPAT3_line(line, i_cr, tfield);
		parse_WEBPAT3_Citing_Patents(tfield, uspto[i].pid, cstream);
		parse_WEBPAT3_line(line, i_ipc, tfield);
		parse_WEBPAT3_CPC_Current(tfield, &uspto[i].nipc, uspto[i].ipc, uspto[i].ipc_cnt);
		parse_WEBPAT3_line(line, i_an, tfield);
		parse_WEBPAT3_assignee_names(tfield, &uspto[i].nassignee, uspto[i].assignee, nasgns, assignees);
		parse_WEBPAT3_line(line, i_ti, tfield);		
		parse_title(tfield, 0, &uspto[i].ntkws, uspto[i].tkws, ntkeywords, tkeyword);	
		parse_WEBPAT3_line(line, i_ab, tfield);		// parse abstract
		parse_title(tfield, uspto[i].ntkws, &uspto[i].ntkws, uspto[i].tkws, ntkeywords, tkeyword);	// continue to add abstract keywords to the wos[].tkws[] array
		i++;	
		if (gret == NULL)
			break;
	}
		
	// following block that process country data is added 2017/03/05
	// parse country data, do it seperately because it needs complete assignee data
	rewind(sstream);	// point back to the begining of the file
	fgetws(line, XXLBUF_SIZE, sstream);	// skip the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;
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
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WEBPAT3_line(line, i_nt, tfield);
		//fwprintf(logstream, L"##### %d %s [%s]\n", i, uspto[i].pid, tfield); fflush(logstream);
		parse_countries_WEBPAT3(i, tfield, nasgns, assignees);
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
		wos[i].country[0] = uspto[i].country[0]; wos[i].country[1] = uspto[i].country[1]; wos[i].country[2] = '\0';	
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
	link_assignee_WOS();

	// following co-assignee codes are added 2016/12/24
	// count the number of times each assignee as the 1st assignee
	for (i = 0; i < nasgns; i++) assignees[i].cnt1 = 0;	// initialize 1st assignee count
	for (i = 0; i < nwos; i++) assignees[wos[i].DE[0]].cnt1++;	
	ret = coassignee(sname, 1); if (ret != 0) return ret;	// output coassignee network that contains only assignees that have been the 1st assignees
	ret = coassignee(sname, 0); if (ret != 0) return ret;	// output coassignee network that contains all assignees
	
	ret = keyword_year_matrix(ntkeywords, tkeyword);
	if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;	// added 2018/05/02

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
	relink_assignee_WOS();	// this is necessary after the wos[] arrary is sorted

	Jfree(line, L"read_WEBPAT3: line"); 
	Jfree(tfield, L"read_WEBPAT3: tfield");
	done_wfgetws();

	return 0;
}


//
// parse the field name line of the WebPat file
//
int parse_WEBPAT3_1st_line(wchar_t *line, int separator, wchar_t *tfname)
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
int parse_WEBPAT3_line(wchar_t *line, int tind, wchar_t *tfield)
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
// parse the "國內引證" field string, citations are delimeted by "," or ";" (as defined in the variable WEBPAT3_delimiter)
//
int parse_WEBPAT3_Citing_Patents(wchar_t *str, wchar_t *pid, FILE *stream)
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
			if (ch == WEBPAT3_delimiter)
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
					if (bfr[0] == 'U' && bfr[1] == 'S') // if the patent ID begins with 'US"
						fwprintf(stream, L"%s %s\n", &bfr[2], pid);	// write the results
					else
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
// parse the "CPC" field string, each is delimited by a ',' or ';'
//
int parse_WEBPAT3_CPC_Current(wchar_t *str, int *nipc, wchar_t *ipc_code, int *ipc_code_cnt)
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
			if (ch == WEBPAT3_delimiter_CPC)
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
// parse the "發明人" string
// and then save them to the global author name array
//
int parse_store_WEBPAT3_inventor_first(wchar_t *astr, int *nau, struct AUTHORS *au)
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
		if (ch == WEBPAT3_delimiter_inventor) // author names are separated by a ',' or ';'
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
				if (nau_this_article >= MAX_INVENTORS)	// take only MAX_AUTHORS authors, modified 2018/08/09
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
// parse a string of "發明人"
// save the results into the wos[] array
//
int parse_WEBPAT3_inventor_first_names(wchar_t *astr, int *nau, int *au, int nathrs, struct AUTHORS *athrs)
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
		if (ch == WEBPAT3_delimiter_inventor) 
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
int parse_WEBPAT3_assignee_names(wchar_t *istr, int *nasgn, int *asgn, int nasgns, struct ASSIGNEES *asgns)
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
		if (ch == WEBPAT3_delimiter_inventor)	// assignee names are seperated by a ',' or ';'
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
int parse_WEBPAT3_store_assignees(wchar_t *astr, int *nasgns, struct ASSIGNEES *asgn)
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
		if (ch == WEBPAT3_delimiter_inventor)		// the delimiter is ',' or ';'
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
// WEBPAT3 date format 
// date_format 0: not yet determined (by the program), 1: 2017/12/30, 2: 12/30/2017
// this function is largely modified 2017/06/20
//
int parse_WEBPAT3_year(wchar_t *tfield, int *year, int *month)
{
	int i, tyear;
	wchar_t *sp, *tp;
	wchar_t tmps[BUF_SIZE];

	if (*tfield == '\0')
	{
		*year = 1900;
		*month = 1;
		return 0;
	}

	if (date_format == 0) // not yet determined
	{
		sp = tfield; tp = tmps;
		for (i = 0; i < 4; i++)
			*tp++ = *sp++;
		*tp = '\0';
		tyear = _wtoi(tmps);
		if (tyear < 1900)
		{
			date_format = 2;
			goto dformat2;
		}
		else 
		{
			date_format = 1;
			goto dformat1;
		}
	}
	else if (date_format == 1) // standard format ==> 2017/12/30
	{
dformat1:
		sp = tfield;
		tp = tmps;
		for (i = 0; i < 4; i++)
			*tp++ = *sp++;
		*tp = '\0';
		*year = _wtoi(tmps);
		sp++; tp = tmps;
		for (i = 0; i < 2; i++)
			*tp++ = *sp++;
		*tp = '\0';
		*month = _wtoi(tmps);
	}
	else	// format after 2017/06/19 ==> 12/30/2017
	{
dformat2:
		sp = tfield;
		tp = tmps;
		while (*sp != '/') *tp++ = *sp++;
		*tp = '\0';
		*month = _wtoi(tmps);
		sp++;
		while (*sp != '/') *tp++ = *sp++;
		sp++;
		tp = tmps;
		while (*sp != '\0') *tp++ = *sp++;
		*tp = '\0';
		*year = _wtoi(tmps);
	}

	return 0;
}

//
// correct pid in WEBPAT data and then write to the given address
// WEBPAT data has a leading '0'
// added to check also leading "US" and the ending "B2", 2017/04/21, and 'B1', 2017/10/29
// 2018/02/10, changed to not removing the leading '0'
// 2018/06/27, added code to allow published applications, format: US20170297111A1
//
int correct_WEBPAT3_pid(wchar_t *pid, wchar_t *original)
{
	wchar_t *sp, *tp;
	wchar_t *coriginal;

	sp = original;
	while (*sp != '\0') sp++;
	sp -= 2;
	if (sp[0] == 'B' && (sp[1] == '1' || sp[1] == '2')) // remove the ending 'B2' and 'B1', fixed 2017/10/29
		*sp = '\0';
	else 
		if (sp[0] == 'A' && sp[1] == '1') // remove the ending 'A1', added 2018/06/27
			*sp = '\0';
	if (original[0] == 'U' && original[1] == 'S') // if the patent ID begins with 'US"
		coriginal = original + 2; // skip "US"
	else 
		coriginal = original;

	if (coriginal[0] == 'R' && coriginal[1] == 'E')
	{
		sp = coriginal + 2; tp = pid;
		tp[0] = 'R'; tp[1] = 'E'; tp += 2;
		//while (*sp == '0') sp++;	// removed 2018/02/10
		while (*sp != '\0') *tp++ = *sp++;
	}
	else if (coriginal[0] == 'H')
	{
		sp = coriginal + 1; tp = pid;
		tp[0] = 'H'; tp += 1;
		//while (*sp == '0') sp++;	// removed 2018/02/10
		while (*sp != '\0') *tp++ = *sp++;
	}
	else
		wcscpy(pid, coriginal);

	return 0;
}

//
// check if the given assignee name is an university
//
int is_university(wchar_t *name)
{
	//fwprintf(logstream, L"***** [%s]\n", name); fflush(logstream);
	if (index_of(0, name, L"university") != -1 ||  
		index_of(0, name, L"univ") != -1 || 
		index_of(0, name, L"college") != -1 || 
		index_of(0, name, L"polytech") != -1 ||
		index_of(0, name, L"polytechnic") != -1 || 
		index_of(0, name, L"polytechnique") != -1 || 
		index_of(0, name, L"institute of technology") != -1 || 
		index_of(0, name, L"institut fur technologie") != -1 ||
		index_of(0, name, L"georgia tech") != -1 ||
		index_of(0, name, L"universiteit") != -1 ||	// Dutch
		index_of(0, name, L"universite") != -1 ||	// French
		index_of(0, name, L"universitet") != -1 ||	// Danish
		index_of(0, name, L"universidad") != -1 ||	// Spanish
		index_of(0, name, L"universidade") != -1 ||	// Portugese
		index_of(0, name, L"universita") != -1 ||	// Italian
		index_of(0, name, L"universitat") != -1)	// German
		return 1;
	else
		return 0;
}

