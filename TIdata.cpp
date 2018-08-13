// 
// TIdata.cpp
//
// Author: John Liu
// 
// this file contains function that handles data from the Thomson Innovation Patent database
//
//
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include "network.h"
#include "resource.h"

#define XXLBUF_SIZE 65536
#define BUF_SIZE 1024
#define FNAME_SIZE 260


//
// read WOS data and put it into the wos[] array
//
int read_TI(wchar_t *sname)
{
	int i, k, ndx;
	int nsr, cnt;
	int i_ut, i_au, i_de, i_id, i_py, i_so, i_tc, i_sc;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (_wfopen_s(&sstream, sname, L"r") != 0)
		return MSG_WOSFILE_NOTFOUND;

	// 1st pass, count the number of target records 
	nsr = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		nsr++;	
	}
	nwos = nsr - 1;	// less the 1st line (format line)

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// look for the index of the specified target field name 
	i_ut = parse_WOS_1st_line(line, SP_TAB, L"UT");	// document ID
	i_au = parse_WOS_1st_line(line, SP_TAB, L"AU");	// author names
	i_de = parse_WOS_1st_line(line, SP_TAB, L"DE");	// autor keywords
	i_id = parse_WOS_1st_line(line, SP_TAB, L"ID");	// ID plus keyword
	i_py = parse_WOS_1st_line(line, SP_TAB, L"PY");	// year of publication
	i_so = parse_WOS_1st_line(line, SP_TAB, L"SO");	// journal name
	i_tc = parse_WOS_1st_line(line, SP_TAB, L"TC");	// total citation
	i_sc = parse_WOS_1st_line(line, SP_TAB, L"SC");	// discipline
#ifdef DEA_APPLICATIONS
	int i_in;
	i_in = parse_WOS_1st_line(line, SP_TAB, L"IN");	// type of DEA applications
#endif DEA_APPLICATIONS
	if (i_ut == -1 || i_au == -1 || i_de == -1 || i_id == -1 || i_py == -1 || i_so == -1 || i_tc == -1 || i_sc == -1)
		return MSG_WOSFILE_FORMAT_ERROR;

	// 2nd pass, get keyword data and save it
	// allocate memory for the author keyword data
	kwde = (struct KWORDS *)malloc(nwos * 12 * sizeof(struct KWORDS));	// estimate in average 12 keywords per document
	// read source file line by line
	i = 0; nkwde = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_de, tfield);
		parse_store_keywords(tfield, &nkwde, kwde);
		//fwprintf(logstream, L"%d:%d: %s\n", i, nkwde, tfield); fflush(logstream);
		i++;	
	}
	qsort((void *)kwde, (size_t)nkwde, sizeof(struct KWORDS), compare_kname);
	// consolidate duplicate keywords
	wchar_t prev_name[MAX_JOURNAL_NAME];
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < nkwde; i++)
	{
		if (wcscmp(kwde[i].name, prev_name) != 0)	// hit new name
		{
			wcscpy_s(kwde[k++].name, MAX_KEYWORD_NAME, kwde[i].name); 
			wcscpy_s(prev_name, MAX_KEYWORD_NAME, kwde[i].name); 
		}
	}
	nkwde = k;

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the Keyword Plus keyword data
	kwid = (struct KWORDS *)malloc(nwos * 12 * sizeof(struct KWORDS));	// estimate in average 12 keywords per document
	// read source file line by line
	i = 0; nkwid = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_id, tfield);
		parse_store_keywords(tfield, &nkwid, kwid);
		i++;	
	}
	qsort((void *)kwid, (size_t)nkwid, sizeof(struct KWORDS), compare_kname);
	// consolidate duplicate keywords
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < nkwid; i++)
	{
		if (wcscmp(kwid[i].name, prev_name) != 0)	// hit new name
		{
			wcscpy_s(kwid[k++].name, MAX_KEYWORD_NAME, kwid[i].name); 
			wcscpy_s(prev_name, MAX_KEYWORD_NAME, kwid[i].name); 
		}
	}
	nkwid = k;

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the author name array
	authors = (struct AUTHORS *)malloc(nwos * 8 * sizeof(struct AUTHORS));	// estimate in average 8 authors per document
	// read source file line by line
	i = 0; naus = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_au, tfield);
		parse_store_authors(tfield, &naus, authors);
		//fwprintf(logstream, L"%d: %s\n", i, tfield); fflush(logstream);
		i++;	
	}
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_author);
	// consolidate duplicate author names
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

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the journal name array
	journals = (struct JOURNALS *)malloc(nwos * sizeof(struct JOURNALS));
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;		
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_so, tfield);
		wcscpy_s(journals[i].name, MAX_JOURNAL_NAME, tfield);
		i++;	
	}
	njrs = i;
	qsort((void *)journals, (size_t)njrs, sizeof(struct JOURNALS), compare_journal);
	// consolidate duplicate journal names
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < njrs; i++)
	{
		if (wcscmp(journals[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) journals[k-1].np = cnt;
			wcscpy_s(journals[k++].name, MAX_JOURNAL_NAME, journals[i].name); 
			wcscpy_s(prev_name, MAX_JOURNAL_NAME, journals[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	journals[k-1].np = cnt;
	njrs = k;

	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the discipline data
	dsplns = (struct DISCIPLINES *)malloc(nwos * 5 * sizeof(struct DISCIPLINES));	// estimate in average 5 disciplines per document
	// read source file line by line
	i = 0; ndsplns = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_sc, tfield);
		parse_store_disciplines(tfield, &ndsplns, dsplns);
		//fwprintf(logstream, L"%d:%d: %s\n", i, ndsplns, tfield); fflush(logstream);
		i++;	
	}
	qsort((void *)dsplns, (size_t)ndsplns, sizeof(struct DISCIPLINES), compare_dpname);
	// consolidate duplicate keywords
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < ndsplns; i++)
	{
		if (wcscmp(dsplns[i].name, prev_name) != 0)	// hit new name
		{
			wcscpy_s(dsplns[k++].name, MAX_DISCIPLINE_NAME, dsplns[i].name); 
			wcscpy_s(prev_name, MAX_DISCIPLINE_NAME, dsplns[i].name); 
		}
	}
	ndsplns = k;

	// 3rd pass, get all other required data
	rewind(sstream);	// point back to the begining of the file
	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	// allocate memory for the list of WOS data
	wos = (struct WOS *)malloc(nwos * sizeof(struct WOS));
	// read source file line by line
	i = 0; 
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_WOS_line(line, SP_TAB, i_ut, tfield);
		wcscpy_s(wos[i].docid, MAX_DOC_ID, &tfield[4]);	// ignore the 4 leading characters -- "ISI:" 
		parse_WOS_line(line, SP_TAB, i_au, tfield);
		wos[i].nau = get_number_authors(tfield);
		parse_author_names(tfield, &wos[i].nau, wos[i].author, naus, authors);
#ifdef DEBUG
		for (k = 0; k < wos[i].nau; k++) { fwprintf(logstream, L"%d:%d:%d:%d %s\n", i, wos[i].nau, k, wos[i].author[k], authors[wos[i].author[k]].name); fflush(logstream); }
		fwprintf(logstream, L"\n");
#endif DEBUG
		parse_WOS_line(line, SP_TAB, i_py, tfield);
		wos[i].year = _wtoi(tfield);
		//fwprintf(logstream, L"%d:%d %s[%s]\n", i, wos[i].year, wos[i].docid, wos[i].author);
		parse_WOS_line(line, SP_TAB, i_de, tfield);
		parse_keywords(tfield, &wos[i].nde, wos[i].DE, nkwde, kwde);
		parse_WOS_line(line, SP_TAB, i_id, tfield);
		parse_keywords(tfield, &wos[i].nid, wos[i].ID, nkwid, kwid);
		parse_WOS_line(line, SP_TAB, i_so, tfield);
		ndx = jname_search(journals, njrs, tfield);
		wos[i].journal = ndx;
		parse_WOS_line(line, SP_TAB, i_tc, tfield);
		wos[i].tc = _wtoi(tfield);
		parse_WOS_line(line, SP_TAB, i_sc, tfield);
		parse_disciplines(tfield, &wos[i].ndspln, wos[i].dspln, ndsplns, dsplns);
#ifdef DEA_APPLICATIONS
		parse_WOS_line(line, SP_TAB, i_in, tfield);	// the content of this field is the type of applications, or empty
		if (tfield[0] == '\0' || tfield[0] == ' ')
			wos[i].app_type = 0;	// theoretical
		else
			wos[i].app_type = 1;	// non-theoretical
#endif DEA_APPLICATIONS
		//fwprintf(logstream, L"%d:%s", i, wos[i].docid);
		//fwprintf(logstream, L"\n"); fflush(logstream);
		i++;	
	}
	fclose(sstream); 

#ifdef DEBUG
	for (i = 0; i < nwos; i++)
	{
		fwprintf(logstream, L"%s: ", wos[i].docid);
		//for (k = 0; k < wos[i].ndspln; k++)
		//	fwprintf(logstream, L"%s ", dsplns[wos[i].dspln[k]].name);
		fwprintf(logstream, L"%d: %s\n", i+1, journals[wos[i].journal].name);
		fwprintf(logstream, L"\n");
		fflush(logstream);
	}
#endif DEBUG

	prepare_alias();
	link_author_WOS();	
	link_journal_WOS();	
	coauthor();
	calculate_author_h_index();
	calculate_journal_h_index();
	basic_wos_statistics();

	// sort the data by ISI document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);

	return 0;
}
