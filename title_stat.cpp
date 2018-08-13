//
// title_stat.cpp
//

//
// Revision History:
// 
// 2013/07/13 basic function works
// 
// 2013/08/20 Changed to automatically determine the beginning year for presenting title statistics (in title_stat())
// 2013/11/09 Added a new function keyword_year_matrix() to replace the function title_stat(). The new function uses a more reliable way to establish the keyword-year matrix
// 2014/05/17 Fixed the sorting problem in presenting keyword-year matrix (take keyword in abstract into consideration when sorting the keyword count)
// 2014/06/02 Added codes to handle keyword alias
// 2014/11/04 Added a check to the case that wos[i].year=0 to avoid system crash, but be carefult that year=0 is not a normal case
// 2015/03/03 Changed to determine the number of keywords (KYM_NKEYWORDS) in a flexible way (0.5% of nwos)
// 2015/06/11 Changed to close "kstream" in the function keyword_year_matrix()
// 2015/07/10 Added a new function consolidate_keywords(). It counts the appearance of keywords.
//            Added also codes to calculate tkws[].acnt2.
// 2015/08/24 Removed the codes to treat '-' as delimiter so that we can find keywords such as "catch-up", "cross-border", etc.
// 2016/01/25 Modification  : changed the file opening to Unicode mode (ccs=UTF-8)
// 2016/03/10 Modified to display keywords up to where average appearance is greater than 0.1%
// 2016/04/08 Largely extend the memory allocation size for kword and tkword, this is because TIP (patent) data can have very very long titles
// 2016/12/18 Added codes to limit the year of keyword-year to no earlier than 1900
// 2016/12/23 Added codes to limit KYM_NKEYWORDS to 10000 words
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "resource.h"
#include "network.h"
#include <windows.h>

#define BUF_SIZE 1024
extern FILE *logstream;

int stopword_search(wchar_t **, int, wchar_t *);
int is_stopwords(wchar_t *);
int tkeyword_search(wchar_t *, int, struct TKEYWORDS *);
int compare_wordcount2(const void *, const void *);
int compare_keywordx(const void *, const void *);

struct TKEYWORDS *tword = NULL;	// changed from a local to a global variable so that it can be used in critical_transition(), 2015/03/04 
int nkreport = 0;	// added 2015/03/04

extern int nwos;
extern struct WOS *wos;
extern int nkeywordalias;	// number of items in the keyword alias table
extern struct KEYWORD_ALIAS *keywordalias;

extern int index_of(int, wchar_t *, wchar_t *);

extern int keywordalias_search(struct KEYWORD_ALIAS *, int, wchar_t *);
extern int compare_tkeyword(const void *, const void *);
int compare_wordcount(const void *, const void *);

//
// stop words, this information is taken from the website: http://norm.al/2009/04/14/list-of-english-stop-words/
// added "e", "g", as stop words, 2014/07/23
//
#define N_STOPWORDS 320
wchar_t *stopwords[N_STOPWORDS] = {L"a", L"about", L"above", L"above", L"across", L"after", L"afterwards", L"again", L"against", L"all", L"almost", L"alone", L"along", L"already", L"also", L"although", L"always", L"am", L"among", L"amongst", L"amoungst", L"amount",  L"an", L"and", L"another", L"any", L"anyhow", L"anyone", L"anything", L"anyway", L"anywhere", L"are", L"around", L"as",  L"at", 
	L"back", L"be", L"became", L"because", L"become", L"becomes", L"becoming", L"been", L"before", L"beforehand", L"behind", L"being", L"below", L"beside", L"besides", L"between", L"beyond", L"bill", L"both", L"bottom", L"but", L"by", 
	L"call", L"can", L"cannot", L"cant", L"co", L"con", L"could", L"couldnt", L"cry", 
	L"de", L"describe", L"detail", L"do", L"done", L"down", L"due", L"during", 
	L"e",L"each", L"eg", L"eight", L"either", L"eleven",	L"else", L"elsewhere", L"empty", L"enough", L"etc", L"even", L"ever", L"every", L"everyone", L"everything",	L"everywhere", L"except", 
	L"few", L"fifteen", L"fify", L"fill", L"find", L"fire", L"first", L"five", L"for", L"former", L"formerly", L"forty", L"found", L"four", L"from", L"front", L"full", L"further", 
	L"g",L"get", L"give", L"go", 
	L"had", L"has", L"hasnt", L"have", L"he", L"hence", L"her", L"here", L"hereafter", L"hereby", L"herein", L"hereupon", L"hers", L"herself", L"him", L"himself", L"his", L"how", L"however", L"hundred", 
	L"ie", L"if", L"in", L"inc", L"indeed", L"interest", L"into", L"is", L"it", L"its", L"itself", 
	L"keep", 
	L"last", L"latter", L"latterly", L"least", L"less", L"ltd", 
	L"made", L"many", L"may", L"me", L"meanwhile", L"might", L"mill", L"mine", L"more", L"moreover", L"most", L"mostly", L"move", L"much", L"must", L"my", L"myself", 
	L"name", L"namely", L"neither", L"never", L"nevertheless", L"next", L"nine", L"no", L"nobody", L"none", L"noone", L"nor", L"not", L"nothing", L"now", L"nowhere", 
	L"of", L"off", L"often", L"on", L"once", L"one", L"only", L"onto", L"or", L"other", L"others", L"otherwise", L"our", L"ours", L"ourselves", L"out", L"over", L"own",
	L"part", L"per", L"perhaps", L"please", L"put", 
	L"rather", L"re", 
	L"same", L"see", L"seem", L"seemed", L"seeming", L"seems", L"serious", L"several", L"she", L"should", L"show", L"side", L"since", L"sincere", L"six", L"sixty", L"so", L"some", L"somehow", L"someone", L"something", L"sometime", L"sometimes", L"somewhere", L"still", L"such", L"system", 
	L"take", L"ten", L"than", L"that", L"the", L"their", L"them", L"themselves", L"then", L"thence", L"there", L"thereafter", L"thereby", L"therefore", L"therein", L"thereupon", L"these", L"they", L"thickv", L"thin", L"third", L"this", L"those", L"though", L"three", L"through", L"throughout", L"thru", L"thus", L"to", L"together", L"too", L"top", L"toward", L"towards", L"twelve", L"twenty", L"two", 
	L"un", L"under", L"until", L"up", L"upon", L"us", 
	L"very", L"via", 
	L"was", L"we", L"well", L"were", L"what", L"whatever", L"when", L"whence", L"whenever", L"where", L"whereafter", L"whereas", L"whereby", L"wherein", L"whereupon", L"wherever", L"whether",	L"which", L"while", L"whither", L"who", L"whoever", L"whole", L"whom", L"whose", L"why", L"will", L"with", L"within", L"without", L"would", 
	L"yet", L"you", L"your", L"yours", L"yourself", L"yourselves"};

//
// parse the title of an article, store the results to the given WOS entry
//
int parse_title(wchar_t *ptitle, int ival, int *ntkws, int *tkws, int ntkwords, struct TKEYWORDS *tkwords)
{
	int i, kwcnt, ndx2;
	int ndx, tkx;
	int tntkws;
	int bx, ex;
	wchar_t ch, *sp, *tp;
	wchar_t mp[BUF_SIZE];
	struct TKEYWORDS *kword, *tkword;
	int tbx, tex, tndx;

	kword = (struct TKEYWORDS *)Jmalloc(10000 * sizeof(struct TKEYWORDS), L"parse_title: kword");	// extend from 1000 to 10000, 2016/04/08
	if (kword == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tkword = (struct TKEYWORDS *)Jmalloc(10000 * sizeof(struct TKEYWORDS), L"parse_title: tkword");	// extend from 1000 to 10000, 2016/04/08
	if (tkword == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// 1st pass, get single word
	ndx = 0; tndx = 0; 
	tntkws = ival;
	sp = ptitle;
	tp = kword[ndx].name;
	while (*sp != '\0')
	{
		ch = towlower(*sp);
		switch (ch)
		{
			case ' ':
			case '\"':
			case '\'':
			//case '&':	// removed 2013/11/09 to keep "R&D"
			//case '-': // removed 2015/08/24 to keep phrases such as "catch-up", "cross-border", etc.
			case '(':
			case ')':
				*tp = '\0';
				if (kword[ndx].name[0] != '\0' && kword[ndx].name[0] != ' ')	// ignore empty strings and those leading with spaces
				{
					wcscpy(tkword[tndx].name, kword[ndx].name); // keep the original keyword, no matter what
					ndx2 = keywordalias_search(keywordalias, nkeywordalias, kword[ndx].name);
					if (ndx2 >= 0)	// found that this is a variation of the keyword
						wcscpy_s(kword[ndx].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
					tkx = tkeyword_search(kword[ndx].name, ntkwords, tkwords);
					ndx++; tndx++;
					if (tkx != -1) // stop words will get -1
					{
						//fwprintf(logstream, L"[%s]%d  ", kword[ndx].name, tkx); fflush(logstream);
						tkws[tntkws] = tkx;
						tntkws++; 
						if (tntkws >= MAX_TKEYWORDS)	// take only MAX_TKEYWORDS keywords
						{
							*ntkws = tntkws; Jfree(kword, L"parse_title: tkeyword");
							return tntkws;
						}
					}
				}
				tp = kword[ndx].name;
				break;
			case':':
			case ',':
			case '.':
			case '?':
				*tp = '\0';
				if (kword[ndx].name[0] != '\0' && kword[ndx].name[0] != ' ')	// ignore empty strings and those leading with spaces
				{
					wcscpy(tkword[tndx].name, kword[ndx].name); // keep the original keyword, no matter what
					ndx2 = keywordalias_search(keywordalias, nkeywordalias, kword[ndx].name);
					if (ndx2 >= 0)	// found that this is a variation of the keyword
						wcscpy_s(kword[ndx].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
					tkx = tkeyword_search(kword[ndx].name, ntkwords, tkwords);
					ndx++; tndx++;
					if (tkx != -1) // stop words will get -1
					{
						//fwprintf(logstream, L"[%s]%d  ", kword[ndx].name, tkx); fflush(logstream);
						tkws[tntkws] = tkx;
						tntkws++; 
						if (tntkws >= MAX_TKEYWORDS)	// take only MAX_TKEYWORDS keywords
						{
							*ntkws = tntkws; Jfree(kword, L"parse_title: tkeyword");
							return tntkws;
						}
					}
				}
				tp = kword[ndx].name;
				sp++;
				while (*sp == ' ') sp++;	// skip spaces before the next word
				continue;
			default:
				*tp++ = ch;
				break;
		}
		sp++;
	}
	// to handle the last word
	*tp = '\0';
	if (kword[ndx].name[0] != '\0' && kword[ndx].name[0] != ' ')	// ignore empty strings and those leading with spaces
	{
		wcscpy(tkword[tndx].name, kword[ndx].name); // keep the original keyword, no matter what
		ndx2 = keywordalias_search(keywordalias, nkeywordalias, kword[ndx].name);
		if (ndx2 >= 0)	// found that this is a variation of the keyword
			wcscpy_s(kword[ndx].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
		tkx = tkeyword_search(kword[ndx].name, ntkwords, tkwords);
		ndx++; tndx++;
		if (tkx != -1) 
		{
			//fwprintf(logstream, L"[%s]%d  ", kword[ndx].name, tkx); fflush(logstream);
			tkws[tntkws] = tkx;
			tntkws++; 
			if (tntkws >= MAX_TKEYWORDS)	// take only MAX_TKEYWORDS keywords
			{
				*ntkws = tntkws; Jfree(kword, L"parse_title: tkeyword");
				return tntkws;
			}
		}
	}

	bx = 0; ex = ndx; 
	tbx = 0; tex = tndx;
	// 2nd pass, get two-word phrases
	for (i = tbx; i < (tex -1); i++)
	{
		if (!is_stopwords(tkword[i].name) && !is_stopwords(tkword[i+1].name))	// only if the leading word and the ending word are not stopwords
		{
			swprintf(mp, L"%s %s", tkword[i].name, tkword[i+1].name);	// use the original keywords, not the converted keywords
			ndx2 = keywordalias_search(keywordalias, nkeywordalias, mp);
			if (ndx2 >= 0)	// found that this is a variation of the keyword
				wcscpy_s(mp, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
			tkx = tkeyword_search(mp, ntkwords, tkwords);			
			if (tkx != -1) // stop words will get -1
			{
				//fwprintf(logstream, L"[%s]%d  ", mp, tkx); fflush(logstream);
				tkws[tntkws] = tkx;
				tntkws++; 
				if (tntkws >= MAX_TKEYWORDS)	// take only MAX_TKEYWORDS keywords
				{
					*ntkws = tntkws; Jfree(kword, L"parse_title: tkeyword");
					return tntkws;
				}
			}		
		}
	}

	// 3rd pass, get three-word phrases
	for (i = tbx; i < (tex -2); i++)
	{
		if (!is_stopwords(tkword[i].name) && !is_stopwords(tkword[i+2].name))	// only if the leading word and the ending word are not stopwords
		{
			swprintf(mp, L"%s %s %s", tkword[i].name, tkword[i+1].name, tkword[i+2].name); // use the original keywords, not the converted keywords
			ndx2 = keywordalias_search(keywordalias, nkeywordalias, mp);
			if (ndx2 >= 0)	// found that this is a variation of the keyword
				wcscpy_s(mp, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
			tkx = tkeyword_search(mp, ntkwords, tkwords);			
			if (tkx != -1) // stop words will get -1
			{
				//fwprintf(logstream, L"[%s]%d  ", mp, tkx); fflush(logstream);
				tkws[tntkws] = tkx;
				tntkws++; 
				if (tntkws >= MAX_TKEYWORDS)	// take only MAX_TKEYWORDS keywords
				{
					*ntkws = tntkws; Jfree(kword, L"parse_title: tkeyword");
					return tntkws;
				}
			}		
		}
	}

	// 4th pass, get four-word phrases
	for (i = tbx; i < (tex -3); i++)
	{
		if (!is_stopwords(tkword[i].name) && !is_stopwords(tkword[i+3].name))	// only if the leading word and the ending word are not stopwords
		{
			swprintf(mp, L"%s %s %s %s", tkword[i].name, tkword[i+1].name, tkword[i+2].name, tkword[i+3].name); // use the original keywords, not the converted keywords
			ndx2 = keywordalias_search(keywordalias, nkeywordalias, mp);
			if (ndx2 >= 0)	// found that this is a variation of the keyword
				wcscpy_s(mp, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
			tkx = tkeyword_search(mp, ntkwords, tkwords);			
			if (tkx != -1) // stop words will get -1
			{
				//fwprintf(logstream, L"[%s]%d  ", mp, tkx); fflush(logstream);
				tkws[tntkws] = tkx;
				tntkws++; 
				if (tntkws >= MAX_TKEYWORDS)	// take only MAX_TKEYWORDS keywords
				{
					*ntkws = tntkws; Jfree(kword, L"parse_title: tkeyword");
					return tntkws;
				}
			}		
		}
	}

	*ntkws = tntkws; 
	Jfree(kword, L"parse_title: kword");
	Jfree(tkword, L"parse_title: tkword");

#ifdef DEBUG
	fwprintf(logstream, L"***%d: ", *ntkws); fflush(logstream);
	for (i = 0; i < *ntkws; i++)
	{
		fwprintf(logstream, L"[%s] ", tkwords[tkws[i]].name);
	}
	fwprintf(logstream, L"\n"); fflush(logstream);
#endif DEBUG

	return tntkws;
}

//
// parse the title of an article, store the results to the specified array "kword"
//
int parse_store_title(int ndx, struct TKEYWORDS *kword, wchar_t *ptitle)
{
	int i, ndx2;
	int cnt;
	int bx, ex;
	int tbx, tex, tndx;
	wchar_t ch, *sp, *tp;
	struct TKEYWORDS tkword[200];	// temporary spaces, extended from 50 to 100, 2014/07/06, to 200 2014/07/23

	if (*ptitle == '\0') return ndx;	// do nothing for empty title, added 2014/07/23

	// 1st pass, get single word
	bx = ndx; tndx = 0; tbx = tndx;
	sp = ptitle;
	tp = kword[ndx].name;
	while (*sp != '\0')
	{
		ch = towlower(*sp);
		switch (ch)
		{
			case ' ':
			case '\"':
			case '\'':
			//case '&':	// removed 2013/11/09 to keep "R&D"
			//case '-': // removed 2015/08/24 to keep phrases such as "catch-up", "cross-border", etc.
			case '(':
			case ')':
				*tp = '\0';
				wcscpy(tkword[tndx].name, kword[ndx].name); // keep the original keyword, no matter what
				ndx2 = keywordalias_search(keywordalias, nkeywordalias, kword[ndx].name);
				if (ndx2 >= 0)	// found that this is a variation of the keyword
					wcscpy_s(kword[ndx].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
				if (kword[ndx].name[0] != '\0' && kword[ndx].name[0] != ' ')	// ignore empty strings and those leading with spaces
				{
					ndx++; tndx++;
				}
				tp = kword[ndx].name;
				break;
			case':':
			case ',':
			case '.':
			case '?':
				*tp = '\0';
				wcscpy(tkword[tndx].name, kword[ndx].name); // keep the original keyword, no matter what
				ndx2 = keywordalias_search(keywordalias, nkeywordalias, kword[ndx].name);
				if (ndx2 >= 0)	// found that this is a variation of the keyword
					wcscpy_s(kword[ndx].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
				if (kword[ndx].name[0] != '\0' && kword[ndx].name[0] != ' ')	// ignore empty strings and those leading with spaces
				{
					ndx++; tndx++;
				}
				tp = kword[ndx].name;
				sp++;
				while (*sp == ' ') sp++;	// skip spaces before the next word
				continue;
			default:
				*tp++ = ch;
				break;
		}
		sp++;
	}
	// to handle the last word
	*tp = '\0';
	wcscpy(tkword[tndx].name, kword[ndx].name); // keep the original keyword, no matter what
	ndx2 = keywordalias_search(keywordalias, nkeywordalias, kword[ndx].name);
	if (ndx2 >= 0)	// found that this is a variation of the keyword
		wcscpy_s(kword[ndx].name, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
	if (kword[ndx].name[0] != '\0' && kword[ndx].name[0] != ' ')	// ignore empty strings and those leading with spaces
	{
		ndx++; tndx++;
	}
	tp = kword[ndx].name;
	ex = ndx; tex = tndx;

	// 2nd pass, get two-word phrases
	for (i = tbx; i < (tex - 1); i++)
	{
		if (!is_stopwords(tkword[i].name) && !is_stopwords(tkword[i+1].name))	// only if the leading word and the ending word are not stopwords
		{
			swprintf(tp, L"%s %s", tkword[i].name, tkword[i+1].name); // use the original keywords, not the converted keywords
			ndx2 = keywordalias_search(keywordalias, nkeywordalias, tp);
			if (ndx2 >= 0)	// found that this is a variation of the keyword
				wcscpy_s(tp, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
			ndx++;
			tp = kword[ndx].name;
		}
	}

	// 3rd pass, get three-word phrases
	for (i = tbx; i < (tex - 2); i++)
	{
		if (!is_stopwords(tkword[i].name) && !is_stopwords(tkword[i+2].name))	// only if the leading word and the ending word are not stopwords
		{
			swprintf(tp, L"%s %s %s", tkword[i].name, tkword[i+1].name, tkword[i+2].name); // use the original keywords, not the converted keywords
			ndx2 = keywordalias_search(keywordalias, nkeywordalias, tp);
			if (ndx2 >= 0)	// found that this is a variation of the keyword
				wcscpy_s(tp, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
			ndx++;
			tp = kword[ndx].name;
		}
	}

	// 4th pass, get four-word phrases
	for (i = tbx; i < (tex - 3); i++)
	{
		if (!is_stopwords(tkword[i].name) && !is_stopwords(tkword[i+3].name))	// only if the leading word and the ending word are not stopwords
		{
			swprintf(tp, L"%s %s %s %s", tkword[i].name, tkword[i+1].name, tkword[i+2].name, tkword[i+3].name); // use the original keywords, not the converted keywords
			ndx2 = keywordalias_search(keywordalias, nkeywordalias, tp);
			if (ndx2 >= 0)	// found that this is a variation of the keyword
				wcscpy_s(tp, MAX_KEYWORD_NAME, keywordalias[ndx2].alias);
			ndx++;
			tp = kword[ndx].name;
		}
	}

	return ndx;
}

#ifdef OBSOLETE
//
// keyword-year matrix for keywords in article titles
//
int title_stat(struct TKEYWORDS *tkeyword, int ntkeywords, struct PTITLES *ptitle, int nptitles)
{
	int i, j, k;
	int at;
	int kym_nyears;
	int beg_year;

	// automatically determine the beginning year (for statistics), added 2013/08/20
	beg_year = 3000;
	for (k = 0; k < nptitles; k++)
	{
		if (ptitle[k].year < beg_year)
			beg_year = ptitle[k].year;
	}
	kym_nyears = 2020 - beg_year + 1;	// upto year 2020

	kymatrix = (int *)Jmalloc(KYM_NKEYWORDS * kym_nyears * sizeof(int), L"title_stat: kymatrix");	
	if (kymatrix == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < KYM_NKEYWORDS * kym_nyears; i++) kymatrix[i] = 0;	// initialization

	for (k = 0; k < nptitles; k++)
	{
		for (i = 0; i < KYM_NKEYWORDS; i++)
		{
			int begin_at = 0;
			while (TRUE)
			{
				at = index_of(begin_at, ptitle[k].name, tkeyword[i].name);
				if (at == -1)
					break;
				else
				{
fwprintf(logstream, L"[%s] [%s]\n", tkeyword[i].name, ptitle[k].name);
					kymatrix[KYM_NKEYWORDS*(ptitle[k].year-beg_year)+i]++;
					begin_at = at + 1;	// continue from the next position
				}
			}
		}
	}
	// print the results
	fwprintf(logstream, L"\nYear");
	for (i = 0; i < KYM_NKEYWORDS; i++)
		fwprintf(logstream, L"\t%s", tkeyword[i].name);
	fwprintf(logstream, L"\n");
	for (j = 0; j < kym_nyears; j++)
	{
		fwprintf(logstream, L"%d\t", j+beg_year);
		for (i = 0; i < KYM_NKEYWORDS; i++)
			fwprintf(logstream, L"%d\t", kymatrix[KYM_NKEYWORDS*j+i]);
		fwprintf(logstream, L"\n", tkeyword[i].name);

	}
	fwprintf(logstream, L"\n");

	free(kymatrix);	// added 2013/08/19

	return 0;
}
#endif OBSOLETE

//
// calculate and print keyword-year matrix
//
// #define KYM_NKEYWORDS 500	// removed, 2015/03/03
int *kymatrix;
int keyword_year_matrix(int ntkeywords, struct TKEYWORDS *tkeyword)
{	
	int i, j, k;
	int ndx;
	int at;
	int kym_nyears;
	int beg_year;
	FILE *kstream;
	wchar_t line[BUF_SIZE], *tline;
	int KYM_NKEYWORDS;	// added, 2015/03/03

	// calculate the "title+abstract" word counts for all keywords ("title" word counts was calculated when reading data)	
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].ntkws; k++)
		{
			tkeyword[wos[i].tkws[k]].acnt += 1;		// each keyword count only once for each article, modified 2015/07/10
			tkeyword[wos[i].tkws[k]].acnt2 += wos[i].tkws_cnt[k];	// each keyword count as many times as it appears for each artilce, added 2015/07/10
		}
	}

	qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_wordcount2);	// added 2014/05/17
	for (i = 0; i < ntkeywords; i++) tkeyword[i].ranking = i;	// rank begins from 0

	KYM_NKEYWORDS = ntkeywords;	// default, added 2015/03/03
	for (i = 0; i < ntkeywords; i++)	// added 2015/03/03
	{
		//fwprintf(logstream, L"##### %04d\t[%s]\t%04d\t%d\n", i, tkeyword[i].name, tkeyword[i].acnt, 1000 * tkeyword[i].acnt / nwos);
		if ((1000 * tkeyword[i].acnt / nwos) < 1)	// want to display up to where average appearance is greater than 0.5%, changed to 0.1% on 2016/03/10
		{
			KYM_NKEYWORDS = i + 1;
			break;
		}
	}
	if (KYM_NKEYWORDS > 10000) KYM_NKEYWORDS = 10000;	// set an upper limit, added 201/12/23
	fwprintf(logstream, L"\nNumber of keywords: %d\n", KYM_NKEYWORDS);
	qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_tkeyword); // sort again, back to the alphabetical order
	
	// automatically determine the beginning year (for statistics)
	beg_year = 3000;
	for (i = 0; i < nwos; i++)
	{
		if (wos[i].year <= 0) continue;	// ignore years that at zero or negative
		if (wos[i].year < beg_year)
			beg_year = wos[i].year;
	}
	if (beg_year < 1900) beg_year = 1900;	// no earlier than 1900, added 2016/12/18
	kym_nyears = 2020 - beg_year + 1;	// upto year 2020

	kymatrix = (int *)Jmalloc(KYM_NKEYWORDS * kym_nyears * sizeof(int), L"title_stat: kymatrix");	
	if (kymatrix == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < KYM_NKEYWORDS * kym_nyears; i++) kymatrix[i] = 0;	// initialization

	// establish the keyword-year matrix
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].ntkws; k++)
		{
			if (tkeyword[wos[i].tkws[k]].ranking < KYM_NKEYWORDS)
			{
				//fwprintf(logstream, L"i=%d k=%d %s %d\n", i, k, wos[i].alias, wos[i].tkws[k]); fflush(logstream);
				//fwprintf(logstream, L"ranking=%d\n", tkeyword[wos[i].tkws[k]].ranking); fflush(logstream);
				if (wos[i].year > 0)	// this check is added 2014/11/04, but be aware that year=0 is not a normal case
					kymatrix[KYM_NKEYWORDS*(wos[i].year-beg_year)+tkeyword[wos[i].tkws[k]].ranking]++;
			}
		}
	}

	qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_wordcount2);	// added 2014/05/17
	// print the results
	fwprintf(logstream, L"\nYear");
	for (i = 0; i < KYM_NKEYWORDS; i++)
		fwprintf(logstream, L"\t%s", tkeyword[i].name);
	fwprintf(logstream, L"\nCount (Title only)");
	for (i = 0; i < KYM_NKEYWORDS; i++)
		fwprintf(logstream, L"\t%d", tkeyword[i].cnt);
	fwprintf(logstream, L"\nCount (Title+Abstract)");
	for (i = 0; i < KYM_NKEYWORDS; i++)
		fwprintf(logstream, L"\t%d", tkeyword[i].acnt);
	fwprintf(logstream, L"\nCount (Title+Abstract, allow duplicates)");	// added 2015/07/10
	for (i = 0; i < KYM_NKEYWORDS; i++)
		fwprintf(logstream, L"\t%d", tkeyword[i].acnt2);
	fflush(logstream);

	qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_tkeyword); // sort again, back to the alphabetical order
	fwprintf(logstream, L"\n");

	for (j = 0; j < kym_nyears; j++)
	{
		fwprintf(logstream, L"%d\t", j+beg_year);
		for (i = 0; i < KYM_NKEYWORDS; i++)
			fwprintf(logstream, L"%d\t", kymatrix[KYM_NKEYWORDS*j+i]);
		fwprintf(logstream, L"\n");

	}
	fwprintf(logstream, L"\n");

	// if the "Keyword report.txt" file is provided, print keyword report accordingly
	if (_wfopen_s(&kstream, L"Keyword report.txt", L"rt, ccs=UTF-8") == 0)	// open as UTF-8 file	
	{
		int *kmatrix;	// keyword matrix that includes only the specified keywords
		int nk, kk, ndx2;
		// count the number of specified keywords
		nk = 0;
		while (TRUE)
		{		
			if(fgetws(line, BUF_SIZE, kstream) == NULL)
				break;		
			if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
				continue;
			nk++;
		}
		nkreport = nk;
		fwprintf(logstream, L"Number of specificed keywords in the \"Keyword report.txt\" file: %d\n", nk);
		kmatrix = (int *)Jmalloc(nk * kym_nyears * sizeof(int), L"keyword_year_matrix: kmatrix");	
		if (kmatrix == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (i = 0; i < nk * kym_nyears; i++) kmatrix[i] = 0;	// initialization
		tword = (struct TKEYWORDS *)Jmalloc(nk * sizeof(struct TKEYWORDS), L"keyword_year_matrix: tword");	
		if (tword == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (i = 0; i < nk; i++) tword[i].cross_ndx = -1;
		for (i = 0; i < ntkeywords; i++) tkeyword[i].cross_ndx = -1;
		// prepare the shortened matrix
		rewind(kstream);
		kk = 0;
		while (TRUE)
		{		
			if(fgetws(line, BUF_SIZE, kstream) == NULL)
				break;		
			if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
				continue;	
			tline = line;
			if (kk == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
			{
				if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
					tline = &line[1];	// skip the BOM
			}
			wchar_t *tp, *sp;
			sp = tp = tline; while (*sp !='\0' && *sp != '\n') *tp++ = towlower(*sp++); *tp = '\0';	
			ndx = tkeyword_search(tline, ntkeywords, tkeyword);	
			if (ndx >= 0)	
			{
				tword[kk].cross_ndx = ndx; tkeyword[ndx].cross_ndx = kk;	// cross reference between the two array
				wcscpy(tword[kk].name, tkeyword[ndx].name);
				tword[kk].acnt = tkeyword[ndx].acnt;
				tword[kk].acnt2 = tkeyword[ndx].acnt2;
				tword[kk].cnt = tkeyword[ndx].cnt;
				ndx2 = tkeyword[ndx].ranking;
				for (j = 0; j < kym_nyears; j++)	// move the corresponding data over to the shortened matrix
				{
					if (ndx2 < KYM_NKEYWORDS)
						kmatrix[nk*j+kk] = kymatrix[KYM_NKEYWORDS*j+ndx2];
				}
			}
			else // keyword that makes no sense
			{
				wcscpy(tword[kk].name, tline);
				tword[kk].acnt = 0;
				tword[kk].acnt2 = 0;
				tword[kk].cnt = 0;
				for (j = 0; j < kym_nyears; j++)	// move the corresponding data over to the shortened matrix
					kmatrix[nk*j+kk] = 0;
			}
			kk++;
		}
		// print the results
		fwprintf(logstream, L"\nYear");
		for (i = 0; i < nk; i++)
			fwprintf(logstream, L"\t%s", tword[i].name);
		fwprintf(logstream, L"\nCount (Title only)");
		for (i = 0; i < nk; i++)
			fwprintf(logstream, L"\t%d", tword[i].cnt);
		fwprintf(logstream, L"\nCount (Title+Abstract)");
		for (i = 0; i < nk; i++)
			fwprintf(logstream, L"\t%d", tword[i].acnt);
		fwprintf(logstream, L"\nCount (Title+Abstract, allow duplicates)");	// added 2015/07/10
		for (i = 0; i < nk; i++)
			fwprintf(logstream, L"\t%d", tword[i].acnt2);

		//qsort((void *)tkeyword, (size_t)ntkeywords, sizeof(struct TKEYWORDS), compare_tkeyword); // sort again, back to the alphabetical order
		fwprintf(logstream, L"\n");
		for (j = 0; j < kym_nyears; j++)
		{
			fwprintf(logstream, L"%d\t", j+beg_year);
			for (i = 0; i < nk; i++)
				fwprintf(logstream, L"%d\t", kmatrix[nk*j+i]);
			fwprintf(logstream, L"\n");

		}
		fwprintf(logstream, L"\n");
		Jfree(kmatrix, L"keyword_year_matrix: kmatrix");
		fclose(kstream);	// added 2015/06/11
	}

	Jfree(kymatrix, L"keyword_year_matrix: kymatrix");

	return 0;
}

//
//
//
int is_stopwords(wchar_t *str)
{
	int i;
	int ret;

	ret = stopword_search(stopwords, N_STOPWORDS, str);

	if (ret < 0)
		return FALSE;
	else
		return TRUE;
}


//
// use binary search to find the proper position of a stopwords 
//
int stopword_search(wchar_t *d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0]) < 0)
		return -1;
	if (wcscmp(str, d[num-1]) > 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur]) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high]) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur]) > 0)
			low = cur;
		else
			high = cur;
	}
}


//
// use binary search to find the proper position of a keyword in an "TKEYWORDS" array
//
int tkeyword_search(wchar_t *str, int num, struct TKEYWORDS d[])
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
// NOTE: order from large to small
// 
int compare_wordcount2(const void *n1, const void *n2)
{
	struct TKEYWORDS *t1, *t2;
	
	t1 = (struct TKEYWORDS *)n1;
	t2 = (struct TKEYWORDS *)n2;
	if (t2->acnt > t1->acnt)
		return 1;
	else if (t2->acnt == t1->acnt)
		return 0;
	else return -1;
}

//
// this function is added 2015/07/10
// rearrange the keywords for each article
// count the number of duplicated keywords
//
struct KEYWORD_IND { int ind; };
int consolidate_keywords()
{
	int i, k, ii, cnt, cnt1, cnt2;
	int prev;
	struct KEYWORD_IND c[MAX_TKEYWORDS];

	// initialization
	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].ntkws; k++)
			wos[i].tkws_cnt[k] = 1;
	}

	for (i = 0; i < nwos; i++)
	{
		if (wos[i].ntkws == 0) continue;
		cnt = 0;
		for (k = 0; k < wos[i].ntkws; k++)
			c[cnt++].ind = wos[i].tkws[k];
		qsort((void *)c, (size_t)cnt, sizeof(struct KEYWORD_IND), compare_keywordx);

		cnt1 = 0; cnt2 = 0;
		prev = c[0].ind;
		for (ii = 0; ii < cnt; ii++)
		{
			if (c[ii].ind != prev)	// hit a new keyword
			{
				wos[i].tkws[cnt1] = prev;
				wos[i].tkws_cnt[cnt1] = cnt2;
				cnt2 = 1; cnt1++;
				prev = c[ii].ind;
			}
			else
				cnt2++;
		}
		wos[i].tkws[cnt1] = prev;
		wos[i].tkws_cnt[cnt1] = cnt2;
		wos[i].ntkws = cnt1+1;
	}

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_keywordx(const void *n1, const void *n2)
{
	struct KEYWORD_IND *t1, *t2;
	
	t1 = (struct KEYWORD_IND *)n1;
	t2 = (struct KEYWORD_IND *)n2;
	if (t2->ind < t1->ind)
		return 1;
	else if (t2->ind == t1->ind)
		return 0;
	else return -1;
}
