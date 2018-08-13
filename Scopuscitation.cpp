//
// Scopuscitation.cpp
//

// Revision History:
// 2014/04/xx Basic function works
// 2014/10/11 changed the algorithm in parsing the "References" field. The new algorithm is: 
//              if author name is given correctly, need to be correct on three of the four information: year, volume, page start, and page end.
//              If author name is wrong or not given, need to be correct on all four information: year, volume, page start, and page end.
// 2014/10/31 added a new check in parsing for the case that author informatin is not given
// 2017/10/08 added a special check for "Nakamoto", the Bitcoin inventor, when "Nakamoto, S" appears as the author, take it no matter what
//            added a new function: check_Scopus_reference_format()
//
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

extern int nauthoralias;	// number of items in the author alias table
extern struct AUTHOR_ALIAS *authoralias;

extern int nwos;
extern struct WOS *wos;
extern FILE *logstream;

int check_Scopus_citation_info(wchar_t *, struct CITING_DOC *);
int extract_Scopus_citation_info1(wchar_t *);
int extract_Scopus_citation_info2(wchar_t *);
int compare_1st_author(const void *, const void *);
int Scopus_match_and_write(struct CITING_DOC, int, wchar_t *, FILE *, wchar_t *);
int check_Scopus_reference_format(wchar_t *);

extern int authoralias_search(struct AUTHOR_ALIAS *, int, wchar_t *);
extern int kname_search(struct KWORDS *, int, wchar_t *);
extern int aname_search(struct AUTHORS *, int, wchar_t *);
extern int jname_search(struct JOURNALS *, int, wchar_t *);
extern int compare_author(const void *, const void *);
extern int compare_journal(const void *, const void *);
extern int compare_doi(const void *, const void *);
extern int double_word_last_name(wchar_t *);
extern int compare_doc_alias(const void *, const void *);
extern int dalias_search(struct WOS *, int, wchar_t *);
extern int doi_search(struct WOS *, int, wchar_t *);
extern int name1st_search(struct WOS *, int, wchar_t *);
extern int format_aname(wchar_t *, wchar_t *);
extern int compare_authoralias(const void *, const void *);

extern int naus;
extern struct AUTHORS *authors;	// author name array
extern int nkwde;
extern struct KWORDS *kwde;	// author keywords
extern wchar_t *tpname[6];
extern struct WOS *dwos;	// a duplicate of "wos", to be sorted by DOI
extern struct WOS *awos;	// a duplicate of "wos", to be sorted by author name

//
// parse a string of citation information (the "References" string in the Scopus database)
// write the result to the given output file stream
//
int parse_Scopus_citation_info(wchar_t *crstr, int iwos, wchar_t *trgtid, FILE *ostream)
{
	int i;
	wchar_t ch, pch, qch, *sp, *tp;
	wchar_t tmps[LBUF_SIZE];
	wchar_t tmpdis[LBUF_SIZE];
	int ptype;
	int reference_type;
	struct CITING_DOC cd;

	sp = crstr;
	// start parsing
	tp = tmps; ch = '\0';
	while (*sp != '\0')
	{
		pch = ch; sp++; qch = *sp; sp--;
		ch = *sp; 
		// hit the end of a citation? the delimiter is "; " (semi-colon followed by a space)
		// Special case: some DOI contains ';' but they are always followed by a 'O'
		if (ch == ';' && !(pch == 'O' && qch != ' '))
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				reference_type = check_Scopus_reference_format(tmps);
				wcsncpy(tmpdis, tmps, 120);
				fwprintf(logstream, L"  Reference Type: %d\t%s\n", reference_type, tmpdis);
				ptype = check_Scopus_citation_info(tmps, &cd);
				//fwprintf(logstream, L"[%s\t%d\t%s\t%d\t%s\t%d\t%d\t%d]\n%s\n\n", tpname[cd.type-1], cd.nf, cd.aname, cd.year, cd.jname, cd.volume, cd.page, cd.page2, tmps);
				//fflush(logstream);
				Scopus_match_and_write(cd, iwos, trgtid, ostream, tmps);
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
		reference_type = check_Scopus_reference_format(tmps);
		wcsncpy(tmpdis, tmps, 120);
		fwprintf(logstream, L"  Reference Type: %d\t%s\n", reference_type, tmpdis);
		ptype = check_Scopus_citation_info(tmps, &cd);
		//fwprintf(logstream, L"[%s\t%d\t%s\t%d\t%s\t%d\t%d\t%d]\n%s\n\n", tpname[cd.type-1], cd.nf, cd.aname, cd.year, cd.jname, cd.volume, cd.page, cd.page2, tmps);
		Scopus_match_and_write(cd, iwos, trgtid, ostream, tmps);
	}

	return 0;
}

//
// find matching publication and write to the citation file
//
int Scopus_match_and_write(struct CITING_DOC cd, int iwos, wchar_t *trgtid, FILE *ostream, wchar_t *tmps)
{
	wchar_t author_name[MAX_AUTHOR_NAME*2];	
	wchar_t *sp;
	int ndx, ndx2, i;
	int match;
	int gotone;
	wchar_t mstring[30];

	gotone = 0;	// added 2010/10/08
	if (cd.aname[0] != '\0' && cd.year != 0 && cd.volume != 0 && cd.page != 0)	
	{
		// this 'References' entry contains author name, year, volume and page information
		format_aname(author_name, cd.aname);	// change the format of an author name
		ndx2 = authoralias_search(authoralias, nauthoralias, author_name);
		if (ndx2 >= 0)	// found that this is a variation of the author name
			wcscpy(author_name, authoralias[ndx2].alias);
		ndx = name1st_search(awos, nwos, author_name);
		if (ndx >= 0)	// for this 1st author, verify volume and page number of the publication
		{
			match = FALSE;
			while (wcscmp(authors[awos[ndx].author[0]].name, author_name) == 0)	// go through all the entries with the same 1st author name
			{
				//fwprintf(logstream, L"XXX[%d, %d, %d]\n", cd.year, cd.volume, cd.page);
				int xmatch = 1;
				if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume && cd.page == awos[ndx].bpage && cd.page2 == awos[ndx].epage) 
					wcscpy(mstring, L"N/Y/V/BP/EP");
				else if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume && cd.page == awos[ndx].bpage) 
					wcscpy(mstring, L"N/Y/V/BP/??");
				else if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume && cd.page2 == awos[ndx].epage) 
					wcscpy(mstring, L"N/Y/V/??/EP");
				else if (cd.year == awos[ndx].year && cd.page == awos[ndx].bpage && cd.page2 == awos[ndx].epage) 
					wcscpy(mstring, L"N/Y/?/BP/EP");
				else if (cd.volume == awos[ndx].volume && cd.page == awos[ndx].bpage && cd.page2 == awos[ndx].epage) 
					wcscpy(mstring, L"N/?/V/BP/EP");				
				else
					xmatch = 0;
				if (xmatch)
				{
					if (wcscmp(awos[ndx].docid, trgtid) == 0)	// citing itself
						fwprintf(logstream, L"  Citing itself: %s\n", tmps);
					else
					{
						gotone = 1;
						fwprintf(logstream, L"  %s <== %s %s: %s\n", mstring, awos[ndx].docid, awos[ndx].alias, tmps);
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
				fwprintf(logstream, L"  Found no matching N/Y/V/BP/EP: %s\n", tmps);
		}
		else	// author is provided, but does not match any 
		{
			for (i = 0; i < nwos; i++)	// kind of stupid, but ... for now!
			{
				ndx = i;
				if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume && cd.page == awos[ndx].bpage && cd.page2 == awos[ndx].epage) 
				{		
					gotone = 1;
					fwprintf(logstream, L"  ?/Y/V/BP/EP <== %s %s: %s\n", awos[ndx].docid, awos[ndx].alias, tmps);
					if (wos[iwos].year < awos[ndx].year)	
						fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, awos[ndx].year);
					else
						fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
					break;
				}
			}
			if (i == nwos)
				fwprintf(logstream, L"  Found no matching ?/Y/V/BP/EP: %s\n", tmps);
		}
	}
	else if (cd.year != 0 && cd.volume != 0 && cd.page != 0)	// author is not provided in the reference string, the check is added 2014/10/31
	{
		for (i = 0; i < nwos; i++)	// kind of stupid, but the situation that author is not provided is rare
		{
			ndx = i;
			if (cd.year == awos[ndx].year && cd.volume == awos[ndx].volume && cd.page == awos[ndx].bpage && cd.page2 == awos[ndx].epage) 
			{		
				gotone = 1;
				fwprintf(logstream, L"  -/Y/V/BP/EP <== %s %s: %s\n", awos[ndx].docid, awos[ndx].alias, tmps);
				if (wos[iwos].year < awos[ndx].year)	
					fwprintf(logstream, L"     ERROR (reject): citing later document (%d citing %d)\n", wos[iwos].year, awos[ndx].year);
				else
					fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
				break;
			}
		}
		if (i == nwos)
			fwprintf(logstream, L"  Found no matching -/Y/V/BP/EP: %s\n", tmps);
	}
	else if (cd.aname[0] != '\0' && gotone == 0) // this special was added 2017/10/08, for Bitcoin data 
	{
		format_aname(author_name, cd.aname);	// change the format of an author name
		ndx = name1st_search(awos, nwos, author_name);
		if (ndx >= 0)	
		{
			if (wcscmp(L"nakamoto, s", author_name) == 0)	
			{
				wcscpy(mstring, L"N/Nakamoto");	
				fwprintf(logstream, L"  %s <== %s %s: %s\n", mstring, awos[ndx].docid, awos[ndx].alias, tmps);	
				fwprintf(ostream, L"  %s %s\n", awos[ndx].docid, trgtid);
			}
		}
	}

	fflush(logstream);

	return 0;
}

//
// given a citation string, provide a smart guess on the type of the paper (journal, proceedings, reports, books, etc.)
// a typical Scopus reference line
// "Bakos, J.Y., The emerging role of electronic marketplaces on the Internet (1998) Communications of the ACM, 41 (8), pp. 35-42;"
//
#define WAIT_AUTHOR_LNAME 1
#define WAIT_AUTHOR_FNAME 2
#define WAIT_TITLE 3
#define WAIT_YEAR 4
#define WAIT_JNAME 5
#define WAIT_VOLUME 6
#define WAIT_ISSUE 7
#define WAIT_BPAGE 8
#define WAIT_EPAGE 9
#define WAIT_NOMORE 10
int check_Scopus_citation_info(wchar_t *tmps, struct CITING_DOC *cd)
{
	wchar_t *sp, *tp, *op;
	wchar_t buf[LBUF_SIZE];
	wchar_t lname[LBUF_SIZE];
	int nf, cnt;
	int state;
	int ptype;
	int hit_bracket;			
	
	//fwprintf(logstream, L"==>%s\n", tmps); fflush(logstream);

	nf = 0;  
	cd->nf = 0; cd->aname[0] = '\0'; cd->year = 0; cd->jname[0] = '\0'; cd->volume = 0; cd->page = 0; cd->page2 = 0; cd->doi[0] = '\0';	// initialization
	sp = tmps; tp = buf;
	state = WAIT_AUTHOR_LNAME;
	while (*sp != '\0')
	{
		switch (state)
		{
		case WAIT_AUTHOR_LNAME:	// here, we want only the name of the 1st author
			if (*sp == '('	&& (*(sp+1) == '1' || *(sp+1) == '2'))  // hit the publishing year and find not author name, a special case
			{
				state = WAIT_YEAR; tp = buf; buf[0] = '\0'; sp++; break;
			}
			if (*sp == ',') 
			{ 
				*tp = '\0'; wcscpy_s(lname, MAX_AUTHOR_NAME-1, buf); tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; nf++; 
				state = WAIT_AUTHOR_FNAME; 
				break; 
			}
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; wcscpy_s(lname, MAX_AUTHOR_NAME-1,buf); tp = buf; nf++; break; }
			*tp++ = *sp++;
			break;
		case WAIT_AUTHOR_FNAME:
			if (*sp == ',') 
			{ 
				*tp = '\0'; swprintf_s(cd->aname, MAX_AUTHOR_NAME-1, L"%s %s", lname, buf); tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; 
				state = WAIT_TITLE; 
				break; 
			}
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; swprintf_s(cd->aname, MAX_AUTHOR_NAME-1, L"%s %s", lname, buf); tp = buf; break; }
			if (*sp == '.')
				sp++;	// skip the period after the 1st initial
			else
				*tp++ = *sp++;
			break;
		case WAIT_TITLE:
			if (*sp == '(') { state = WAIT_YEAR; tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; break;	}
			if (*sp == '\0') { state = WAIT_NOMORE; tp = buf; break; }
			*tp++ = *sp++;
			break;
		case WAIT_YEAR:
			if (*sp == ')') { state = WAIT_JNAME; *tp = '\0'; cd->year = _wtoi(buf); tp = buf; buf[0] = '\0'; sp++; if (*sp == '.') sp++; while (*sp == ' ') sp++; nf++; break; }
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->year = _wtoi(buf); tp = buf; nf++; break; }
			*tp++ = *sp++;
			break;
		case WAIT_JNAME:
			if (*sp == ',') 
			{ 
				*tp = '\0'; wcscpy_s(cd->jname, MAX_JOURNAL_NAME-1,buf);  tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; nf++; 
				state = WAIT_VOLUME; op = sp;
				break;
			}
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; wcscpy_s(cd->jname, MAX_JOURNAL_NAME-1,buf); tp = buf; nf++; break; }
			*tp++ = *sp++;
			break;
		case WAIT_VOLUME:
			// For books, there should be no volume numbers
			if (*sp == '(') 
			{ 	
				*tp = '\0'; 
				if (iswdigit(buf[0])) { cd->volume = _wtoi(buf); tp = buf; buf[0] = '\0'; while (*sp != ')') sp++; sp++; sp++; nf++; state = WAIT_BPAGE; break; }
				else
					state = WAIT_NOMORE;				
			}
			else if (*sp == ',') 
			{ 	
				*tp = '\0'; 
				if (iswdigit(buf[0])) { cd->volume = _wtoi(buf); tp = buf; buf[0] = '\0'; sp++; nf++; state = WAIT_BPAGE; break; }
				else if (buf[0] == 'p' && buf[1] == 'p' && buf[2] == '.') { tp = buf; buf[0] = '\0'; sp = op; state = WAIT_BPAGE; break; }	// back to the position of the previous state
				else
					state = WAIT_NOMORE;				
			}
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->volume = _wtoi(buf); tp = buf; nf++; break; }
			*tp++ = *sp++;
			break;
		case WAIT_BPAGE:
			// Note: page number is in the format "pp. 1187-1190"
			if (*sp == '.' || *sp == ' ') {tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; break; }
			if (*sp == '-') { state = WAIT_EPAGE; *tp = '\0'; cd->page = _wtoi(buf); tp = buf; buf[0] = '\0'; sp++; while (*sp == ' ') sp++; nf++; break; }
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->page = _wtoi(buf); tp = buf; nf++; break; }
			*tp++ = *sp++; 
			break;
		case WAIT_EPAGE:
			// Note: page number is in the format "pp. 1187-1190"
			if (*sp == '\0') { state = WAIT_NOMORE; *tp = '\0'; cd->page = _wtoi(buf); tp = buf; break; }
			*tp++ = *sp++; 
			break;
		default:
			break;
		}
		if (state == WAIT_NOMORE) break;
	}
	if (state == WAIT_EPAGE) { *tp = '\0'; cd->page2 = _wtoi(buf); }
	else if (state == WAIT_BPAGE) { *tp = '\0'; cd->page = _wtoi(buf); nf++; }
	else if (state == WAIT_JNAME) { *tp = '\0'; wcscpy_s(cd->jname, MAX_JOURNAL_NAME-1,buf); nf++; }

	if (nf >= 5 && cd->volume != 0 && cd->page != 0)
	{
		cd->type = P_JOURNAL; cd->nf = nf;
		ptype = P_JOURNAL;
	}
	else if ((nf == 4 || nf == 3) && cd->volume == 0)
	{
		cd->type = P_PROCEEDING_BOOK;  cd->nf = nf;
		ptype = P_PROCEEDING_BOOK;
	}
	else
	{
		cd->type = P_OTHER; 
		ptype = P_OTHER;
		cd->nf = nf;
	}

	return ptype;
}

//
// check the format type of a string in the "References"
// there can be references in different format within an article
// format 1: "Nakamoto, S., (2009) Bitcoin: A Peer-to-peer Electronic Cash System., ..."
// format 2: "Ober, M., Katzenbeisser, S., Hamacher, K., Structure and anonymity of the bitcoin transaction graph (2013) Future Internet, 5 (2), pp. 237-250"
//
int check_Scopus_reference_format(wchar_t *cstr)
{
	wchar_t *sp, *tp, *tmp;
	int format_type;
	int state;

	sp = cstr; 
	format_type = 2;	// default type
	state = 1;
	while (*sp != '\0')
	{
		switch (state)
		{
		case 1:	// waiting for '.'
			if (*sp == '.') 
				state = 2;
			break;
		case 2:	// waiting for ','
			if (*sp == ',')
				state = 3;
			else
				state = 1;
			break;
		case 3:	// waiting for ' '
			if (*sp == ' ')
				state = 4;
			else
				state = 1;
			break;
		case 4:	// waiting for '('
			if (*sp == '(')
				state = 5;
			else
				state = 1;
			break;
		case 5:	// waiting for a digit
			if (isdigit(*sp))
				format_type = 1;
			else
				state = 1;
			break;
		default:
			break;
		}
		sp++;
	}

	return format_type;
}

//
// given a citation string, exract the author, year, volume and beginning page information
// the new string is written over the original pointer
//
int extract_Scopus_citation_info1(wchar_t *tmps)
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
int extract_Scopus_citation_info2(wchar_t *tmps)
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
