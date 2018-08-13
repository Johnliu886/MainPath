// 
// modality.cpp
//
// Author: John Liu
// 
// this file contains functions that are related to the MOST project "The modality of humanity and social science research in Taiwan"
//
//

// Revision History:
// 2017/06/14 basic function works
// 2017/07/04 first release of the codes (MainPath release 373M)
// 2017/07/10 added codes to read "英文期刊表.txt" and "中文期刊表.txt"
// 2017/07/10 added several new indices on scholar performance
// 2017/07/26 added codes to use percentage rank rather than absolute rank
// 2017/08/15 extened the MAX_FIELD_NAME from 120 to 200
// 2017/08/21 correct an error, which redefines "nwos"
// 2017/10/10 change the formula of calculating publishing frequency (依據陳聖杰建議)
//            added position (in numbers) and sex columns in the "CombinedSAAdata.csv" file
//            removed ',' and ';' in the position data
// 2017/11/26 fixed the problem of not reading 'outstanding' correctly
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "resource.h"
#include "network.h"
#include "modality.h"

int nscholars;
struct SCHOLAR *scholars;

// following definitions are for the parallel data (means the TCI data)
static int nnodes_p;
static struct PN *nw_p;
static int nwos_p;
static struct WOS *wos_p;
static int naus_p;
static struct AUTHORS *authors_p;
static int nkwde_p;
static struct KWORDS *kwde_p;	// author keywords
static int nkwid_p;
static struct KWORDS *kwid_p;	// Keyword Plus keywords
static int ntkeywords_p;
static struct TKEYWORDS *tkeyword_p;	// title keywords
static int njrs_p;
static struct JOURNALS *journals_p;	// author name array

static int nsn_Chinese;
static struct SAMENAME_CHINESE *sname_Chinese;
static int nsn_English;
static struct SAMENAME_ENGLISH *sname_English;

#define MAX_FIELD_NAME 200	// extended from 120 to 200, 2017/08/15
struct CJOURNAL {
	wchar_t jname[MAX_JOURNAL_NAME];
	int tssci;
	wchar_t field[MAX_FIELD_NAME];
};
struct EJOURNAL {
	wchar_t jname[MAX_JOURNAL_NAME];
	double impactf;
	int rank;
	double rank_percentage;
	wchar_t field[MAX_FIELD_NAME];
	int outstanding;
};
static int nCjournals;
struct CJOURNAL *Cjournal;
static int nEjournals;
struct EJOURNAL *Ejournal;

extern FILE *logstream;

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
extern int njrs;
extern struct JOURNALS *journals;	// author name array
extern int text_type;

int parse_scholar_data(wchar_t *, struct SCHOLAR *);
int parse_Cjournal_data(wchar_t *, struct CJOURNAL *);
int parse_Ejournal_data(wchar_t *, struct EJOURNAL *);
int establish_scholar_pauthor_link();
int scholar_ename_search(struct SCHOLAR *, int, wchar_t *);
int compare_scholar_ename(const void *, const void *);
int scholar_cname_search(struct SCHOLAR *, int, wchar_t *);
int compare_scholar_cname(const void *, const void *);
int compare_scholar_ndx(const void *, const void *);
int samename_Chinese_search(struct SAMENAME_CHINESE *, int, wchar_t *);
int samename_English_search(struct SAMENAME_ENGLISH *, int, wchar_t *);
int remove_spaces_at_the_end(wchar_t *);
int find_average_author_count();
int read_Cjournal_list(wchar_t *);
int read_Ejournal_list(wchar_t *);
int compare_Cjournal_title(const void *, const void *);
int compare_Ejournal_title(const void *, const void *);
int Cjournal_search(struct CJOURNAL *, int, wchar_t *);
int Ejournal_search(struct EJOURNAL *, int, wchar_t *);

extern int read_TCI(wchar_t *, int);
extern int compare_author(const void *, const void *);
extern int assemble_multiple_TCI_data(wchar_t *, wchar_t *);
extern int check_file_type(wchar_t *, int *);

//
// list the authors (in the parallel data set, i.e. TCI) that are on the TScholar 中文 same name list
//
int list_same_name_authors_parallel_data(wchar_t *dpath)
{
	int i, k, m, ndx;
	wchar_t sex[5];
	wchar_t oname[FNAME_SIZE];
	int cnt;
	FILE *ostream;

	swprintf(oname, L"%sPAuthors-中文同名同姓.txt", dpath);
	// Open the output file (will fail if the file does not exist)
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(ostream, L"序號\t中文姓名\t英文姓名)\t英文名(FIRST NAME)\t性別\t服務機關／就讀學校1(中文)\t職稱1(中文)\t最高學歷學位名稱\t最高學歷訖年月\t學門\t領域\n");
	// sort the authors (in the parallel) data by their Chinese names
	qsort((void *)authors_p, (size_t)naus_p, sizeof(struct AUTHORS), compare_author);
	// find authors that are on the Chinese same name list
	cnt = 0;
	for (m = 0; m < naus_p; m++)
	{
		ndx = samename_Chinese_search(sname_Chinese, nsn_Chinese, authors_p[m].name);
		if (ndx >= 0)	// in the same name list
		{
			for (k = 0; k < sname_Chinese[ndx].nsame; k++)
			{
				i = sname_Chinese[ndx].peers[k];
				if (scholars[i].sex == SEX_MALE)
					wcscpy(sex, L"男");
				else if (scholars[i].sex == SEX_FEMALE)
					wcscpy(sex, L"女");
				fwprintf(ostream, L"%d\t%s\t%s\t%s\t%s\t%s\t%s\t\t%d\t%s\t%s\n", cnt+1, scholars[i].cname, scholars[i].ename, scholars[i].e1stname, sex,	
					scholars[i].orgname, scholars[i].position, scholars[i].phdyear, scholars[i].afield, scholars[i].afield2);
			}
			cnt++;
		}
	}
	fwprintf(logstream, L"Number of authors (in the parallel data) that are in the same Chinese name list = %d\n", cnt);

	fclose(ostream);

	return 0;
}

//
// list the authors (in the primary data set, i.e. WOS) that are on the TScholar 英文 same name list
//
int list_same_name_authors_primary_data(wchar_t *dpath)
{
	int i, k, m, ndx;
	wchar_t sex[5];
	wchar_t oname[FNAME_SIZE];
	int cnt;
	FILE *ostream;

	swprintf(oname, L"%sPAuthors-英文同名同姓.txt", dpath);
	// Open the output file (will fail if the file does not exist)
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(ostream, L"序號\t中文姓名\t英文姓名)\t英文名(FIRST NAME)\t性別\t服務機關／就讀學校1(中文)\t職稱1(中文)\t最高學歷學位名稱\t最高學歷訖年月\t學門\t領域\n");
	// sort the authors (in the parallel) data by their Chinese names
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_author);
	// find authors that are on the English same name list
	cnt = 0;
	for (m = 0; m < naus; m++)
	{
		ndx = samename_English_search(sname_English, nsn_English, authors[m].name);
		if (ndx >= 0)	// in the same name list
		{
			for (k = 0; k < sname_English[ndx].nsame; k++)
			{
				i = sname_English[ndx].peers[k];
				if (scholars[i].sex == SEX_MALE)
					wcscpy(sex, L"男");
				else if (scholars[i].sex == SEX_FEMALE)
					wcscpy(sex, L"女");
				fwprintf(ostream, L"%d\t%s\t%s\t%s\t%s\t%s\t%s\t\t%d\t%s\t%s\n", cnt+1, scholars[i].cname, scholars[i].ename, scholars[i].e1stname, sex,	
					scholars[i].orgname, scholars[i].position, scholars[i].phdyear, scholars[i].afield, scholars[i].afield2);
			}
			cnt++;
		}
	}
	fwprintf(logstream, L"Number of authors (in the primary data) that are in the same English name list = %d\n", cnt);

	fclose(ostream);

	return 0;
}

//
// list the Taiwan scholars with the same name
// the list is consolidated from the scholar list from MOST 
//
int list_same_name_TScholars(wchar_t *dpath)
{
	int i, k, m;
	wchar_t sex[5];
	wchar_t oname[FNAME_SIZE];
	wchar_t modified_name[MAX_AUTHOR_NAME];
	wchar_t prev_name[MAX_AUTHOR_NAME];
	int nsnames;
	int cnt;
	FILE *ostream;

//
// 中文同名同姓
//
	// sort the scholar data by their Chinese names
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_cname);
	// find scholars with the same last/1st Chinese names
	// 1st pass, find the number of "names"
	prev_name[0] = '\0';
	cnt = 1; nsnames = -1;
	for (i = 0; i < nscholars; i++)
	{
		if (wcscmp(scholars[i].cname, prev_name) != 0)	// hit a new name
		{
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, scholars[i].cname); 
			cnt = 1;
		}
		else
		{
			if (cnt == 1)	// if new
				nsnames++;
			cnt++;
		}
	}
	nsnames++;
	fwprintf(logstream, L"Number of the same Chinese names = %d\n\n", nsnames);
	// allocate memory for the same Chinese name array
	sname_Chinese = (struct SAMENAME_CHINESE *)Jmalloc(nsnames * sizeof(struct SAMENAME_CHINESE), L"list_same_name_scholars: sname_Chinese");
	if (sname_Chinese == NULL) return MSG_NOT_ENOUGH_MEMORY;

	for (m = 0; m < nsnames; m++) sname_Chinese[m].nsame = 0;
	// 2nd pass, establish the same Chinese name table
	prev_name[0] = '\0';
	cnt = 1; nsnames = -1;
	for (i = 0; i < nscholars; i++)
	{
		if (wcscmp(scholars[i].cname, prev_name) != 0)	// hit a new name
		{
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, scholars[i].cname); 
			cnt = 1;
		}
		else
		{
			if (cnt == 1)	// if new
			{
				nsnames++;
				wcscpy(sname_Chinese[nsnames].cname, scholars[i-1].cname);
				sname_Chinese[nsnames].peers[sname_Chinese[nsnames].nsame] = scholars[i-1].ndx;
				sname_Chinese[nsnames].nsame++;
			}
			sname_Chinese[nsnames].peers[sname_Chinese[nsnames].nsame] = scholars[i].ndx;
			sname_Chinese[nsnames].nsame++;
			cnt++;
		}
	}
	nsnames++;
	nsn_Chinese = nsnames;
	// sort the scholar data back to their original order
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_ndx);
	for (m = 0; m < nsn_Chinese; m++)
	{
		for (k = 0; k < sname_Chinese[m].nsame; k++)
		{
			i = sname_Chinese[m].peers[k];
			swprintf(modified_name, L"%s%d", scholars[i].cname, k+1);
			wcscpy(scholars[i].cname, modified_name); // modify the name of the same-name scholar (by appending a number)
		}
	}

//
// 英文同名同姓
//
	// sort the scholar data by their Chinese names
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_ename);
	// find scholars with the same last/1st English names
	// 1st pass, find the number of "names"
	prev_name[0] = '\0';
	cnt = 1; nsnames = -1;
	for (i = 0; i < nscholars; i++)
	{
		if (wcscmp(scholars[i].ename, prev_name) != 0)	// hit a new name
		{
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, scholars[i].ename); 
			cnt = 1;
		}
		else
		{
			if (cnt == 1)	// if new
				nsnames++;
			cnt++;
		}
	}
	nsnames++;
	fwprintf(logstream, L"Number of the same English names = %d\n\n", nsnames);
	// allocate memory for the same English name array
	sname_English = (struct SAMENAME_ENGLISH *)Jmalloc(nsnames * sizeof(struct SAMENAME_ENGLISH), L"list_same_name_scholars: sname_English");
	if (sname_English == NULL) return MSG_NOT_ENOUGH_MEMORY;

	for (m = 0; m < nsnames; m++) sname_English[m].nsame = 0;
	// 2nd pass, establish the same English name table
	prev_name[0] = '\0';
	cnt = 1; nsnames = -1;
	for (i = 0; i < nscholars; i++)
	{
		if (wcscmp(scholars[i].ename, prev_name) != 0)	// hit a new name
		{
			wcscpy_s(prev_name, MAX_AUTHOR_NAME, scholars[i].ename); 
			cnt = 1;
		}
		else
		{
			if (cnt == 1)	// if new
			{
				nsnames++;
				wcscpy(sname_English[nsnames].ename, scholars[i-1].ename);
				sname_English[nsnames].peers[sname_English[nsnames].nsame] = scholars[i-1].ndx;
				sname_English[nsnames].nsame++;
			}
			sname_English[nsnames].peers[sname_English[nsnames].nsame] = scholars[i].ndx;
			sname_English[nsnames].nsame++;
			cnt++;
		}
	}
	nsnames++;
	nsn_English = nsnames;
	// sort the scholar data back to their original order
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_ndx);
	for (m = 0; m < nsn_English; m++)
	{
		for (k = 0; k < sname_English[m].nsame; k++)
		{
			i = sname_English[m].peers[k];
			swprintf(modified_name, L"%s%d", scholars[i].ename, k+1);
			wcscpy(scholars[i].ename, modified_name); // also, modify the name of the same-name scholar (by appending a number)
		}
	}

//
// output the results
//
	swprintf(oname, L"%sTScholars-中文同名同姓.txt", dpath);
	// Open the output file (will fail if the file does not exist)
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(ostream, L"序號\t中文姓名\t英文姓名)\t英文名(FIRST NAME)\t性別\t服務機關／就讀學校1(中文)\t職稱1(中文)\t最高學歷學位名稱\t最高學歷訖年月\t學門\t領域\n");
	for (m = 0; m < nsn_Chinese; m++)
	{
		for (k = 0; k < sname_Chinese[m].nsame; k++)
		{
			i = sname_Chinese[m].peers[k];
			if (scholars[i].sex == SEX_MALE)
				wcscpy(sex, L"男");
			else if (scholars[i].sex == SEX_FEMALE)
				wcscpy(sex, L"女");
			fwprintf(ostream, L"%d\t%s\t%s\t%s\t%s\t%s\t%s\t\t%d\t%s\t%s\n", m+1, scholars[i].cname, scholars[i].ename, scholars[i].e1stname, sex,	
				scholars[i].orgname, scholars[i].position, scholars[i].phdyear, scholars[i].afield, scholars[i].afield2);
		}
	}
	fclose(ostream);

	swprintf(oname, L"%sTScholars-英文同名同姓.txt", dpath);
	// Open the output file (will fail if the file does not exist)
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(ostream, L"序號\t中文姓名\t英文姓名)\t英文名(FIRST NAME)\t性別\t服務機關／就讀學校1(中文)\t職稱1(中文)\t最高學歷學位名稱\t最高學歷訖年月\t學門\t領域\n");
	for (m = 0; m < nsn_English; m++)
	{
		for (k = 0; k < sname_English[m].nsame; k++)
		{
			i = sname_English[m].peers[k];
			if (scholars[i].sex == SEX_MALE)
				wcscpy(sex, L"男");
			else if (scholars[i].sex == SEX_FEMALE)
				wcscpy(sex, L"女");
			fwprintf(ostream, L"%d\t%s\t%s\t%s\t%s\t%s\t%s\t\t%d\t%s\t%s\n", m+1, scholars[i].cname, scholars[i].ename, scholars[i].e1stname, sex,	
				scholars[i].orgname, scholars[i].position, scholars[i].phdyear, scholars[i].afield, scholars[i].afield2);
		}
	}
	fclose(ostream);

	return 0;
}

//
// read the scholar file, which contains basic information for the scholars
//
int read_scholar_data(wchar_t *dpath)
{
	int i;
	FILE *sstream;
	wchar_t *line;
	wchar_t sname[FNAME_SIZE];

	// allocate the working memory
	line = (wchar_t *)malloc(SBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	swprintf(sname, L"%sTScholars.txt", dpath);
	// 
	// Open the source file (will fail if the file does not exist)
	//	
	if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)
		return MSG_WOSFILE_NOTFOUND;

	// 1st pass, obtain the number of scholars
	fgetws(line, SBUF_SIZE, sstream);	// ignore the 1st line
	nscholars = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nscholars++;	
	}

	scholars = (struct SCHOLAR *)Jmalloc(nscholars * sizeof(struct SCHOLAR), L"scholar_data: scholar");
	// read the data line by line
	rewind(sstream);
	fgetws(line, SBUF_SIZE, sstream);	// ignore the 1st line
	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_scholar_data(line, &scholars[i]);
		scholars[i].ndx = i;
		i++;
	}

#ifdef DEBUG
	for (i = 0; i < nscholars; i++)
		fwprintf(logstream, L"#####\t%s\t%s\t%d\t%s\t%d\t%d\n", 
			scholars[i].cname, scholars[i].ename, scholars[i].afield, scholars[i].orgname, scholars[i].phdyear, scholars[i].sex);
#endif DEBUG

	fclose(sstream);
	free(line);

	return 0;
}

//
// convert the given 1st name to WOS form, which use only initials
//
int convert_1stname(wchar_t *name)
{
	wchar_t *sp, *tp;
	wchar_t bfr[50];

	sp = name;
	tp = bfr;

	*tp++ = *sp++;
	while (*sp != '\0')
	{
		if (*sp == ' ' || *sp == '-' || *sp == ',' || *sp == '.')
		{
			sp++;	// skip the space or '-'
			*tp++ = *sp;
		}
		sp++;
	}
	*tp = '\0';
	wcscpy(name, bfr);

	return 0;
}

//
// parse lines in the scholar data file
//
int parse_scholar_data(wchar_t *str, struct SCHOLAR *scholars)
{
	wchar_t ch, *sp, *tp;
	wchar_t *tsp, *ttp;
	wchar_t tmps[SBUF_SIZE];
	wchar_t elname[MAX_AUTHOR_NAME];
	int state;
	int in_quote;

	state = 0; in_quote = 0;
	sp = str;  tp = tmps;
	while (*sp != '\0' && *sp != '\r' && *sp != '\n')
	{
		ch = *sp; 
		if (in_quote)
		{
			if (ch == '\"') 
				in_quote = 0;
			else
				*tp++ = towlower(*sp); 
			sp++;
			continue;
		}
		else if (ch == '\"')
		{
			sp++;
			in_quote = 1;
			continue;
		}
		if (ch == '\t')
		{ 
			*tp++ = '\0'; sp++; 
			switch (state)
			{
			case 0:	// 序號
				state++;
				break;
			case 1:	// 中文姓名
				remove_spaces_at_the_end(tmps);
				wcscpy(scholars->cname, tmps);	
				state++;
				break;
			case 2:	// 英文姓(LAST NAME)
				remove_spaces_at_the_end(tmps);
				wcscpy(elname, tmps);
				state++;
				break;
			case 3:	// 英文名(FIRST NAME)
				remove_spaces_at_the_end(tmps);
				wcscpy(scholars->e1stname, tmps);	
				if (elname[0] != '\0')
				{
					//convert_1stname(tmps); // this becomes unnecessary because we are now taking "AF" rather than "AU" columns from WOS for author names
					swprintf(scholars->ename, L"%s, %s", elname, tmps);	
				}
				else
					wcscpy(scholars->ename, scholars->cname);	// empty English last name, use Chinese name instead
				state++;
				break;
			case 4:	// 性別
				if (wcscmp(tmps, L"男") == 0)
					scholars->sex = SEX_MALE;
				else if (wcscmp(tmps, L"女") == 0)
					scholars->sex = SEX_FEMALE;
				state++;
				break;;
			case 5:	// 服務機關／就讀學校1(中文)
				wcscpy(scholars->orgname, tmps);	
				state++;
				break;
			case 6:	// 	職稱1(中文)
				tsp = tmps; ttp = tmps;
				while (*tsp != '\0')	// replace ',' and ';' with '/'
				{
					if (*tsp == ',' || *tsp == ';')
						*ttp++ = '/';
					else if (*tsp == ' ')
						*ttp++ = '/';
					else
						*ttp++ = *tsp;
					tsp++;
				}
				*ttp = '\0';
				wcscpy(scholars->position, tmps);	
				state++;
				break;
			case 7:	// 	最高學歷學位名稱
				state++;
				break;
			case 8:	// 	最高學歷訖年月
				scholars->phdyear = _wtoi(tmps);	
				state++;
				break;
			case 9:	// 學門
				wcscpy(scholars->afield, tmps);	
				state++;
				break;
			case 10:// 領域
				if (tmps[0] != '\0')
					wcscpy(scholars->afield2, tmps);
				else
					wcscpy(scholars->afield2, L"UNKNOWN");
				break;
			default:
				break;
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
	if (tmps[0] != '\0')
		wcscpy(scholars->afield2, tmps);
	else
		wcscpy(scholars->afield2, L"UNKNOWN");


	return 0;
}

//
// read parallel data (ex. 法律、語言、管理各學門資料)
//
int read_parallel_data(wchar_t *dpath)
{
	wchar_t pname[FNAME_SIZE];
	wchar_t dname[FNAME_SIZE];
	wchar_t buf[SBUF_SIZE];
	wchar_t *sp;
	FILE *pstream;
	int data_type;

	swprintf(pname, L"%sParallelData.txt", dpath);
	fwprintf(logstream, L"Parallel data indicator file: %s\n", pname);
	if (_wfopen_s(&pstream, pname, L"rt, ccs=UTF-8") != 0)
	{
		fwprintf(logstream, L"WARNING: Cannot open parallel data indicator file: %s\n", pname);
		return -1;
	}
	fwprintf(logstream, L"Parallel data indicator file opened successfully\n");

	fgetws(buf, SBUF_SIZE, pstream);
	sp = buf;
	while (*sp != '\n') sp++; *sp = '\0';	// remove the line feed at the end of line
	fclose(pstream);

	// open parallel data
	// save proper information
	int nnodes_s = nnodes;
	struct PN *nw_s = nw;
	int nwos_s = nwos;
	struct WOS *wos_s = wos;
	int naus_s = naus;
	struct AUTHORS *authors_s = authors;
	int nkwde_s = nkwde;
	struct KWORDS *kwde_s = kwde;	// author keywords
	int nkwid_s = nkwid;
	struct KWORDS *kwid_s = kwid;	// Keyword Plus keywords
	int ntkeywords_s = ntkeywords;
	struct TKEYWORDS *tkeyword_s = tkeyword;	// title keywords
	int njrs_s = njrs;
	struct JOURNALS *journals_s = journals;	// author name array

	swprintf(dname, L"%s%s", dpath, buf);
	// check data type of the parallel data
	data_type = WOS_or_Others(dname);
	if (data_type == TCI_MULTIPLE_DATA)
	{
		wchar_t assembled_fname[FNAME_SIZE];
		fwprintf(logstream, L"TCI data file as indicated by \"ParallelDATA.txt\": %s\n", dname);
		if (assemble_multiple_TCI_data(dname, assembled_fname) != 0)
			return -1;
		// replace the frecordname, as if user had entered the assembled file prepared by the software
		//wcscpy(frecordname, assembled_fname);
		check_file_type(assembled_fname, &text_type);	// because the original frecordname and this one may have different file type
		read_TCI(assembled_fname, 1);	// 1 indicate "secondary_data"
	}
	else if (data_type == TCI_DATA)
		read_TCI(dname, 1);

	// assign the new set of data and pointers to "_p" signifiying parallel data
	nnodes_p = nnodes;
	nw_p = nw;
	nwos_p = nwos;
	wos_p = wos;
	naus_p = naus;
	authors_p = authors;
	nkwde_p = nkwde;
	kwde_p = kwde;	// author keywords
	nkwid_p = nkwid;
	kwid_p = kwid;	// Keyword Plus keywords
	ntkeywords_p = ntkeywords;
	tkeyword_p = tkeyword;	// title keywords
	njrs_p = njrs;
	journals_p = journals;	
	establish_scholar_pauthor_link();	// this establish the link between the scholars[] array and authors[] array for the parallel data

	// restore information
	nnodes = nnodes_s;
	nw = nw_s;
	nwos = nwos_s;
	wos = wos_s;
	naus = naus_s;
	authors = authors_s;
	nkwde = nkwde_s;
	kwde = kwde_s;	// author keywords
	nkwid = nkwid_s;
	kwid = kwid_s;	// Keyword Plus keywords
	ntkeywords = ntkeywords_s;
	tkeyword = tkeyword_s;	// title keywords
	njrs = njrs_s;
	journals = journals_s;	

	return 0;
}

//
// match names between the authors[] and scholars[] array
//
int establish_scholar_author_link()
{
	int i, k;
	int ndx;

	for (i = 0; i < nscholars; i++) scholars[i].andx = -1;	// assume no corresponding authors in the WOS data

	// sort the scholar data by their english names
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_ename);

	for (i = 0; i < naus; i++)
	{
		ndx = scholar_ename_search(scholars, nscholars, authors[i].name);
		if (ndx != -1)
		{
			scholars[ndx].andx = i;
			authors[i].sndx = scholars[ndx].ndx;	// point to the original order
			//fwprintf(logstream, L"##### %d %d [%s]\n", i, ndx, scholars[ndx].ename); fflush(logstream);
		}
		else
			authors[i].sndx = -1;
	}

	// sort the scholar data back to their original order
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_ndx);

	return 0;
}

//
// match names between the authors_p[] and scholars[] array
// the only difference of this function with establish_scholar_pauthor_link() is that it assignes the value pndx rather than andx
//
int establish_scholar_pauthor_link()
{
	int i, k;
	int ndx;

	for (i = 0; i < nscholars; i++) scholars[i].pndx = -1;	// assume no corresponding authors in the parallel data

	// sort the scholar data by their Chinese names
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_cname);
	for (i = 0; i < naus_p; i++)
	{
		//fwprintf(logstream, L"##### %d [%s]\n", i, authors_p[i].name); fflush(logstream);
		ndx = scholar_cname_search(scholars, nscholars, authors_p[i].name);
		if (ndx != -1)
		{
			scholars[ndx].pndx = i;
			authors_p[i].sndx = scholars[ndx].ndx;	// point to the original order;
			//fwprintf(logstream, L"@@@@@ %d %d [%s]\n", i, ndx, scholars[ndx].cname); fflush(logstream);
		}
		else
			authors_p[i].sndx = -1;
	}

	// sort the scholar data back to their original order
	qsort((void *)scholars, (size_t)nscholars, sizeof(struct SCHOLAR), compare_scholar_ndx);

	return 0;
}

//
// calculate publishing frequency
// updated 2017/1010
//
#define DATA_END_YEAR 2017
int scholar_publishing_frequency()
{
	int i, k, m, p;
	int nps, byear, eyear;

	for (i = 0; i < nscholars; i++)
	{
		m = scholars[i].andx;	// index to the main data (WOS)
		p = scholars[i].pndx;	// index to the parallel data (TCI)
		byear = 9999; eyear = 0;
		if (m >= 0 && p >= 0)
		{
			nps = authors_p[p].np + authors[m].np;
			for (k = 0; k < authors[m].np; k++)	// check the publishing years of the English papaers
			{
				if (wos[authors[m].paper[k]].year > eyear)
					eyear = wos[authors[m].paper[k]].year;
				if (wos[authors[m].paper[k]].year < byear)
					byear = wos[authors[m].paper[k]].year;
			}
			for (k = 0; k < authors_p[p].np; k++) // take into consideration also the publishing years of the Chinese papaers
			{
				if (wos_p[authors_p[p].paper[k]].year > eyear)
					eyear = wos_p[authors_p[p].paper[k]].year;
				if (wos_p[authors_p[p].paper[k]].year < byear)
					byear = wos_p[authors_p[p].paper[k]].year;
			}
			scholars[i].byear = byear;
			scholars[i].eyear = eyear;
			//fwprintf(logstream, L"##### %d %s %d %d %d\n", i, scholars[i].cname, nps, eyear, byear); fflush(logstream);
			scholars[i].pfreq = (double)nps / (DATA_END_YEAR - byear + 1);
		}
		else if (m > 0 && p < 0)
		{
			nps = authors[m].np;
			for (k = 0; k < authors[m].np; k++)
			{
				if (wos[authors[m].paper[k]].year > eyear)
					eyear = wos[authors[m].paper[k]].year;
				if (wos[authors[m].paper[k]].year < byear)
					byear = wos[authors[m].paper[k]].year;
			}
			scholars[i].byear = byear;
			scholars[i].eyear = eyear;
			//fwprintf(logstream, L"##### %d %s %d %d %d\n", i, scholars[i].cname, nps, eyear, byear); fflush(logstream);
			scholars[i].pfreq = (double)nps / (DATA_END_YEAR - byear + 1);
		}
		else if (m < 0 && p > 0)
		{
			nps = authors_p[p].np;
			for (k = 0; k < authors_p[p].np; k++)
			{
				if (wos_p[authors_p[p].paper[k]].year > eyear)
					eyear = wos_p[authors_p[p].paper[k]].year;
				if (wos_p[authors_p[p].paper[k]].year < byear)
					byear = wos_p[authors_p[p].paper[k]].year;
			}
			scholars[i].byear = byear;
			scholars[i].eyear = eyear;
			//fwprintf(logstream, L"##### %d %s %d %d %d\n", i, scholars[i].cname, nps, eyear, byear); fflush(logstream);
			scholars[i].pfreq = (double)nps / (DATA_END_YEAR - byear + 1);
		}
		else
			scholars[i].pfreq = 0;	// means no publications at all
	}

	return 0;
}

#ifdef OBSOLETE
//
// calculate publishing frequency
//
int scholar_publishing_frequency()
{
	int i, k, m, p;
	int nps, byear, eyear;

	for (i = 0; i < nscholars; i++)
	{
		m = scholars[i].andx;	// index to the main data (WOS)
		p = scholars[i].pndx;	// index to the parallel data (TCI)
		byear = 9999; eyear = 0;
		if (m >= 0 && p >= 0)
		{
			nps = authors_p[p].np + authors[m].np;
			if (nps == 1) // only one paper
				scholars[i].pfreq = -1;	// means not applicable
			else if (nps > 1)
			{
				for (k = 0; k < authors[m].np; k++)	// check the publishing years of the English papaers
				{
					if (wos[authors[m].paper[k]].year > eyear)
						eyear = wos[authors[m].paper[k]].year;
					if (wos[authors[m].paper[k]].year < byear)
						byear = wos[authors[m].paper[k]].year;
				}
				for (k = 0; k < authors_p[p].np; k++) // take into consideration also the publishing years of the Chinese papaers
				{
					if (wos_p[authors_p[p].paper[k]].year > eyear)
						eyear = wos_p[authors_p[p].paper[k]].year;
					if (wos_p[authors_p[p].paper[k]].year < byear)
						byear = wos_p[authors_p[p].paper[k]].year;
				}
				scholars[i].byear = byear;
				scholars[i].eyear = eyear;
				//fwprintf(logstream, L"##### %d %s %d %d %d\n", i, scholars[i].cname, nps, eyear, byear); fflush(logstream);
				scholars[i].pfreq = (double)nps / (eyear - byear + 1);
			}
		}
		else if (m > 0 && p < 0)
		{
			nps = authors[m].np;
			if (nps == 1) // only one paper
				scholars[i].pfreq = -1;	// means not applicable
			else if (nps > 1)
			{
				for (k = 0; k < authors[m].np; k++)
				{
					if (wos[authors[m].paper[k]].year > eyear)
						eyear = wos[authors[m].paper[k]].year;
					if (wos[authors[m].paper[k]].year < byear)
						byear = wos[authors[m].paper[k]].year;
				}
				scholars[i].byear = byear;
				scholars[i].eyear = eyear;
				//fwprintf(logstream, L"##### %d %s %d %d %d\n", i, scholars[i].cname, nps, eyear, byear); fflush(logstream);
				scholars[i].pfreq = (double)nps / (eyear - byear + 1);
			}
		}
		else if (m < 0 && p > 0)
		{
			nps = authors_p[p].np;
			if (nps == 1) // only one paper
				scholars[i].pfreq = -1;	// means not applicable
			else if (nps > 1)
			{
				for (k = 0; k < authors_p[p].np; k++)
				{
					if (wos_p[authors_p[p].paper[k]].year > eyear)
						eyear = wos_p[authors_p[p].paper[k]].year;
					if (wos_p[authors_p[p].paper[k]].year < byear)
						byear = wos_p[authors_p[p].paper[k]].year;
				}
				scholars[i].byear = byear;
				scholars[i].eyear = eyear;
				//fwprintf(logstream, L"##### %d %s %d %d %d\n", i, scholars[i].cname, nps, eyear, byear); fflush(logstream);
				scholars[i].pfreq = (double)nps / (eyear - byear + 1);
			}
		}
		else
			scholars[i].pfreq = 0;	// means no publications at all
	}

	return 0;
}
#endif OBSOLETE

//
// calculate citation counts of individual scholar
//
int scholar_citation_counts()
{
	int i, k, m, p;
	int Ccitations, Ecitations;

	for (i = 0; i < nscholars; i++)
	{
		m = scholars[i].andx;	// index to the main data (WOS)
		p = scholars[i].pndx;	// index to the parallel data (TCI)
		Ccitations = Ecitations = 0;
		if (m >= 0 && p >= 0)
		{
			for (k = 0; k < authors[m].np; k++)	// check the publishing years of the English papaers
				Ecitations += wos[authors[m].paper[k]].tc;
			for (k = 0; k < authors_p[p].np; k++) // take into consideration also the publishing years of the Chinese papaers
				Ccitations += wos_p[authors_p[p].paper[k]].tc;
		}
		else if (m > 0 && p < 0)
		{
			for (k = 0; k < authors[m].np; k++)	// check the publishing years of the English papaers
				Ecitations += wos[authors[m].paper[k]].tc;
		}
		else if (m < 0 && p > 0)
		{
			for (k = 0; k < authors_p[p].np; k++) // take into consideration also the publishing years of the Chinese papaers
				Ccitations += wos_p[authors_p[p].paper[k]].tc;
		}
		scholars[i].tc = Ecitations + Ccitations;
	}

	return 0;
}

//
// calculate statistics related to journals that a scholar publishing his papers
//
int scholar_published_journal_stats()
{
	int i, k, m, p;
	int endx, cndx;
	int nepapers;
	double sum_rank;	// sum up the rank of the journal of all published papers (WOS journal only)
	int outstanding_cnt;	// number of times published in outstanding SCI/SSCI journal
	int TSSCI_cnt;			// number of times published in TSSCI journal

	for (i = 0; i < nscholars; i++)
	{
		//fwprintf(logstream, L"%s\t%s\n", scholars[i].cname, scholars[i].ename); fflush(logstream);
		m = scholars[i].andx;	// index to the main data (WOS)
		p = scholars[i].pndx;	// index to the parallel data (TCI)
		sum_rank = 0;
		outstanding_cnt = 0; nepapers = 0;
		TSSCI_cnt = 0;
		if (m >= 0 && p >= 0)
		{
			for (k = 0; k < authors[m].np; k++)	
			{
				endx = Ejournal_search(Ejournal, nEjournals, journals[wos[authors[m].paper[k]].journal].name);
				//fwprintf(logstream, L"@@@@@ %d [%s] %d\n", i, journals[wos[authors[m].paper[k]].journal].name, endx);
				if (endx != -1)
				{
					outstanding_cnt += Ejournal[endx].outstanding;
					if (Ejournal[endx].rank != 0)	// rank=0 means no rank data, NOTE: but rank_percentage begins with 0.0
					{
						//fwprintf(logstream, L"rank=%d\n", Ejournal[endx].rank);
						sum_rank += Ejournal[endx].rank_percentage;
						nepapers++;
					}
				}
			}
			for (k = 0; k < authors_p[p].np; k++) 
			{
				cndx = Cjournal_search(Cjournal, nCjournals, journals_p[wos_p[authors_p[p].paper[k]].journal].name);
				//fwprintf(logstream, L"@@@@@ %d [%s] %d\n", i, journals_p[wos_p[authors_p[p].paper[k]].journal].name, cndx);
				if (cndx != -1)
				{
					//fwprintf(logstream, L"TSSCI=%d\n", Cjournal[cndx].tssci);
					TSSCI_cnt += Cjournal[cndx].tssci;
				}
			}
		}
		else if (m > 0 && p < 0)
		{
			for (k = 0; k < authors[m].np; k++)	
			{
				endx = Ejournal_search(Ejournal, nEjournals, journals[wos[authors[m].paper[k]].journal].name);
				//fwprintf(logstream, L"@@@@@ %d [%s] %d\n", i, journals[wos[authors[m].paper[k]].journal].name, endx);
				if (endx != -1)
				{
					outstanding_cnt += Ejournal[endx].outstanding;
					if (Ejournal[endx].rank != 0)	// rank=0 means no rank data
					{
						//fwprintf(logstream, L"rank=%d\n", Ejournal[endx].rank);
						sum_rank += Ejournal[endx].rank_percentage;
						nepapers++;
					}
				}
			}
		}
		else if (m < 0 && p > 0)
		{
			for (k = 0; k < authors_p[p].np; k++) 
			{
				cndx = Cjournal_search(Cjournal, nCjournals, journals_p[wos_p[authors_p[p].paper[k]].journal].name);
				//fwprintf(logstream, L"@@@@@ %d [%s] %d\n", i, journals_p[wos_p[authors_p[p].paper[k]].journal].name, cndx);
				if (cndx != -1)
				{
					//fwprintf(logstream, L"TSSCI=%d\n", Cjournal[cndx].tssci);
					TSSCI_cnt += Cjournal[cndx].tssci;
				}
			}
		}
		//fwprintf(logstream, L"Sum rank=%.0f, nepapers=%d\n", sum_rank, nepapers);
		if (nepapers != 0)
			scholars[i].average_journal_rank = sum_rank / nepapers;
		else
			scholars[i].average_journal_rank = 99999;
		scholars[i].TSSCI_cnt = TSSCI_cnt;
		scholars[i].outstanding_cnt = outstanding_cnt;
	}

	return 0;
}

//
// convert position to numbers
// 0-博士後/講師, 1-助理教授, 2-副教授, 3-教授, 4-特聘教授, 5-講座教授 (研究員視同教授)
//
int position_in_numbers()
{
	int i;
	wchar_t *sp;

	for (i = 0; i < nscholars; i++)
	{
		sp = scholars[i].position;
		if (wcsncmp(sp, L"約聘專案", 4) == 0 || wcsncmp(sp, L"約僱專案", 4) == 0 
			|| wcsncmp(sp, L"專案計畫", 4) == 0 || wcsncmp(sp, L"專案專任", 4) == 0
			|| wcsncmp(sp, L"東海大學", 4) == 0)
			sp += 4;
		else if (wcsncmp(sp, L"會計系", 3) == 0 || wcsncmp(sp, L"政治系", 3) == 0)
			sp += 3;
		else if (wcsncmp(sp, L"專任", 2) == 0 || wcsncmp(sp, L"專案", 2) == 0 
			|| wcsncmp(sp, L"專技", 2) == 0 || wcsncmp(sp, L"兼任", 2) == 0 
			|| wcsncmp(sp, L"特約", 2) == 0 || wcsncmp(sp, L"約聘", 2) == 0
			|| wcsncmp(sp, L"終身", 2) == 0 || wcsncmp(sp, L"退休", 2) == 0
			|| wcsncmp(sp, L"名譽", 2) == 0 || wcsncmp(sp, L"台大", 2) == 0
			|| wcsncmp(sp, L"學術", 2) == 0)
			sp += 2;
		if (wcsncmp(sp, L"博士後", 3) == 0 || wcsncmp(sp, L"講師", 2) == 0)
			scholars[i].pos = 0;	// 博士後/講師
		else if (wcsncmp(sp, L"助理", 2) == 0)
			scholars[i].pos = 1;	// 助理教授/助理研究員
		else if (wcsncmp(sp, L"assistant professor", 19) == 0)
			scholars[i].pos = 1;	// 助理教授
		else if (wcsncmp(sp, L"a. professor", 12) == 0)
			scholars[i].pos = 1;	// 助理教授
		else if (wcsncmp(sp, L"主治", 2) == 0)
			scholars[i].pos = 1;
		else if (wcsncmp(sp, L"助研究員", 4) == 0)
			scholars[i].pos = 1;	// 助研究員
		else if (wcsncmp(sp, L"副教授", 3) == 0)
			scholars[i].pos = 2;	// 副教授
		else if (wcsncmp(sp, L"associate professor", 19) == 0)
			scholars[i].pos = 2;	// 副教授
		else if (wcsncmp(sp, L"副研究員", 4) == 0)
			scholars[i].pos = 2;	// 副研究員
		else if (wcsncmp(sp, L"professor", 9) == 0)
			scholars[i].pos = 3;	// 教授
		else if (wcsncmp(sp, L"主任", 2) == 0)
			scholars[i].pos = 3;	// 教授
		else if (wcsncmp(sp, L"教授", 2) == 0)
			scholars[i].pos = 3;	// 教授
		else if (wcsncmp(sp, L"研究員", 3) == 0)
			scholars[i].pos = 3;	// 研究員
		else if (wcsncmp(sp, L"館長", 2) == 0)
			scholars[i].pos = 3;
		else if (wcsncmp(sp, L"副校長", 3) == 0)
			scholars[i].pos = 3;
		else if (wcsncmp(sp, L"特聘", 2) == 0 || wcsncmp(sp, L"優聘", 2) == 0)
			scholars[i].pos = 4;	// 特聘教授/特聘研究員
		else if (wcsncmp(sp, L"講座", 2) == 0)
			scholars[i].pos = 5;	// 講座教授
		else if (wcsncmp(sp, L"榮譽", 2) == 0)
			scholars[i].pos = 5;	// 榮譽教授
		else if (wcsncmp(sp, L"司長", 2) == 0)
			scholars[i].pos = 5;
		else if (wcsncmp(sp, L"校長", 2) == 0)
			scholars[i].pos = 5;
		else if (wcsncmp(sp, L"大法官", 3) == 0)
			scholars[i].pos = 5;
		else
			scholars[i].pos = 0;	// 不明者視同講師，通常非學術機構人員
	}

	return 0;
}

//
// write out a combined scholars[], authors[], and authors_p[] data file, based on the scholars[] array
//
int write_combined_scholar_author_pauthor_data(wchar_t *dpath)
{
	int i, k, m, p;
	int sex;	// 1 for male, 2 for female
	wchar_t dname[FNAME_SIZE];
	FILE *dstream;

	swprintf(dname, L"%sCombinedSAAdata.csv", dpath);
	if (_wfopen_s(&dstream, dname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;


	find_average_author_count();
	scholar_publishing_frequency();	
	scholar_citation_counts();
	read_Cjournal_list(dpath);
	read_Ejournal_list(dpath);
	scholar_published_journal_stats();
	position_in_numbers();	// convert position to numbers

	// write out all information
	fwprintf(dstream, L"序號,中文姓名,英文姓名,性別,服務機關,最高學歷訖年月,學門,領域,職位,職位2,論文平均作者數,發表頻率,論文起始年,論文結束年,WOS引證數,平均期刊排名,傑出國際期刊數,TSSCI論文數,h-index,g-index,平均引證數,論文總數,中文論文數,英文論文數,論文\n");
	for (i = 0; i < nscholars; i++)
	{
		m = scholars[i].andx;	// index to the main data (WOS)
		p = scholars[i].pndx;	// index to the parallel data (TCI)
		sex = scholars[i].sex;
		if (m >= 0 || p >= 0)
		{
			fwprintf(dstream, L"%d,%s,\"%s\",%d,\"%s\",%d,%s,%s,%s,%d,%.3f,%.4f,%d,%d,%d,%.2f,%d,%d", i, scholars[i].cname, scholars[i].ename, sex, scholars[i].orgname, 
				scholars[i].phdyear, scholars[i].afield, scholars[i].afield2, scholars[i].position, scholars[i].pos,
				scholars[i].avg_author_cnt, scholars[i].pfreq, scholars[i].byear, scholars[i].eyear, scholars[i].tc, scholars[i].average_journal_rank, scholars[i].outstanding_cnt, scholars[i].TSSCI_cnt);
			if (m >= 0 && p >= 0)
				fwprintf(dstream, L",%d,%d,%.1f,%d,%d,%d", authors[m].h, authors[m].g, (double)scholars[i].tc/authors[m].np, authors_p[p].np+authors[m].np, authors_p[p].np, authors[m].np);
			else if (m > 0 && p <= 0)
				fwprintf(dstream, L",%d,%d,%.1f,%d,%d,%d", authors[m].h, authors[m].g, (double)scholars[i].tc/authors[m].np, authors[m].np, 0, authors[m].np);
			else if (m <= 0 && p > 0)
				fwprintf(dstream, L",%d,%d,%.1f,%d,%d,%d", 0, 0, (double)0, authors_p[p].np, authors_p[p].np, 0);
			// print paper list
			if (p > 0)
			{
				for (k = 0; k < authors_p[p].np; k++)
					fwprintf(dstream, L",%s", wos_p[authors_p[p].paper[k]].alias);
				fflush(dstream);
			}
			if (m > 0)
			{
				for (k = 0; k < authors[m].np; k++)
					fwprintf(dstream, L",%s", wos[authors[m].paper[k]].alias);
				fflush(dstream);
			}
			fwprintf(dstream, L"\n");
		}
	}

	fclose(dstream);

	return 0;
}

//
// write out the authors that are not in the scholar list
//
int write_authors_not_in_the_scholar_list(wchar_t *dpath)
{
	int i, cnt1, cnt2, ndx;
	wchar_t oname1[FNAME_SIZE];
	FILE *ostream1;
	wchar_t oname2[FNAME_SIZE];
	FILE *ostream2;

	swprintf(oname1, L"%s非名單內作者-中文.txt", dpath);
	if (_wfopen_s(&ostream1, oname1, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	cnt1 = 0;
	for (i = 0; i < naus_p; i++)
	{
		if (authors_p[i].sndx == -1)	// not in the TScholar list
		{	
			ndx = samename_Chinese_search(sname_Chinese, nsn_Chinese, authors_p[i].name);
			if (ndx == -1)	// nor in the same name list
			{
				cnt1++;
				fwprintf(ostream1, L"%d\t%s\n", cnt1, authors_p[i].name);
			}
		}
	}
	fclose(ostream1);

	swprintf(oname2, L"%s非名單內作者-英文.txt", dpath);
	if (_wfopen_s(&ostream2, oname2, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	cnt2 = 0;
	for (i = 0; i < naus; i++)
	{
		if (authors[i].sndx == -1)
		{
			ndx = samename_English_search(sname_English, nsn_English, authors[i].name);
			if (ndx == -1)	// nor in the same name list
			{
				cnt2++;
				fwprintf(ostream2, L"%d\t%s\n", cnt2, authors[i].name);
			}
		}
	}
	fclose(ostream2);

	return 0;
}

//
// find average author count of a scholar's papers
//
int find_average_author_count()
{
	int i, m, p, k;
	int cntE, cntC;

	for (i = 0; i < nscholars; i++)
	{
		cntE = 0; cntC = 0;
		m = scholars[i].andx;
		p = scholars[i].pndx;
		if (m >= 0 && p >= 0)
		{
			for (k = 0; k < authors[m].np; k++)	// run through all papers in WOS
				cntE += wos[authors[m].paper[k]].nau;
			for (k = 0; k < authors_p[p].np; k++)	// run through all papers in TCI
				cntC += wos_p[authors_p[p].paper[k]].nau;
			scholars[i].avg_author_cnt = (double)(cntE + cntC) / (authors[m].np + authors_p[p].np);
		}
		else if (m >= 0 && p < 0)
		{
			for (k = 0; k < authors[m].np; k++)	// run through all papers in WOS
				cntE += wos[authors[m].paper[k]].nau;
			scholars[i].avg_author_cnt = (double)cntE / authors[m].np;
		}
		else if (m < 0 && p >= 0)
		{
			for (k = 0; k < authors_p[p].np; k++)	// run through all papers in TCI
				cntC += wos_p[authors_p[p].paper[k]].nau;
			scholars[i].avg_author_cnt = (double)cntC / authors_p[p].np;
		}
		else
			scholars[i].avg_author_cnt = 0;
	}

	return 0;
}

//
// summary statistics for modality
//
int modality_stats()
{
	int i, m, p;
	int total_local, total_international;

	fwprintf(logstream, L"\n==========================\n");
	fwprintf(logstream, L"\"臺灣人文及社會科學研究樣態\"統計資料 Modality Statistics\n");

	total_local = 0;
	total_international = 0;
	for (i = 0; i < nscholars; i++)
	{
		m = scholars[i].andx;
		p = scholars[i].pndx;
		if (p > 0)
		{
			total_local += authors_p[p].np;
		}
		if (m > 0)
		{
			total_international += authors[m].np;
		}
	}

	fwprintf(logstream, L"Total Local Papers = %d, Total International Papers = %d\n", total_local, total_international);
	fwprintf(logstream, L"==========================\n\n");

	return 0;
}

//
// parse lines in the Chinese journal list file
//
int parse_Cjournal_data(wchar_t *str, struct CJOURNAL *cjrs)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[SBUF_SIZE];
	int state;

	state = 0;
	sp = str;  tp = tmps;
	while (*sp != '\0' && *sp != '\r' && *sp != '\n')
	{
		ch = *sp; 
		if (ch == '\t')
		{ 
			*tp++ = '\0'; sp++; 
			switch (state)
			{
			case 0:	// 期刊名稱
				remove_spaces_at_the_end(tmps);
				wcscpy(cjrs->jname, tmps);	
				state++;
				break;
			case 1:	// 索引
				remove_spaces_at_the_end(tmps);
				if (wcscmp(tmps, L"tssci") == 0)
					cjrs->tssci = 1;
				else 
					cjrs->tssci = 0;
				state++;
				break;
			case 2:	// 學門
				remove_spaces_at_the_end(tmps);
				wcscpy(cjrs->field, tmps);
				break;
			default:
				break;
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
	if (tmps[0] != '\0')
		wcscpy(cjrs->field, tmps);
	else
		wcscpy(cjrs->field, L"UNKNOWN");

	return 0;
}

//
// read the file "中文期刊表.txt", which is a list of TSSCI journals
//
int read_Cjournal_list(wchar_t *dpath)
{
	int i;
	FILE *sstream;
	wchar_t *line;
	wchar_t sname[FNAME_SIZE];
	wchar_t tbuf1[SBUF_SIZE], tbuf2[SBUF_SIZE];

	// allocate the working memory
	line = (wchar_t *)malloc(SBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	swprintf(sname, L"%s中文期刊表.txt", dpath);
	// 
	// Open the source file (will fail if the file does not exist)
	//	
	if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)
	{
		fwprintf(logstream, L"WARNING: Cannot open 中文期刊表: %s\n", sname);
		return MSG_WOSFILE_NOTFOUND;
	}

	// 1st pass, obtain the number of journals
	fgetws(line, SBUF_SIZE, sstream);	// ignore the 1st line
	nCjournals = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nCjournals++;	
	}

	Cjournal = (struct CJOURNAL *)Jmalloc(nCjournals * sizeof(struct CJOURNAL), L"read_Cjournal_list: Cjournal");
	// read the data line by line
	rewind(sstream);
	fgetws(line, SBUF_SIZE, sstream);	// ignore the 1st line
	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_Cjournal_data(line, &Cjournal[i]);
		i++;
	}

#ifdef DEBUG
	for (i = 0; i < nCjournals; i++)
		fwprintf(logstream, L"#####\t%s\t%d\t%s\n", Cjournal[i].jname, Cjournal[i].tssci, Cjournal[i].field);
#endif DEBUG

	qsort((void *)Cjournal, (size_t)nCjournals, sizeof(struct CJOURNAL), compare_Cjournal_title);

	fclose(sstream);
	free(line);

	return 0;
}

//
// parse lines in the English journal list file
//
int parse_Ejournal_data(wchar_t *str, struct EJOURNAL *ejrs)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[SBUF_SIZE];
	int state;

	state = 0;
	sp = str;  tp = tmps;
	while (*sp != '\0' && *sp != '\r' && *sp != '\n')
	{
		ch = *sp; 
		if (ch == '\t')
		{ 
			*tp++ = '\0'; sp++; 
			switch (state)
			{
			case 0:	// rank (within its category)
				remove_spaces_at_the_end(tmps);
				ejrs->rank = _wtoi(tmps);
				state++;
				break;
			case 1:	// full journal title
				remove_spaces_at_the_end(tmps);
				wcscpy(ejrs->jname, tmps);	
				state++;
				break;
			case 2:	// journal impact factor
				remove_spaces_at_the_end(tmps);
				ejrs->impactf = _wtof(tmps);
				state++;
				break;
			case 3:	// field
				remove_spaces_at_the_end(tmps);
				if (tmps[0] != '\0')
					wcscpy(ejrs->field, tmps);
				else
					wcscpy(ejrs->field, L"UNKNOWN");
				state++;
				break;
			case 4:	// outstanding journal
				remove_spaces_at_the_end(tmps);
				ejrs->outstanding = _wtoi(tmps);
				return 0;	// added 2017/11/26, fixed the problem of not reading 'outstanding' correctly
			default:
				break;
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = '\0';
	ejrs->outstanding = _wtoi(tmps);

	return 0;
}

//
// read the file "英文期刊表.txt", which is a list of SCI/SSCI journals
//
int read_Ejournal_list(wchar_t *dpath)
{
	int i, k, njs_field, njs_all;
	FILE *sstream;
	wchar_t *line;
	wchar_t sname[FNAME_SIZE];
	wchar_t tbuf1[SBUF_SIZE], tbuf2[SBUF_SIZE];

	// allocate the working memory
	line = (wchar_t *)malloc(SBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	swprintf(sname, L"%s英文期刊表.txt", dpath);
	// 
	// Open the source file (will fail if the file does not exist)
	//	
	if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)
	{
		fwprintf(logstream, L"WARNING: Cannot open 英文期刊表: %s\n", sname);
		return MSG_WOSFILE_NOTFOUND;
	}

	// 1st pass, obtain the number of journals
	fgetws(line, SBUF_SIZE, sstream);	// ignore the 1st line
	nEjournals = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nEjournals++;	
	}

	Ejournal = (struct EJOURNAL *)Jmalloc(nEjournals * sizeof(struct EJOURNAL), L"read_Ejournal_list: Ejournal");
	// read the data line by line
	rewind(sstream);
	fgetws(line, SBUF_SIZE, sstream);	// ignore the 1st line
	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_Ejournal_data(line, &Ejournal[i]);
		i++;
	}

	// find the percentage rank for each journal
	// assume that the data is given in rank order with each WOS field
	// added 2017/07/26
	int prev_rank;
	prev_rank = 0;
	njs_field = 0; njs_all = 0;
	for (i = 0; i < nEjournals; i++)
	{
		if (Ejournal[i].rank >= prev_rank) // rank increasing
		{
			njs_field++;
			prev_rank = Ejournal[i].rank;
		}
		else
		{
			for (k = 0; k < njs_field; k++)
				Ejournal[njs_all+k].rank_percentage = (double)100.0 * k / njs_field;
			njs_all += njs_field;
			njs_field = 1; 
			prev_rank = Ejournal[i].rank;
		}
	}
	// for the last field in the data
	for (k = 0; k < njs_field; k++)	
		Ejournal[njs_all+k].rank_percentage = (double)100.0 * k / njs_field;

	fwprintf(logstream, L"英文期刊表 Number of journals in the list = %d\n\n", nEjournals);

	qsort((void *)Ejournal, (size_t)nEjournals, sizeof(struct EJOURNAL), compare_Ejournal_title);

#ifdef DEBUG
	for (i = 0; i < nEjournals; i++)
		fwprintf(logstream, L"@@@@@\t%s\t%.3f\t%d\t%.2f\t%s\t%d\n", Ejournal[i].jname, Ejournal[i].impactf, Ejournal[i].rank, Ejournal[i].rank_percentage, Ejournal[i].field, Ejournal[i].outstanding);
#endif DEBUG

	// consolidate the duplicate journals (they appear in multiple fields)
	wchar_t prev_jname[MAX_JOURNAL_NAME];
	wchar_t combined_field[MAX_FIELD_NAME];
	int cnt, ndup;
	double sum_rank;
	struct EJOURNAL *tEjournal;
	tEjournal = (struct EJOURNAL *)Jmalloc(nEjournals * sizeof(struct EJOURNAL), L"read_Ejournal_list: tEjournal");
	for (i = 0; i < nEjournals; i++)
		tEjournal[i] = Ejournal[i];
	wcscpy(prev_jname, tEjournal[0].jname);
	cnt = 0; ndup = 1;
	for (i = 1; i < nEjournals; i++)
	{
		if (wcscmp(tEjournal[i].jname, prev_jname) != 0)	// hit a new journal name
		{
			if (ndup > 1)
			{
				sum_rank = 0.0;
				combined_field[0] = '\0';
				for (k = 0; k < ndup; k++)
				{
					sum_rank += tEjournal[i-1-k].rank_percentage;
					if (k == 0)
						wcscpy(combined_field, tEjournal[i-1-k].field);
					else
						swprintf(combined_field, L"%s/%s", combined_field, tEjournal[i-1-k].field);
					//fwprintf(logstream, L"%d [%s] in [%s]\n", cnt, tEjournal[i-1-k].jname, tEjournal[i-1-k].field); fflush(logstream);
				}
				Ejournal[cnt] = tEjournal[i-1];
				wcscpy(Ejournal[cnt].field, combined_field);
				Ejournal[cnt].rank_percentage = sum_rank / ndup;	// take the average of ranks in all fields
			}
			else
				Ejournal[cnt] = tEjournal[i-1];
			wcscpy(prev_jname, tEjournal[i].jname);
			cnt++;
			ndup = 1;
		}
		else 
			ndup++;
	}
	nEjournals = cnt;
	fwprintf(logstream, L"英文期刊表 Number of unique journals = %d\n\n", nEjournals); fflush(logstream);

#ifdef DEBUG
	for (i = 0; i < nEjournals; i++)
	{
		fwprintf(logstream, L"#####\t%s\t%.3f\t%d\t%.2f\t%s\t%d\n", Ejournal[i].jname, Ejournal[i].impactf, Ejournal[i].rank, Ejournal[i].rank_percentage, Ejournal[i].field, Ejournal[i].outstanding);
		fflush(logstream);
	}
#endif DEBUG

	fclose(sstream);
	Jfree(tEjournal, L"read_Ejournal_list: tEjournal");
	free(line);

	return 0;
}

//
// remove spaces at the end of the given string
//
int remove_spaces_at_the_end(wchar_t *str)
{
	wchar_t *sp;

	if (str[0] == '\0') return 0;

	sp = str;
	while (*sp != '\0') sp++;
	sp--;
	while (*sp == ' ' || *sp == '\t') sp--;
	sp++;
	*sp = '\0';

	return 0;
}

//
// use binary search 
//
int scholar_ename_search(struct SCHOLAR d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].ename) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].ename) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].ename) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].ename) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search 
//
int scholar_cname_search(struct SCHOLAR d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].cname) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].cname) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].cname) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].cname) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search 
//
int Ejournal_search(struct EJOURNAL d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].jname) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].jname) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].jname) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].jname) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search 
//
int Cjournal_search(struct CJOURNAL d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].jname) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].jname) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].jname) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].jname) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// this fucntion is to be called by qsort() only
// 
int compare_Ejournal_title(const void *n1, const void *n2)
{
	struct EJOURNAL *t1, *t2;
	
	t1 = (struct EJOURNAL *)n1;
	t2 = (struct EJOURNAL *)n2;
	if (wcscmp(t2->jname, t1->jname) < 0)
		return 1;
	else if (wcscmp(t2->jname, t1->jname) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_Cjournal_title(const void *n1, const void *n2)
{
	struct CJOURNAL *t1, *t2;
	
	t1 = (struct CJOURNAL *)n1;
	t2 = (struct CJOURNAL *)n2;
	if (wcscmp(t2->jname, t1->jname) < 0)
		return 1;
	else if (wcscmp(t2->jname, t1->jname) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_scholar_ename(const void *n1, const void *n2)
{
	struct SCHOLAR *t1, *t2;
	
	t1 = (struct SCHOLAR *)n1;
	t2 = (struct SCHOLAR *)n2;
	if (wcscmp(t2->ename, t1->ename) < 0)
		return 1;
	else if (wcscmp(t2->ename, t1->ename) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_scholar_cname(const void *n1, const void *n2)
{
	struct SCHOLAR *t1, *t2;
	
	t1 = (struct SCHOLAR *)n1;
	t2 = (struct SCHOLAR *)n2;
	if (wcscmp(t2->cname, t1->cname) < 0)
		return 1;
	else if (wcscmp(t2->cname, t1->cname) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_scholar_ndx(const void *n1, const void *n2)
{
	struct SCHOLAR *t1, *t2;
	
	t1 = (struct SCHOLAR *)n1;
	t2 = (struct SCHOLAR *)n2;
	if (t2->ndx < t1->ndx)
		return 1;
	else if (t2->ndx == t1->ndx)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of an author name in an SAMENAME_CHINESE array
//
int samename_Chinese_search(struct SAMENAME_CHINESE d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].cname) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].cname) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].cname) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].cname) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search to find the proper position of an author name in an SAMENAME_ENGLISH array
//
int samename_English_search(struct SAMENAME_ENGLISH d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].ename) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].ename) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].ename) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].ename) > 0)
			low = cur;
		else
			high = cur;
	}
}