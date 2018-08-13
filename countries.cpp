//
// countries.cpp
//
// This file contains code to read and process authors' "countries" for WOS data
//
// 2013/04/10 Coding being
// 2013/04/17 Basic function works
// 2013/04/23 Added an "[unknown country]" entry in the country list
//            Added codes to handle the alias for "czech republic" ==> "czechoslovakia"
// 2013/04/24 Added about 30 more countries to the country list
// 2013/06/15 Modified to delete a double quote or a period at the end of a country name before furhter processing
//              in the functions parse_countries_rp(), parse_countries(), preparse_countries()
// 2013/07/19 Added function find_location_code2() and patent country codes location2[].
// 2013/07/20 Updaated country code table location[] for WOS data
// 2014/07/28 Fixed a bug in parsing WOS “RP” data by extending MAX_COUNTRIES_RP from 2 to 100 (in network.h)
// 2015/09/29 Added several new countries such as Iraq, Panama, Brunei, etc.
// 2015/09/29 Added codes to handle special names in the function parse_countries_rp(), i.e, remove spaces in a double surname
// 2015/10/16 Added codes to handle address string that is a null
// 2016/12/29 Added codes related to Thomson Innovation Patent (TIP) data
// 2016/02/28 Added three functions: parse_countries_rp_Scopus(), parse_countries_Scopus(), preparse_countries_Scopus()
// 2017/03/25 Added codes to handle special case in "Authors with affiliations", 
//                  example: "NY 14260-4140, USA (Tel: + 1 716 645 2417 ext. 470; fax: + 1 716 645 2166).
// 2018/03/13 Fixed a problem caused by too many assignees, added check for MAX_ASSIGNEES
// 2018/03/16 Enlarge stack size for the variable "c" in the function consolidate_countries_assignee() and consolidate_areas_assignee() 
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
#include "clustering.h"
#include "countries.h"

int compare_countryx(const void *, const void *);
int compare_countryname(const void *, const void *);
int countryname_search(struct COUNTRIES *, int, wchar_t *);
int aname_search(struct AUTHORS *, int, wchar_t *);
int US_state_search(struct USSTATES *, int, wchar_t *);
int preparse_countries(int, wchar_t *);
int preparse_countries_Scopus(int, wchar_t *);
int location_search(struct LOCATION *, int, wchar_t *);

extern int adjust_author_last_name(wchar_t *);
extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int aalias_search(struct ANAME_ALIAS*, int, wchar_t *);

#define XLBUF_SIZE 65536

extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array
extern int ncountries;				
extern struct COUNTRIES *countries;
extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;
extern int naalias;	// number of items in the assignee alias table
extern struct ANAME_ALIAS *aalias;
extern int nuspto;
extern struct USPTO *uspto;

extern FILE *logstream;

//
// find the code for location (for WOS data)
//
int find_location_code(wchar_t *cname, int *c1, int *c2, int *island)
{
	int ndx;

	ndx = location_search(location, NLOCS, cname);
	if (ndx == -1)
		return -1;
	else
	{
		*c1 = location[ndx].c1;
		*c2 = location[ndx].c2;
		*island = location[ndx].island;

		return 0;
	}
}

//
// find the code for location (for patent data)
//
int find_location_code2(wchar_t *cname, int *c1, int *c2, int *island)
{
	int ndx;

	ndx = location_search(location2, NLOCS2, cname);
	if (ndx == -1)
		return -1;
	else
	{
		*c1 = location2[ndx].c1;
		*c2 = location2[ndx].c2;
		*island = location2[ndx].island;

		return 0;
	}
}

//
// find the code for location (for Scopus data)
//
int find_location_code3(wchar_t *cname, int *c1, int *c2, int *island)
{
	int ndx;

	ndx = location_search(location3, NLOCS3, cname);
	if (ndx == -1)
		return -1;
	else
	{
		*c1 = location3[ndx].c1;
		*c2 = location3[ndx].c2;
		*island = location3[ndx].island;

		return 0;
	}
}

//
// use binary search to find the proper position of a "name" in a "KWORDS" array
//
int location_search(struct LOCATION d[], int num, wchar_t *str)
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

#ifdef NOMORE
//
// write country information to the specified file
//
int write_country_info(FILE *ostream, struct COUNTRIES *cntry)
{
	int i, j, k;
	//int lc1, lc2, ind;
	//wchar_t location[MAX_COUNTRY_NAME];
	//wchar_t location_st[MAX_COUNTRY_NAME];

	//find_significant_country();	

	fwprintf(ostream, L"\nArticle\tCountries\n");
	for (i = 0; i < nwos; i++)
	{
		fwprintf(ostream, L"%s\t%d", wos[i].alias, wos[i].year);
		for (k = 0; k < wos[i].ncountries; k++)
		{
			j = wos[i].countries[k];
			fwprintf(logstream, L"\t%s", cntry[j].name);
		}
		fwprintf(logstream, L"\n");
	}

	return 0;
}
#endif NOMORE

//
// consolidate country information for each author
//
struct COUNTRY_IND { int ind; int cnt; };
int consolidate_countries(int naus, struct AUTHORS *authors)
{
	int i, k, ii, cnt, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[MAX_COUNTRIES];

	// before this function is called, authors[].country_cnt[] are all set to 1
	for (i = 0; i < naus; i++)
	{
		if (authors[i].ncountries == 0)
			continue;
		cnt = 0;
		for (k = 0; k < authors[i].ncountries; k++)
			c[cnt++].ind = authors[i].countries[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct COUNTRY_IND), compare_countryx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new country
			{
				authors[i].countries[cnt1] = prev;
				authors[i].country_cnt[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		authors[i].countries[cnt1] = prev;
		authors[i].country_cnt[cnt1] = cnt2;
		authors[i].ncountries = cnt1+1;
	}

	// before this function is called, authors[].country_cnt_rp[] are all set to 1
	for (i = 0; i < naus; i++)
	{
		if (authors[i].ncountries_rp == 0)
			continue;
		cnt = 0;
		for (k = 0; k < authors[i].ncountries_rp; k++)
			c[cnt++].ind = authors[i].countries_rp[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct COUNTRY_IND), compare_countryx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new country
			{
				authors[i].countries_rp[cnt1] = prev;
				authors[i].country_cnt_rp[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		authors[i].countries_rp[cnt1] = prev;
		authors[i].country_cnt_rp[cnt1] = cnt2;
		authors[i].ncountries_rp = cnt1+1;
	}

	return 0;
}

//
// consolidate country information for each assignee
//
int consolidate_countries_assignee(int nasgn, struct ASSIGNEES *asgn)
{
	int i, k, ii, cnt, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[LBUF_SIZE];	// changed from MAX_COUNTRIES_ASSIGNEE, 2018/03/16

	// before this function is called, assignee[].country_cnt[] are all set to certain values
	for (i = 0; i < nasgn; i++)
	{
		if (asgn[i].ncountries == 0)
			continue;
		cnt = 0;

#ifdef DEBUG
		fwprintf(logstream, L"~~~~~%d %s\t", i, asgn[i].name); fflush(logstream);
		for (k = 0; k < asgn[i].ncountries; k++)
		{
			fwprintf(logstream, L"[%d %d]\t", i, k, asgn[i].countries[k]);
			fflush(logstream);
		}
		fwprintf(logstream, L"\n"); fflush(logstream);
#endif DEBUG

		for (k = 0; k < asgn[i].ncountries; k++)
			c[cnt++].ind = asgn[i].countries[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct COUNTRY_IND), compare_countryx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new country
			{
				asgn[i].countries[cnt1] = prev;
				asgn[i].country_cnt[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		asgn[i].countries[cnt1] = prev;
		asgn[i].country_cnt[cnt1] = cnt2;
		asgn[i].ncountries = cnt1+1;
	}

	return 0;
}

//
// consolidate area information for each assignee
//
int consolidate_areas_assignee(int nasgn, struct ASSIGNEES *asgn)
{
	int i, k, ii, cnt, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[LBUF_SIZE];	// changed from MAX_COUNTRIES_ASSIGNEE, 2018/03/16

	// before this function is called, assignee[].area_cnt[] are all set to certain values
	for (i = 0; i < nasgn; i++)
	{
		if (asgn[i].nareas == 0)
			continue;
		cnt = 0;
		for (k = 0; k < asgn[i].nareas; k++)
			c[cnt++].ind = asgn[i].areas[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct COUNTRY_IND), compare_countryx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new country
			{
				asgn[i].areas[cnt1] = prev;
				asgn[i].area_cnt[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		asgn[i].areas[cnt1] = prev;
		asgn[i].area_cnt[cnt1] = cnt2;
		asgn[i].nareas = cnt1+1;
	}

	return 0;
}

//
// consolidate country information for each patent (TIP data only)
//
int consolidate_countries_patent_TIP(int nuspto, struct USPTO *uspto)
{
	int i, k, ii, cnt, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[MAX_COUNTRIES];

	// before this function is called, uspto[].country_cnt[] are all set to certain values
	for (i = 0; i < nuspto; i++)
	{
		if (uspto[i].ncountries == 0)	// abnormal case
			continue;
		cnt = 0;
		for (k = 0; k < uspto[i].ncountries; k++)
			c[cnt++].ind = uspto[i].countries[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct COUNTRY_IND), compare_countryx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new country
			{
				uspto[i].countries[cnt1] = prev;
				uspto[i].country_cnt[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		uspto[i].countries[cnt1] = prev;
		uspto[i].country_cnt[cnt1] = cnt2;
		uspto[i].ncountries = cnt1+1;
	}

	return 0;
}

//
// consolidate area information for each patent (TIP data only)
//
int consolidate_areas_patent_TIP(int nuspto, struct USPTO *uspto)
{
	int i, k, ii, cnt, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[MAX_COUNTRIES];

	// before this function is called, uspto[].area_cnt[] are all set to certain values
	for (i = 0; i < nuspto; i++)
	{
		if (uspto[i].nareas == 0)	// abnormal case
			continue;
		cnt = 0;
		for (k = 0; k < uspto[i].nareas; k++)
			c[cnt++].ind = uspto[i].areas[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct COUNTRY_IND), compare_countryx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new country
			{
				uspto[i].areas[cnt1] = prev;
				uspto[i].area_cnt[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		uspto[i].areas[cnt1] = prev;
		uspto[i].area_cnt[cnt1] = cnt2;
		uspto[i].nareas = cnt1+1;
	}

	return 0;
}

//
// put together country information for the given group
// this function is created 2015/10/17
//
int consolidate_group_coutries(int ndx, struct RESEARCH_GROUP *rgroups, int naus, struct AUTHORS  *authors)
{
	int i, j, k, ii;
	int cntx, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[MAX_COUNTRIES];

	cntx = 0;
	for (i = 0; i < rgroups[ndx].nscholars; i++)	// for all scholars in this group
	{
		j = rgroups[ndx].scholars[i];
		rgroups[ndx].countries[cntx] = authors[j].location;	// take only the most significant country of this author
		rgroups[ndx].country_cnt[cntx] = 1;
		cntx++;
#ifdef OBSOLETE
		for (k = 0; k < authors[j].ncountries; k++, cntx++)	// for all countries of this scholar
		{
			rgroups[ndx].countries[cntx] = authors[j].countries[k];	// take all countries of this author
			rgroups[ndx].country_cnt[cntx] = authors[j].country_cnt[k];
		}
#endif OBSOLETE
	}

	for (k = 0; k < cntx; k++)
	{
		c[k].ind = rgroups[ndx].countries[k];
		c[k].cnt = rgroups[ndx].country_cnt[k];
	}
	qsort((void *)c, (size_t)cntx, sizeof(struct COUNTRY_IND), compare_countryx);

	cnt1 = 0; cnt2 = 0;
	prev = c[0].ind;
	for (ii = 0; ii < cntx; ii++)
	{
		if (c[ii].ind != prev)	// hit a new country
		{
			rgroups[ndx].countries[cnt1] = prev;
			rgroups[ndx].country_cnt[cnt1] = cnt2;
			cnt2 = c[ii].cnt; cnt1++;
			prev = c[ii].ind;
		}
		else
			cnt2 += c[ii].cnt;
	}
	rgroups[ndx].countries[cnt1] = prev;
	rgroups[ndx].country_cnt[cnt1] = cnt2;
	rgroups[ndx].ncountries = cnt1+1;

	return 0;
}

//
// put together country information for the given group
// this function is created 2017/03/26
//
int consolidate_group_coutries_assignee(int ndx, struct RESEARCH_GROUP *rgroups, int naus, struct ASSIGNEES  *assignees)
{
	int i, j, k, ii;
	int cntx, cnt1, cnt2;
	int prev;
	struct COUNTRY_IND c[MAX_COUNTRIES];

	cntx = 0;
	for (i = 0; i < rgroups[ndx].nscholars; i++)	// for all scholars in this group
	{
		j = rgroups[ndx].scholars[i];
		rgroups[ndx].countries[cntx] = assignees[j].location;	// take only the most significant country of this assignee
		rgroups[ndx].country_cnt[cntx] = 1;
		cntx++;
	}

	for (k = 0; k < cntx; k++)
	{
		c[k].ind = rgroups[ndx].countries[k];
		c[k].cnt = rgroups[ndx].country_cnt[k];
	}
	qsort((void *)c, (size_t)cntx, sizeof(struct COUNTRY_IND), compare_countryx);

	cnt1 = 0; cnt2 = 0;
	prev = c[0].ind;
	for (ii = 0; ii < cntx; ii++)
	{
		if (c[ii].ind != prev)	// hit a new country
		{
			rgroups[ndx].countries[cnt1] = prev;
			rgroups[ndx].country_cnt[cnt1] = cnt2;
			cnt2 = c[ii].cnt; cnt1++;
			prev = c[ii].ind;
		}
		else
			cnt2 += c[ii].cnt;
	}
	rgroups[ndx].countries[cnt1] = prev;
	rgroups[ndx].country_cnt[cnt1] = cnt2;
	rgroups[ndx].ncountries = cnt1+1;

	return 0;
}

#define IN_SQUARE_BRACKET 1
#define OUTOFF_SQUARE_BRACKET 2

#ifdef NOMORE
//
// split a given author address string into several 'countries'
// and then save the countires to the global 'countries' array
//
int parse_store_countries(wchar_t *cpstr, int *ncntry, struct COUNTRIES *cntry)
{
	int i;
	wchar_t ch, *sp, *tp, *cp;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	int tncntry;
	int ind;
	int state;

	state = OUTOFF_SQUARE_BRACKET;
	tncntry = *ncntry;
	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps;
	while (*sp != '\0')
	{
		switch (state)
		{
		case IN_SQUARE_BRACKET:
			if (*sp == ']')
				state = OUTOFF_SQUARE_BRACKET;
			sp++;
			break;
		case OUTOFF_SQUARE_BRACKET:
			ch = *sp; 
			if (ch == '[')
			{
				state = IN_SQUARE_BRACKET;
				sp++;
				break;
			}
			if (ch == ';') 
			{ 
				*tp++ = '\0'; sp++; 
				if (tmps[0] != '\0')
				{				
					cp = tp - 1;
					while (*cp != ',') cp--;	// going backward to locate country names
					cp++; while (*cp == ' ') cp++;
					i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
					if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
						swprintf_s(cntry[tncntry++].name, MAX_COUNTRY_NAME, L"usa");
					else
						wcscpy_s(cntry[tncntry++].name, MAX_COUNTRY_NAME, cp);
				}
				tp = tmps;
				while (*sp == ' ') sp++;
			}
			else 
				*tp++ = towlower(*sp++); 
			break;
		default:
			break;
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		cp = tp - 1;
		while (*cp != ',') cp--;	// going backward to locate country names
		cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntry[tncntry++].name, MAX_COUNTRY_NAME, L"usa");
		else
			wcscpy_s(cntry[tncntry++].name, MAX_COUNTRY_NAME, cp);
	}
	*ncntry = tncntry;

	return 0;
}
#endif NOMORE

//
// given a WOS "RP" string, assign the countries to authors
//
int parse_countries_rp(int wosi, wchar_t *cpstr, int naus, struct AUTHORS *au)
{
	int i, m;
	wchar_t *sp, *tp, *cp, *ap;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t cntryname[MAX_COUNTRY_NAME];
	wchar_t aname[MAX_AUTHOR_NAME];
	int ind, ind2;
	int state;
	int aind, ndx2;	// author index
	int ncomma;

	// special case, the address string is a null
	if (*cpstr == '\0')	// added 2015/10/16
	{
		wos[wosi].countries_rp[wos[wosi].ncountries_rp++] = 1;	// use "[unknown country]" as the country name
		return 0;
	}

	state = OUTOFF_SQUARE_BRACKET;
	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	tp = tmps; ap = aname; ncomma = 0;
	while (*sp != '\0') 
	{
		*tp = towlower(*sp++); 
		if (*tp == ',' || *tp == '(') ncomma++;	// 2014/07/28, added condition '(', to cover the format such as "tone, k (reprint author)"
		if (ncomma < 2) *ap++ = *tp;
		else if (ncomma == 2) 
		{
			*ap = '\0'; 
			ap--; while (*ap == ' ') ap--;	// added 2014/07/28, remove the spaces at the end
			ap++; *ap = '\0';
		}
		tp++;
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
		while (*cp != ',') cp--;	// going backward to locate country names
		cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntryname, MAX_COUNTRY_NAME, L"usa");
		else
		{
			if (wcscmp(cp, L"fed rep ger") == 0)	// special case, added 2013/04/16
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"germany");
			else if (wcscmp(cp, L"czechoslovakia") == 0)	// special case, added 2013/04/23
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"czech republic");
			else if (wcscmp(cp, L"italia") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"italy");
			else
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
		}
		ind2 = location_search(location, NLOCS, cntryname);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nERROR: In WOS:%s, RP, no country found for \"%s\"\n", wos[wosi].docid, cntryname);
			ind2 = 1;	// points to "[unknonwn country]"
		}
		wos[wosi].countries_rp[wos[wosi].ncountries_rp++] = ind2;
		// need to handle special names here, i.e., remove spaces in a double surname, added 2015/09/29
		adjust_author_last_name(aname);	// added 2015/09/29
		ndx2 = authoralias_search(authoralias, nauthoralias, aname);	// added 2015/09/29
		if (ndx2 >= 0)	// found that this is a variation of the author name
			wcscpy_s(aname, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
		aind = aname_search(au, naus, aname);
		if (aind != -1 && ind2 != -1)		// for unknow author name, most likely because of database error, ignore
		{
			authors[aind].countries_rp[authors[aind].ncountries_rp] = ind2;
			authors[aind].country_cnt_rp[authors[aind].ncountries_rp] = 1;	// set to 1 here, will change in the later code
			authors[aind].ncountries_rp++;
		}
		else 
			fwprintf(logstream, L"Warning: In parse_countries_rp(), %d, [%s]\n", aind, aname);
	}

#ifdef XXX
	if (acnt < wos[wosi].nau)
		fwprintf(logstream, L"{%s} ", cp);
	fwprintf(logstream, L"\n");
	if (acnt != wos[wosi].nau)
		fwprintf(logstream, L"|%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);
#endif

	return 0;
}

//
// given a WOS "C1" string, assign the countries to authors
//
#define ADDRESS_NOT_GIVEN 0
#define ADDRESS_MATCH 1
#define ADDRESS_ALL_IN_ONE 2
#define ADDRESS_MULTIPLE_BUT_SAME_COUNTRY 3
#define ADDRESS_VAGUE 4
int parse_countries(int wosi, wchar_t *cpstr, int naus, struct AUTHORS *au)
{
	int i, m;
	wchar_t ch, *sp, *tp, *cp;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t cntryname[MAX_COUNTRY_NAME];
	int ind, ind2;
	int state;
	int acnt;	// address count
	int aind;	// author index
	int situation, first_time;

	// special case, the address string is a null
	if (*cpstr == '\0')	// added 2015/10/16
	{
		wos[wosi].countries[wos[wosi].ncountries++] = 1;	// use "[unknown country]" as the country name
		acnt = 0;
		for (m = 0; m < wos[wosi].nau; m++)
		{
			aind = wos[wosi].author[acnt++];
			authors[aind].countries[authors[aind].ncountries] = 1;	// use "[unknown country]" as the country name
			authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
			authors[aind].ncountries++;
		}
		return 0;
	}

	situation = preparse_countries(wosi, cpstr);

	state = OUTOFF_SQUARE_BRACKET;
	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps; acnt = 0; first_time = TRUE;
	while (*sp != '\0')
	{
		switch (state)
		{
		case IN_SQUARE_BRACKET:
			if (*sp == ']')
				state = OUTOFF_SQUARE_BRACKET;
			sp++;
			break;
		case OUTOFF_SQUARE_BRACKET:
			ch = *sp; 
			if (ch == '[')
			{
				state = IN_SQUARE_BRACKET;
				sp++;
				break;
			}
			if (ch == ';') 
			{ 
				*tp++ = '\0'; sp++; 
				if (tmps[0] != '\0')
				{				
					cp = tp - 1;
					if (*cp == '\"') // ignore the double quote at the end of the string
					{
						*cp = '\0'; cp--; 
						if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
					}
					else if (*cp == '.')
						*cp = '\0';	// ignore the '.' at the end of the string
					while (*cp != ',') cp--;	// going backward to locate country names
					cp++; while (*cp == ' ') cp++;
					i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
					if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
						swprintf_s(cntryname, MAX_COUNTRY_NAME, L"usa");
					else
					{
						if (wcscmp(cp, L"fed rep ger") == 0)	// special case, added 2013/04/16
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"germany");
						else if (wcscmp(cp, L"czechoslovakia") == 0)	// special case, added 2013/04/23
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"czech republic");
						else if (wcscmp(cp, L"italia") == 0)	
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"italy");
						else
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
					}
					//ind2 = countryname_search(cntry, ncntry, cntryname);
					ind2 = location_search(location, NLOCS, cntryname);
					if (ind2 == -1)
					{
						fwprintf(logstream, L"\nERROR: In WOS:%s, C1, no country found for \"%s\"\n", wos[wosi].docid, cntryname);
						ind2 = 1;	// points to "[unknonwn country]"
					}
					wos[wosi].countries[wos[wosi].ncountries++] = ind2;
					if (situation == ADDRESS_MATCH)	// one to one assignment
					{
						aind = wos[wosi].author[acnt++];
						authors[aind].countries[authors[aind].ncountries] = ind2;
						authors[aind].country_cnt[authors[aind].ncountries] = 1;	// set to 1 here, will change in the later code
						authors[aind].ncountries++;
					}
					else if (situation == ADDRESS_ALL_IN_ONE && first_time == TRUE)	// one to all assignment
					{
						for (m = 0; m < wos[wosi].nau; m++)
						{
							aind = wos[wosi].author[acnt++];
							authors[aind].countries[authors[aind].ncountries] = ind2;
							authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
							authors[aind].ncountries++;
						}
						first_time = FALSE;
					}
					else if (situation == ADDRESS_MULTIPLE_BUT_SAME_COUNTRY && first_time == TRUE)	// one to all assignment
					{
						for (m = 0; m < wos[wosi].nau; m++)
						{
							aind = wos[wosi].author[acnt++];
							authors[aind].countries[authors[aind].ncountries] = ind2;
							authors[aind].country_cnt[authors[aind].ncountries] = 1;	// set to 1 here, will change in the later code
							authors[aind].ncountries++;
						}
						first_time = FALSE;
					}
					else if (first_time == TRUE)	// cannot resovle addresses<==>authors corrspondence, added 2015/10/16
					{
						for (m = 0; m < wos[wosi].nau; m++)
						{
							aind = wos[wosi].author[acnt++];
							authors[aind].countries[authors[aind].ncountries] = 0;	// use "[VAGUE]" as country name
							authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
							authors[aind].ncountries++;
						}
						first_time = FALSE;
					}
				}
				tp = tmps;
				while (*sp == ' ') sp++;
			}
			else 
				*tp++ = towlower(*sp++); 
			break;
		default:
			break;
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
		while (*cp != ',') cp--;	// going backward to locate country names
		cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntryname, MAX_COUNTRY_NAME, L"usa");
		else
		{
			if (wcscmp(cp, L"fed rep ger") == 0)	// special case, added 2013/04/16
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"germany");
			else if (wcscmp(cp, L"czechoslovakia") == 0)	// special case, added 2013/04/23
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"czech republic");
			else if (wcscmp(cp, L"italia") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"italy");
			else
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
		}
		ind2 = location_search(location, NLOCS, cntryname);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nERROR: In WOS:%s, C1, no country found for \"%s\"\n", wos[wosi].docid, cntryname);
			ind2 = 1;	// points to "[unknonwn country]"
		}
		wos[wosi].countries[wos[wosi].ncountries++] = ind2;
		if (situation == ADDRESS_MATCH)	// one to one assignment
		{
			aind = wos[wosi].author[acnt++];
			authors[aind].countries[authors[aind].ncountries] = ind2;
			authors[aind].country_cnt[authors[aind].ncountries] = 1;	// set to 1 here, will change in the later code
			authors[aind].ncountries++;
		}
		else if (situation == ADDRESS_ALL_IN_ONE && first_time == TRUE)	// one to all assignment
		{
			for (m = 0; m < wos[wosi].nau; m++)
			{
				aind = wos[wosi].author[acnt++];
				authors[aind].countries[authors[aind].ncountries] = ind2;
				authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
				authors[aind].ncountries++;
			}
		}
		else if (situation == ADDRESS_MULTIPLE_BUT_SAME_COUNTRY && first_time == TRUE)	// one to all assignment
		{
			for (m = 0; m < wos[wosi].nau; m++)
			{
				aind = wos[wosi].author[acnt++];
				authors[aind].countries[authors[aind].ncountries] = ind2;
				authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
				authors[aind].ncountries++;
			}
		}
		else if (first_time == TRUE)	// cannot resovle addresses<==>authors corrspondence, added 2015/10/16
		{
			for (m = 0; m < wos[wosi].nau; m++)
			{
				aind = wos[wosi].author[acnt++];
				authors[aind].countries[authors[aind].ncountries] = 0;	// use "[VAGUE]" as country name
				authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
				authors[aind].ncountries++;
			}
		}
	}

#ifdef XXX
	if (acnt < wos[wosi].nau)
		fwprintf(logstream, L"{%s} ", cp);
	fwprintf(logstream, L"\n");
	if (acnt != wos[wosi].nau)
		fwprintf(logstream, L"|%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);
#endif

	return 0;
}

//
// given a WOS "C1" string, find out how many author addresses are given
// return the situation (the number of authors and the number of addresses)
//
int preparse_countries(int wosi, wchar_t *cpstr)
{
	int i;
	wchar_t ch, *sp, *tp, *cp;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t cntryname[MAX_COUNTRY_NAME];
	int tncntry;
	int ind, ind2;
	int state;	
	int acnt;	// address count
	int cinds[MAX_COUNTRIES];
	int same;

	state = OUTOFF_SQUARE_BRACKET;
	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps; acnt = 0;
	while (*sp != '\0')
	{
		switch (state)
		{
		case IN_SQUARE_BRACKET:
			if (*sp == ']')
				state = OUTOFF_SQUARE_BRACKET;
			sp++;
			break;
		case OUTOFF_SQUARE_BRACKET:
			ch = *sp; 
			if (ch == '[')
			{
				state = IN_SQUARE_BRACKET;
				sp++;
				break;
			}
			if (ch == ';') 
			{ 
				*tp++ = '\0'; sp++; 
				if (tmps[0] != '\0')
				{				
					cp = tp - 1;
					if (*cp == '\"') // ignore the double quote at the end of the string
					{
						*cp = '\0'; cp--; 
						if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
					}
					else if (*cp == '.')
						*cp = '\0';	// ignore the '.' at the end of the string
					while (*cp != ',') cp--;	// going backward to locate country names
					cp++; while (*cp == ' ') cp++;
					i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
					if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
						swprintf_s(cntryname, MAX_COUNTRY_NAME, L"usa");
					else
					{
						if (wcscmp(cp, L"fed rep ger") == 0)	// special case, added 2013/04/16
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"germany");
						else if (wcscmp(cp, L"czechoslovakia") == 0)	// special case, added 2013/04/23
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"czech republic");
						else if (wcscmp(cp, L"italia") == 0)	
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"italy");
						else
							wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
					}
					//ind2 = countryname_search(cntry, ncntry, cntryname);
					ind2 = location_search(location, NLOCS, cntryname);
					if (ind2 == -1)
					{
						fwprintf(logstream, L"\nERROR: In WOS:%s, C1, no country found for \"%s\"\n", wos[wosi].docid, cntryname);
						ind2 = 1;	// points to "[unknonwn country]"
					}
					cinds[acnt++] = ind2;
				}
				tp = tmps;
				while (*sp == ' ') sp++;
			}
			else 
				*tp++ = towlower(*sp++); 
			break;
		default:
			break;
		}
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
		while (*cp != ',') cp--;	// going backward to locate country names
		cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntryname, MAX_COUNTRY_NAME, L"usa");
		else
		{
			if (wcscmp(cp, L"fed rep ger") == 0)	// special case, added 2013/04/16
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"germany");
			else if (wcscmp(cp, L"czechoslovakia") == 0)	// special case, added 2013/04/23
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"czech republic");
			else if (wcscmp(cp, L"italia") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"italy");
			else
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
		}
		//ind2 = countryname_search(cntry, ncntry, cntryname);
		ind2 = location_search(location, NLOCS, cntryname);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nERROR: In WOS:%s, C1, no country found for \"%s\"\n", wos[wosi].docid, cntryname);
			ind2 = 1;	// points to "[unknonwn country]"
		}
		cinds[acnt++] = ind2;
	}

	if (acnt == 0)
		return ADDRESS_NOT_GIVEN;
	else if (acnt == wos[wosi].nau)
		return ADDRESS_MATCH;
	else if (acnt > wos[wosi].nau)
	{
		same = TRUE;
		for (i = 1; i < acnt; i++) { if (cinds[0] != cinds[i]) { same = FALSE; break; } }
		if (same == TRUE)
			return ADDRESS_MULTIPLE_BUT_SAME_COUNTRY;
		else
		{
			//fwprintf(logstream, L"*** |%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);			
			return ADDRESS_VAGUE;
		}
	}
	else if (acnt < wos[wosi].nau)
	{
		if (acnt == 1)
			return ADDRESS_ALL_IN_ONE;
		else
		{
			same = TRUE;
			for (i = 1; i < acnt; i++) { if (cinds[0] != cinds[i]) { same = FALSE; break; } }
			if (same == TRUE)
				return ADDRESS_MULTIPLE_BUT_SAME_COUNTRY;
			else 
			{
				//fwprintf(logstream, L"### |%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);
				return ADDRESS_VAGUE;
			}
		}
	}
}

//
// given a Scopus "Correspondence Address" string, assign the countries to authors
//
int parse_countries_rp_Scopus(int wosi, wchar_t *cpstr, int naus, struct AUTHORS *au)
{
	int i, m;
	wchar_t *sp, *tp, *cp, *ap;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t cntryname[MAX_COUNTRY_NAME];
	wchar_t aname[MAX_AUTHOR_NAME];
	int ind, ind2;
	int aind, ndx2;	// author index
	int nscomma;

	// special case, the address string is a null
	if (*cpstr == '\0')	
	{
		wos[wosi].countries_rp[wos[wosi].ncountries_rp++] = 1;	// use "[unknown country]" as the country name
		return 0;
	}

	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	tp = tmps; ap = aname; nscomma = 0;
	while (*sp != '\0' && wcsncmp(sp, L"; email:", 8) != 0)	// end of string or "; email:"
	{
		*tp = towlower(*sp); 
		if (*tp == ';' && nscomma == 0)	// encountering the 1st ';'
		{
			nscomma++;
			*ap = '\0'; 
			ap--; while (*ap == ' ') ap--;	// remove the spaces at the end
			ap++; *ap = '\0';
		}
		else if (*tp != '.' && nscomma == 0)	// skip the '.' in author name
			*ap++ = towlower(*sp);
		tp++; sp++;
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
		while (*cp != ',' && cp != tmps) cp--;	// going backward to locate country names
		if (cp != tmps) cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntryname, MAX_COUNTRY_NAME, L"united states");
		else
		{
			if (wcscmp(cp, L"usa") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united states");
			else if (wcscmp(cp, L"uk") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united kingdom");
			else if (wcscmp(cp, L"r.o.c") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"republic of china") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"taipei") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"beijing") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"p. r. china") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"macau") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"russian federation") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"russia");
			else
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
		}
		ind2 = location_search(location3, NLOCS3, cntryname);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nERROR: In Scopus: %s, \"Correspondence Address\", no country found for \"%s\"\n", wos[wosi].docid, cntryname);
			ind2 = 1;	// points to "[unknonwn country]"
		}
		wos[wosi].countries_rp[wos[wosi].ncountries_rp++] = ind2;
		// need to handle special names here, i.e., remove spaces in a double surname
		adjust_author_last_name(aname);	
		ndx2 = authoralias_search(authoralias, nauthoralias, aname);	
		if (ndx2 >= 0)	// found that this is a variation of the author name
			wcscpy_s(aname, MAX_AUTHOR_NAME, authoralias[ndx2].alias);
		aind = aname_search(au, naus, aname);
		if (aind != -1 && ind2 != -1)		// for unknow author name, most likely because of database error, ignore
		{
			authors[aind].countries_rp[authors[aind].ncountries_rp] = ind2;
			authors[aind].country_cnt_rp[authors[aind].ncountries_rp] = 1;	// set to 1 here, will change in the later code
			authors[aind].ncountries_rp++;
		}
		else 
			fwprintf(logstream, L"Warning: In Scopus %s, \"Correspondence Address\", unknown author name [%s]\n", wos[wosi].docid, aname);
	}

#ifdef XXX
	if (acnt < wos[wosi].nau)
		fwprintf(logstream, L"{%s} ", cp);
	fwprintf(logstream, L"\n");
	if (acnt != wos[wosi].nau)
		fwprintf(logstream, L"|%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);
#endif

	return 0;
}

//
// given a Scopus "Authors with affiliations" string, assign the countries to authors
//
int parse_countries_Scopus(int wosi, wchar_t *cpstr, int naus, struct AUTHORS *au)
{
	int i, m;
	wchar_t ch, *sp, *tp, *cp;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t cntryname[MAX_COUNTRY_NAME];
	int ind, ind2;
	int acnt;	// address count
	int aind;	// author index
	int situation, first_time;
	int parenthesis;

	// special case, the address string is a null
	if (*cpstr == '\0')	
	{
		wos[wosi].countries[wos[wosi].ncountries++] = 1;	// use "[unknown country]" as the country name
		acnt = 0;
		for (m = 0; m < wos[wosi].nau; m++)
		{
			aind = wos[wosi].author[acnt++];
			authors[aind].countries[authors[aind].ncountries] = 1;	// use "[unknown country]" as the country name
			authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
			authors[aind].ncountries++;
		}
		return 0;
	}

	situation = preparse_countries_Scopus(wosi, cpstr);

	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps; acnt = 0; first_time = TRUE; parenthesis = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (parenthesis == 0)	// added 2017/03/24
		{
			if (ch == '(') { parenthesis = 1; sp++; continue; }	// ignore '('
		}
		else 
		{
			if (ch == ')') parenthesis = 0;
			sp++; continue;	// ignore characters in the parenthesis
		}
		if (ch == ';') 
		{ 
			*tp = '\0'; sp++; 
			if (tmps[0] != '\0')
			{				
				cp = tp - 1;
				if (*cp == '\"') // ignore the double quote at the end of the string
				{
					*cp = '\0'; cp--; 
					if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
				}
				else if (*cp == '.' && cp != tmps)
					*cp = '\0';	// ignore the '.' at the end of the string
				while (*cp != ',') cp--;	// going backward to locate country names
				if (cp != tmps) cp++; while (*cp == ' ') cp++;
				i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
				if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
					swprintf_s(cntryname, MAX_COUNTRY_NAME, L"united states");
				else
				{
					if (wcscmp(cp, L"usa") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united states");
					else if (wcscmp(cp, L"uk") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united kingdom");
					else if (wcscmp(cp, L"r.o.c") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
					else if (wcscmp(cp, L"republic of china") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
					else if (wcscmp(cp, L"taipei") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
					else if (wcscmp(cp, L"beijing") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
					else if (wcscmp(cp, L"p. r. china") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
					else if (wcscmp(cp, L"macau") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
					else if (wcscmp(cp, L"russian federation") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"russia");
					else
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
				}
				ind2 = location_search(location3, NLOCS3, cntryname);
				if (ind2 == -1)
				{
					fwprintf(logstream, L"\nERROR: In Scopus:%s, \"Authors with affiliations\", no country found for \"%s\"\n", wos[wosi].docid, cntryname);
					ind2 = 1;	// points to "[unknonwn country]"
				}
				wos[wosi].countries[wos[wosi].ncountries++] = ind2;
				if (situation == ADDRESS_MATCH)	// one to one assignment
				{
					aind = wos[wosi].author[acnt++];
					authors[aind].countries[authors[aind].ncountries] = ind2;
					authors[aind].country_cnt[authors[aind].ncountries] = 1;	// set to 1 here, will change in the later code
					authors[aind].ncountries++;
				}
				else if (situation == ADDRESS_ALL_IN_ONE && first_time == TRUE)	// one to all assignment
				{
					for (m = 0; m < wos[wosi].nau; m++)
					{
						aind = wos[wosi].author[acnt++];
						authors[aind].countries[authors[aind].ncountries] = ind2;
						authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
						authors[aind].ncountries++;
					}
					first_time = FALSE;
				}
				else if (situation == ADDRESS_MULTIPLE_BUT_SAME_COUNTRY && first_time == TRUE)	// one to all assignment
				{
					for (m = 0; m < wos[wosi].nau; m++)
					{
						aind = wos[wosi].author[acnt++];
						authors[aind].countries[authors[aind].ncountries] = ind2;
						authors[aind].country_cnt[authors[aind].ncountries] = 1;	// set to 1 here, will change in the later code
						authors[aind].ncountries++;
					}
					first_time = FALSE;
				}
				else if (first_time == TRUE)	// cannot resovle addresses<==>authors corrspondence, added 2015/10/16
				{
					for (m = 0; m < wos[wosi].nau; m++)
					{
						aind = wos[wosi].author[acnt++];
						authors[aind].countries[authors[aind].ncountries] = 0;	// use "[VAGUE]" as country name
						authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
						authors[aind].ncountries++;
					}
					first_time = FALSE;
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
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
		while (*cp != ',' && cp != tmps) cp--;	// going backward to locate country names
		if (cp != tmps) cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntryname, MAX_COUNTRY_NAME, L"united states");
		else
		{
			if (wcscmp(cp, L"usa") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united states");
			else if (wcscmp(cp, L"uk") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united kingdom");
			else if (wcscmp(cp, L"r.o.c") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"republic of china") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"taipei") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"beijing") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"p. r. china") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"macau") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"russian federation") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"russia");
			else
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
		}
		ind2 = location_search(location3, NLOCS3, cntryname);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nERROR: In Scopus:%s, \"Authors with affiliations\", no country found for \"%s\"\n", wos[wosi].docid, cntryname);
			ind2 = 1;	// points to "[unknonwn country]"
		}
		wos[wosi].countries[wos[wosi].ncountries++] = ind2;
		if (situation == ADDRESS_MATCH)	// one to one assignment
		{
			aind = wos[wosi].author[acnt++];
			authors[aind].countries[authors[aind].ncountries] = ind2;
			authors[aind].country_cnt[authors[aind].ncountries] = 1;	// set to 1 here, will change in the later code
			authors[aind].ncountries++;
		}
		else if (situation == ADDRESS_ALL_IN_ONE && first_time == TRUE)	// one to all assignment
		{
			for (m = 0; m < wos[wosi].nau; m++)
			{
				aind = wos[wosi].author[acnt++];
				authors[aind].countries[authors[aind].ncountries] = ind2;
				authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
				authors[aind].ncountries++;
			}
		}
		else if (situation == ADDRESS_MULTIPLE_BUT_SAME_COUNTRY && first_time == TRUE)	// one to all assignment
		{
			for (m = 0; m < wos[wosi].nau; m++)
			{
				aind = wos[wosi].author[acnt++];
				authors[aind].countries[authors[aind].ncountries] = ind2;
				authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
				authors[aind].ncountries++;
			}
		}
		else if (first_time == TRUE)	// cannot resovle addresses<==>authors corrspondence, added 2015/10/16
		{
			for (m = 0; m < wos[wosi].nau; m++)
			{
				aind = wos[wosi].author[acnt++];
				authors[aind].countries[authors[aind].ncountries] = 0;	// use "[VAGUE]" as country name
				authors[aind].country_cnt[authors[aind].ncountries] = 1;// set to 1 here, will change in the later code
				authors[aind].ncountries++;
			}
		}
	}

#ifdef XXX
	if (acnt < wos[wosi].nau)
		fwprintf(logstream, L"{%s} ", cp);
	fwprintf(logstream, L"\n");
	if (acnt != wos[wosi].nau)
		fwprintf(logstream, L"|%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);
#endif

	return 0;
}

//
// given a Scopus "Authors with affiliations" string, find out how many author addresses are given
// return the situation (the number of authors and the number of addresses)
//
int preparse_countries_Scopus(int wosi, wchar_t *cpstr)
{
	int i;
	wchar_t ch, *sp, *tp, *cp;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t cntryname[MAX_COUNTRY_NAME];
	int tncntry;
	int ind, ind2;
	int acnt;	// address count
	int cinds[MAX_COUNTRIES];
	int same;
	int parenthesis;

	sp = cpstr;
	// remove the leading double quotes
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps; acnt = 0; parenthesis = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		if (parenthesis == 0)	// added 2017/03/24
		{
			if (ch == '(') { parenthesis = 1; sp++; continue; }	// ignore '('
		}
		else 
		{
			if (ch == ')') parenthesis = 0;
			sp++; continue;	// ignore characters in the parenthesis
		}
		if (ch == ';') 
		{ 
			*tp = '\0'; sp++; 
			if (tmps[0] != '\0')
			{				
				cp = tp - 1;
				if (*cp == '\"') // ignore the double quote at the end of the string
				{
					*cp = '\0'; cp--; 
					if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
				}
				else if (*cp == '.')
					*cp = '\0';	// ignore the '.' at the end of the string
				while (*cp != ',' && cp != tmps) cp--;	// going backward to locate country names
				if (cp != tmps) cp++; while (*cp == ' ') cp++;
				i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
				if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
					swprintf_s(cntryname, MAX_COUNTRY_NAME, L"united states");
				else
				{
					if (wcscmp(cp, L"usa") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united states");
					else if (wcscmp(cp, L"uk") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united kingdom");
					else if (wcscmp(cp, L"r.o.c") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
					else if (wcscmp(cp, L"republic of china") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
					else if (wcscmp(cp, L"taipei") == 0)	// special case
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
					else if (wcscmp(cp, L"beijing") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
					else if (wcscmp(cp, L"p. r. china") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
					else if (wcscmp(cp, L"macau") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
					else if (wcscmp(cp, L"russian federation") == 0)	
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"russia");
					else
						wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
				}
				ind2 = location_search(location3, NLOCS3, cntryname);
				if (ind2 == -1)
				{
					fwprintf(logstream, L"\nERROR: In Scopus:%s, \"Authors with affiliations\", no country found for \"%s\"\n", wos[wosi].docid, cntryname);
					ind2 = 1;	// points to "[unknonwn country]"
				}
				cinds[acnt++] = ind2;
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
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
		while (*cp != ',' && cp != tmps) cp--;	// going backward to locate country names
		if (cp != tmps) cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = cp[i]; i++; } leads[i] = '\0';
		if ((ind = US_state_search(usstates, N_USSTATES, leads)) != -1)
			swprintf_s(cntryname, MAX_COUNTRY_NAME, L"united states");
		else
		{
			if (wcscmp(cp, L"usa") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united states");
			else if (wcscmp(cp, L"uk") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"united kingdom");
			else if (wcscmp(cp, L"r.o.c") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"republic of china") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"taipei") == 0)	// special case
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"taiwan");
			else if (wcscmp(cp, L"beijing") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"p. r. china") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"macau") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"china");
			else if (wcscmp(cp, L"russian federation") == 0)	
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, L"russia");
			else
				wcscpy_s(cntryname, MAX_COUNTRY_NAME, cp);
		}
		ind2 = location_search(location3, NLOCS3, cntryname);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nERROR: In Scopus:%s, \"Authors with affiliations\", no country found for \"%s\"\n", wos[wosi].docid, cntryname);
			ind2 = 1;	// points to "[unknonwn country]"
		}
		cinds[acnt++] = ind2;
	}

	if (acnt == 0)
		return ADDRESS_NOT_GIVEN;
	else if (acnt == wos[wosi].nau)
		return ADDRESS_MATCH;
	else if (acnt > wos[wosi].nau)
	{
		same = TRUE;
		for (i = 1; i < acnt; i++) { if (cinds[0] != cinds[i]) { same = FALSE; break; } }
		if (same == TRUE)
			return ADDRESS_MULTIPLE_BUT_SAME_COUNTRY;
		else
		{
			//fwprintf(logstream, L"*** |%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);			
			return ADDRESS_VAGUE;
		}
	}
	else if (acnt < wos[wosi].nau)
	{
		if (acnt == 1)
			return ADDRESS_ALL_IN_ONE;
		else
		{
			same = TRUE;
			for (i = 1; i < acnt; i++) { if (cinds[0] != cinds[i]) { same = FALSE; break; } }
			if (same == TRUE)
				return ADDRESS_MULTIPLE_BUT_SAME_COUNTRY;
			else 
			{
				//fwprintf(logstream, L"### |%d, %d| %s\n", wos[wosi].nau, acnt, cpstr);
				return ADDRESS_VAGUE;
			}
		}
	}
}

//
// given a TIP "Assignee - Original w/address" string, assign the countries to each assignee
//
int parse_countries_TIP(int ndx, wchar_t *cpstr, int nasgn, struct ASSIGNEES *asgn)
{
	int i, m;
	wchar_t ch, *sp, *tp, *cp;
	wchar_t tmps[XLBUF_SIZE];
	wchar_t leads[100];
	wchar_t statename[3];
	int ind, ind2, andx;
	int state;
	int acnt;	// address count
	int aind;	// author index

	// special case, the address string is a null
	if (*cpstr == '\0')	
	{
		uspto[ndx].ncountries = 1;
		uspto[ndx].countries[uspto[ndx].ncountries] = NLOCS2 - 1;	// use "[unknown country]" as the country name
		uspto[ndx].nareas = 0;
		acnt = 0;
		for (m = 0; m < uspto[ndx].nassignee; m++)
		{
			aind = uspto[ndx].assignee[acnt++];
			asgn[aind].countries[asgn[aind].ncountries] = NLOCS2 - 1;	// use "[unknown country]" as the country name
			asgn[aind].country_cnt[asgn[aind].ncountries] = 1;// set to 1 here, will change in the later code
			asgn[aind].ncountries++;
		}
		return 0;
	}

	sp = cpstr;
	// remove the leading double quotes if there is one
	while (*sp == '\"') sp++;

	// start parsing
	tp = tmps; acnt = 0; 
	while (*sp != L'\0' && *sp != L'\r' && *sp != L'\n')
	{
		ch = *sp; 
		if (ch == '|')	// hit the separator
		{ 
			*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
			sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
			if (tmps[0] != L'\0')
			{
				if (acnt >= MAX_ASSIGNEES) return 0;	// added 2018/03/13
				andx = aalias_search(aalias, naalias, tmps);
				if (andx == -1)	// the assignee name is not be in the alias table
				{
					//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
					// do nothing
				}
				else
				{
					wcscpy_s(tmps, MAX_ASSIGNEE_NAME, aalias[andx].alias);
					tp = tmps; while (*tp != '\0') tp++;
				}
				cp = tp - 1;
				if (*cp == '\"') // ignore the double quote at the end of the string if there is one
				{
					*cp = '\0'; cp--; 
					if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
				}
				else if (*cp == '.')
					*cp = '\0';	// ignore the '.' at the end of the string if there is one
				cp--; if (*cp == ' ') *cp = '\0';
				while (*cp != ',' && *cp != ' ') cp--;	// going backward to locate country names
				cp++; while (*cp == ' ') cp++;
				i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = toupper(cp[i]); i++; } leads[i] = '\0';
				//fwprintf(logstream, L"!!!!!%d [%s][%s]\n", ndx, tmps, leads);
				ind2 = location_search(location2, NLOCS2, leads);
				if (ind2 == -1)
				{
					fwprintf(logstream, L"\nWARNING: In TIP:%s, C1, no country found for \"%s\"\n", uspto[ndx].pid, leads);
					ind2 = NLOCS2 - 1;	// points to "[unknonwn country]"
				}
				uspto[ndx].countries[uspto[ndx].ncountries++] = ind2;
				aind = uspto[ndx].assignee[acnt++];
				asgn[aind].countries[asgn[aind].ncountries] = ind2;
				asgn[aind].country_cnt[asgn[aind].ncountries] = 1;	// set to 1 here, will be changed in the later code
				asgn[aind].ncountries++;
				if (wcscmp(leads, L"US") == 0)	// if it is an US address, find the state information
				{
					cp--; // remember that cp was pointing to "US"
					cp--; if (*cp == ' ') cp--;
					cp--;
					statename[0] = cp[0]; statename[1] = cp[1]; statename[2] = '\0';
					//fwprintf(logstream, L"@@@@@ [%s]\n", statename);
					ind = US_state_search(usstates, N_USSTATES, statename);
					if (ind == -1)
					{
						fwprintf(logstream, L"\nWARNING: In TIP:%s, unknown US state name [%s]\n", uspto[ndx].pid, statename);
						ind = N_USSTATES - 1;	// points to "[unknonwn state]"
					}
					uspto[ndx].areas[uspto[ndx].nareas] = ind;
					uspto[ndx].area_cnt[uspto[ndx].nareas] = 1;	// set to 1 here, will be changed in the later code
					uspto[ndx].nareas++;					
					asgn[aind].areas[asgn[aind].nareas] = ind;
					asgn[aind].area_cnt[asgn[aind].nareas] = 1;	// set to 1 here, will be changed in the later code
					asgn[aind].nareas++;			
				}
			}
			tp = tmps;
			while (*sp == ' ') sp++;
		}
		else 
			*tp++ = towlower(*sp++); 
	}
	*tp = L'\0'; tp--; if (*tp == ' ') *tp = L'\0';	// ignore the space after the delimiter
	sp++; if (*sp == ' ') sp++;	// skip the space after the delimiter
	if (tmps[0] != L'\0')
	{
		if (acnt >= MAX_ASSIGNEES) return 0;	// added 2018/03/13
		andx = aalias_search(aalias, naalias, tmps);
		if (andx == -1)	// the assignee name is not be in the alias table
		{
			//fwprintf(logstream, L"[%d %s]\n", ndx, tmps); fflush(logstream);
			// do nothing
		}
		else
		{
			wcscpy_s(tmps, MAX_ASSIGNEE_NAME, aalias[andx].alias);
			tp = tmps; while (*tp != '\0') tp++;
		}
		cp = tp - 1;
		if (*cp == '\"') // ignore the double quote at the end of the string
		{
			*cp = '\0'; cp--; 
			if (*cp == '.')	*cp = '\0';	// ignore the '.' as well
		}
		else if (*cp == '.')
			*cp = '\0';	// ignore the '.' at the end of the string
				cp--; if (*cp == ' ') *cp = '\0';
		while (*cp != ',' && *cp != ' ') cp--;	// going backward to locate country names
		cp++; while (*cp == ' ') cp++;
		i = 0; while (cp[i] != ' ' && cp[i] != '\0') { leads[i] = toupper(cp[i]); i++; } leads[i] = '\0';
		//fwprintf(logstream, L"!!!!!%d [%s][%s]\n", ndx, tmps, leads);
		ind2 = location_search(location2, NLOCS2, leads);
		if (ind2 == -1)
		{
			fwprintf(logstream, L"\nWARNING: In TIP:%s, C1, no country found for \"%s\"\n", uspto[ndx].pid, leads);
			ind2 = NLOCS2 - 1;	// points to "[unknonwn country]"
		}
		uspto[ndx].countries[uspto[ndx].ncountries++] = ind2;
		aind = uspto[ndx].assignee[acnt++];
		asgn[aind].countries[asgn[aind].ncountries] = ind2;
		asgn[aind].country_cnt[asgn[aind].ncountries] = 1;	// set to 1 here, will be changed in the later code
		asgn[aind].ncountries++;
		if (wcscmp(leads, L"US") == 0)	// if it is an US address, find the state information
		{
			cp--; // remember that cp was pointing to "US"
			cp--; if (*cp == ' ') cp--;
			cp--;
			statename[0] = cp[0]; statename[1] = cp[1]; statename[2] = '\0';
			//fwprintf(logstream, L"@@@@@ [%s]\n", statename);
			ind = US_state_search(usstates, N_USSTATES, statename);
			if (ind == -1)
			{
				fwprintf(logstream, L"\nWARNING: In TIP:%s, unknown US state name [%s]\n", uspto[ndx].pid, statename);
				ind = N_USSTATES - 1;	// points to "[unknonwn state]"
			}
			uspto[ndx].areas[uspto[ndx].nareas] = ind;
			uspto[ndx].area_cnt[uspto[ndx].nareas] = 1;	// set to 1 here, will be changed in the later code
			uspto[ndx].nareas++;					
			asgn[aind].areas[asgn[aind].nareas] = ind;
			asgn[aind].area_cnt[asgn[aind].nareas] = 1;	// set to 1 here, will be changed in the later code
			asgn[aind].nareas++;		
		};
	}

	return 0;
}
//
// given a WEBPAT3 "專利權人國家" string, assign the countries to each assignee
// WARNING!!! this function needs to be improved to handle multiple assignees, such "TW;US"
//
int parse_countries_WEBPAT3(int ndx, wchar_t *cpstr, int nasgn, struct ASSIGNEES *asgn)
{
	int i, m;
	wchar_t ch, *sp, *tp, *cp;
	int ind, ind2, andx;
	int acnt;	// address count
	int aind;	// author index

	// special case, the address string is a null
	if (*cpstr == '\0')	
	{
		wcscpy(uspto[ndx].country, L"--");
		uspto[ndx].ncountries = 1;
		uspto[ndx].countries[uspto[ndx].ncountries] = NLOCS2 - 1;	// use "[unknown country]" as the country name
		uspto[ndx].nareas = 0;
		acnt = 0;
		for (m = 0; m < uspto[ndx].nassignee; m++)
		{
			aind = uspto[ndx].assignee[acnt++];
			asgn[aind].countries[asgn[aind].ncountries] = NLOCS2 - 1;	// use "[unknown country]" as the country name
			asgn[aind].country_cnt[asgn[aind].ncountries] = 1;// set to 1 here, will change in the later code
			asgn[aind].ncountries++;
		}
		return 0;
	}

	wcscpy(uspto[ndx].country, cpstr);
	ind2 = location_search(location2, NLOCS2, cpstr);
	if (ind2 == -1)
	{
		fwprintf(logstream, L"\nWARNING: In WEBPAT3: %s, 專利權人國家, no country found for \"%s\"\n", uspto[ndx].pid, cpstr);
		ind2 = NLOCS2 - 1;	// points to "[unknonwn country]"
	}

	acnt = 0;
	uspto[ndx].countries[uspto[ndx].ncountries++] = ind2;
	aind = uspto[ndx].assignee[acnt++];
	asgn[aind].countries[asgn[aind].ncountries] = ind2;
	asgn[aind].country_cnt[asgn[aind].ncountries] = 1;	// set to 1 here, will be changed in the later code
	asgn[aind].ncountries++;
	if (wcscmp(cpstr, L"US") == 0)	// if it is an US address, thers is not state information to be found
	{
		uspto[ndx].areas[uspto[ndx].nareas] = ind;
		uspto[ndx].area_cnt[uspto[ndx].nareas] = 1;	// set to 1
		uspto[ndx].nareas++;					
		asgn[aind].areas[asgn[aind].nareas] = ind;
		asgn[aind].area_cnt[asgn[aind].nareas] = 1;	
		asgn[aind].nareas++;		
	}

	return 0;
}

//
// use binary search to find the proper position of a US state abbreviation
//
int US_state_search(struct USSTATES d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].abbr) < 0)
		return -1;
	if (wcscmp(d[N_USSTATES-1].abbr, str) < 0)	// added 2012/11/21
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].abbr) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].abbr) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].abbr) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search to find the proper position of a country in a COUNTRIES array
//
int countryname_search(struct COUNTRIES d[], int num, wchar_t *str)
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
int compare_countryx(const void *n1, const void *n2)
{
	struct COUNTRY_IND *t1, *t2;
	
	t1 = (struct COUNTRY_IND *)n1;
	t2 = (struct COUNTRY_IND *)n2;
	if (t2->ind < t1->ind)
		return 1;
	else if (t2->ind == t1->ind)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_countryname(const void *n1, const void *n2)
{
	struct COUNTRIES *t1, *t2;
	
	t1 = (struct COUNTRIES *)n1;
	t2 = (struct COUNTRIES *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// find the most significant country for each patent
// added 2016/12/28
//
int find_significant_country_patent(int nuspto, struct USPTO *uspto)
{
	int i, k, max, kmax, max_cnt;

	for (i = 0; i < nuspto; i++)
	{	
		// 1st pass
		max = 0; kmax = -1; 
		//fwprintf(logstream, L"##### %s %d:\t", uspto[i].pid, uspto[i].ncountries); fflush(logstream);
		for (k = 0; k < uspto[i].ncountries; k++)
		{
			//fwprintf(logstream, L"%s ", location2[uspto[i].countries[k]].name); fflush(logstream);
			if (uspto[i].country_cnt[k] >= max)
			{
				max = uspto[i].country_cnt[k];
				kmax = k;
			}
		}
		//fwprintf(logstream, L"\n"); fflush(logstream);
		max_cnt = 0;
		// find out the number of countries that have the same miximum count
		for (k = 0; k < uspto[i].ncountries; k++)		
			if (uspto[i].country_cnt[k] == max) max_cnt++;
		uspto[i].location_status = max_cnt;
		if (kmax == -1)
		{
			uspto[i].location = -1;
			continue;
		}

		// 2nd pass, make the decision, ignore [VAGUE] and [unknown country] unless they are the only information
		if (uspto[i].ncountries == 1 && (uspto[i].countries[kmax] == 0 || uspto[i].countries[kmax] == NLOCS2-1)) // 0=>[VAGUE], NLOCS2-1=>[unknown country]
		{
			uspto[i].location = uspto[i].countries[kmax];
			continue;
		}
		max = 0; 
		for (k = 0; k < uspto[i].ncountries; k++)
		{
			if (uspto[i].countries[k] == 0 || uspto[i].countries[k] == NLOCS2 - 1)	// ignore [VAGUE] and [unknown country]
				continue;
			if (uspto[i].country_cnt[k] >= max)
			{
				max = uspto[i].country_cnt[k];
				kmax = k;
			}
		}
		uspto[i].location = uspto[i].countries[kmax];
	}

	return 0;
}

//
// find the most significant area (under each country) for each patent
// added 2016/12/28
//
int find_significant_area_patent(int nuspto, struct USPTO *uspto)
{
	int i, k, max, kmax, max_cnt;

	for (i = 0; i < nuspto; i++)
	{	
		// 1st pass
		max = 0; kmax = -1; 
		for (k = 0; k < uspto[i].nareas; k++)
		{
			if (uspto[i].area_cnt[k] >= max)
			{
				max = uspto[i].area_cnt[k];
				kmax = k;
			}
		}
		max_cnt = 0;
		// find out the number of countries that have the same miximum count
		for (k = 0; k < uspto[i].nareas; k++)		
			if (uspto[i].area_cnt[k] == max) max_cnt++;
		uspto[i].location_area_status = max_cnt;
		if (kmax == -1)
		{
			uspto[i].location_area = -1;
			continue;
		}

		// 2nd pass, make the decision, ignore [unknown state] unless they are the only information
		if (uspto[i].nareas == 1 && uspto[i].areas[kmax] == N_USSTATES-1) // unknown state
		{
			uspto[i].location_area = uspto[i].areas[kmax];
			continue;
		}
		max = 0; 
		for (k = 0; k < uspto[i].nareas; k++)
		{
			if (uspto[i].areas[k] == N_USSTATES-1)	// [unknown state]
				continue;
			if (uspto[i].area_cnt[k] >= max)
			{
				max = uspto[i].area_cnt[k];
				kmax = k;
			}
		}
		uspto[i].location_area = uspto[i].areas[kmax];
	}

	return 0;
}

//
// find the most significant country for each assignee
// added 2016/12/23
//
int find_significant_country_assignee(int nasgns, struct ASSIGNEES *assignees)
{
	int i, k, max, kmax, max_cnt;

	for (i = 0; i < nasgns; i++)
	{	
		// 1st pass
		max = 0; kmax = -1; 
		for (k = 0; k < assignees[i].ncountries; k++)
		{
			if (assignees[i].country_cnt[k] >= max)
			{
				max = assignees[i].country_cnt[k];
				kmax = k;
			}
		}
		max_cnt = 0;
		// find out the number of countries that have the same miximum count
		for (k = 0; k < assignees[i].ncountries; k++)		
			if (assignees[i].country_cnt[k] == max) max_cnt++;
		assignees[i].location_status = max_cnt;
		if (kmax == -1)
		{
			assignees[i].location = NLOCS2-1;
			continue;
		}

		// 2nd pass, make the decision, ignore [VAGUE] and [unknown country] unless they are the only information
		if (assignees[i].ncountries == 1 && (assignees[i].countries[kmax] == 0 || assignees[i].countries[kmax] == NLOCS2-1)) // 0=>[VAGUE]
		{
			assignees[i].location = assignees[i].countries[kmax];
			continue;
		}
		max = 0; 
		for (k = 0; k < assignees[i].ncountries; k++)
		{
			if (assignees[i].countries[k] == 0 || assignees[i].countries[k] == NLOCS2-1)	// ignore [VAGUE] and [unknown country]
				continue;
			if (assignees[i].country_cnt[k] >= max)
			{
				max = assignees[i].country_cnt[k];
				kmax = k;
			}
		}
		assignees[i].location = assignees[i].countries[kmax];
	}

	return 0;
}

//
// find the most significant area (under each country) for each assignee
// added 2016/12/23
//
int find_significant_area_assignee(int nasgns, struct ASSIGNEES *assignees)
{
	int i, k, max, kmax, max_cnt;

	for (i = 0; i < nasgns; i++)
	{	
		// 1st pass
		max = 0; kmax = -1; 
		for (k = 0; k < assignees[i].nareas; k++)
		{
			if (assignees[i].area_cnt[k] >= max)
			{
				max = assignees[i].area_cnt[k];
				kmax = k;
			}
		}
		max_cnt = 0;
		// find out the number of countries that have the same miximum count
		for (k = 0; k < assignees[i].nareas; k++)		
			if (assignees[i].area_cnt[k] == max) max_cnt++;
		assignees[i].location_area_status = max_cnt;
		if (kmax == -1)
		{
			assignees[i].location_area = N_USSTATES-1;
			continue;
		}

		// 2nd pass, make the decision, ignore [unknown state] unless they are the only information
		if (assignees[i].nareas == 1 && assignees[i].areas[kmax] == N_USSTATES-1)
		{
			assignees[i].location_area = assignees[i].areas[kmax];
			continue;
		}
		max = 0; 
		for (k = 0; k < assignees[i].nareas; k++)
		{
			if (assignees[i].areas[k] == N_USSTATES-1)	// [unknown state]
				continue;
			if (assignees[i].area_cnt[k] >= max)
			{
				max = assignees[i].area_cnt[k];
				kmax = k;
			}
		}
		assignees[i].location_area = assignees[i].areas[kmax];
	}

	return 0;
}

