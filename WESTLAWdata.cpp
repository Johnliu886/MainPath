// 
// WESTLAWdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles WESTLAW data 
//
//

//
// Revision History:
// 2012/04/25 Basic function works
// 2012/04/28 Added codes to take "Negative Cases", "Positive Cases", "Examined", "Discussed" and "Mentioned" into consideration.
//               Previoius code considers only "Cited".
// 2012/05/02 Fixed the problem of taking in the negative cases even though the negative cases are not selected
// 2012/05/02 Fixed a problem. Should not open citation when the citation file is given.
// 2012/05/05 Changed to display the informations of all the cases in the log file. 
// 2012/05/19 Added codes to detect the depth (the number of *s) for negative cases from the case string
//            From now on, the negative cases also have depth level.
// 2012/06/23 Added codes to handle the case that a case id referred to in a citation is ended not with ',', but with a ' ('
// 2013/02/05 Added codes to read-in the 2nd case ID if it exist
// 2013/02/05 Added function replace_alias_case_id() to modify citation file (replace the 2nd case id with the 1st case id)
// 2013/02/19 Added codes to close opened file if not exiting from the end of the function read_WL()
// 2012/02/21 Added codes to handle "Disapproval Recognized by"
// 2013/03/12 Fixed yet another problem of taking in the negative cases even though the negative cases are not selected
// 2015/06/03 Modify codes so that "Overruled in Part by" can be recognized
// 2015/06/03 Modify codes to take 4 "Cited as" ids into consideration 
// 2015/07/30 Added "Deciding" as a keyword before year information (appeared in 395 U.S. 100)
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
#include <io.h>

#define XXLBUF_SIZE 65536
#define BUF_SIZE 1024

int read_WL(wchar_t *, int, FILE *, CITE_OPTIONS *);
int parse_case_string(wchar_t *, wchar_t *, double, FILE *, CITE_OPTIONS *);
int parse_citeas_string(wchar_t *, int);
int compare_csid(const void *, const void *);
int case_alias_search(struct CASEID_ALIAS *, int, wchar_t *);
extern int parse_line(wchar_t *, int, wchar_t *, wchar_t *, double *);

int nlawcases;
struct LAWCASE *lawcase;

extern int link_author_WOS();
extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array, inventors are stored in the same array
extern FILE *logstream;
extern int no_wos_citation_file;

#define MNSIZE 12
struct MONTHS 
{
	wchar_t mn[MNSIZE];
	int len;
};

#define NMSTRINGS 24
struct MONTHS mnth[NMSTRINGS] = {
L"January ", 8,
L"February ", 9,
L"March ", 6,
L"April ", 6,
L"May ", 4,
L"June ", 5,
L"July ", 5,
L"August ", 7,
L"September ", 10,
L"October ", 8,
L"November ", 9,
L"December ", 9,
L"Jan. ", 5,
L"Feb. ", 5,
L"Mar. ", 5,
L"Apr. ", 5,
L"Jun. ", 5,
L"Jul. ", 5,
L"Aug. ", 5,
L"Sep. ", 5,
L"Oct. ", 5,
L"Nov. ", 5,
L"Dec. ", 5,
L"Sept. ", 6,
};

//
// read WESTLAW data 
//
int read_WESTLAW(wchar_t *dname, wchar_t *tname, CITE_OPTIONS *opt)
{
	int i, k;
	wchar_t fname[FNAME_SIZE];
	wchar_t cname[FNAME_SIZE], *sp, *tp;
	FILE *cstream;		// for citation file
	struct _wfinddata_t fs;
	intptr_t hFile;
	int ccnt;
	int ret;

	// 1st pass, count the number of law cases
	swprintf(fname, L"%s\\*.txt", dname);
	if ((hFile = _wfindfirst(fname, &fs)) == -1L)
		return MSG_WESTLAW_FILE_NOTFOUND;
	nlawcases = 0;
	do	
	{	
		nlawcases++;
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);

	// allocate memory for the list of USPTO data
	lawcase = (struct LAWCASE *)malloc(nlawcases * sizeof(struct LAWCASE));
	if (lawcase == NULL) return MSG_NOT_ENOUGH_MEMORY;
			
	if (no_wos_citation_file == 1)	// citation file is not given, the normal situation
	{
		// open the citation file (for writing)
		ret = swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname);	// 2013/02/19, changed from dname to tname
		if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
		if (_wfopen_s(&cstream, cname, L"w") != 0)
			return MSG_CFILE_CANNOTOPEN;
	}

	// 2nd pass, open and parse the Weatlaw data file one by one	
	if ((hFile = _wfindfirst(fname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	ccnt = 0;
	do	
	{	
		swprintf(fname, L"%s\\%s", dname, fs.name);
		if ((ret = read_WL(fname, ccnt, cstream, opt)) != 0)
			return ret;
		ccnt++;
	} 
	while (_wfindnext(hFile, &fs) == 0);

	_findclose(hFile);
	if (no_wos_citation_file == 1)	fclose(cstream);

	//for (i = 0; i < nlawcases; i++)
	//	fwprintf(logstream, L"%d: %s %s %04d %04d\n", i, lawcase[i].flag, lawcase[i].cid, lawcase[i].year, lawcase[i].ayear);

	// assign virtual plaintiff data for now
	naus = nlawcases;
	authors = (struct AUTHORS *)malloc(naus * sizeof(struct AUTHORS));	// estimate in average 1 plaintiff per document
	if (authors == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < naus; i++)
	{
		swprintf(authors[i].name, L"Plaintiff%03d", i+1);
	}
	for (i = 0; i < nlawcases; i++)
	{
		lawcase[i].nplaintiff = 1;
		lawcase[i].plaintiff[0] = i;
		lawcase[i].tc = 1;
	}

	nwos = nlawcases;
	// allocate memory for the list of WOS data
	wos = (struct WOS *)malloc(nwos * sizeof(struct WOS));
	if (wos == NULL) return MSG_NOT_ENOUGH_MEMORY;
	// move proper information to the wos[] array, because all the following action use data in the wos[] array 
	for (i = 0; i < nlawcases; i++)
	{
		wcscpy_s(wos[i].docid, MAX_DOC_ID, lawcase[i].cid);
		wos[i].nau = lawcase[i].nplaintiff;
		for (k = 0; k < lawcase[i].nplaintiff; k++)
			wos[i].author[k] = lawcase[i].plaintiff[k];
		wos[i].year = lawcase[i].year;
		wos[i].tc = lawcase[i].tc;
	}

	// append the year information to the patent number as alias
	for (i = 0; i < nwos; i++)
		swprintf_s(wos[i].alias, MAX_ALIAS, L"%s_%d", wos[i].docid, wos[i].year);

	link_author_WOS();		

	return 0;
}

#define LOOKING_FOR_CASE_ID 1
#define LOOKING_FOR_FLAG_TYPE 2
#define LOOKING_FOR_DATE 3
#define LOOKING_FOR_DECIDED_DATE 4
#define LOOKING_FOR_MARK_KEYCITE 5
#define LOOKING_FOR_CASE_LINE1 6
#define LOOKING_FOR_CASE_LINE2 7
#define LOOKING_FOR_MARK_CITING_REFERENCES 8
#define LOOKING_FOR_MARK_NEGPOS 9
#define LOOKING_FOR_MARK_SOURCEN 10
#define LOOKING_FOR_CITED_CASESN 11
#define LOOKING_FOR_CITED_CASESN1 12
#define LOOKING_FOR_CITED_CASESN2 13
#define LOOKING_FOR_MARK_POSITIVE 14
#define LOOKING_FOR_MARK_CITED_P 15
#define LOOKING_FOR_CITED_CASESP 16
#define LOOKING_FOR_CITED_CASESP1 17
#define LOOKING_FOR_CITED_CASESP2 18
#define LOOKING_FOR_NOTHING 99
//
// read and parse the content of one Westlaw case
//
int read_WL(wchar_t *fname, int ccnt, FILE *cstream, CITE_OPTIONS *opt)
{
	int i;
	int n_leadingspaces;
	int argued, decided;
	FILE *sstream;
	int state;
	wchar_t bfr[LBUF_SIZE], *bptr;
	wchar_t line[XXLBUF_SIZE], *lptr, *tp;
	double weight;	// the KEYCITE depath, from 1 (stars) to 4 (stars), can be negative if it is cited negatively

	if (_wfopen_s(&sstream, fname, L"r") != 0)
		return MSG_WOSFILE_NOTFOUND;

	state = LOOKING_FOR_CASE_ID;
	while (TRUE)
	{		
		lptr = line;
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		n_leadingspaces = 0; 
		while (*lptr == ' ' || *lptr == '\t') { lptr++; n_leadingspaces++; }
		switch (state)
		{
		case LOOKING_FOR_CASE_ID:	
			if (wcsncmp(lptr, L"(Cite as:", 8) == 0)
			{
				//fwprintf(logstream, L"\nCASE ID: %s", lptr);
				parse_citeas_string(&lptr[10], ccnt);
				//fwprintf(logstream, L"\n==> [%s] [%s]\n", lawcase[ccnt].cid, lawcase[ccnt].cid2);
				state = LOOKING_FOR_FLAG_TYPE; 
			}
			break;
		case LOOKING_FOR_FLAG_TYPE:		// flag type follows "(Cite as:" string
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines
			//fwprintf(logstream, L"    FLAG TYPE: %s", lptr);
			tp = lawcase[ccnt].flag;
			while (*lptr != ' ' && *lptr != '\n' && *lptr != '\r') *tp++ = *lptr++; 
			*tp = '\0';
			state = LOOKING_FOR_DATE; argued = decided = FALSE;
			break;
		case LOOKING_FOR_DATE:	
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines
			if (n_leadingspaces >= 20)	// so that we don't have to check all the lines
			{
				// some cases are labeled the date with "Argued:/Heard" and followed by "Decided"
				if (wcsncmp(lptr, L"Argued and Submitted", 20) == 0) { lptr += 20; argued = TRUE; }
				else if (wcsncmp(lptr, L"Argued", 6) == 0) { lptr += 6; argued = TRUE; }
				else if (wcsncmp(lptr, L"Heard", 5) == 0) { lptr += 5; argued = TRUE; }
				else if (wcsncmp(lptr, L"Submitted", 9) == 0) { lptr += 9; argued = TRUE; }
				else if (wcsncmp(lptr, L"Decided and Filed", 17) == 0) { lptr += 17; argued = FALSE; }
				else if (wcsncmp(lptr, L"Decided", 7) == 0) { lptr += 7; argued = FALSE; }
				else if (wcsncmp(lptr, L"Deciding", 7) == 0) { lptr += 7; argued = FALSE; }	// added 2015/07/30
				else if (wcsncmp(lptr, L"DECIDED", 7) == 0) { lptr += 7; argued = FALSE; }
				while (*lptr == ':') lptr++;	// skip the ':' follows
				while (*lptr == ' ') lptr++;	// skip the spaces follows
				for (i = 0; i < NMSTRINGS; i++)
				{
					if (wcsncmp(lptr, mnth[i].mn, mnth[i].len) == 0)
					{
						if (argued == FALSE)
						{
							//fwprintf(logstream, L"    DATE: %s", lptr);
							bptr = bfr; lptr += mnth[i].len;
							while (*lptr != ',') lptr++;	lptr += 2; // standard date format "March 12, 2011."
							while (*lptr != '.') *bptr++ = *lptr++;	*bptr = '\0';
							lawcase[ccnt].year = _wtoi(bfr);
							lawcase[ccnt].ayear = 0;
							fwprintf(logstream, L"%d: %s [%s] %04d %04d\n", ccnt+1, lawcase[ccnt].flag, lawcase[ccnt].cid, lawcase[ccnt].year, lawcase[ccnt].ayear);		
							if (no_wos_citation_file == 1)	// citation file is not given, the normal situation
								state = LOOKING_FOR_MARK_KEYCITE; 
							else
							{
								fclose(sstream);	// added 2013/02/19
								return 0;
							}
						}
						else
						{
							//fwprintf(logstream, L"    Argued/Heard/Submitted DATE: %s", lptr);
							bptr = bfr; lptr += mnth[i].len;
							while (*lptr != ',') lptr++;	lptr++; // standard date format "March 12, 2011."
							while (*lptr != '.') *bptr++ = *lptr++;	*bptr = '\0';
							lawcase[ccnt].ayear = _wtoi(bfr);
							state = LOOKING_FOR_DECIDED_DATE; 
						}
						break;
					}
				}
			}
			break;
		case LOOKING_FOR_DECIDED_DATE:	
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines
			if (n_leadingspaces >= 20)	// so that we don't have to check all the lines
			{				
				// some cases are labeled the date with "Argued:/Heard" and followed by "Decided"
				if (wcsncmp(lptr, L"Decided and Filed", 17) == 0) { lptr += 17; decided = TRUE; }
				else if (wcsncmp(lptr, L"DECIDED", 7) == 0) { lptr += 7; decided = TRUE; }
				else if (wcsncmp(lptr, L"Decided", 7) == 0) { lptr += 7; decided = TRUE; }
				else if (wcsncmp(lptr, L"Filed", 5) == 0) { lptr += 5; decided = TRUE; }
				while (*lptr == ':') lptr++;	// skip the ':' follows
				while (*lptr == ' ') lptr++;	// skip the spaces follows
				for (i = 0; i < NMSTRINGS; i++)
				{
					if (wcsncmp(lptr, mnth[i].mn, mnth[i].len) == 0)
					{
						//fwprintf(logstream, L"    Decided DATE: %s", lptr);
						bptr = bfr; lptr += mnth[i].len;
						while (*lptr != ',') lptr++;	lptr++; // standard date format "March 12, 2011."
						while (*lptr != '.') *bptr++ = *lptr++;	*bptr = '\0';
						lawcase[ccnt].year = _wtoi(bfr);
						fwprintf(logstream, L"\%d: %s [%s] %04d %04d\n", ccnt+1, lawcase[ccnt].flag, lawcase[ccnt].cid, lawcase[ccnt].year, lawcase[ccnt].ayear);		
						if (no_wos_citation_file == 1)	// citation file is not given, the normal situation
							state = LOOKING_FOR_MARK_KEYCITE; 
						else
						{
							fclose(sstream);	// added 2013/02/19
							return 0;	// if the citation file is given, done!
						}
						break;
					}
				}
			}
			break;
		case LOOKING_FOR_MARK_KEYCITE:
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines
			if (n_leadingspaces >= 20)	// so that we don't have to check all the lines
			{	
				if (wcsncmp(lptr, L"KEYCITE", 7) == 0)
				{
					//fwprintf(logstream, L"    KEYCITE: %s", lptr);
					state = LOOKING_FOR_CASE_LINE1; 
				}
			}
			break;
		case LOOKING_FOR_CASE_LINE1:
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines
			//fwprintf(logstream, L"    %s", lptr);
			state = LOOKING_FOR_CASE_LINE2; 
			break;
		case LOOKING_FOR_CASE_LINE2:
			//fwprintf(logstream, L"    %s", lptr);
			state = LOOKING_FOR_MARK_CITING_REFERENCES;
			break;
		case LOOKING_FOR_MARK_CITING_REFERENCES:
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines	
			state = LOOKING_FOR_MARK_NEGPOS; 
			break;
		case LOOKING_FOR_MARK_NEGPOS:
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines				
			if (n_leadingspaces >= 20)	// so that we don't have to check all the lines
			{	
				if (wcsncmp(lptr, L"Negative Cases", 14) == 0)
				{
					fwprintf(logstream, L"    NEGATIVE CASES:\n");
					state = LOOKING_FOR_MARK_SOURCEN; 
				}
				else if (wcsncmp(lptr, L"Positive Cases", 14) == 0)
				{
					fwprintf(logstream, L"    POSITIVE CASES:\n");
					state = LOOKING_FOR_MARK_CITED_P; 
				}
			}
			break;
		case LOOKING_FOR_MARK_SOURCEN:
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines	
			if (wcsncmp(lptr, L"Positive Cases", 14) == 0)
			{
				fwprintf(logstream, L"    POSITIVE CASES:\n");
				state = LOOKING_FOR_MARK_CITED_P; 
				break;
			}
			else if (wcsncmp(lptr, L"Declined", 8) == 0)	// "Declined to Extend by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Distinguished", 13) == 0)	// "Distinguished by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Called", 6) == 0)	// "Called into Doubt by" 
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Rejection", 9) == 0)	// "Rejection Recognized by" 
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Rejected", 8) == 0)	// "Rejected by" 
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Disagree", 8) == 0)	// "Disagreed With by" "Disagreement Recognized by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Not", 3) == 0)		// "Not Followed as Dicta"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Overrul", 7) == 0)		// "Overruling Recognized by" "Overruled in Part by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Abrogat", 7) == 0)		// "Abrogation Recognized by" "Abrogated by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Superseded", 10) == 0)		// "Superseded by Statute as Stated in"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Criticized", 10) == 0)		// "Criticized by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Implied", 7) == 0)		// "Implied Overruling Recognized by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Among", 5) == 0)		// "Among Conflicting Authorities Noted in"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Modifi", 6) == 0)		// "Modification Recognized by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Holding", 7) == 0)		// "Holding Limited by"
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			else if (wcsncmp(lptr, L"Disapproval", 11) == 0)	// "Disapproval Recognized by", added 2013/02/21
			{
				weight = -1.0; fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_CITED_CASESN; 
			}
			break;
		case LOOKING_FOR_CITED_CASESN:	// should encounter an empty line here
			state = LOOKING_FOR_CITED_CASESN1;
			break;
		case LOOKING_FOR_CITED_CASESN1:	// expecting the 1st line of the case description
			if (line[0] == '\n' || line[0] == '\r')		// but encountered an empty line
			{
				state = LOOKING_FOR_MARK_SOURCEN; 
				break;
			}
			else
			{
				bptr = bfr;
				while (*lptr != '\0')
					*bptr++ = *lptr++;	// copy the string 
				bptr--;	// remove the line feed at the end of the line
				*bptr++ = ' ';	// pad a space
				state = LOOKING_FOR_CITED_CASESN2;
			}
			break;
		case LOOKING_FOR_CITED_CASESN2:	// expecting the rest of the case description
			if (line[0] == '\n' || line[0] == '\r')		// end of the case description
			{
				*bptr = '\0';
				//fwprintf(logstream, L"    %s\n", bfr);
				parse_case_string(bfr, lawcase[ccnt].cid, weight, cstream, opt);
				state = LOOKING_FOR_CITED_CASESN1;
			}
			else
			{
				while (*lptr != '\0') *bptr++ = *lptr++;	// continue copying the string 
				bptr--;
			}
			break;
		case LOOKING_FOR_MARK_CITED_P:
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines		
			if (wcsncmp(lptr, L"*", 1) == 0)
			{	
				while (*lptr == '*') lptr++;	// skip *'s
				while (*lptr == ' ' || *lptr == '\t') lptr++;	// skip spaces
				if (opt->include_examined == 1 && wcsncmp(lptr, L"Examined", 8) == 0) // 4 stars
				{
					weight = 4.0; fwprintf(logstream, L"    %s", lptr);
					state = LOOKING_FOR_CITED_CASESP; 
				}
				else if (opt->include_discussed == 1 && wcsncmp(lptr, L"Discussed", 9) == 0) // 3 stars
				{
					weight = 3.0; fwprintf(logstream, L"    %s", lptr);
					state = LOOKING_FOR_CITED_CASESP; 
				}
				else if (opt->include_cited == 1 && wcsncmp(lptr, L"Cited", 5) == 0) // 2 stars
				{
					weight = 2.0; fwprintf(logstream, L"    %s", lptr);
					state = LOOKING_FOR_CITED_CASESP; 
				}
				else if (opt->include_mentioned == 1 && wcsncmp(lptr, L"Mentioned", 9) == 0) // 1 stars
				{
					weight = 1.0; fwprintf(logstream, L"    %s", lptr);
					state = LOOKING_FOR_CITED_CASESP; 
				}
			}
			break;
		case LOOKING_FOR_CITED_CASESP:	// should encounter an empty line here
			state = LOOKING_FOR_CITED_CASESP1;
			break;
		case LOOKING_FOR_CITED_CASESP1:	// expecting the 1st line of the case description
			if (line[0] == '\n' || line[0] == '\r')		// but encountered an empty line
			{
				state = LOOKING_FOR_MARK_CITED_P; 
				break;
			}
			else
			{
				bptr = bfr;
				while (*lptr != '\0')
					*bptr++ = *lptr++;	// copy the string 
				bptr--;	// remove the line feed at the end of the line
				*bptr++ = ' ';	// pad a space
				state = LOOKING_FOR_CITED_CASESP2;
			}
			break;
		case LOOKING_FOR_CITED_CASESP2:	// expecting the rest of the case description
			if (line[0] == '\n' || line[0] == '\r')		// end of the case description
			{
				*bptr = '\0';
				//fwprintf(logstream, L"    %s\n", bfr);
				parse_case_string(bfr, lawcase[ccnt].cid, weight, cstream, opt);
				state = LOOKING_FOR_CITED_CASESP1;
			}
			else
			{
				while (*lptr != '\0') *bptr++ = *lptr++;	// continue copying the string 
				bptr--;
			}
			break;
		default:
			break;
		}
	}
	fclose(sstream);

	return 0;
}

#define LK_FOR_CASE_FLAG 1
#define LK_FOR_A_NUMBER 2
#define GETTING_A_NUMBER 3
#define LK_FOR_CASE_TITLE 4
#define GETTING_CASE_TITLE1 5
#define GETTING_CASE_TITLE2 6
#define GETTING_CASE_TITLE3 7
#define LK_FOR_CASE_ID 9
#define LK_FOR_CASE_DEPTH1 10	// for negative cases only
#define LK_FOR_CASE_DEPTH2 11	// for negative cases only
#define LK_FOR_NOTHING 99
//
// parse a Westlaw case string, follwing is a typical case string:
// H   128   Medimmune, LLC v. PDL Biopharma, INC., 2010 WL 760443, *2 (N.D.Cal. Mar 04, 2010) (NO. C 08-5590 JF (HRL))  " HN: 5 (F.3d)
//
int parse_case_string(wchar_t *str, wchar_t *from, double weight, FILE *cstream, CITE_OPTIONS *opt)
{
	int i;
	int state;
	int neg_depth;
	wchar_t bfr[LBUF_SIZE], *tp, *sp;
	wchar_t cid[LBUF_SIZE];

	sp = str; tp = bfr;
	state = LK_FOR_CASE_FLAG;
	while (TRUE)
	{		
		if(*sp == '\0') 
		{	
			if (state == LK_FOR_CASE_DEPTH2)	// the negative case string ends with *s
			{
				*tp = '\0'; 
				fwprintf(logstream, L"%d*\n", neg_depth);
				if ((opt->include_1star && neg_depth == 1) || (opt->include_2star && neg_depth == 2)
					|| (opt->include_3star && neg_depth == 3) || (opt->include_4star && neg_depth == 4))	// added 2013/03/12
				{
					weight = -1 * neg_depth;
					fwprintf(cstream, L"%s %s %.0f\n", from, cid, weight);	// write out citation information for negative cases
				}
			}
			break;
		}
		switch (state)
		{
		case LK_FOR_CASE_FLAG:	// NOTE: for "YEL FLG" take only the "YEL"
			if (*sp == ' ' || isdigit(*sp))	
			{
				*tp = '\0'; 
				if (weight <= 0.0) 
					fwprintf(logstream, L"    %d  ", (int)weight);
				else
					fwprintf(logstream, L"    %d*  ", (int)weight);
				fwprintf(logstream, L"%s\t", bfr);
				tp = bfr; 
				state = LK_FOR_A_NUMBER;
				break;
			}
			else
				*tp++ = *sp++;
			break;
		case LK_FOR_A_NUMBER:	
			if (isdigit(*sp))
			{
				*tp++ = *sp++;
				state = GETTING_A_NUMBER;
			}
			else
				sp++;
			break;
		case GETTING_A_NUMBER:	
			if (isdigit(*sp))
				*tp++ = *sp++;
			else
			{
				*tp = '\0'; fwprintf(logstream, L"%s\t", bfr);
				tp = bfr; 
				state = LK_FOR_CASE_TITLE;
			}
			break; 
		case LK_FOR_CASE_TITLE:
			if (isalpha(*sp))
			{
				*tp++ = *sp++;
				state = GETTING_CASE_TITLE1;
			}
			else
				sp++;
			break;
		case GETTING_CASE_TITLE1:
			if (*sp == ',')
				state = GETTING_CASE_TITLE2;
			*tp++ = *sp++;
			break;
		case GETTING_CASE_TITLE2:
			if (*sp == ' ')	// expecting a space here
				state = GETTING_CASE_TITLE3;
			else
				state = GETTING_CASE_TITLE1;	// back to chech the sequence ',' ' ' and a digit
			*tp++ = *sp++;
			break;
		case GETTING_CASE_TITLE3:
			if (isdigit(*sp))	// hit a digit character, assume that all case id begin with a digit character
			{
				tp -= 2; *tp = '\0'; fwprintf(logstream, L"%s\t", bfr);
				tp = bfr;
				state = LK_FOR_CASE_ID;
			}
			else
			{
				*tp++ = *sp++;
				state = GETTING_CASE_TITLE1;	// back to check the sequence ',' ' ' and a digit
			}
			break;
		case LK_FOR_CASE_ID:
			if (*sp == ',' || *sp == '(')	// added check for '(', 2012/06/23
			{
				tp--; 
				if (*tp == '_') *tp = '\0'; // remove the ending space (was replaced with '_')
				else
				{ tp++; *tp = '\0'; }	// no ending space to remove
				wcscpy(cid, bfr);
				if (weight > 0)
				{
					fwprintf(logstream, L"[%s]\n", cid);
					fwprintf(cstream, L"%s %s %.0f\n", from, cid, weight);	// write out citation information for positive cases
					tp = bfr; sp++;
					state = LK_FOR_NOTHING;
				}
				else
				{
					fwprintf(logstream, L"[%s] ", cid);
					tp = bfr; sp++;
					neg_depth = 0;
					state = LK_FOR_CASE_DEPTH1;
				}
			}
			else
			{
				if (*sp == ' ')	
				{
					*tp++ = '_'; // change the spaces in case ID by underlines
					sp++;
				}
				else
					*tp++ = *sp++;
			}
			break;
		case LK_FOR_CASE_DEPTH1:	// case depth is indicated in "*"s
			if (*sp == '*')
			{
				neg_depth = 1;
				sp++;
				state = LK_FOR_CASE_DEPTH2;
			}
			else
				sp++;
			break;
		case LK_FOR_CASE_DEPTH2:	// case depth is indicated in "*"s
			if (isdigit(*sp))	// the format like "*16+" does not indicate depth
			{
				sp++;
				neg_depth = 0;
				state = LK_FOR_CASE_DEPTH1;
			}			
			else if (*sp == ' ' || *sp == 'H') // there are case like "**HN" 
			{
				*tp = '\0'; 
				fwprintf(logstream, L"%d*\n", neg_depth);
				if ((opt->include_1star && neg_depth == 1) || (opt->include_2star && neg_depth == 2)
					|| (opt->include_3star && neg_depth == 3) || (opt->include_4star && neg_depth == 4))
				{
					weight = -1 * neg_depth;
					fwprintf(cstream, L"%s %s %.0f\n", from, cid, weight);	// write out citation information for negative cases
				}
				tp = bfr; sp++;
				state = LK_FOR_NOTHING;
			}
			else
			{
				sp++;
				neg_depth++;
			}
			break;
		default:
			sp++;
			break;
		}
	}

	return 0;
}

//
// parse the "(Cite as:" string to obtain the case ID
//
int parse_citeas_string(wchar_t *str, int ccnt)
{
	int i, ncnt;
	wchar_t bfr[LBUF_SIZE], *tp, *sp;

	lawcase[ccnt].cid[MAX_LAWCASE_ID] = '\0';	// assume that there is no 2nd case name
	lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0';	// assume that there is no 3rd case name, added 2015/06/03
	lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';	// assume that there is no 4th case name, added 2015/06/03
	ncnt = 0;
	sp = str; tp = bfr;
	while (TRUE)
	{		
		if(*sp == '\0') break;
		else if (*sp == ')')
		{
			*tp = '\0';
			wcscpy(&lawcase[ccnt].cid[ncnt*MAX_LAWCASE_ID], bfr);	// modified 2015/06/03
			//fwprintf(logstream, L"CASE ID: [%s]\n", bfr);
			ncnt++;
			break;
		}
		else if (*sp == ',')
		{
			*tp = '\0';
			wcscpy(&lawcase[ccnt].cid[ncnt*MAX_LAWCASE_ID], bfr);	// modified 2015/06/03
			//fwprintf(logstream, L"CASE ID: [%s]\n", bfr);
			ncnt++; sp++;
			while (*sp == ' ') { sp++; }	// skip the spaces
			tp = bfr;						// continue to get the 2nd case ID
		}
		else if (*sp == '(')
		{
			if (*--sp = ' ') *--tp = '\0'; else *tp = '\0';
			wcscpy(&lawcase[ccnt].cid[ncnt*MAX_LAWCASE_ID], bfr);	// modified 2015/06/03
			//fwprintf(logstream, L"CASE ID: [%s]\n", bfr);
			ncnt++;
			break;
		}
		else
		{
			if (*sp == ' ')	
			{
				*tp++ = '_'; // change the spaces in case ID by underlines
				sp++;
			}
			else
				*tp++ = *sp++;
		}
	}

	return 0;
}

//
// given a citation file, replace the case alias id by the primary case id
// only need to check the 2nd column because the 1st column will never have alias id
//
struct CASEID_ALIAS 
{
	wchar_t cid[MAX_LAWCASE_ID];	// modified 2015/06/03
	wchar_t cid2[MAX_LAWCASE_ID];	// modified 2015/06/03
};

int replace_alias_case_id(wchar_t *ptcname)
{
	int i, k;
	int nc_alias;	// number of cases with alias id
	struct CASEID_ALIAS *atable;

	// establish the case id alias table
	// count the number of alias
	nc_alias = 0;
	for (i = 0; i < nlawcases; i++)
	{
		for (k = 1; k < 4; k++)
		{
			if (lawcase[i].cid[k*MAX_LAWCASE_ID] != '\0')
				nc_alias++;
		}
	}
	atable = (struct CASEID_ALIAS *)Jmalloc(nc_alias * sizeof(struct CASEID_ALIAS), L"replace_alias_case_id: atable");
	if (atable == NULL) return MSG_NOT_ENOUGH_MEMORY;
	nc_alias = 0;
	for (i = 0; i < nlawcases; i++)
	{
		if (lawcase[i].cid[MAX_LAWCASE_ID] != '\0')
		{
			wcscpy(atable[nc_alias].cid, lawcase[i].cid);
			wcscpy(atable[nc_alias].cid2, &lawcase[i].cid[MAX_LAWCASE_ID]);
			nc_alias++;
		}
		if (lawcase[i].cid[2*MAX_LAWCASE_ID] != '\0')
		{
			wcscpy(atable[nc_alias].cid, lawcase[i].cid);
			wcscpy(atable[nc_alias].cid2, &lawcase[i].cid[2*MAX_LAWCASE_ID]);
			nc_alias++;
		}
		if (lawcase[i].cid[3*MAX_LAWCASE_ID] != '\0')
		{
			wcscpy(atable[nc_alias].cid, lawcase[i].cid);
			wcscpy(atable[nc_alias].cid2, &lawcase[i].cid[3*MAX_LAWCASE_ID]);
			nc_alias++;
		}
	}

	qsort((void *)atable, (size_t)nc_alias, sizeof(struct CASEID_ALIAS), compare_csid);

	fwprintf(logstream, L"\nCases with alias ID:\n");
	for (i = 0; i < nc_alias; i++)
		fwprintf(logstream, L"  %s\t%s\n", atable[i].cid, atable[i].cid2);

	//
	// now, the alias table is ready, open the citation file
	//
	int nlines;
	int ndx;
	wchar_t line[LBUF_SIZE+1];
	wchar_t tname[FNAME_SIZE];
	wchar_t name1[MAX_LAWCASE_ID], name2[MAX_LAWCASE_ID];
	double weight;
	FILE *fstream, *tstream; 
	int nreplacements;

	if (_wfopen_s(&fstream, ptcname, L"r") != 0)
		return MSG_IFILE_NOTFOUND;
	swprintf(tname, L"%s.XXX", ptcname);
	if (_wfopen_s(&tstream, tname, L"w") != 0)
		return MSG_IFILE_NOTFOUND;

	fwprintf(logstream, L"\nCitation lines replaced with alians IDs:\n");
	// read in the citation lines
	nlines = 0; nreplacements = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, fstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_line(line, SP_SPACE, name1, name2, &weight);
		ndx = case_alias_search(atable, nc_alias, name2);
		if (ndx == -1)
			fwprintf(tstream, L"%s", line);	// write original line back to the file
		else
		{
			fwprintf(tstream, L"%s %s %.0f\n", name1, atable[ndx].cid, weight);	// write new information to the file
			fwprintf(logstream, L"%s %s ==> %s %s\n", name1, name2, name1, atable[ndx].cid);
		}
		nlines++;
	}

	//fwprintf(logstream, L"Number of citation lines taken: %d\n\n", nls);

	fclose(fstream); fclose(tstream);

	_wunlink(ptcname);
	_wrename(tname, ptcname);

	Jfree(atable, L"replace_alias_case_id: atable");

	return 0;
}

//
// apply binary search to find the proper position of an alias name
//
int case_alias_search(struct CASEID_ALIAS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].cid2) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].cid2) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].cid2) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].cid2) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// this fucntion is to be called by qsort() only
// 
int compare_csid(const void *n1, const void *n2)
{
	struct CASEID_ALIAS *t1, *t2;
	
	t1 = (struct CASEID_ALIAS *)n1;
	t2 = (struct CASEID_ALIAS *)n2;
	if (wcscmp(t2->cid2, t1->cid2) < 0)
		return 1;
	else if (wcscmp(t2->cid2, t1->cid2) == 0)
		return 0;
	else return -1;
}
