//
// TCIcitation.cpp
//

// Revision History:
// 2016/04/xx Basic function works
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

#define BUF_SIZE 1024

extern int nwos;
extern struct WOS *wos;
extern FILE *logstream;
extern struct TTD_TITLE *ttd_title;

int check_TCI_citation_info(wchar_t *, struct CITING_DOC *);
int TCI_match_and_write(struct CITING_DOC, int, wchar_t *, FILE *, wchar_t *);
int remove_angle_brackets(wchar_t *);

extern int ttdtitle_search(struct TTD_TITLE *, int, wchar_t *);

//
// parse a string of citation information (the "論文參考文獻" string in the TCI database)
// write the result to the given output file stream
//
int parse_TCI_citation_info(wchar_t *crstr, int iwos, wchar_t *trgtid, FILE *ostream)
{
	wchar_t ch, pch, qch, *sp, *tp;
	wchar_t tmps[LBUF_SIZE];
	int ptype;
	struct CITING_DOC cd;

	sp = crstr;
	if (*sp == '\n') return 0;

	fwprintf(logstream, L"***** %s\t%s\n", wos[iwos].docid, wos[iwos].alias);

	// start parsing
	tp = tmps; ch = '\0';
	while (*sp != '\0')
	{
		pch = ch; sp++; qch = *sp; sp--;
		ch = *sp; 
		if (ch == '\n') // hit the end of a citation? the delimiter is "\n" 
		{ 
			*tp++ = '\0'; sp++; 
			if (tmps[0] != '\0')
			{
				ptype = check_TCI_citation_info(tmps, &cd);
				TCI_match_and_write(cd, iwos, trgtid, ostream, tmps);
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
		ptype = check_TCI_citation_info(tmps, &cd);
		TCI_match_and_write(cd, iwos, trgtid, ostream, tmps);
	}

	return 0;
}

//
// find matching publication and write to the citation file
// typical TCI reference lines
// 年齡、性別、成就目標、目標導向與創意生活經驗、創造力之關係    劉士豪  1998
// Social Psychology of Creativity    Amabile, T. M.  1983
//
int TCI_match_and_write(struct CITING_DOC cd, int iwos, wchar_t *trgtid, FILE *ostream, wchar_t *str)
{
	wchar_t author_name[MAX_AUTHOR_NAME*2];	
	int k;
	int spcnt, cnt;
	int index;
	wchar_t buf[BUF_SIZE];
	wchar_t *sp, *tp;

	if (cd.type == P_ENGLISH)
		return 0;	// ignore English references

	// parse the remaining reference strings
	sp = str; tp = buf; cnt = 0; spcnt = 0;
	while (*sp != '\0')
	{
		if (*sp == ' ' && spcnt == 3) 
		{
			// remove the spaces at the end
			tp--; while (*tp == ' ') tp--; 
			tp++; *tp = '\0';
			// remove the text enclosed in angle brackets
			remove_angle_brackets(buf);
			//fwprintf(logstream, L"@@@[%s]\n", buf);
			index = ttdtitle_search(ttd_title, nwos, buf);
			if (index != -1)
			{
				k = ttd_title[index].ndx;
				if (k != iwos)	// rule out the situation of citing self
				{
					if (wos[index].year >= wos[k].year)	// only if citing backward
					{
						if (wcscmp(wos[k].docid, trgtid) == 0)
							fwprintf(logstream, L"?????%s %s\n", wos[k].docid, trgtid);
						fwprintf(logstream, L"     Cites: %d [%s]\n", wos[k].year, buf);
						fwprintf(ostream, L"%s %s\n", wos[k].docid, trgtid);
						fflush(logstream);
					}
				}
			}
			break;
		}
		else
		{
			if (*sp == ' ') 
				spcnt++;
			*tp++ = *sp++;
			cnt++;
			if (cnt >= BUF_SIZE)
				break;	// string too long, forget about the remaining characters
		}
	}

	return 0;
}

//
// given a citation string, provide a smart guess on the type of the paper (journal, proceedings, reports, books, etc.)
// typical TCI reference lines
// 年齡、性別、成就目標、目標導向與創意生活經驗、創造力之關係    劉士豪  1998
// Social Psychology of Creativity    Amabile, T. M.  1983
//
int check_TCI_citation_info(wchar_t *str, struct CITING_DOC *cd)
{
	int ptype;
	wchar_t *sp;
	
	sp = str;
	if (*sp == '<')	// skip the leading <...> if they exist
	{
		sp++;
		while (*sp != '>') sp++;
		sp++;
	}

	if (iswascii(sp[0]) && iswascii(sp[3]) && iswascii(sp[6]))
	{
		cd->type = P_ENGLISH; ptype = P_ENGLISH;
	}
	else
	{
		cd->type = P_CHINESE; ptype = P_CHINESE;
	}

	return ptype;
}

//
// remove texts enclosed in angle brackets
//
int remove_angle_brackets(wchar_t *buf)
{
	wchar_t *sp, *tp;
	int state;

	sp = tp = buf;
	state = 0;
	while (*sp != '\0')
	{
		switch (state)
		{
		case 0:
			if (*sp == '<')
			{
				state = 1;
				break;
			}
			*tp++ = *sp++;
			break;
		case 1:
			if (*sp == '>')
				state = 0;
			sp++;
			break;
		}
	}
	*tp = '\0';

	return 0;
}
