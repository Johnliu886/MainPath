//
// select.cpp
//
// this file is created 2014/05/15.
//

//
// Revision History:
// 2014/11/05 Added codes to handle the case of data with "EID" (Scopus)
// 2016/04/05 Changed the method of assigning docid, the counting method used previously cannot work after clustering, changed to use author name 
// 2016/06/17 Added codes to write alias label in front of every record for the case of RW_ALL (only for TCI data)
// 2016/07/07 Added new functions readwrite_TCI_direct() and readwrite_Scopus_direct()
// 2016/10/29 Added codes to ignore lines begin with ",," for Scopus data
// 2016/11/25 Replace to call wfgetws() in readwrite_WEBPAT3(), was calling pfgetws()
// 2016/12/06 Added a new function readwrite_UI_USPTO(), this is used only for UNIVERSTY_INDUSTRY_PROJECT
// 2016/12/07 Modified such that empty lines are also copied for USPTO data
// 2017/02/25 Changed fopen() mode to ccs=UTF-8 for TIP data
// 2017/04/22 Added to write out a ".csv" format file for TaiwanTD data (for reading purpose)
// 2017/06/26 Added function assemble_multiple_WOS_data(), to assemble multiple WOS data
// 2017/06/27 Added function assemble_multiple_TCI_data(), to assemble multiple TCI data
// 2017/07/06 Changed to call correct_WEBPAT3_pid() rather than correct_WEBPAT_pid() inside the function readwrite_WEBPAT3()
// 2017/07/14 Added functions assemble_files_in_a_directory() and count_files_in_a_directory() to read and count data in the subdirectory
// 2017/10/20 Added function readwrite_WOS_based_on_wos_array()
// 2018/01/03 Added codes to take English identifier line for WEBPAT3 data, in readwrite_WEBPAT3()
// 2018/01/03 Added functions to support multiple data files for WEBPAT3. Added functions assemble_multiple_WEBPAT3_data() and assemble_files_in_a_directory_WEBPAT3()

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

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1024
#define FNAME_SIZE 260

//
// function types
//
int compare_str(const void *, const void *);
int str_search2(wchar_t **tr, int, wchar_t *);
int count_files_in_a_directory(wchar_t *);
int assemble_files_in_a_directory(wchar_t *, FILE *);

extern int index_of(wchar_t *, wchar_t *);
extern int WOS_or_Others(wchar_t *);
extern int read_WOS(wchar_t *sname);
extern int docid_search_nw(struct PN *, int, wchar_t *);
extern int docid_search_wos(struct WOS *, int, wchar_t *);
extern int parse_WOS_line(wchar_t *, int, int, wchar_t *);
extern int parse_WOS_1st_line(wchar_t *, int, wchar_t *);
extern int parse_Scopus_1st_line(wchar_t *, wchar_t *);
extern int parse_Scopus_line(wchar_t *, int, wchar_t *);
extern int parse_WEBPAT_2nd_line(wchar_t *, int, wchar_t *);
extern int parse_WEBPAT_line(wchar_t *, int, wchar_t *);
extern int parse_WEBPAT3_1st_line(wchar_t *, int, wchar_t *);
extern int parse_WEBPAT3_line(wchar_t *, int, wchar_t *);
extern int correct_WEBPAT_pid(wchar_t *, wchar_t *);
extern int correct_WEBPAT3_pid(wchar_t *, wchar_t *);
extern int parse_LAWSNOTE_1st_line(wchar_t *, wchar_t *);
extern int parse_LAWSNOTE_line(wchar_t *, int, wchar_t *);

extern int create_TCI_docid(wchar_t *, wchar_t *, int, int, int, int);
extern int create_Scopus_docid(wchar_t *, wchar_t, int, int, int);
extern int parse_TIP_2nd_line(wchar_t *, int, wchar_t *);
extern int parse_TIP_line(wchar_t *, int, wchar_t *);
extern int init_xfgetws(FILE *);
extern int done_xfgetws();
extern wchar_t *xfgetws(wchar_t *, int, FILE *);
extern int parse_TCI_1st_line(wchar_t *, wchar_t *);
extern int parse_TCI_line(wchar_t *, int, wchar_t *);
extern int init_pfgetws(FILE *);
extern int done_pfgetws();
extern wchar_t *pfgetws(wchar_t *, int, FILE *);
extern int init_wfgetws(FILE *);
extern int done_wfgetws();
extern wchar_t *wfgetws(wchar_t *, int, FILE *);
extern int init_lnfgetws(FILE *);
extern int done_lnfgetws();
extern wchar_t *lnfgetws(wchar_t *, int, FILE *);
extern int valid_patent_id(wchar_t *);

//
// global variables
//
extern FILE *logstream;
extern int full_record_type;	// WOS, patent, etc.
extern int text_type;	// ASCII, UTFxx, etc.
extern int Scopus_EID;
extern wchar_t *pline;	// this is defined in the file WEBPATdata2.cpp
extern wchar_t *wline;	// this is defined in the file WEBPATdata3.cpp
extern int just_exit_at_next_line;	// this is defined in the file WEBPATdata2.cpp

//extern int nwos;
//extern struct WOS *wos;
extern int nnodes;
extern struct PN *nw;
extern int naus;
extern struct AUTHORS *authors;	// author name array
extern struct PFID pfid[];
extern struct TDID tdid[];
extern int nwos;
extern struct WOS *wos;
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern struct TKEYWORDS *tkeyword;	// title keywords
extern struct TTD_TITLE *ttd_title;
extern int n_fields;

extern int nsds;
extern struct SERIALDOCS *serialdocs;

//
// read WOS files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
// based on the nw[] array
//
int readwrite_WOS(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield, *tf2;
	int ut_ind;
	int nw_ndx;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2018/05/09
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
		return -1;

	if (text_type == FT_UTF_8)
		ln = &line[3];	// that's 3 bytes
	else if (text_type == FT_UTF_16_BE || text_type == FT_UTF_16_LE)
		ln = &line[1];	// that's 2 bytes
	else if (text_type == FT_UTF_32_BE || text_type == FT_UTF_32_LE)
		ln = &line[1];	// that's 4 bytes
	else
		ln = line;

	if (which == RW_MAINPATH)	// added 2014/07/11
		fwprintf(ostream, L"Labels\t%s", ln);		// write out the 1st line, will insert a Label column
	else
		fwprintf(ostream, L"%s", ln);		// write out the 1st line

	ut_ind = parse_WOS_1st_line(ln, SP_TAB, L"UT");
	// read source file line by line
	// if the target record matches, write to the output file
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_WOS_line(line, SP_TAB, ut_ind, tfield);
		if (wcsncmp(tfield, L"ISI:", 4) == 0 || wcsncmp(tfield, L"WOS:", 4) == 0)	// remove "ISI:" or "WOS:"
			tf2 = &tfield[4];		
		else
			tf2 = tfield;
		nw_ndx = docid_search_nw(nw, nnodes, tf2);
		if (nw_ndx >= 0)
		{
			if (which == RW_MAINPATH)	// added 2014/07/11
			{
				if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					// if the alias lead with "FM" and a digit, it is then a family-paper, expand it
					if (nw[nw_ndx].alias[0] == 'F' && nw[nw_ndx].alias[1] == 'M' && isdigit(nw[nw_ndx].alias[2]))
					{
						wchar_t ttmp[3]; int nsd; int id_serialdoc;
						ttmp[0] = nw[nw_ndx].alias[2]; ttmp[1] = '\0';
						id_serialdoc = _wtoi(ttmp) - 1;
						nsd = serialdocs[id_serialdoc].nd;
						for (i = 0; i < nsd; i++)
						{
							nw_ndx = serialdocs[id_serialdoc].ndx[i];
							fwprintf(ostream, L"%s\t%s", nw[nw_ndx].alias, line); 
							nw[nw_ndx].flag = 1;	// raise flag
						}
					}
					else
					{
						fwprintf(ostream, L"%s\t%s", nw[nw_ndx].alias, line); 
						nw[nw_ndx].flag = 1;	// raise flag
					}
				}
			}
			else if (which == RW_PARTITION)
			{
				//fwprintf(logstream, L"@@@@@ %d %d [%s] %d\n", partition, nw[nw_ndx].partition, nw[nw_ndx].alias, nw_ndx);
				if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read WOS files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
// bases on the wos[] array
// this function is added 2017/10/20
//
int readwrite_WOS_based_on_wos_array(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield, *tf2;
	int ut_ind;
	int wos_ndx;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2018/05/09
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read the 1st line of the source file, it contains the field names
	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
		return -1;

	if (text_type == FT_UTF_8)
		ln = &line[3];	// that's 3 bytes
	else if (text_type == FT_UTF_16_BE || text_type == FT_UTF_16_LE)
		ln = &line[1];	// that's 2 bytes
	else if (text_type == FT_UTF_32_BE || text_type == FT_UTF_32_LE)
		ln = &line[1];	// that's 4 bytes
	else
		ln = line;

	if (which == RW_MAINPATH)	// added 2014/07/11
		fwprintf(ostream, L"Labels\t%s", ln);		// write out the 1st line, will insert a Label column
	else
		fwprintf(ostream, L"%s", ln);		// write out the 1st line

	ut_ind = parse_WOS_1st_line(ln, SP_TAB, L"UT");
	// read source file line by line
	// if the target record matches, write to the output file
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_WOS_line(line, SP_TAB, ut_ind, tfield);
		if (wcsncmp(tfield, L"ISI:", 4) == 0 || wcsncmp(tfield, L"WOS:", 4) == 0)	// remove "ISI:" or "WOS:"
			tf2 = &tfield[4];		
		else
			tf2 = tfield;
		wos_ndx = docid_search_wos(wos, nwos, tf2);
		if (wos_ndx >= 0)
		{
			if (which == RW_PARTITION)
			{
				//fwprintf(logstream, L"@@@@@ %d %d [%s] %d\n", partition, nw[nw_ndx].partition, nw[nw_ndx].alias, nw_ndx);
				if (wos[wos_ndx].partition == partition)
					fwprintf(ostream, L"%s", line); 
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read Scopus files and writes records with selected partition id to a file
// select records from the nw[] array
//
int readwrite_Scopus(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield;
	int nw_ndx;
	int i_au, i_py, i_vl, i_bp, i_ut;
	int year, volume, bpage;
	wchar_t docid[MAX_DOC_ID];

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2018/05/09
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read the 1st line of the source file, it contains the field names
	if (text_type == FT_ASCII)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = line;
	}
	else if (text_type == FT_UTF_8)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = &line[3];	// the fourth byte
		if (line[3] == 0xef && line[4] == 0xbb && line[5] == 0xbf)	// check if the UTF-8 mark repeat, this happens for Scopus csv data
			ln = &line[6];	// start from the 7th byte
	}
	else	// Unicode type
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = &line[1];	// actually the 3rd byte
	}

	if (which == RW_MAINPATH)	// added 2014/07/11
		fwprintf(ostream, L"Labels,%s", ln);		// write out the 1st line, will insert a Label column
	else
		fwprintf(ostream, L"%s", ln);		// write out the 1st line

	i_au = parse_Scopus_1st_line(ln, L"Authors");	// author names
	i_py = parse_Scopus_1st_line(ln, L"Year");			// year of publication
	i_vl = parse_Scopus_1st_line(ln, L"Volume");	// volume
	i_bp = parse_Scopus_1st_line(ln, L"Page start");	// beginning page
	if (Scopus_EID == 1) 
		i_ut = parse_Scopus_1st_line(ln, L"EID");	// beginning page

	// read source file line by line
	// if the target record matches, write to the output file
	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_py, tfield);	year = _wtoi(tfield);
		parse_Scopus_line(line, i_vl, tfield);	volume = _wtoi(tfield);
		parse_Scopus_line(line, i_bp, tfield);	bpage = _wtoi(tfield);
		if (iswdigit(tfield[0]) == 0 && tfield[0] != '\0')	// 1st character is not a digit
			bpage = _wtoi(tfield+1);	// skip the 1st character whether it's a 'A' or 'B', etc	// 2011/08/22
		else
			bpage = _wtoi(tfield);
		parse_Scopus_line(line, i_au, tfield);	
		//fwprintf(logstream, L"     [%s]\n", tfield); fflush(logstream);
		//if (wcsncmp(tfield, L"Li H., Li J.", 12) == 0)
		//{
		//	i = i+1;
		//	fwprintf(logstream, L"herehere\n");
		//	nw_ndx = i;
		//}
		if (Scopus_EID == 0) // create docid only if the target data does not contain Scopus EID 
			create_Scopus_docid(docid, towlower(tfield[0]), year, volume, bpage);
		else
		{
			parse_Scopus_line(line, i_ut, tfield);	
			wcscpy(docid, tfield);
		}
		//fwprintf(logstream, L"##### [%s]\n", docid); fflush(logstream);
		nw_ndx = docid_search_nw(nw, nnodes, docid);
		//fwprintf(logstream, L"$$$$$ [%s] %d\n", docid, nw_ndx); fflush(logstream);
		if (nw_ndx >= 0)
		{
			if (which == RW_MAINPATH)	// added 2014/07/11
			{
				if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s,%s", nw[nw_ndx].alias, line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
			else if (which == RW_PARTITION)
			{
				if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read Scopus files and writes records with selected partition id to a file
// select records from the wos[] array
//
int readwrite_Scopus_direct(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield;
	int wos_ndx;
	int i_au, i_py, i_vl, i_bp, i_ut;
	int year, volume, bpage;
	wchar_t docid[MAX_DOC_ID];

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2018/05/09
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read the 1st line of the source file, it contains the field names
	if (text_type == FT_ASCII)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = line;
	}
	else if (text_type == FT_UTF_8)
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = &line[3];	// the fourth byte
		if (line[3] == 0xef && line[4] == 0xbb && line[5] == 0xbf)	// check if the UTF-8 mark repeat, this happens for Scopus csv data
			ln = &line[6];	// start from the 7th byte
	}
	else	// Unicode type
	{
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
		ln = &line[1];	// actually the 3rd byte
	}

	fwprintf(ostream, L"Labels,%s", ln);		// write out the 1st line, will insert a Label column

	i_au = parse_Scopus_1st_line(ln, L"Authors");	// author names
	i_py = parse_Scopus_1st_line(ln, L"Year");			// year of publication
	i_vl = parse_Scopus_1st_line(ln, L"Volume");	// volume
	i_bp = parse_Scopus_1st_line(ln, L"Page start");	// beginning page
	if (Scopus_EID == 1) 
		i_ut = parse_Scopus_1st_line(ln, L"EID");	// beginning page

	// read source file line by line
	// if the target record matches, write to the output file
	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		if (line[0] == ',' && line[1] == ',')	// ignore lines begins with two commas, added 2016/10/29
			continue;
		parse_Scopus_line(line, i_py, tfield);	year = _wtoi(tfield);
		parse_Scopus_line(line, i_vl, tfield);	volume = _wtoi(tfield);
		parse_Scopus_line(line, i_bp, tfield);	bpage = _wtoi(tfield);
		if (iswdigit(tfield[0]) == 0 && tfield[0] != '\0')	// 1st character is not a digit
			bpage = _wtoi(tfield+1);	// skip the 1st character whether it's a 'A' or 'B', etc	// 2011/08/22
		else
			bpage = _wtoi(tfield);
		parse_Scopus_line(line, i_au, tfield);	
		//if (wcsncmp(tfield, L"Penney K", 8) == 0)
		//{
		//	i = i+1;
		//	fwprintf(logstream, L"herehere\n");
		//	nw_ndx = i;
		//}
		if (Scopus_EID == 0) // create docid only if the target data does not contain Scopus EID 
			create_Scopus_docid(docid, towlower(tfield[0]), year, volume, bpage);
		else
		{
			parse_Scopus_line(line, i_ut, tfield);	
			wcscpy(docid, tfield);
		}

		if (which == RW_PARTITION)
		{	
			wos_ndx = docid_search_wos(wos, nwos, docid);
			if (wos_ndx >= 0)
			{
				if (wos[wos_ndx].partition == partition)
					fwprintf(ostream, L"%s,%s", wos[wos_ndx].alias, line); 
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read Thomson Innovation Patent files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
//
int readwrite_TIP(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield;
	int ut_ind;
	int nw_ndx;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)	// changed to UTF-8, 2017/02/25
		return MSG_WOSFILE_NOTFOUND;

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2017/02/25
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line
	fwprintf(ostream, L"%s", line);		// write out the 1st line

	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line

	ut_ind = parse_TIP_2nd_line(line, SP_COLON, L"Publication Number");	
	ln = line;

	if (which == RW_MAINPATH)	// added 2014/07/11
		fwprintf(ostream, L"Labels,%s", ln);		// write out the 2nd line, will insert a Label column
	else
		fwprintf(ostream, L"%s", ln);		// write out the 2nd line

	// read source file line by line
	// if the target record matches, write to the output file
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_TIP_line(line, ut_ind, tfield);
		nw_ndx = docid_search_nw(nw, nnodes, tfield);
		if (nw_ndx >= 0)
		{
			if (which == RW_MAINPATH)	
			{
				if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s,%s", nw[nw_ndx].alias, line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
			else if (which == RW_PARTITION)
			{
				if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read Thomson Innovation Patent files and writes records with the UI (university-industry) flag set
// assume that the order in the input patent file is the same as that in the uspto[] structure
// this function is added 2016/12/22
//
int readwrite_UI_TIP(wchar_t *sname, wchar_t *oname, struct USPTO *uspto, FILE *sstream)
{
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i, k;
	wchar_t *tfield;
	int ut_ind;
	int nw_ndx;
	int pcnt;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// point back to the begining of the file, NOTE: the caller did not close the file, it will be closed in the function
//	
	rewind(sstream);

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2017/02/25
		return MSG_OFILE_CANNOTOPEN;

	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 1st line
	fwprintf(ostream, L"%s", &line[1]);		// write out the 1st line, the leading characters are the UTF-8 leading codes, &line[1] works, but the reason is not known!!!

	if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;	// read the 2nd line

	ut_ind = parse_TIP_2nd_line(line, SP_COLON, L"Publication Number");	
	ln = line;

	fwprintf(ostream, L"Assignee/Applicant - NTUST,%s", ln);		// write out the 2nd line

	// read source file line by line
	// if the target record matches, write to the output file
	pcnt = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_TIP_line(line, ut_ind, tfield);
		//fwprintf(logstream, L"##### %d [%s] [%s] %d %d\n", pcnt, tfield, uspto[pcnt].pid, uspto[pcnt].nassignee, uspto[pcnt].UI); fflush(logstream);
		if (wcscmp(tfield, uspto[pcnt].pid) == 0)	// make sure that the patent id is the same
		{
			wchar_t a_ntust[LBUF_SIZE];
			if (uspto[pcnt].UI != 0)
			{
				a_ntust[0] = '\0';
				for (k = 0; k < uspto[pcnt].nassignee; k++)
				{
					wcscat(a_ntust, assignees[uspto[pcnt].assignee[k]].name);
					if (k < (uspto[pcnt].nassignee - 1))
						wcscat(a_ntust, L" | ");
				}
				fwprintf(ostream, L"\"%s\",%s", a_ntust, ln); 
			}
		}
		pcnt++;
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read WEBPAT (.csv) files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
//
int readwrite_WEBPAT2(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *gret;
	wchar_t *line, *ln;
	wchar_t pid[MAX_PATENT_ID];
	int i;
	wchar_t *tfield;
	int i_ut;
	int nw_ndx;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//		
	if (text_type == FT_ASCII)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII	
			return MSG_WOSFILE_NOTFOUND;
	}
	else if (text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)	// open as text mode for UTF-8 type		
			return MSG_WOSFILE_NOTFOUND;
	}
	else	// Unicode type
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE") != 0)	// for Unicode data		
			return MSG_WOSFILE_NOTFOUND;
	}

	// open the output file
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	init_pfgetws(sstream); // prepare for xfgetws() to work
	fgetws(line, XXLBUF_SIZE, sstream);	// read the 1st line ==> ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
	fwprintf(ostream, L"%s", line);		// write out the 1st line
	fgetws(line, XXLBUF_SIZE, sstream);	// read the 2nd line ==> 編號,專利號/公開號,專利名稱,公告(公開)日,申請號,申請日,專利類型, .....
	fwprintf(ostream, L"%s", line);		// write out the 2nd line
	i_ut = parse_WEBPAT_2nd_line(line, SP_COLON, L"專利號/公開號");	// document ID
	fgetws(line, XXLBUF_SIZE, sstream); // read the 3rd line, copy it to the buffer (pline), this set the 1st call to pfgetws() to a proper setting 
	wcscpy(pline, line); just_exit_at_next_line = 1;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read source file line by line
	// if the target record matches, write to the output file
	while (TRUE)
	{		
		gret = pfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_WEBPAT_line(line, i_ut, tfield);
		correct_WEBPAT_pid(pid, tfield);	// write to .pid after correction
		nw_ndx = docid_search_nw(nw, nnodes, pid);
		if (nw_ndx >= 0)
		{
			if (which == RW_MAINPATH)	
			{
				if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
			else if (which == RW_PARTITION)
			{
				if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
		}
		if (gret == NULL)
			break;
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);
	done_pfgetws();

	return MSG_FILE_CREATED;
}

//
// read WEBPAT (.csv) files and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
//
int readwrite_WEBPAT3(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *gret;
	wchar_t *line, *ln;
	wchar_t pid[MAX_PATENT_ID];
	int i;
	wchar_t *tfield;
	int i_ut;
	int nw_ndx;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//		
	if (text_type == FT_ASCII)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII	
			return MSG_WOSFILE_NOTFOUND;
	}
	else if (text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)	// open as text mode for UTF-8 type		
			return MSG_WOSFILE_NOTFOUND;
	}
	else	// Unicode type
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE") != 0)	// for Unicode data		
			return MSG_WOSFILE_NOTFOUND;
	}

	// open the output file
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	init_wfgetws(sstream); // prepare for xfgetws() to work
	fgetws(line, XXLBUF_SIZE, sstream);	// read the 1st line ==> 公告(開)號,申請號,專利名稱,摘要,主CPC,CPC,主IPC,IPC,主UPC,UPC,LOC,FI,F-TERM,優先權國別,...
	fwprintf(ostream, L"%s", line);		// write out the 1st line
	i_ut = parse_WEBPAT3_1st_line(line, SP_COLON, L"公告(開)號");	// document ID
	if (i_ut == -1)
		i_ut = parse_WEBPAT3_1st_line(line, SP_COLON, L"Publication Number");	// document ID, added 2018/01/03
	fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (pline), this set the 1st call to wfgetws() to a proper setting
	if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
	wcscpy(wline, line); just_exit_at_next_line = 1;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	// read source file line by line
	// if the target record matches, write to the output file
	while (TRUE)
	{		
		gret = wfgetws(line, XXLBUF_SIZE, sstream);
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_WEBPAT3_line(line, i_ut, tfield);
		correct_WEBPAT3_pid(pid, tfield);	// write to .pid after correction, modified an errror here 2017/07/06
		nw_ndx = docid_search_nw(nw, nnodes, pid);
		if (nw_ndx >= 0)
		{
			if (which == RW_MAINPATH)	
			{
				if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
			else if (which == RW_PARTITION)
			{
				if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
				{
					fwprintf(ostream, L"%s", line); 
					nw[nw_ndx].flag = 1;	// raise flag
				}
			}
		}
		if (gret == NULL)
			break;
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);
	done_wfgetws();

	return MSG_FILE_CREATED;
}

//
// read a given Webpat file and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
// this function is added 2014/07/22
//
int readwrite_USPTO(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	int i;
	wchar_t *tfield;
	int state, lcnt, pcnt;
	int nw_ndx;
	int nohit, slen;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

		if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2018/05/09
		return MSG_OFILE_CANNOTOPEN;

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	state = 1; lcnt = 0; pcnt = 0;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
		{
			if (nohit == 0) fwprintf(ostream, L"%s", line);	// duplicate also carriage returns and line feeds, 2016/12/07
			continue;
		}
		switch (state)
		{
		case 1:	// patent number
			if (lcnt == 0)	// UTF code in the beginning of the file
			{
				if (text_type == FT_UTF_8)
					ln = &line[3];	// that's 3 bytes
				else if (text_type == FT_UTF_16_BE || text_type == FT_UTF_16_LE)
					ln = &line[1];	// that's 2 bytes
				else if (text_type == FT_UTF_32_BE || text_type == FT_UTF_32_LE)
					ln = &line[1];	// that's 4 bytes
				else
					ln = line;
			}
			else ln = line;
			nohit = 1;	// assume no hit
			if (wcsncmp(ln, pfid[state-1].lstr, 18) == 0)	
			{
				slen = wcslen(&ln[18]);
				wcsncpy_s(tfield, MAX_PATENT_ID, &ln[18], slen-1);	// do not copy the '0x0d' at the end of the line
				nw_ndx = docid_search_nw(nw, nnodes, tfield);
				if (nw_ndx >= 0)
				{
					if (which == RW_MAINPATH)	
					{
						if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
						{
							nohit = 0;	// hit the wanted patent
							fwprintf(ostream, L"%s", ln); 
							nw[nw_ndx].flag = 1;	// raise flag
						}
					}
					else if (which == RW_PARTITION)
					{
						if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
						{
							nohit = 0;	// hit the wanted patent
							fwprintf(ostream, L"%s", ln); 
							nw[nw_ndx].flag = 1;	// raise flag
						}
					}
				}
				state = 2;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;		
		case 2:	// issue date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 3; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 3:	// Appl_Number
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 4; else return MSG_WOSFILE_FORMAT_ERROR; 
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 4:	// Appl_Date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 5; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 5:	// Country
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 6; else	return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 6:	// Title
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 7; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 7:	// Abstract
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 8; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 8:	// Claim, there are many lines in the claim
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 9; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 9:	// Examiner
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 10;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break; 
		case 10:// Agent
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 11;	else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 11:// Assignee
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 12; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 12:// Foreign Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 13; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 13:// Priority 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 14; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 14:// IPC
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 15; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 15:// Inventor
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 16; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 16:// US Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 17; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 17:// US_Class 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 18; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 18:// separation line 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				if (nohit == 0) fwprintf(ostream, L"%s", line); 
				pcnt++;
				state = 1;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read a given Webpat file and writes records with the UI (university-industr) flag set
// assume that the order in the input patent file is the same as that in the uspto[] structure
// this function is added 2016/12/06
//
int readwrite_UI_USPTO(wchar_t *sname, wchar_t *oname, struct USPTO *uspto)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *ln;
	wchar_t *tfield;
	int state, lcnt, pcnt;
	int nohit, slen;

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//	
	if (text_type == FT_ASCII || text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII and UTF-8 type	
			return MSG_WOSFILE_NOTFOUND;
	}
	else
	{
		if (_wfopen_s(&sstream, sname, L"rb") != 0)	// need binary mode for Unicode data	
			return MSG_WOSFILE_NOTFOUND;
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) // changed to UTF-8, 2018/05/09
		return MSG_OFILE_CANNOTOPEN;

	state = 1; lcnt = 0; pcnt = 0; nohit = 1;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
		{
			if (nohit == 0) fwprintf(ostream, L"%s", line);	// duplicate also carriage returns and line feeds, 2016/12/07
			continue;
		}
		switch (state)
		{
		case 1:	// patent number
			if (lcnt == 0)	// UTF code in the beginning of the file
			{
				if (text_type == FT_UTF_8)
					ln = &line[3];	// that's 3 bytes
				else if (text_type == FT_UTF_16_BE || text_type == FT_UTF_16_LE)
					ln = &line[1];	// that's 2 bytes
				else if (text_type == FT_UTF_32_BE || text_type == FT_UTF_32_LE)
					ln = &line[1];	// that's 4 bytes
				else
					ln = line;
			}
			else ln = line;
			nohit = 1;	// assume no hit
			if (wcsncmp(ln, pfid[state-1].lstr, 18) == 0)	
			{
				slen = wcslen(&ln[18]);
				wcsncpy_s(tfield, MAX_PATENT_ID, &ln[18], slen-1);	// do not copy the '0x0d' at the end of the line
				if (wcscmp(tfield, uspto[pcnt].pid) == 0)	// make sure that the patend id is the same
				{
					if (uspto[pcnt].UI == 1)
					{
						nohit = 0;	// hit the wanted patent
						fwprintf(ostream, L"%s", ln); 
					}
				}
				state = 2;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;		
		case 2:	// issue date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 3; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 3:	// Appl_Number
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 4; else return MSG_WOSFILE_FORMAT_ERROR; 
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 4:	// Appl_Date
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 5; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 5:	// Country
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 6; else	return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 6:	// Title
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 7; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 7:	// Abstract
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 8; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 8:	// Claim, there are many lines in the claim
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 9; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 9:	// Examiner
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 10;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break; 
		case 10:// Agent
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 11;	else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 11:// Assignee
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 12; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 12:// Foreign Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 13; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 13:// Priority 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 14; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 14:// IPC
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 15; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 15:// Inventor
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 16; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 16:// US Reference
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 17; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 17:// US_Class 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	state = 18; else return MSG_WOSFILE_FORMAT_ERROR;
			if (nohit == 0) fwprintf(ostream, L"%s", line); 
			break;
		case 18:// separation line 
			if (wcsncmp(line, pfid[state-1].lstr, 18) == 0)	
			{
				if (nohit == 0) fwprintf(ostream, L"%s", line); 
				pcnt++;
				state = 1;
			}
			else
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);

	return MSG_FILE_CREATED;
}

//
// read a given TaiwanT&D data file and writes records with selected partition id to a file
// if the given partition is 99999, write all nodes in the networks
// this function is added 2016/03/19
//
int readwrite_TaiwanTD(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file, original text format
	FILE *ostream2;		// for output file, ".csv" format
	wchar_t *line;
	wchar_t *dp, *tp;
	wchar_t *lp, *bp, *buffer;
	int i, k, jj;
	wchar_t *tfield;
	int state, lcnt, pcnt;
	int nw_ndx;
	int nohit, slen;
	wchar_t cur_author[MAX_AUTHOR_NAME];

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
	buffer = (wchar_t *)malloc(XXLBUF_SIZE * 2 * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;

// 
// Open the source file (will fail if the file does not exist)
//		
	if (text_type == FT_ASCII)
	{
		if (_wfopen_s(&sstream, sname, L"r") != 0)	// open as text mode for ASCII	
			return MSG_WOSFILE_NOTFOUND;
	}
	else if (text_type == FT_UTF_8)
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8") != 0)	// open as text mode for UTF-8 type		
			return MSG_WOSFILE_NOTFOUND;
	}
	else	// Unicode type
	{
		if (_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE") != 0)	// for Unicode data		
			return MSG_WOSFILE_NOTFOUND;
	}

	if (which == RW_ALL)	// special case, prepare a ".csv" format file, added 2017/04/22
	{
		// open the output file (".csv" format)
		if (_wfopen_s(&ostream2, oname, L"wt, ccs=UTF-8") != 0) 
			return MSG_OFILE_CANNOTOPEN;
		else
			fwprintf(ostream2, L"Taiwan T&D in \".csv\" format\n");	// write out the identification header for Taiwan T&D data
		for (i = 0; i < nnodes; i++)
		{
			jj = nw[i].ndx2wos;
			fwprintf(ostream2, L"%s,%s,%d,%s,", wos[jj].alias, ttd_title[wos[jj].ndx2title].name, wos[jj].year, authors[wos[jj].author[0]].name);
			if (tkeyword[wos[jj].tkws[0]].name != '\0')
				fwprintf(ostream2, L"%s", tkeyword[wos[jj].tkws[0]].name); 
			for (k = 1; k < wos[jj].ntkws; k++)
				fwprintf(ostream2, L";%s", tkeyword[wos[jj].tkws[k]].name);
			fwprintf(ostream2, L"\n");
		}
		fclose(sstream); 
		fclose(ostream2);	
		free(line); free(tfield); free(buffer);
		return MSG_FILE_CREATED;
	}

	// open the output file (original text format)
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;
	else
		fwprintf(ostream, L"Taiwan T&D\n");	// write out the identification header for Taiwan T&D data

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag

	state = 1; lcnt = 0; pcnt = 0; bp = buffer;
	while (TRUE)
	{		
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		lp = line; 
		while (TRUE) // copy the text read to the buffer continuously
		{
			if (*lp != '\0')
				*bp++ = *lp++;
			else
				break;
		}
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		switch (state)
		{
		case 1:	// "論文名稱：", it may take several lines to find this label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{				
				nohit = 1;	// assume no hit
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0' && *tp != L' ') tp++; tp--; *tp = L'\0';	// remove the spaces and line feed at the end
				//fwprintf(logstream, L"###%d %s\n", pcnt, dp); fflush(logstream);
				state = 2;
			}
			break;		
		case 2:	// 論文名稱(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 3; break; } else state = 3; 
		case 3:	// 論文出版年：, this label can be missing
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 4; break; } else state = 4; 
		case 4:	// 研究生：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				dp = tp = &line[tdid[state-1].len];
				while (*tp != L'\0') tp++; tp--; tp--; // backward 2 steps (null and line feed)
				while (*tp == ' ') tp--; tp++; *tp = '\0';	// remove the spaces and line feed at the end
				wcscpy(cur_author, dp);		// added 2016/04/05
				state = 5; 
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 5:	// 研究生(外文)：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 6; break; } else state = 6; 
		case 6:	// 指導教授：, there can be multiple advisers
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 6;
				break;
			}
			else 
				state = 7;
		case 7:	// 指導教授(外文), optional and multiple 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 7;
				break;
			}
			else 
				state = 8; 
		case 8:	// 口試委員：, there can be multiple oral committee members
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 8;
				break;
			}
			else 
				state = 9; 
		case 9:	// 口試委員(外文)：, optional and multiple 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				state = 9;
				break;
			}
			else 
				state = 10; 
		case 10:	// 學位類別：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 11; 
				break;
			}
			else
			{
				wos[pcnt].info |= TAIWANTD_DEGREE_UNKNOWN;
				state = 11; 
			}
		case 11:	// 校院名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 12; 
			break; 
		case 12:// 系所名稱：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
				state = 13;	
			else 
				return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 13:// 畢業學年度：
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				dp = &line[tdid[state-1].len];
				wos[pcnt].year = _wtoi(dp) + 1911;
				//fwprintf(logstream, L"[%d] \n", wos[pcnt].year);
				// make up the paper ID (wos[].docid)
				if ((wos[pcnt].info &= 0xff) == TAIWANTD_MASTER)
					swprintf(wos[pcnt].docid, L"%d_1_%s", wos[pcnt].year, cur_author);	// modified 2016/04/05
				else if ((wos[pcnt].info &= 0xff) == TAIWANTD_PHD)
					swprintf(wos[pcnt].docid, L"%d_2_%s", wos[pcnt].year, cur_author);	// modified 2016/04/05
				else
					swprintf(wos[pcnt].docid, L"%d_?_%s", wos[pcnt].year, cur_author);	// modified 2016/04/05	
				nw_ndx = docid_search_nw(nw, nnodes, wos[pcnt].docid);
				if (nw_ndx >= 0)
				{
					if (which == RW_MAINPATH)	
					{
						if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
						{
							nohit = 0;	// hit the wanted patent
							fwprintf(logstream, L"%s", wos[pcnt].docid);
							nw[nw_ndx].flag = 1;	// raise flag
						}
					}
					else if (which == RW_PARTITION)
					{
						if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
						{
							nohit = 0;	// hit the wanted patent
							fwprintf(logstream, L"%s", wos[pcnt].docid);
							nw[nw_ndx].flag = 1;	// raise flag
						}
					}
				}
				state = 14; 
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 14:// 語文別：, optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 15; break; } else state = 15; 
		case 15:// 論文頁數： , optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 16; break; } else state = 16; 
		case 16:// 中文關鍵詞：, there can be repeated "中文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 16;
				break;
			}
			else 
				state = 17;
		case 17:// 外文關鍵詞：, there can be repeated "外文關鍵詞：" label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
			{
				state = 17;
				break;
			}
			else 
				state = 18;
		case 18:// 論文摘要：	// optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 19; break; } else state = 19; 
		case 19:// 論文外文摘要： // optional
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 20; break; } else state = 20; 
		case 20:// 論文目次：, optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 21; break; } else state = 21;
		case 21:// 論文參考文獻： , optional label
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	{ state = 22; break; } else state = 22;
		case 22:// 被引用:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	
				state = 23; 
			break;
		case 23:// 點閱:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 24; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 24:// 評分:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 25; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 25:// 下載: 
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 26; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 26:// 書目收藏:
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)	state = 27; else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		case 27:// "URI "
			if (wcsncmp(line, tdid[state-1].lstr, tdid[state-1].len) == 0)
			{
				if (nohit)
					bp = buffer;	// ignore the content in the buffer by resetting bp
				else
				{
					//fwprintf(logstream, L"Hit [%s]\n", wos[pcnt].docid); fflush(logstream);
					*bp = '\0';
					fputws(buffer, ostream); fflush(ostream);
					fwprintf(ostream, L"%d =============================================================\n", pcnt);
					bp = buffer;
				}
				pcnt++;
				state = 1;
			}
			else return MSG_WOSFILE_FORMAT_ERROR;
			break;
		default:
			break;
		}
		lcnt++;	
	}

	fclose(sstream); 	
	fclose(ostream);	
	free(line); free(tfield); free(buffer);

	return MSG_FILE_CREATED;
}

//
// read Taiwan Citation Index (TCI) files and writes records with selected partition id to a file
// select records from the nw[] array
//
int readwrite_TCI(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *nline;
	int i;
	wchar_t *tfield;
	wchar_t *sp, *tp;
	int nw_ndx, wos_ndx;
	int i_au, i_py, i_vl, i_pg, i_ut;
	int year, volume, issue, bpage;
	wchar_t docid[MAX_DOC_ID];

	// allocate working memory
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
	}
	else if (text_type == FT_UTF_8)
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8");	// open as text mode for UTF-8 type	
	}
	else	// Unicode type
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE");	// for Unicode data	
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	init_xfgetws(sstream); // prepare for xfgetws() to work

	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	nline = line;
	i_au = parse_TCI_1st_line(nline, L"作者");	// author names
	i_py = parse_TCI_1st_line(nline, L"出版日期");			// year of publication
	i_vl = parse_TCI_1st_line(nline, L"卷期");	// volume
	i_pg = parse_TCI_1st_line(nline, L"頁次");	// page

	if (which == RW_MAINPATH || which == RW_ALL)	
		fwprintf(ostream, L"Labels,%s", line);		// write out the 1st line, will insert a Label column
	else
		fwprintf(ostream, L"%s", line);		// write out the 1st line

	// read source file line by line
	// if the target record matches, write to the output file
	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag
	while (TRUE)
	{		
		if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_TCI_line(line, i_py, tfield);	
		year = _wtoi(tfield);	
		parse_TCI_line(line, i_vl, tfield);
		wchar_t tbuf[20];
		sp = tfield; tp = tbuf;
		while (*sp != ':' && *sp != '\0') *tp++ = *sp++; *tp = '\0';
		volume = _wtoi(tbuf);
		if (*sp == ':')
		{
			sp++; tp = tbuf;
			while (*sp != ':' && *sp != '\0' && *sp != '=') *tp++ = *sp++; *tp = '\0';
			issue = _wtoi(tbuf);
		}
		else 
			issue = 0;
		parse_TCI_line(line, i_pg, tfield);
		sp = tfield; tp = tbuf;
		if (iswdigit(*sp) == 0 && *sp != '\0')	// 1st character is not a digit
			sp++;	
		while (*sp != '-' && *sp != '\0') *tp++ = *sp++; *tp = '\0';
		bpage = _wtoi(tbuf);	
		parse_TCI_line(line, i_au, tfield);	
		sp = tfield;
		while (*sp != '\0') 
		{
			if (*sp == '\n') { *sp = '\0'; break; } // remove the '\n' at the end of the author name if there is one
			sp++;
		}
		create_TCI_docid(docid, tfield, year, volume, issue, bpage);
		if (which == RW_ALL)	// added 2016/06/17
		{
			wos_ndx = docid_search_wos(wos, nwos, docid);
			if (wos_ndx >= 0)
				fwprintf(ostream, L"%s,%s", wos[wos_ndx].alias, line); 
		}
		else
		{
			nw_ndx = docid_search_nw(nw, nnodes, docid);
			if (nw_ndx >= 0)
			{
				if (which == RW_MAINPATH)	
				{
					if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
					{
						fwprintf(ostream, L"%s,%s", nw[nw_ndx].alias, line); 
						nw[nw_ndx].flag = 1;	// raise flag
					}
				}
				else if (which == RW_PARTITION)
				{
					if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
					{
						fwprintf(ostream, L"%s", line); 
						nw[nw_ndx].flag = 1;	// raise flag
					}
				}
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);
	done_xfgetws();

	return MSG_FILE_CREATED;
}

//
// read Taiwan Citation Index (TCI) files and writes records with selected partition id to a file
// select records from the wos[] array
//
int readwrite_TCI_direct(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *nline;
	int i;
	wchar_t *tfield;
	wchar_t *sp, *tp;
	int nw_ndx, wos_ndx;
	int i_au, i_py, i_vl, i_pg, i_ut;
	int year, volume, issue, bpage;
	wchar_t docid[MAX_DOC_ID];

	// allocate working memory
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
	}
	else if (text_type == FT_UTF_8)
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8");	// open as text mode for UTF-8 type	
	}
	else	// Unicode type
	{
		_wfopen_s(&sstream, sname, L"rt, ccs=UNICODE");	// for Unicode data	
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	init_xfgetws(sstream); // prepare for xfgetws() to work

	// read the 1st line of the source file, it contains the field names
	if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	nline = line;
	i_au = parse_TCI_1st_line(nline, L"作者");	// author names
	i_py = parse_TCI_1st_line(nline, L"出版日期");			// year of publication
	i_vl = parse_TCI_1st_line(nline, L"卷期");	// volume
	i_pg = parse_TCI_1st_line(nline, L"頁次");	// page

	fwprintf(ostream, L"%s", line);		// write out the 1st line

	// read source file line by line
	// if the target record matches, write to the output file
	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag
	while (TRUE)
	{		
		if(xfgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		parse_TCI_line(line, i_py, tfield);	
		year = _wtoi(tfield);	
		parse_TCI_line(line, i_vl, tfield);
		wchar_t tbuf[20];
		sp = tfield; tp = tbuf;
		while (*sp != ':' && *sp != '\0') *tp++ = *sp++; *tp = '\0';
		volume = _wtoi(tbuf);
		if (*sp == ':')
		{
			sp++; tp = tbuf;
			while (*sp != ':' && *sp != '\0' && *sp != '=') *tp++ = *sp++; *tp = '\0';
			issue = _wtoi(tbuf);
		}
		else 
			issue = 0;
		parse_TCI_line(line, i_pg, tfield);
		sp = tfield; tp = tbuf;
		if (iswdigit(*sp) == 0 && *sp != '\0')	// 1st character is not a digit
			sp++;	
		while (*sp != '-' && *sp != '\0') *tp++ = *sp++; *tp = '\0';
		bpage = _wtoi(tbuf);	
		parse_TCI_line(line, i_au, tfield);	
		sp = tfield;
		while (*sp != '\0') 
		{
			if (*sp == '\n') { *sp = '\0'; break; } // remove the '\n' at the end of the author name if there is one
			sp++;
		}
		create_TCI_docid(docid, tfield, year, volume, issue, bpage);
		if (which == RW_PARTITION)
		{
			wos_ndx = docid_search_wos(wos, nwos, docid);
			if (wos_ndx >= 0)
			{
				if (wos[wos_ndx].partition == partition)
						fwprintf(ostream, L"%s", line); 
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);
	done_xfgetws();

	return MSG_FILE_CREATED;
}

//
// read LAWSNOTE files and writes records with selected partition id to a file
// select records from the nw[] array
//
int readwrite_LAWSNOTE(wchar_t *sname, wchar_t *oname, int which, int partition)
{
	FILE *sstream;		// for source file
	FILE *ostream;		// for output file
	wchar_t *line, *nline;
	int i;
	wchar_t *tfield;
	wchar_t *sp, *tp;
	int nw_ndx, wos_ndx;
	int i_au, i_py, i_ut;
	int year, volume, issue, bpage;
	wchar_t docid[MAX_DOC_ID];

	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tfield = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (tfield == NULL) return MSG_NOT_ENOUGH_MEMORY;
// 
// Open the source file (will fail if the file does not exist)
//	
	_wfopen_s(&sstream, sname, L"rt, ccs=UTF-8");	// open as text mode for UTF-8 type	

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	init_lnfgetws(sstream); // prepare for xfgetws() to work

	// read the 1st line of the source file, it contains the field names
	if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL) return MSG_WOSFILE_FORMAT_ERROR;
	nline = line;
	i_ut = 1; // temp code
	i_py = parse_LAWSNOTE_1st_line(nline, L"date");			// date

	fwprintf(ostream, L"%s", line);		// write out the 1st line

	// read source file line by line
	// if the target record matches, write to the output file
	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// clear the general flag
	while (TRUE)
	{		
		if(lnfgetws(line, XXLBUF_SIZE, sstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		parse_LAWSNOTE_line(line, i_ut, tfield);
		if (which == RW_ALL)
		{
			wos_ndx = docid_search_wos(wos, nwos, tfield);
			if (wos_ndx >= 0)
				fwprintf(ostream, L"%s", line); 
		}
		else
		{
			nw_ndx = docid_search_nw(nw, nnodes, tfield);
			if (nw_ndx >= 0)
			{
				if (which == RW_MAINPATH)	
				{
					if (nw[nw_ndx].mainp == TRUE && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
					{
						fwprintf(ostream, L"%s,%s", nw[nw_ndx].alias, line); 
						nw[nw_ndx].flag = 1;	// raise flag
					}
				}
				else if (which == RW_PARTITION)
				{
					if ((nw[nw_ndx].partition == partition || partition == 99999) && nw[nw_ndx].flag == 0)	// to avoid writing record (duplicated in the source) twice
					{
						fwprintf(ostream, L"%s", line); 
						nw[nw_ndx].flag = 1;	// raise flag
					}
				}
			}
		}
	}

	fclose(sstream); 	
	fclose(ostream);
	free(line); free(tfield);
	done_lnfgetws();

	return MSG_FILE_CREATED;
}

//
// use binary search to find the proper position of a document id in a nw[] array
//
int docid_search_nw(struct PN d[], int num, wchar_t *str)
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
// use binary search to find the proper position of a document id in a wos[] array
//
int docid_search_wos(struct WOS d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].docid) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].docid) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].docid) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].docid) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// assemble all the given WOS file into one file
// the file is denoted with "xxx-Mdd.txt", where "dd" is the number of WOS files
//
int assemble_multiple_WOS_data(wchar_t *ifname, wchar_t *ofname)
{	
	int i; 
	wchar_t sdirname[FNAME_SIZE], tname[FNAME_SIZE];
	wchar_t srcname[FNAME_SIZE], dstname[FNAME_SIZE];
	struct _wfinddata_t fs;
	intptr_t hFile;
	int nwosfiles, rcnt, npapers;
	FILE *sstream, *dstream, *wstream;
	wchar_t *lp, *dp;
	wchar_t *line;
	
	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// read the pointing file to get the data directory
	if (_wfopen_s(&wstream, ifname, L"rt, ccs=UTF-8") != 0)	
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(logstream, L"WOS data file read successfully: %s\n", ifname);
	fgetws(line, LBUF_SIZE, wstream);	// ignore the first line, it has been checked in the previous code
	if(fgetws(line, LBUF_SIZE, wstream) == NULL)	// the 2nd line should have the name of the directory that contains the multiple WOS data
		return UNKNOWN_DATA;
	if (line[0] == '\n' || line[0] == '\r')
		return UNKNOWN_DATA;
	lp = line; dp = sdirname;
	while (*lp != '\0') *dp++ = *lp++; dp--; lp--; 
	if (*lp == '\r' || *lp == '\n') 
		*dp = '\0';	// remove the line feed at the end of the line
	else
	{
		dp++; *dp = '\0';
	}
	fclose(wstream);

	// read WOS data from the indicated directory
	swprintf(srcname, L"%s\\*.txt", sdirname);
	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	nwosfiles = 0;
	do	
	{	
		nwosfiles++;
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);

	fwprintf(logstream, L"Number of WOS files = %d\n", nwosfiles);
	fwprintf(logstream, L"Reading multiple WOS files....\n");

	// open and copy the WOS data file one by one (to the directory of the pointing file)
	lp = ifname; dp = tname;
	while (*lp != '\0') *dp++ = *lp++;
	while (*dp != '.') dp--;
	*dp = '\0';
	swprintf(dstname, L"%s-M%d.txt", tname, nwosfiles);
	wcscpy(ofname, dstname);
	if (_wfopen_s(&dstream, dstname, L"wt, ccs=UTF-8") != 0)	
		return MSG_WOSFILE_NOTFOUND;

	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	nwosfiles = 0; npapers = 0;
	do	
	{	
		swprintf(srcname, L"%s\\%s", sdirname, fs.name);
		if (_wfopen_s(&sstream, srcname, L"rt, ccs=UTF-8") != 0)	
			return MSG_WOSFILE_NOTFOUND;
		fwprintf(logstream, L"File %s opened, ", srcname);
		rcnt = 0;
		while (TRUE)
		{		
			if(fgetws(line, XXLBUF_SIZE, sstream) == NULL)
				break;
			if (nwosfiles > 0 && rcnt == 0) { rcnt++; continue; }	// copy the 1st line only for the 1st file
			fputws(line, dstream);
			rcnt++;
		}
		npapers += rcnt - 1;
		fwprintf(logstream, L"%d record read.\n", rcnt-1);
		fclose(sstream);
		nwosfiles++;
	} 
	while (_wfindnext(hFile, &fs) == 0);

	fwprintf(logstream, L"Total number of WOS papers = %d\n", npapers);
	_findclose(hFile);
	fclose(dstream);
	free(line);

	return 0;
}

//
// count the total number of files in a directory (and its subdirectory)
// this is a recursive function
//
static int ntcifiles;
static int ntcidirs;
int count_files_in_a_directory(wchar_t *sdirname)
{
	struct _wfinddata_t fs;
	intptr_t hFile;
	wchar_t srcname[FNAME_SIZE], ndirname[FNAME_SIZE];

	swprintf(srcname, L"%s\\*.*", sdirname);
	//fwprintf(logstream, L"TCI data directory: [%s]\n", sdirname);
	// read TCI data from the indicated directory
	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	do	
	{	
		if (fs.attrib & _A_SUBDIR)	// its a directory
		{
			if (wcscmp(fs.name, L".") == 0 || wcscmp(fs.name, L"..") == 0)
				continue;
			else
			{
				swprintf(ndirname, L"%s\\%s", sdirname, fs.name);
				ntcidirs++;
				count_files_in_a_directory(ndirname);
				continue;
			}
		}
		else
			ntcifiles++;
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);
	
	return 0;
}

//
// assemble all the files in a directory (and its subdirectory) for TCI data
// this is a recursive function
//
static FILE *dstream;
static int npapers;
int assemble_files_in_a_directory_TCI(wchar_t *sdirname, FILE *dstream)
{
	struct _wfinddata_t fs;
	intptr_t hFile;
	wchar_t srcname[FNAME_SIZE], ndirname[FNAME_SIZE];
	FILE *sstream;
	wchar_t *line, *sp;
	wchar_t *xret;
	int rcnt;
	int rcnt_directory;
	
	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	rcnt_directory = 0;
	swprintf(srcname, L"%s\\*.*", sdirname);
	fwprintf(logstream, L"TCI data directory: [%s]\n", sdirname);
	// read TCI data from the indicated directory
	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	do	
	{	
		if (fs.attrib & _A_SUBDIR)	// its a directory
		{
			if (wcscmp(fs.name, L".") == 0 || wcscmp(fs.name, L"..") == 0)
				continue;
			else
			{
				swprintf(ndirname, L"%s\\%s", sdirname, fs.name);
				ntcidirs++;
				assemble_files_in_a_directory_TCI(ndirname, dstream);
				continue;
			}
		}
		else 
		{
			swprintf(srcname, L"%s\\%s", sdirname, fs.name);
			if (_wfopen_s(&sstream, srcname, L"rt, ccs=UTF-8") != 0)	
				return MSG_WOSFILE_NOTFOUND;
			fwprintf(logstream, L"   File %s opened, ", srcname); fflush(logstream);
			if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) // read the first line
				return UNKNOWN_DATA;
			if (ntcifiles == 0)
			{
				fputws(line, dstream);	// write out the 1st line, only for the 1st time
				sp = line; n_fields = 0;
				while (*sp != '\n') { if (*sp == ',') n_fields++; sp++;	} // find the number of fields from the data in the 1st line
				n_fields++;	// but the number of delimiter (',') is n_fields-1, n_fileds is used in the function xfgetws()
			}
			init_xfgetws(sstream); // prepare for xfgetws() to work
			rcnt = 0;
			while (TRUE)
			{		
				xret = xfgetws(line, XXLBUF_SIZE, sstream);
//#ifdef DEBUG
				wchar_t tmp[200];
				wcsncpy(tmp, line, 40);
				fwprintf(logstream, L"$$$$$ [%s]\n", tmp); fflush(logstream);
//#endif DEBUG
				fputws(line, dstream);
				rcnt++; rcnt_directory++;
				if (xret== NULL) break;	
			}
			npapers += rcnt;
			fwprintf(logstream, L"%d records read.\n", rcnt); fflush(logstream);
			fclose(sstream);
			done_xfgetws();
			ntcifiles++;
		}
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);
	free(line);	
	fwprintf(logstream, L"   Total %d records read from the directory [%s].\n", rcnt_directory, sdirname); fflush(logstream);

	return 0;
}

//
// assemble all the given TCI files into one file
// the file is denoted with "xxx-Mdd.txt", where "dd" is the number of TCI files
//
int assemble_multiple_TCI_data(wchar_t *ifname, wchar_t *ofname)
{	
	int i; 
	wchar_t sdirname[FNAME_SIZE], tname[FNAME_SIZE];
	wchar_t srcname[FNAME_SIZE], dstname[FNAME_SIZE];
	struct _wfinddata_t fs;
	intptr_t hFile;
	FILE *wstream;
	wchar_t *lp, *dp, *sp;
	wchar_t *line;
	
	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// read the pointing file to get the data directory
	if (_wfopen_s(&wstream, ifname, L"rt, ccs=UTF-8") != 0)	
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(logstream, L"TCI data file read successfully.\n");
	fgetws(line, LBUF_SIZE, wstream);	// ignore the first line, it has been checked in the previous code
	if(fgetws(line, LBUF_SIZE, wstream) == NULL)	// the 2nd line should have the name of the directory that contains the multiple TCI data
		return UNKNOWN_DATA;
	if (line[0] == '\n' || line[0] == '\r')
		return UNKNOWN_DATA;
	lp = line; dp = sdirname;
	while (*lp != '\0') *dp++ = *lp++; dp--; lp--; 
	if (*lp == '\r' || *lp == '\n') 
		*dp = '\0';	// remove the line feed at the end of the line
	else
	{
		dp++; *dp = '\0';
	}
	fclose(wstream);

	// count the number of files in the indicated directory (and its subdirrectory)

	ntcifiles = ntcidirs = 0;
	count_files_in_a_directory(sdirname);
#ifdef XXX
	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	ntcifiles = 0;
	do	
	{	
		ntcifiles++;
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);
#endif XXX
	fwprintf(logstream, L"Number of TCI data directories = %d, files = %d\n", ntcidirs+1, ntcifiles);
	fwprintf(logstream, L"Reading multiple TCI files....\n");

	// open and copy the TCI data file one by one (to the directory of the pointing file)
	lp = ifname; dp = tname;
	while (*lp != '\0') *dp++ = *lp++;
	while (*dp != '.') dp--;
	*dp = '\0';
	swprintf(dstname, L"%s-M%d-%d.csv", tname, ntcidirs+1, ntcifiles);
	wcscpy(ofname, dstname);
	if (_wfopen_s(&dstream, dstname, L"wt, ccs=UTF-8") != 0)	
		return MSG_WOSFILE_NOTFOUND;

	ntcifiles = ntcidirs = 0; npapers = 0;
	assemble_files_in_a_directory_TCI(sdirname, dstream);

#ifdef XXX
	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	ntcifiles = 0; npapers = 0;
	do	
	{	
		swprintf(srcname, L"%s\\%s", sdirname, fs.name);
		if (_wfopen_s(&sstream, srcname, L"rt, ccs=UTF-8") != 0)	
			return MSG_WOSFILE_NOTFOUND;
		fwprintf(logstream, L"File %s opened, ", srcname); fflush(logstream);
		if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) // read the first line
			return UNKNOWN_DATA;
		if (ntcifiles == 0)
		{
			fputws(line, dstream);	// write out the 1st line, only for the 1st time
			sp = line; n_fields = 0;
			while (*sp != '\n') { if (*sp == ',') n_fields++; sp++;	} // find the number of fields from the data in the 1st line
			n_fields++;	// but the number of delimiter (',') is n_fields-1, n_fileds is used in the function xfgetws()
		}
		init_xfgetws(sstream); // prepare for xfgetws() to work
		rcnt = 0;
		while (TRUE)
		{		
			xret = xfgetws(line, XXLBUF_SIZE, sstream);
//#ifdef DEBUG
			wchar_t tmp[200];
			wcsncpy(tmp, line, 40);
			fwprintf(logstream, L"$$$$$ [%s]\n", tmp); fflush(logstream);
//#endif DEBUG
			fputws(line, dstream);
			rcnt++;
			if (xret== NULL) break;	
		}
		npapers += rcnt;
		fwprintf(logstream, L"%d record read.\n", rcnt); fflush(logstream);
		fclose(sstream);
		done_xfgetws();
		ntcifiles++;
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);
#endif XXX

	fwprintf(logstream, L"Total number of TCI papers = %d\n", npapers); fflush(logstream);
	fclose(dstream);
	free(line);

	return 0;
}

//
// assemble all the files in a directory (and its subdirectory) for WEBPAT3 data
// this is a recursive function
// this function is added 2018/01/03
//
int assemble_files_in_a_directory_WEBPAT3(wchar_t *sdirname, FILE *dstream)
{
	struct _wfinddata_t fs;
	intptr_t hFile;
	wchar_t srcname[FNAME_SIZE], ndirname[FNAME_SIZE];
	FILE *sstream;
	wchar_t *line, *sp;
	wchar_t *xret;
	int rcnt;
	int rcnt_directory;
	
	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	rcnt_directory = 0;
	swprintf(srcname, L"%s\\*.*", sdirname);
	fwprintf(logstream, L"WEBPAT3 data directory: [%s]\n", sdirname);
	// read WEBPAT3 data from the indicated directory
	if ((hFile = _wfindfirst(srcname, &fs)) == -1L)
		return MSG_WOSFILE_NOTFOUND;
	do	
	{	
		if (fs.attrib & _A_SUBDIR)	// its a directory
		{
			if (wcscmp(fs.name, L".") == 0 || wcscmp(fs.name, L"..") == 0)
				continue;
			else
			{
				swprintf(ndirname, L"%s\\%s", sdirname, fs.name);
				ntcidirs++;
				assemble_files_in_a_directory_WEBPAT3(ndirname, dstream);
				continue;
			}
		}
		else 
		{
			swprintf(srcname, L"%s\\%s", sdirname, fs.name);
			if (_wfopen_s(&sstream, srcname, L"rt, ccs=UTF-8") != 0)	
				return MSG_WOSFILE_NOTFOUND;
			fwprintf(logstream, L"   File %s opened, ", srcname); fflush(logstream);
			if(fgetws(line, XXLBUF_SIZE, sstream) == NULL) // read the first line
				return UNKNOWN_DATA;
			if (ntcifiles == 0)
			{
				fputws(line, dstream);	// write out the 1st line, only for the 1st time
				sp = line; n_fields = 0;
				while (*sp != '\n') { if (*sp == ',') n_fields++; sp++;	} // find the number of fields from the data in the 1st line
				n_fields++;	// but the number of delimiter (',') is n_fields-1, n_fileds is used in the function xfgetws()
			}
			init_wfgetws(sstream); // prepare for wfgetws() to work
			fgetws(line, XXLBUF_SIZE, sstream); // read the 2nd line, copy it to the buffer (wline), this set the 1st call to wfgetws() to a proper setting 
			if (!valid_patent_id(line)) { line[0] = '\n'; line[1] = '\0'; }	// 2nd line does not have valid patent id
			wcscpy(wline, line); just_exit_at_next_line = 1;
			rcnt = 0;
			while (TRUE)
			{		
				xret = wfgetws(line, XXLBUF_SIZE, sstream);
				fputws(line, dstream);
				rcnt++; rcnt_directory++;
				if (xret== NULL) break;	
			}
			npapers += rcnt;
			fwprintf(logstream, L"%d records read.\n", rcnt); fflush(logstream);
			fclose(sstream);
			done_wfgetws();
			ntcifiles++;
		}
	} 
	while (_wfindnext(hFile, &fs) == 0);
	_findclose(hFile);
	free(line);	
	fwprintf(logstream, L"   Total %d records read from the directory [%s].\n", rcnt_directory, sdirname); fflush(logstream);

	return 0;
}

//
// assemble all the given WEBPAT3 files into one file
// the output file is denoted with "xxx-Mdd.txt", where "dd" is the number of WEBPAT3 files
//
int assemble_multiple_WEBPAT3_data(wchar_t *ifname, wchar_t *ofname)
{	
	int i; 
	wchar_t sdirname[FNAME_SIZE], tname[FNAME_SIZE];
	wchar_t srcname[FNAME_SIZE], dstname[FNAME_SIZE];
	struct _wfinddata_t fs;
	intptr_t hFile;
	FILE *wstream;
	wchar_t *lp, *dp, *sp;
	wchar_t *line;
	
	// allocate working memory
	line = (wchar_t *)malloc(XXLBUF_SIZE * sizeof(wchar_t));
	if (line == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// read the pointing file to get the data directory
	if (_wfopen_s(&wstream, ifname, L"rt, ccs=UTF-8") != 0)	
		return MSG_WOSFILE_NOTFOUND;
	fwprintf(logstream, L"WEBPAT3 data file read successfully.\n");
	fgetws(line, LBUF_SIZE, wstream);	// ignore the first line, it has been checked in the previous code
	if(fgetws(line, LBUF_SIZE, wstream) == NULL)	// the 2nd line should have the name of the directory that contains the multiple WEBPAT3 data
		return UNKNOWN_DATA;
	if (line[0] == '\n' || line[0] == '\r')
		return UNKNOWN_DATA;
	lp = line; dp = sdirname;
	while (*lp != '\0') *dp++ = *lp++; dp--; lp--; 
	if (*lp == '\r' || *lp == '\n') 
		*dp = '\0';	// remove the line feed at the end of the line
	else
	{
		dp++; *dp = '\0';
	}
	fclose(wstream);

	// count the number of files in the indicated directory (and its subdirrectory)

	ntcifiles = ntcidirs = 0;
	count_files_in_a_directory(sdirname);
	fwprintf(logstream, L"Number of WEBPAT3 data directories = %d, files = %d\n", ntcidirs+1, ntcifiles);
	fwprintf(logstream, L"Reading multiple WEBPAT3 files....\n");

	// open and copy the WEBPAT3 data file one by one (to the directory of the pointing file)
	lp = ifname; dp = tname;
	while (*lp != '\0') *dp++ = *lp++;
	while (*dp != '.') dp--;
	*dp = '\0';
	swprintf(dstname, L"%s-M%d-%d.csv", tname, ntcidirs+1, ntcifiles);
	wcscpy(ofname, dstname);
	if (_wfopen_s(&dstream, dstname, L"wt, ccs=UTF-8") != 0)	
		return MSG_WOSFILE_NOTFOUND;

	ntcifiles = ntcidirs = 0; npapers = 0;
	assemble_files_in_a_directory_WEBPAT3(sdirname, dstream);

	fwprintf(logstream, L"Total number of WEBPAT3 patents = %d\n", npapers); fflush(logstream);
	fclose(dstream);
	free(line);

	return 0;
}

