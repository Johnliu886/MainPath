// 
// WESTLAWdata2.cpp
//
// Author: John Liu
// 
// sometime in 2015, WESTLAW stop to provide plain text file. Data in PDF format is provided instead.
// this file contains codes that handles text data converted from PDF files.
// the conversion program is "PdfAide.exe"
//

//
// Revision History:
// 2016/05/20 Basic function works
// 2016/06/22 Improved the accuracy of the function read_WESTLAW2()
// 2016/06/23 Added "U.S." as a legal case name
// 2016/06/25 Added codes to handle the situation that pagehead and the beginning of a case carry two different case ids
// 2016/06/27 Added more rules to restrict the legal citation cases
// 2016/07/09 Fixed a problem in determining the year of a case ==> it now determined by the information in page header
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

int read_WL2(wchar_t *, int, FILE *, CITE_OPTIONS *);
int parse_pagehead_string(wchar_t *, int);

extern int index_of(int, wchar_t *, wchar_t *);
extern int index_of2(int, wchar_t *, wchar_t *);	// this function does not do the separate word check

int parse_case_string2(wchar_t *, wchar_t *, double, FILE *, CITE_OPTIONS *);
int parse_citeas_string2(wchar_t *, int);
extern int compare_csid(const void *, const void *);
extern int case_alias_search(struct CASEID_ALIAS *, int, wchar_t *);
extern int parse_line(wchar_t *, int, wchar_t *, wchar_t *, double *);

extern int nlawcases;
extern struct LAWCASE *lawcase;

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
struct MONTHS mnth2[NMSTRINGS] = {
L"January", 7,
L"February", 8,
L"March", 5,
L"April", 5,
L"May", 3,
L"June", 4,
L"July", 4,
L"August", 6,
L"September", 9,
L"October", 7,
L"November", 8,
L"December", 8,
L"Jan.", 4,
L"Feb.", 4,
L"Mar.", 4,
L"Apr.", 4,
L"Jun.", 4,
L"Jul.", 4,
L"Aug.", 4,
L"Sep.", 4,
L"Oct.", 4,
L"Nov.", 4,
L"Dec.", 4,
L"Sept.", 5,
};
extern struct MONTHS mnth[NMSTRINGS];

//
// read WESTLAW data 
//
int read_WESTLAW2(wchar_t *dname, wchar_t *tname, CITE_OPTIONS *opt)
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

	// allocate memory for the list of law case data
	lawcase = (struct LAWCASE *)malloc(nlawcases * sizeof(struct LAWCASE));
	if (lawcase == NULL) return MSG_NOT_ENOUGH_MEMORY;
		
	if (no_wos_citation_file == 1)	// citation file is not given, the normal situation
	{
		// open the citation file (for writing)
		ret = swprintf_s(cname, FNAME_SIZE, L"%s citation.txt", tname);	
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
		if ((ret = read_WL2(fname, ccnt, cstream, opt)) != 0)
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

#define PARSE_PAGEHEAD_STRING 0
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



#define LOOKING_FOR_KC_TREATMENT 77
#define LOOKING_FOR_KC_TITLE 78
#define LOOKING_FOR_KC_CITED_CASE 79
#define LOOKING_FOR_KC_DATE_OR_TYPE 80
#define LOOKING_FOR_MARK_KEYCITE1 90
#define LOOKING_FOR_MARK_KEYCITE2 91
#define LOOKING_FOR_MARK_KEYCITE3 93
#define LOOKING_FOR_MARK_KEYCITE4 94
#define LOOKING_FOR_MARK_KEYCITE5 95
#define LOOKING_FOR_MARK_KEYCITE6 96
#define LOOKING_FOR_NOTHING 99
//
// read and parse the content of one Westlaw case
// parse the text file converted from PDF file, the coversion software is: "PdfAide.exe"
//
int read_WL2(wchar_t *fname, int ccnt, FILE *cstream, CITE_OPTIONS *opt)
{
	int i;
	int ndx, clcnt;
	int n_leadingspaces;
	int argued, decided;
	int at;
	FILE *sstream;
	int state;
	wchar_t prevline[LBUF_SIZE];
	wchar_t cid2[LBUF_SIZE];
	wchar_t bfr[LBUF_SIZE], *bptr;
	wchar_t line[XXLBUF_SIZE], *lptr, *tp, *sp;
	double weight;	// the KEYCITE depath, from 1 (stars) to 4 (stars), can be negative if it is cited negatively

	fwprintf(logstream, L"=====>%s\n", fname); fflush(logstream);

	if (_wfopen_s(&sstream, fname, L"rt, ccs=UNICODE") != 0)	// the text file after converted from PDF is encoded in UCS-2 little endian
		return MSG_WOSFILE_NOTFOUND;

	state = PARSE_PAGEHEAD_STRING;
	while (TRUE)
	{		
		lptr = line;
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		n_leadingspaces = 0; 
		while (*lptr == ' ' || *lptr == '\t') { lptr++; n_leadingspaces++; }
		switch (state)
		{
		case PARSE_PAGEHEAD_STRING:
			//fwprintf(logstream, L"$$$$$ %d [%s]", ccnt+1, lptr);
			parse_pagehead_string(lptr, ccnt);
			if (lawcase[ccnt].cid[0] == '\0' || wcsncmp(lawcase[ccnt].cid, L"Not_Reported", 12) == 0 || wcsncmp(lawcase[ccnt].cid, L"Slip_Copy", 9) == 0)
				state = LOOKING_FOR_CASE_ID; 
			else
			{
				//fwprintf(logstream, L"==> %d [%s] [%d]\n", ccnt+1, lawcase[ccnt].cid, lawcase[ccnt].year);
				state = LOOKING_FOR_NOTHING; 
			}
			state = LOOKING_FOR_CASE_ID; 
			break;
		case LOOKING_FOR_CASE_ID:
			//fwprintf(logstream, L"%d [%s]\n", ccnt+1, lptr);	
			if ((at=index_of(0, lptr, L"F.Supp.2d")) != -1 || (at=index_of(0, lptr, L"F.Supp.3d")) != -1 ||
				(at=index_of(0, lptr, L"F.Supp.")) != -1 || (at=index_of(0, lptr, L"F.")) != -1 ||
				(at=index_of(0, lptr, L"F.2d")) != -1 || (at=index_of(0, lptr, L"F.3d")) != -1 ||
				(at=index_of(0, lptr, L"App.D.C.")) != -1 || (at=index_of(0, lptr, L"U.S.")) != -1 ||
				(at=index_of(0, lptr, L"WL")) != -1 || (at=index_of(0, lptr, L"S.Ct.")) != -1 ||
				(at=index_of(0, lptr, L"Fed.Appx.")) != -1 ||(at=index_of(0, lptr, L"Fed.Cl.")) != -1 ||
				(at=index_of(0, lptr, L"F.R.D.")) != -1)
			{
				sp = lptr; tp = cid2;
				while (*sp != '\0' && *sp != '\n' && *sp != ',' && *sp != '(') 
				{
					if (*sp == ' ') { *tp++ = '_'; sp++; }
					else
						*tp++ = *sp++;
				}
				if (*sp == '(') tp--;	// to address the situation ==> "559 Fed.Appx. 1009 (Mem)"
				*tp = '\0';
				tp--; if (!iswdigit(*tp)) break;	// do not take it if the end of case ID is not a digit, a very special case
				if (wcscmp(cid2, lawcase[ccnt].cid) == 0)	// case ids in pagehead and beginning text are the same, added 2016/06/25
				{
					lawcase[ccnt].cid[1*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';
				}
				else // found inconsistent case ids (in pagehead and beginning text), added 2016/06/25
				{
					//fwprintf(logstream, L"XXXXX [%s] [%s]\n", lawcase[ccnt].cid, cid2);
					// need to determing which one to take as the main case id
					if (wcsncmp(lawcase[ccnt].cid, L"Slip_Copy", 9) == 0 || wcsncmp(lawcase[ccnt].cid, L"Not_Reported", 12) == 0 || 
						wcsncmp(lawcase[ccnt].cid, L"---", 3) == 0 ||
						(at=index_of2(0, lawcase[ccnt].cid, L"Cranch")) != -1)		// invalid case id in the pagehead
					{
						wcscpy(lawcase[ccnt].cid, cid2);	// take the one in the beginning text (cid2)
						lawcase[ccnt].cid[1*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';
					}
					else if ((at=index_of2(0, lawcase[ccnt].cid, L"C.C.P.A.")) != -1)
					{
						wcscpy(&lawcase[ccnt].cid[1*MAX_LAWCASE_ID], lawcase[ccnt].cid); // take the "C.C.P.A" as the 2nd case id
						wcscpy(lawcase[ccnt].cid, cid2);	// take the one in the beginning text as the main case id
						lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';
					}
					else if ((at=index_of2(0, lawcase[ccnt].cid, L"U.S.")) != -1)
					{
						wcscpy(&lawcase[ccnt].cid[1*MAX_LAWCASE_ID], cid2);	// take the one in the beginning text as the 2nd case id
						lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';
					}
					else if ((at=index_of2(0, lawcase[ccnt].cid, L"F.2d")) != -1 || (at=index_of2(0, lawcase[ccnt].cid, L"F.3d")) != -1 ||
						     (at=index_of2(0, lawcase[ccnt].cid, L"F.")) != -1)
					{
						wcscpy(&lawcase[ccnt].cid[1*MAX_LAWCASE_ID], cid2);	// take the one in the beginning text as the 2nd case id
						lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';
					}
					else	// other situations
					{
						wcscpy(&lawcase[ccnt].cid[1*MAX_LAWCASE_ID], lawcase[ccnt].cid); // take the one in pagehead as the 2nd case id
						wcscpy(lawcase[ccnt].cid, cid2);	// take the one in the beginning text as the main case id
						lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0'; lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';
					}
				}
				state = LOOKING_FOR_DATE; 
			} 			
			break;
		case LOOKING_FOR_DATE:	
			for (i = 0; i < NMSTRINGS; i++)	// for all possible presentaitons for months
			{
				at = index_of(0, lptr, mnth2[i].mn);
				if (at != -1)
				{
					bptr = bfr; lptr += at + mnth2[i].len;
					while (*lptr != ',') lptr++;	lptr += 2; // standard date format "March 12, 2011."
					while (*lptr != '.' && *lptr != '\0') *bptr++ = *lptr++;	*bptr = '\0';
					if (!iswdigit(bfr[0])) break;	// not the correct format (no year information follows the month/date information)
					//lawcase[ccnt].year = _wtoi(bfr);	// year is now determined by the information in page header
					lawcase[ccnt].ayear = 0;
					state = LOOKING_FOR_MARK_KEYCITE; 
					break;
				}
			}
			break;
		case LOOKING_FOR_MARK_KEYCITE:	// format: "Citing References (23)"
			if (wcsncmp(lptr, L"Citing References (", 19) == 0)
			{
				bptr = bfr; lptr += 19;
				while (*lptr != ')') *bptr++ = *lptr++;
				*bptr = '\0';
				lawcase[ccnt].tc = _wtoi(bfr);
				//fwprintf(logstream, L"##### %d: [%s] [%s] %04d tc=%d\n", ccnt+1, lawcase[ccnt].cid,  &lawcase[ccnt].cid[1*MAX_LAWCASE_ID], lawcase[ccnt].year, lawcase[ccnt].tc);	fflush(logstream);
				state = LOOKING_FOR_MARK_KEYCITE1; 
			}
			break;
		case LOOKING_FOR_MARK_KEYCITE1:	// "Treatment"
			state = LOOKING_FOR_MARK_KEYCITE2;
			break;
		case LOOKING_FOR_MARK_KEYCITE2:	// "Title"
			state = LOOKING_FOR_MARK_KEYCITE3;
			break;
		case LOOKING_FOR_MARK_KEYCITE3:	// "Date"
			state = LOOKING_FOR_MARK_KEYCITE4;
			break;
		case LOOKING_FOR_MARK_KEYCITE4:	// "Type"
			state = LOOKING_FOR_MARK_KEYCITE5;
			break;
		case LOOKING_FOR_MARK_KEYCITE5:	// "Depth"
			state = LOOKING_FOR_MARK_KEYCITE6;
			break;
		case LOOKING_FOR_MARK_KEYCITE6:	// "Headnote(s)"
			state = LOOKING_FOR_KC_TREATMENT;
			break;
		case LOOKING_FOR_KC_TREATMENT:  // "Examined by", "Discussed by", "Cited by", "Mentioned by", etc.		
			if (*lptr == '¡X')	// this denotes the end of cited reference for cases
			{
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_NOTHING; 	// the loop also exit here
				wcscpy(prevline, lptr);	// save for reference later
			}
			else if (wcsncmp(lptr, L"Examined by", 11) == 0) // 4 stars
			{
				weight = 4.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
				wcscpy(prevline, lptr);	// save for reference later
			}
			else if (wcsncmp(lptr, L"Discussed by", 12) == 0) // 3 stars
			{
				weight = 3.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
				wcscpy(prevline, lptr);	// save for reference later
			}
			else if (wcsncmp(lptr, L"Cited by", 8) == 0) // 2 stars
			{
				weight = 2.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
				wcscpy(prevline, lptr);	// save for reference later
			}
			else if (wcsncmp(lptr, L"Mentioned by", 12) == 0) // 1 stars
			{
				weight = 1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
				wcscpy(prevline, lptr);	// save for reference later
			}
			else if (wcsncmp(lptr, L"Overruled by", 12) == 0) // negative, -1
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
				wcscpy(prevline, lptr);	// save for reference later
			}
			else if (wcsncmp(lptr, L"Overruling", 10) == 0) // negative level information is lost when converting from PDF to text file
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
				wcscpy(prevline, lptr);	// save for reference later
			}			
			else if (wcsncmp(lptr, L"Declined", 8) == 0)	// "Declined to Extend by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Distinguished", 13) == 0)	// "Distinguished by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Called", 6) == 0)	// "Called into Doubt by" 
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Rejection", 9) == 0)	// "Rejection Recognized by" 
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Rejected", 8) == 0)	// "Rejected by" 
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Disagree", 8) == 0)	// "Disagreed With by" "Disagreement Recognized by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Not", 3) == 0)		// "Not Followed as Dicta"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Overrul", 7) == 0)		// "Overruling Recognized by" "Overruled in Part by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Abrogat", 7) == 0)		// "Abrogation Recognized by" "Abrogated by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Superseded", 10) == 0)		// "Superseded by Statute as Stated in"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Criticized", 10) == 0)		// "Criticized by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Implied", 7) == 0)		// "Implied Overruling Recognized by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Among", 5) == 0)		// "Among Conflicting Authorities Noted in"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Modifi", 6) == 0)		// "Modification Recognized by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Holding", 7) == 0)		// "Holding Limited by"
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else if (wcsncmp(lptr, L"Disapproval", 11) == 0)	// "Disapproval Recognized by", added 2013/02/21
			{
				weight = -1.0; 
				//fwprintf(logstream, L"    %s", lptr);
				state = LOOKING_FOR_KC_TITLE; 
			}
			else 
				wcscpy(prevline, lptr);	// save for reference later
			break;
		case LOOKING_FOR_KC_TITLE: 
			while (*lptr == ' ') lptr++;
			if (iswdigit(*lptr))	// if the line begins with a digit
			{
				bptr = bfr;
				while (iswdigit(*lptr)) *bptr++ = *lptr++;
				if (*lptr == '.')	// the line begins with digits follows by a '.'
				{
					//fwprintf(logstream, L"&&&&[%s]", prevline);	// printf the content of the previoius line
					//fwprintf(logstream, L"@@@@@%s", lptr); fflush(logstream);
					*bptr = '\0';
					ndx = _wtoi(bfr);
					if (ndx >= lawcase[ccnt].tc)
						state = LOOKING_FOR_NOTHING;	// the loop exit here
					else
					{
						clcnt = 0;
						state = LOOKING_FOR_KC_CITED_CASE;
					}
				}
			}
			break;
		case LOOKING_FOR_KC_CITED_CASE:
			if ((at=index_of(0, lptr, L"F.Supp.2d")) != -1 || (at=index_of(0, lptr, L"F.Supp.3d")) != -1 ||
				(at=index_of(0, lptr, L"F.Supp.")) != -1 || (at=index_of(0, lptr, L"F.")) != -1 ||
				(at=index_of(0, lptr, L"F.2d")) != -1 || (at=index_of(0, lptr, L"F.3d")) != -1 ||
				(at=index_of(0, lptr, L"App.D.C.")) != -1 || (at=index_of(0, lptr, L"U.S.")) != -1 ||
				(at=index_of(0, lptr, L"WL")) != -1 || (at=index_of(0, lptr, L"S.Ct.")) != -1 ||
				(at=index_of(0, lptr, L"Fed.Appx.")) != -1 ||(at=index_of(0, lptr, L"Fed.Cl.")) != -1 ||
				(at=index_of(0, lptr, L"F.R.D.")) != -1)
			{
				if (at <= 8 && at != 0)	// only if they are close to the begining of a line, but not at the very beginning
				{
					int clen;
					clen = 0;
					sp = lptr;
					while (*sp != ',' && *sp != '(' && *sp != '\n') { clen++; if (*sp == ' ') *sp++ = '_'; else sp++; }
					if (*sp == ',') *sp = '\0';
					else if (*sp == '(') { sp--; *sp = '\0'; }
					// take only those start with a digit (a case id always start with a digit),
					//                 not ended with '\n' (those are not case ids)
					//                 and more than 5 in length
					if (iswdigit(*lptr) && *sp != '\n' && clen >=5)		// 2016/06/27
					{
						fwprintf(cstream, L"%s %s %.0f\n", lawcase[ccnt].cid, lptr, weight);	// write out citation information
						//fwprintf(logstream, L"#####[%s]\n", lptr); fflush(logstream);
						state = LOOKING_FOR_KC_DATE_OR_TYPE;
					}
				}
			}
			clcnt++;
			if (clcnt >= 4)	// look no more if there is no case ID within 5 lines
				state = LOOKING_FOR_KC_DATE_OR_TYPE;
			break; 
		case LOOKING_FOR_KC_DATE_OR_TYPE: 
			for (i = 0; i < NMSTRINGS; i++)	// for all possible presentaitons for months
			{
				if (wcsncmp(lptr, mnth[i].mn, mnth[i].len) == 0)
				{
					state = LOOKING_FOR_KC_TREATMENT;
					break;
				}
			}
			// then check type strings
			if (wcsncmp(lptr, L"Case", 4) == 0 || wcsncmp(lptr, L"Law Review", 10) == 0 || 
				wcsncmp(lptr, L"Other", 5) == 0 || (lptr, L"ALR", 3) == 0 ||
				wcsncmp(lptr, L"Westlaw", 7) == 0 || 
				wcsncmp(lptr, L"Administrative", 7) == 0 || wcsncmp(lptr, L"Decision", 8) == 0)	// "Administrative Decision", need to catch one of the two 
				state = LOOKING_FOR_KC_TREATMENT;
			break;
		case LOOKING_FOR_NOTHING:
			fclose(sstream);
			return 0;
			break;
#ifdef XXX
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
				parse_case_string2(bfr, lawcase[ccnt].cid, weight, cstream, opt);
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
				parse_case_string2(bfr, lawcase[ccnt].cid, weight, cstream, opt);
				state = LOOKING_FOR_CITED_CASESP1;
			}
			else
			{
				while (*lptr != '\0') *bptr++ = *lptr++;	// continue copying the string 
				bptr--;
			}
			break;
#endif XXX
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
int parse_case_string2(wchar_t *str, wchar_t *from, double weight, FILE *cstream, CITE_OPTIONS *opt)
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
// parse page head string to obtain the case ID
// eamples of the page head string:
// "Apple, Inc. v. Samsung Electronics Co., Ltd., 909 F.Supp.2d 1147 (2012)"
// "Avedisian v. Mercedes-Benz USA, LLC, Not Reported in F.Supp.2d (2013)"
// "Microsoft Corp. v. John Does 1-82, Slip Copy (2013)"
// "Mercedes-Benz, U.S.A. LLC v. Coast Automotive Group, Ltd., Not Reported in..."
//
int parse_pagehead_string(wchar_t *str, int ccnt)
{
	int i, ncnt;
	wchar_t bfr[LBUF_SIZE], *tp, *sp;

	lawcase[ccnt].cid[0] = '\0';
	lawcase[ccnt].cid[MAX_LAWCASE_ID] = '\0';	// assume that there is no 2nd case name
	lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0';	// assume that there is no 3rd case name
	lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';	// assume that there is no 4th case name
	ncnt = 0;
	sp = str; tp = bfr;
	while (*sp != '\0') sp++;	// go to the end of the string
	sp -= 3;
	if (wcsncmp(sp, L"...", 3) == 0)	// if "..." is at the end of the string, information is incomplete
		return 0;
	sp += 3;
	while (*sp != ',') sp--;	// then trace back to the ','
	sp++; while (*sp == ' ') sp++;	// skip the spaces after the ','
	while (*sp != '\0')
	{		
		if (*sp == '(')
		{
			if (*--sp = ' ') *--tp = '\0'; else *tp = '\0';
			wcscpy(&lawcase[ccnt].cid[ncnt*MAX_LAWCASE_ID], bfr);
			sp += 2; tp = bfr;
			continue;
		}
		else if (*sp == ')')
		{
			*tp = '\0';
			lawcase[ccnt].year = _wtoi(bfr);
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
// parse the "(Cite as:" string to obtain the case ID
//
int parse_citeas_string2(wchar_t *str, int ccnt)
{
	int i, ncnt;
	wchar_t bfr[LBUF_SIZE], *tp, *sp;

	lawcase[ccnt].cid[MAX_LAWCASE_ID] = '\0';	// assume that there is no 2nd case name
	lawcase[ccnt].cid[2*MAX_LAWCASE_ID] = '\0';	// assume that there is no 3rd case name
	lawcase[ccnt].cid[3*MAX_LAWCASE_ID] = '\0';	// assume that there is no 4th case name
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
