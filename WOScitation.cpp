//
// WOScitation.cpp
//

// Revision History:
// 2011/12/04 Modification  : fixed a problem in the function check_citation_info(), so that it can handle the case where there is long string after the page number
// 2012/03/29 Modification  : added code to correct for "DOI DOI xxxx" pattern and multiple DOI cases 
// 2012/08/06 Modification  : added code to handle citation to patents, and citations with special author name (start with a square bracket '[')
// 2012/08/06 Fixed problems: fixed a boundary problem in the function doi_search()
// 2012/10/11 Fixed problems: fixed a parsing CR problem ==> ',' enclosed in square brackets caused problem
// 2012/10/18 Fixed problems: fixed a problem caused by last fix, which caused miss reading of the contents of the CR field
// 2012/10/24 Fixed problems: fixed a problem in parsing CR which contains two brackets, for example, "[Patents ...], Patent [5,364,111]"
//                            handled a special case in CR when DOI contains 'O;2' pattern, in this case ';' should not be treated as a delimiter
// 2012/11/05 Fixed problems: fixed a problem in parsing CR with the format: 
//                            "Boxma O.J., 1989, Messung, Modellierung und Bewertung von Rechensystemen und  Netzen. 5. GI/ITG-Fachtagung. Proceedings (Measurement, Modelling and  Evaluation of Computer Systems and Networks. 5. GI/ITG-Meeting.  Proceedings);"
// 2012/12/11 Fixed problems: fixed a buffer overflow problem by extending the size of author_name[]
// 2012/12/12 Fixed problems: changed all wcscpy() in this file to wcscpy_s() to prevent buffer overflow problems
// 2013/04/16 Modification  : largely improved the capability of the fucntion format_aname(), can handle names in CR field more properly now
// 2013/04/17 Modification  : largely improved the capability of catching DOI in CR (for the case of N/Y/J followed by DOI
// 2013/06/11 Added function: added an argument to the functions parse_citation_info() and match_and_write()
// 2013/06/11 Added function: added to check if an earlier document is citing a later document, if that happens, ignore the citation
// 2013/06/17 Modification  : added code to modify "lundvall, b-a" to "lundvall, ba" as a special case
// 2013/06/21 Modification  : added code to determine if it needs to replace "lundvall, b-a" to "lundvall, ba" automatically
// 2013/06/22 Modification  : remove (as a general case) '-' in the first names, for example, change "lndvall, b-a" to "lundvall, ba"
// 2014/10/31 Modification  : print memory allocation failure status in prepare_for_CR_search()
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "network.h"
#include "citation.h"

extern int nwos;
extern struct WOS *wos;
extern FILE *logstream;

int check_citation_info(wchar_t *, struct CITING_DOC *);
int extract_citation_info1(wchar_t *);
int extract_citation_info2(wchar_t *);
int dalias_search(struct WOS *, int, wchar_t *);
int compare_doc_alias(const void *, const void *);
int compare_doi(const void *, const void *);
int doi_search(struct WOS *, int, wchar_t *);
int compare_1st_author(const void *, const void *);
int format_aname(wchar_t *, wchar_t *);
int name1st_search(struct WOS *, int, wchar_t *);
int match_and_write(struct CITING_DOC, int, wchar_t *, FILE *, wchar_t *);

extern int kname_search(struct KWORDS *, int, wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int jname_search(struct JOURNALS *, int, wchar_t *);
extern int compare_author(const void *, const void *);
extern int compare_journal(const void *, const void *);

extern int naus;
extern struct AUTHORS *authors;	// author name array
extern int nkwde;
extern struct KWORDS *kwde;	// author keywords

struct WOS *dwos;	// a duplicate of "wos", to be sorted by DOI
struct WOS *awos;	// a duplicate of "wos", to be sorted by author name
static int inno_system;		// added 2013/06/21

//
// parse a string of citation information (the "CR" string in WOS database)
// write the result to the given output file stream
//
wchar_t *tpname[6] = {L"JOURNAL", L"PROCEEDING/BOOK", L"REPORT", L"PATENT", L"XAUTHOR", L"OTHER" };
int parse_citation_info(wchar_t *crstr, int iwos, wchar_t *trgtid, FILE *ostream)
{
	int i;
	wchar_t ch, pch, qch, *sp, *tp;
	wchar_t tmps[LBUF_SIZE];
	int ptype;
	struct CITING_DOC cd;

	//fwprintf(logstream, L"**%d\n", wcslen(crstr));
	inno_system = kname_search(kwde, nkwde, L"innovation system");	// added 2013/06/21

	sp = crstr;
	// start parsing
	tp = tmps; ch = '\0';
	while (*sp != '\0')
	{
		pch = ch; sp++; qch = *sp; sp--;
		ch = *sp; 
		// hit the end of a citation? the delimiter is "; " (semi-colon followed by a space)
		// Special case: some DOI contains ';' but they are always followed by a 'O', modified 2012/10/24
		if (ch == ';' && !(pch == 'O' && qch != ' '))
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				ptype = check_citation_info(tmps, &cd);
				//fwprintf(logstream, L"%s\t%d\t%s\t%d\t%s\t%d\t%d\t%s\t%s\n", tpname[cd.type-1], cd.nf, cd.aname, cd.year, cd.jname, cd.volume, cd.page, cd.doi, tmps);
				match_and_write(cd, iwos, trgtid, ostream, tmps);
			}
			tp = tmps;
			while (*sp == ' ') sp++;	// skip the leading spaces of the next citation 
		}
		else 
			*tp++ = *sp++; 
	}
	*tp = '\0';
	if (tmps[0] != '\0')
	{
		ptype = check_citation_info(tmps, &cd);
		//fwprintf(logstream, L"%s\t%d\t%s\t%d\t%s\t%d\t%d\t%s\t%s\n", tpname[cd.type-1], cd.nf, cd.aname, cd.year, cd.jname, cd.volume, cd.page, cd.doi, tmps);
		match_and_write(cd, iwos, trgtid, ostream, tmps);
	}

	return 0;
}

//
// find matching publication and write to the citation file
//
int match_and_write(struct CITING_DOC cd, int iwos, wchar_t *trgtid, FILE *ostream, wchar_t *tmps)
{
	wchar_t author_name[MAX_AUTHOR_NAME*2];	// extended the size (*2), 2012/12/11
	wchar_t *sp;
	int ndx;
	int match;

	//format_aname(author_name, cd.aname);	// change the format of an author name
	//if (wcscmp(L"schreiber, m", author_name) == 0 && cd.year == 2007)
	//	fwprintf(logstream, L"***** %s\n", tmps);

	if (cd.type == P_PATENT)	// added 2012/08/06
		fwprintf(logstream, L"  Citing patent        : %s\n", tmps);
	else if (cd.type == P_XAUTHOR)	// added 2012/08/06
		fwprintf(logstream, L"  Special author name  : %s\n", tmps);
	else if (cd.doi[0] != '\0')	
	{
		// this CR entry contains DOI information
		sp = cd.doi; while (*sp != '\0') { *sp = towlower(*sp); sp++; }	// turn into lower case
		ndx = doi_search(dwos, nwos, cd.doi);	// check matching DOI directly
		if (ndx >= 0)
		{
			fwprintf(logstream, L"  DOI     <== %s %s: %s\n", dwos[ndx].docid, dwos[ndx].alias, tmps);
			if (wos[iwos].year == dwos[ndx].year - 1)	// added 2013/06/11
			{
				fwprintf(logstream, L"     WARNING (accept): citing later document (%d citing %d)\n", wos[iwos].year, dwos[ndx].year);
				fwprintf(ostream, L"  %s %s\n", dwos[ndx].docid, trgtid);	// accept, if identified through DOI, this is usually okay!
			}
			else if (wos[iwos].year < (dwos[ndx].year - 1))	// added 2013/06/11
				fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, dwos[ndx].year);
			else
				fwprintf(ostream, L"  %s %s\n", dwos[ndx].docid, trgtid);
		}
		else
			fwprintf(logstream, L"  Found no matching DOI: %s\n", tmps);
	}
	else if (cd.aname[0] != '\0' && cd.year != 0 && cd.volume != 0 && cd.page != 0)	
	{
		// this CR entry contains author name, year, volume and page information
		format_aname(author_name, cd.aname);	// change the format of an author name
		ndx = name1st_search(awos, nwos, author_name);
		if (ndx >= 0)	// for this 1st author, verify volume and page number of the publication
		{
			match = FALSE;
			while (wcscmp(authors[awos[ndx].author[0]].name, author_name) == 0)	// go through all the entries with the same 1st author name
			{
				if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume && cd.page == awos[ndx].bpage) 
				{
					if (wcscmp(awos[ndx].docid, trgtid) == 0)	// citing itself
						fwprintf(logstream, L"  Citing itself: %s\n", tmps);
					else
					{
						fwprintf(logstream, L"  N/Y/V/P <== %s %s: %s\n", awos[ndx].docid, awos[ndx].alias, tmps);
						if (wos[iwos].year < awos[ndx].year)	// added 2013/06/11
							fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, awos[ndx].year);
						else
							fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
					}
					match = TRUE;
					break;
				}
				ndx++;
				if (ndx >= nwos) break;
			}
			if (match == FALSE)
				fwprintf(logstream, L"  Found no matching N/Y/V/P: %s\n", tmps);
		}
		else
			fwprintf(logstream, L"  Found no matching author: %s\n", tmps);
	}
	else if (cd.aname[0] != '\0' && cd.year != 0 && cd.volume != 0)	
	{
		// this CR entry contains author name, year and volume information
		format_aname(author_name, cd.aname);	// change the format of an author name
		ndx = name1st_search(awos, nwos, author_name);
		if (ndx >= 0)	// for this 1st author, verify volume and page number of the publication
		{
			match = FALSE;
			while (wcscmp(authors[awos[ndx].author[0]].name, author_name) == 0)	// go through all the entries with the same 1st author name
			{
				if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume) 
				{
					if (wcscmp(awos[ndx].docid, trgtid) == 0)	// citing itself
						fwprintf(logstream, L"  Citing itself: %s\n", tmps);
					else
					{
						fwprintf(logstream, L"  N/Y/V   <== %s %s: %s\n", awos[ndx].docid, awos[ndx].alias, tmps);
						if (wos[iwos].year < awos[ndx].year)	// added 2013/06/11
							fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, awos[ndx].year);
						else
							fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
					}
					match = TRUE;
					break;
				}
				ndx++;
				if (ndx >= nwos) break;
			}
			if (match == FALSE)
				fwprintf(logstream, L"  Found no matching N/Y/V: %s\n", tmps);
		}
		else
			fwprintf(logstream, L"  Found no matching author: %s\n", tmps);
	}
	else if (cd.aname[0] != '\0' && cd.year != 0 && cd.page != 0)	
	{
		// this CR entry contains author name, year and page information
		format_aname(author_name, cd.aname);	// change the format of an author name
		ndx = name1st_search(awos, nwos, author_name);
		if (ndx >= 0)	// for this 1st author, verify volume and page number of the publication
		{
			match = FALSE;
			while (wcscmp(authors[awos[ndx].author[0]].name, author_name) == 0)	// go through all the entries with the same 1st author name
			{
				if (cd.year == awos[ndx].year && cd.page == awos[ndx].bpage) 
				{
					if (wcscmp(awos[ndx].docid, trgtid) == 0)	// citing itself
						fwprintf(logstream, L"  Citing itself: %s\n", tmps);
					else
					{
						fwprintf(logstream, L"  N/Y/P   <== %s %s: %s\n", awos[ndx].docid, awos[ndx].alias, tmps);
						if (wos[iwos].year < awos[ndx].year)	// added 2013/06/11
							fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, awos[ndx].year);
						else
							fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
					}
					match = TRUE;
					break;
				}
				ndx++;
				if (ndx >= nwos) break;
			}
			if (match == FALSE)
				fwprintf(logstream, L"  Found no matching N/Y/P: %s\n", tmps);
		}
		else
			fwprintf(logstream, L"  Found no matching author: %s\n", tmps);
	}
	else if (cd.aname[0] != '\0' && cd.year != 0)	
	{
		// this CR entry contains author name and year
		format_aname(author_name, cd.aname);	// change the format of an author name
		ndx = name1st_search(awos, nwos, author_name);
		if (ndx >= 0)	// for this 1st author, verify volume and page number of the publication
		{
			match = FALSE;
			while (wcscmp(authors[awos[ndx].author[0]].name, author_name) == 0)	// go through all the entries with the same 1st author name
			{
				if (cd.year == awos[ndx].year) 
				{
					if (wcscmp(awos[ndx].docid, trgtid) == 0)	// citing itself
						fwprintf(logstream, L"  Citing itself: %s\n", tmps);
					else
					{
						fwprintf(logstream, L"  N/Y     <== %s %s: %s\n", awos[ndx].docid, awos[ndx].alias, tmps);
						if (wos[iwos].year < awos[ndx].year)	// added 2013/06/11
							fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, awos[ndx].year);
						else
							fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
					}
					match = TRUE;
					break;
				}
				ndx++;
				if (ndx >= nwos) break;
			}
			if (match == FALSE)
				fwprintf(logstream, L"  Found no matching N/Y: %s\n", tmps);
		}
		else
			fwprintf(logstream, L"  Found no matching author: %s\n", tmps);
	}
	else
		fwprintf(logstream, L"  Ignore completely: %s\n", tmps);

	return 0;
}

//
// check if the given name has a double-word last name
//
int double_word_last_name(wchar_t *sname)
{
	wchar_t tmps[100];
	int i;
	
	for (i = 0; i < 3; i++)
		tmps[i] = towlower(sname[i]);
	if (wcsncmp(tmps, L"van", 3) == 0 || wcsncmp(tmps, L"von", 3) == 0 ||
		wcsncmp(tmps, L"al", 2) == 0 || wcsncmp(tmps, L"el", 2) == 0 ||
		wcsncmp(tmps, L"le", 2) == 0 || wcsncmp(tmps, L"la", 2) == 0 ||
		wcsncmp(tmps, L"de", 2) == 0)
		return 1;
	else
		return 0;
}

//
// check if the given name has a triple-word last name
//
int triple_word_last_name(wchar_t *sname)
{
	wchar_t tmps[100];
	int i;
	
	for (i = 0; i < 7; i++)
		tmps[i] = towlower(sname[i]);
	if (wcsncmp(tmps, L"van der", 7) == 0 || wcsncmp(tmps, L"van den", 7) == 0 ||
		wcsncmp(tmps, L"van de", 6) == 0)
		return 1;
	else
		return 0;
}

//
// change the format of an author name
// example: from "SCHREIBER M" to "schreiber, m"
//          or, from "Wang R. Y." to "Wang, RY"
//          or, from "De Santis L." to "De Santis, L"
//
int format_aname(wchar_t *tname, wchar_t *sname)
{
	wchar_t *sp, *tp, *pp;

	//fwprintf(logstream, L"###[%s] ", sname); fflush(logstream);

	//1st pass, check how many legitimate spaces are there in the string, at the same time remove the spaces after "." and change all characters to lower case
	int scnt = 0;
	sp = sname; tp = tname; 
	pp = sp;
	while (*sp != '\0')
	{
		if (*sp == ' ')
		{
			if (*pp == '.')	
			{
				pp = sp; sp++;
				continue;
			}
			else
				scnt++; 
		}
		pp = sp;
		*tp++ = towlower(*sp++);
	}
	*tp = '\0';
	wcscpy(sname, tname);	// put the results above to sname
	
	// 2nd pass, check if is there any spaces, if not, return immediately
	sp = sname;
	while (*sp != '\0') { if (*sp++ == ' ') break; }
	if (*sp == '\0') // special case, no space in name
	{
		wcscpy(tname, sname);	
		fwprintf(logstream, L"[%s]\n", tname); fflush(logstream);
		return 0;
	}

	// 3rd pass, change the ' ' after the last name to ', '
	sp = sname; tp = tname;
	while (*sp != ' ') *tp++ = *sp++;
	if (scnt == 2 && double_word_last_name(sname))	// this indicates that there are double-word last names such as "De Santis", ignore the 1st space
	{
		*tp++ = *sp++;
		while (*sp != ' ') *tp++ = *sp++;
	}
	if (scnt == 3 && triple_word_last_name(sname))// this indicates that there are triple-word last names such as "Van der Zouwen", ignore the 1st and 2nd spaces in the last name
	{	
		*tp++ = *sp++;
		while (*sp != ' ') *tp++ = *sp++;
		*tp++ = *sp++;
		while (*sp != ' ') *tp++ = *sp++;
	}
	*tp++ = ','; *tp++ = ' '; sp++;
	while (*sp != '\0')
		*tp++ = *sp++;
	*tp = '\0';

	// 4th pass, remove '.' and spaces in the first and middle name
	wcscpy(sname, tname);	// put the results above to sname
	sp = sname; tp = tname;
	while (*sp != ',') *tp++ = *sp++;
	*tp++ = *sp++; // move ',' over
	*tp++ = *sp++; // move ' ' over
	while (*sp != '\0')	// ignore any '.' or ' ' in the first and middle name
	{
		if (*sp == '.' || *sp == ' ') sp++;
		else *tp++ = *sp++;
	}
	*tp = '\0';
	//fwprintf(logstream, L"[%s]\n", tname); fflush(logstream);
	// another pass (added 2013/06/22), remove '-' in the first names, for example, change "lndvall, b-a" to "lundvall, ba"
	wcscpy(sname, tname);	// put the results above to sname
	sp = sname; tp = tname;
	while (*sp != ',') 
	{
		if (sp == '\0') break;
		else *tp++ = *sp++;	
	}
	while (*sp != '\0') 
	{
		if (*sp != '-') *tp++ = *sp++; else sp++;
	}
	*tp = '\0';
	// very special case for innovation system data
	if (inno_system >= 0)
	{
		if (wcscmp(tname, L"nelson, r") == 0)		// added 2013/06/22
			wcscpy(tname, L"nelson, rr");
	}

	return 0;
}

//
// given a citation string, provide a smart guess on the type of the paper (journal, proceedings, reports, books, etc.)
// this function also removes the unnecessary "DOI" information
//
#define WAIT_AUTHOR 1
#define WAIT_YEAR 2
#define WAIT_JNAME 3
#define WAIT_VOLUME 4
#define WAIT_BPAGE 5
#define WAIT_DOI 6
#define WAIT_NOMORE 7
int check_citation_info(wchar_t *tmps, struct CITING_DOC *cd)
{
	wchar_t *sp, *tp;
	wchar_t buf[LBUF_SIZE];
	int nf, cnt;
	int bp_yn, vol_yn, doi_yn;
	int state;
	int ptype;
	int hit_bracket;			
	
	nf = 0; bp_yn = FALSE; vol_yn = FALSE; doi_yn = FALSE;
	cd->nf = 0; cd->aname[0] = '\0'; cd->year = 0; cd->jname[0] = '\0'; cd->volume = 0; cd->page = 0; cd->doi[0] = '\0';	// initialization
	if (*tmps == '*')
	{
		cd->type = P_REPORT;
		ptype = P_REPORT;
		return ptype;
	}
	sp = tmps; tp = buf;
	state = WAIT_AUTHOR;
	if (*sp == '2' || *sp == '1') // special case where there is no author name and the string begin with year of publication
	{ 
		state = WAIT_YEAR; 
		cd->aname[0] = '\0'; 
	}	
	else if (*sp == '[')	// handle special author names, added 2012/08/06
	{
		state = WAIT_NOMORE; 
		cd->aname[0] = '['; 
	}
	while (*sp != '\0')
	{
		switch (state)
		{
		case WAIT_AUTHOR:
			if (*sp == ',') 
			{ 
				*tp = '\0'; wcscpy_s(cd->aname, MAX_AUTHOR_NAME-1, buf); tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; nf++; 
				if (*sp == '2' || *sp == '1') // make sure that the next entry is a year
					state = WAIT_YEAR; 
				else	// skip to journal name
				{
					state = WAIT_JNAME; 
					hit_bracket = 0;
					cd->year = 0;
				}
				break; 
			}
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; wcscpy_s(cd->aname, MAX_AUTHOR_NAME-1,buf); tp = buf; break; }
			*tp++ = *sp++;
			break;
		case WAIT_YEAR:
			if (*sp == ',') { state = WAIT_JNAME; *tp = '\0'; cd->year = _wtoi(buf); tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; nf++; break; }
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->year = _wtoi(buf); tp = buf; break; }
			*tp++ = *sp++;
			break;
		case WAIT_JNAME:
			if (*sp == ',' && hit_bracket != 1) // hit a delimiter and not inside a bracket, check for hit_bracket added 2012/10/10
			{ 
				*tp = '\0'; wcscpy_s(cd->jname, MAX_JOURNAL_NAME-1,buf); 
				if (buf[0] == L'U' || *sp == ';')
					state = WAIT_NOMORE;
				if (wcsncmp(cd->jname, L"Patent", 6) == 0 || wcsncmp(cd->jname, L"United States Patent", 20) == 0 || wcsncmp(cd->jname, L"US Patent", 9) == 0 || wcsncmp(cd->jname, L"U.S. Patent", 11) == 0 || wcsncmp(cd->jname, L"US patent", 9) == 0 || wcsncmp(cd->jname, L"Chinese Patent", 14) == 0 || wcsncmp(cd->jname, L"CN Patent", 9) == 0)	// added 2012/08/06
					state = WAIT_NOMORE; 
				else
				{
					state = WAIT_VOLUME; 
					cnt = 0;
				}
				tp = buf; buf[0] = '\0'; sp++; 
				while (*sp == ' ') sp++; nf++; 
				break; 
			}
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; wcscpy_s(cd->jname, MAX_JOURNAL_NAME-1,buf); tp = buf; break; }
			if (hit_bracket != 1)	// changed from '== 0' to '!= 1', 2012/10/24
			{
				if (*sp == '[')	// hit a '[', special case, added 2012/10/10
				{
					hit_bracket = 1;	// at the fisrt character
					sp++;
					break;
				}
				else
					hit_bracket = 2;
			}
			else if (hit_bracket == 1) // wait for closing bracket
			{
				if (*sp == ']') 
				{ 
					sp++; sp++; hit_bracket = 2; 
				}
				sp++;
				break;
			}
			*tp++ = *sp++;
			break;
		case WAIT_VOLUME:
			// Note: volume number is in the format "V12"
			// For books, here one should look for DOI numbers
			if (*sp == ',' || *sp == '\0') 
			{ 	
				if (buf[0] == 'D' && buf[1] == 'O' && buf[2] == 'I')	// added 2013/04/17
				{
					state = WAIT_NOMORE; *tp = '\0'; 
					wcscpy_s(cd->doi, MAX_DOI_LEN-1, &buf[4]); 
					tp = buf; buf[0] = '\0'; sp++; doi_yn = TRUE; 
					break; 
				}
				else if (buf[0] =='V' || buf[0] == 'v')	// this check is added 2012/11/05
				{
					state = WAIT_BPAGE; *tp = '\0'; 
					cd->volume = _wtoi(buf+1); tp = buf; buf[0] = '\0'; sp++; 
					while (*sp == ' ') sp++; nf++; vol_yn = TRUE; 
					break; 
				}
				else	// added 2012/11/05
				{ 
					state = WAIT_NOMORE; *tp = '\0'; cd->volume = 0; tp = buf; break; 
				}
			}
			else if (*sp == 'P' && cnt == 0) state = WAIT_BPAGE;	// "cnt == 0" is added 2012/11/05
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->volume = _wtoi(buf); tp = buf; break; }
			*tp++ = *sp++; cnt++;
			break;
		case WAIT_BPAGE:
			// Note: page number is in the format "P1187" or "A123", "B567" (should have been PA123, or PA567)
			if (*sp == ',') { state = WAIT_DOI; *tp = '\0'; cd->page = _wtoi(buf+1); tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; nf++; bp_yn = TRUE; break; }
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->page = _wtoi(buf); tp = buf; break; }
			*tp++ = *sp++; 
			break;
		case WAIT_DOI:
			if (*sp == '\0' || *sp == ',') 
			{ 
				state = WAIT_NOMORE; *tp = '\0'; 
				wcscpy_s(cd->doi, MAX_DOI_LEN-1, &buf[4]); 
				tp = buf; buf[0] = '\0'; sp++; doi_yn = TRUE; 
				break; 
			} // modified 2011/12/04
			*tp++ = *sp++; 
			break;
		default:
			break;
		}
		if (state == WAIT_NOMORE) break;
	}
	if (state == WAIT_BPAGE) { *tp = '\0'; cd->page = _wtoi(buf+1); bp_yn = TRUE; }
	else if (state == WAIT_JNAME) { *tp = '\0'; wcscpy_s(cd->jname, MAX_JOURNAL_NAME-1,buf); }
	else if (state == WAIT_VOLUME) 
	{ 
		*tp = '\0'; 
		if (buf[0] == 'D' && buf[1] == 'O' && buf[2] == 'I') { wcscpy_s(cd->doi, MAX_DOI_LEN-1, &buf[4]); doi_yn = TRUE; } // added 2013/04/17
		else { cd->volume = _wtoi(buf+1); vol_yn = TRUE; }
	}
	else if (state == WAIT_DOI) { *tp = '\0'; wcscpy_s(cd->doi, MAX_DOI_LEN-1, &buf[4]); doi_yn = TRUE; }
	nf++;

	// correct for "DOI DOI xxxx" pattern, there should have only one DOI, 2012/03/29
	//fwprintf(logstream, L"$$%s\n", cd->doi);
	if (cd->doi[0] == 'D' && cd->doi[1] == 'O' && cd->doi[2] == 'I' && cd->doi[3] == ' ')
		wcscpy_s(cd->doi, MAX_DOI_LEN-1, &cd->doi[4]);
	// correct for multiple DOI (enclosed with []), take only the 1st DOI, 2nd DOI is ignored automatically, 2012/03/29
	if (cd->doi[0] == '[')
		wcscpy_s(cd->doi, MAX_DOI_LEN-1, &cd->doi[1]);

	if (nf >= 5 && vol_yn == TRUE && bp_yn == TRUE)
	{
		cd->type = P_JOURNAL; cd->nf = nf;
		ptype = P_JOURNAL;
	}
	else if (nf == 4 && vol_yn == FALSE && bp_yn == TRUE)
	{
		cd->type = P_PROCEEDING_BOOK;  cd->nf = nf;
		ptype = P_PROCEEDING_BOOK;
	}
	else
	{
		if (wcsncmp(cd->jname, L"Patent", 6) == 0 || wcsncmp(cd->jname, L"United States Patent", 20) == 0 || wcsncmp(cd->jname, L"US Patent", 9) == 0 || wcsncmp(cd->jname, L"U.S. Patent", 11) == 0 || wcsncmp(cd->jname, L"US patent", 9) == 0 || wcsncmp(cd->jname, L"Chinese Patent", 14) == 0 || wcsncmp(cd->jname, L"CN Patent", 14) == 0)	// added 2012/08/06
		{
			cd->type = P_PATENT; 
			ptype = P_PATENT;
		}
		else if (cd->aname[0] == '[')
		{
			cd->type = P_XAUTHOR; 
			ptype = P_XAUTHOR;
		}
		else
		{
			cd->type = P_OTHER; 
			ptype = P_OTHER;
		}
		cd->nf = nf;
	}

	return ptype;
}

//
// prepare dataset for search purpose
//
int prepare_for_CR_search()
{
	int i;

	dwos = (struct WOS *)malloc(nwos * sizeof(struct WOS));
	if (dwos == NULL) 
	{
		fwprintf(logstream, L"Fail to allocate %d bytes of memory (dwos).", nwos*sizeof(struct WOS));
		return MSG_NOT_ENOUGH_MEMORY;
	}
	for (i = 0; i < nwos; i++) dwos[i] = wos[i];	
	qsort((void *)dwos, (size_t)nwos, sizeof(struct WOS), compare_doi);	// sort on DOI
	//for (i = 0; i < nwos; i++)
	//	fwprintf(logstream, L"%d\t%s\n", i+1, dwos[i].doi);

	awos = (struct WOS *)malloc(nwos * sizeof(struct WOS));
	if (awos == NULL) 		
	{
		fwprintf(logstream, L"Fail to allocate %d bytes of memory (dwos).", nwos*sizeof(struct WOS));
		return MSG_NOT_ENOUGH_MEMORY;
	}
	for (i = 0; i < nwos; i++) awos[i] = wos[i];	
	qsort((void *)awos, (size_t)nwos, sizeof(struct WOS), compare_1st_author);	// sort on the name of 1st author
	for (i = 0; i < nwos; i++)
	{ fwprintf(logstream, L"%d\t%s\n", i+1, authors[awos[i].author[0]].name); fflush(logstream); }

	return 0;
}

//
// release memory
//
int clear_preparation_for_CR_search()
{
	free(dwos);
	free(awos);

	return 0;
}

//
// given a citation string, exract the author, year, volume and beginning page information
// the new string is written over the original pointer
//
int extract_citation_info1(wchar_t *tmps)
{
	wchar_t *sp, *tp;

	sp = tmps; tp = tmps;
	// copy over the author information
	while (*sp != ',') { *tp++ = *sp++; }
	*tp++ = *sp++;
	// copy over the year information
	while (*sp != ',') { *tp++ = *sp++; }
	*tp++ = *sp++;
	// skip the proceeding/journal name
	while (*sp != ',') sp++; sp++;
	// copy over the volume number information
	while (*sp != ',') { *tp++ = *sp++; }
	// copy over the beginning page information
	while (*sp != '\0') { *tp++ = *sp++; }
	*tp = '\0';

	return 0;
}

//
// given a citation string, exract the author, year and beginning page information
// the new string is written over the original pointer
//
int extract_citation_info2(wchar_t *tmps)
{
	wchar_t *sp, *tp;

	sp = tmps; tp = tmps;
	// copy over the author information
	while (*sp != ',') { *tp++ = *sp++; }
	*tp++ = *sp++;
	// copy over the year information
	while (*sp != ',') { *tp++ = *sp++; }
	*tp++ = *sp++;
	// skip the proceeding/journal name
	while (*sp != ',') sp++; sp++;
	// copy over the beginning page information
	while (*sp != '\0') { *tp++ = *sp++; }
	*tp = '\0';

	return 0;
}

//
// use binary search to find the beginning position of a (1st) author name in the WOS data array
//
int name1st_search(struct WOS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, authors[d[0].author[0]].name) < 0)
		return -1;
	if (wcscmp(str, authors[d[nwos-1].author[0]].name) > 0)	// this is necessary when there is no guarantee that the given string is in the data array
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, authors[d[cur].author[0]].name) == 0)
		{
			while (cur != 0)	// trace back to the 1st non-duplicate entry
			{
				cur--;
				if (wcscmp(str, authors[d[cur].author[0]].name) != 0) { cur++; return cur; }
			}
		}
		if (cur == low)
		{
			if (wcscmp(str, authors[d[high].author[0]].name) == 0)
			{
				while (high != 0)	// trace back to the 1st non-duplicate entry
				{
					high--;
					if (wcscmp(str, authors[d[high].author[0]].name) != 0) { high++; return high; }
				}
			}
			else 
				return -1;
		}
		else if (wcscmp(str, authors[d[cur].author[0]].name) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search to find the proper position of a DOI in the WOS data array
//
int doi_search(struct WOS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].doi) < 0)
		return -1;
	if (wcscmp(str, d[num-1].doi) > 0)	// added 2012/08/06
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].doi) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].doi) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].doi) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search to find the proper position of document alias in the WOS data array
//
int dalias_search(struct WOS d[], int num, wchar_t *str)
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
// this fucntion is to be called by qsort() only
// 
int compare_doc_alias(const void *n1, const void *n2)
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
int compare_doi(const void *n1, const void *n2)
{
	struct WOS *t1, *t2;
	
	t1 = (struct WOS *)n1;
	t2 = (struct WOS *)n2;
	if (wcscmp(t2->doi, t1->doi) < 0)
		return 1;
	else if (wcscmp(t2->doi, t1->doi) == 0)
		return 0;
	else return -1;
}