// 
// text2Pajek.cpp
//
// Author: John Liu
// 
// This program does following actions:
// 1. read a given text files: specify the relationship
// 2. prepare a corresponding Pajek file
//
// Revision History:
// 2010/04/11 1.00	Basic function works
// 2010/04/18 1.01	Modification  : changed sscanf() to sscanf_s(), etc.
//                  Added function: added command line option 'D', 'N'
//                  Added function: added codes to automatically detect whether the weight is given
// 2010/04/22 1.02	Modificatoin  : changed the name of the program to "ToPajek"
//					Added function: added new command line option "2" for 2-mode network
// 2010/04/30 1.03	Added function: added Windows GUI with the help of ªL¨Ø§±
// 2010/05/05 1.04	Modification  : polish the Windows GUI
// 2010/05/06 1.05	Added function: added function parse_line(), it replace sscanf_s()
// 2010/05/07 1.06	Fixed problems: changed some sprinf_s() to swprintf_s() 
// 2010/05/12 1.07	Fixed problems: fixed a problem in creating 2-mode networks,
//					                added function comapre_type()
// 2010/05/14 1.08	Added function: added acyclic_check.cpp, which include all related acyclic check functions
//					                added conditional control "ACYCLIC_CHECK"
// 2010/05/18 1.09	Fixed problems: fixed the output file name problem in the file :Form1.h"
// 2010/06/14 1.10	Modification  : changed most (if not all) string operations to handle wide characters
// .......
// 2010/12/24 1.16  Modification  : added "id_or_name" argument to the text2pajek() function
//                                  write out a new "... ISI_ID.paj" file
// 2011/12/11		Added function: added code to check if the system time has been moved back
// 2011/12/11		Modification  : modified to use full path name when accessing all files
// 2012/03/10		Added function: added to call read_PGUIDER() function and handle PGUIDER_DATA type of full records
// 2012/04/25		Added function: added codes to handle WESTLAW data
// 2012/05/02		Modification  : fixed a problem in handling the given Westlaw data directory name
// 2012/05/05		Modification  : changed to not displaying Westlaw options in the log file when the citation file is provided.
// 2012/05/19		Modification  : added codes to handle the depth (the number of *s) for negative cases
// 2012/06/10		Added function: added to call create_assignee_network() to create citation network among assignees
// 2012/10/12		Added function: added a function eliminate_duplicate_citations( ) to eliminate duplication citation lines. 
//                                  The source of duplication can come from WOS, or come from mistakes in parsing CR.
// 2012/12/09		Added function: added codes to count the number of self-citations
// 2013/01/25		Added function: added to show the earliest patent on the main path, and patents earlier than this patent (top 20 cited)
// 2013/01/28		Modification  : changed "atable" from global variable to local variable
// 2013/02/04		Modification  : changed C_LEN to 150, because for Westlaw data there are some long case names (due to parsing error)
// 2013/02/28		Added function: added codes to handle WUHAN_DATA, a new patent data format used in data from ªZº~¤j¾Ç¡@¤ý¿P¬Â
// 2013/03/17		Fixed problems: in text2Pajek() => wcscpy_s(..., n2p), changed the 3rd argument from n1p to n2p, that was a big typo error
// 2013/05/03		Added function: added codes to accept "decay factor" passed from the user interface
// 2014/04/07		Added function: added codes to do clustering (Newman's edge-betweenness method). this is done by calling another program "GroupFinder"
// 2014/04/11		Added function: added codes to handle a new variable "groupfinderoptions"
// 2014/04/23		Added function: added codes to handle Scopus data
// 2014/04/29		Modification  : changed to call _wsystem() (rather than system()) in the function execute_GroupFinder()
// 2014/05/25		Modification  : extend the date to 2015/07/31 for student version
// 2014/06/14		Added function: added codes to write out data in the network (in the original WOS or Scopus format)
// 2014/07/14		Added function: added codes to write out data in the network for TIP (Thomson Innovation Patent data)
// 2015/07/16		Added function: added to call write_author_citations()
// 2015/07/18		Added function: added codes to display messages regarding methods for handling citation relevancy level
// 2015/10/07		Modification  : moved all codes related to clustering the citation network to the file "clustering.cpp"
// 2016/01/19		Added function: added codes related to TAIWAN_TD_DATA, mostly open file in UTF-8 mode
// 2016/02/01		Modification  : added codes to check if there is byte-order-mark, this is necessary after the modification of openning file in UTF-8 type
//

// NOTE: remember to edit the version information in the "Form1.h" 

//#define IGNORE_SELF_CITATION

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "network.h"
#include <windows.h>

#define TRUE 1
#define FALSE 0
#define SOURCEBUF_SIZE 20

struct ATABLE
{
	wchar_t truename[MAX_NODE_NAME];
	wchar_t alias[MAX_NODE_NAME];
};

//
// function types
//
int text2Pajek(wchar_t *, wchar_t *, struct ATABLE *, int, int *, int *, int, int, int, int, int, int, int, int);
int compare_tname(const void *, const void *);
int compare_id(const void *, const void *);
int compare_aliasname(const void *, const void *);
int compare_type(const void *, const void *);
int compare_citation(const void *, const void *);
int compare_cind(const void *, const void *);
int compare_patid(const void *, const void *);
int compare_pattc(const void *n1, const void *n2);
int tname_search(struct ATABLE *, int, wchar_t *);
int name_search(struct NODE *, int, wchar_t *);
int parse_line(wchar_t *, int, wchar_t *, wchar_t *, double *);
int establish_links(struct ATABLE *, int, int, int, int, int, int);
int eliminate_duplicate_citations(wchar_t *);
void disable_assertion_message_window( );
int find_earlier_patents(wchar_t *, int);
int mainpaths_with_partition(wchar_t *);
int parse_vertices(wchar_t *, wchar_t *);

extern int link_nodes(int, int, double, int, int);
extern int create_network(int);
extern int mainpaths(wchar_t *, wchar_t *, wchar_t *, wchar_t *, int, int, double, double, int, double, double, int, int, int, int, double, wchar_t *, wchar_t *, int, struct ATABLE *, wchar_t *, wchar_t *, int, int);
extern int link_WOS_nw();
extern int create_alias_table_file(wchar_t *);
extern int create_new_relationship_list(wchar_t *, wchar_t *, int, int, int);
extern int self_citation_check(wchar_t *, wchar_t *);

extern int find_shortest_paths(int nn, double *cmat, int ns, int *st, int ne, int *end, int npaths);
extern int WOS_or_Others(wchar_t *);
extern int read_WESTLAW(wchar_t *, wchar_t *, CITE_OPTIONS *);
extern int read_WOS(wchar_t *);
extern int read_Scopus(wchar_t *);
extern int read_USPTO(wchar_t *);
extern int read_TaiwanTD(wchar_t *);
extern int strip_name_extension(wchar_t *, wchar_t *, wchar_t *);
extern int read_TIP(wchar_t *);
extern int read_PGUIDER(wchar_t *);
extern int read_WUHAN(wchar_t *);
extern int write_last_usage_time(string);
extern int read_last_usage_time(char *);
extern int create_assignee_network();
extern int max_cliques(int, int, struct PN*);
extern int Jmemory();
extern int replace_alias_case_id(wchar_t *);
extern int test_eI();
extern int readwrite_WOS(wchar_t *, wchar_t *, int, int);
extern int readwrite_Scopus(wchar_t *, wchar_t *, int, int);
extern int readwrite_TIP(wchar_t *, wchar_t *, int, int);
extern int readwrite_USPTO(wchar_t *, wchar_t *, int, int);
extern int prepare_coword_data(wchar_t *, int, struct TKEYWORDS *);	// added 2015/06/11
extern int find_significant_country();	

//
// global variables
//
int nnodes = 1;		// number of nodes
int nsn = 0;	// number of source nodes
struct NODE *nodes = NULL;
int nalias;
//struct ATABLE *atable;
int full_record_type;
int no_wos_citation_file;

FILE *ostream;		// for Pajek file
FILE *istream;		// for Input file
FILE *logstream;
int weight_given = FALSE;
int release_id;

#ifdef MDEBUG
FILE *mdebugstream;
#endif MDEBUG

extern int nnodes;
extern struct PN *nw;
extern int nwos;
extern struct WOS *wos;
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int ntkeywords;

//
// all real work start here after receiving input options
//
int main_function(wchar_t *iname, wchar_t *oname, wchar_t *frecordname, wchar_t *branchname, int separator, int groupfinderoptions, 
				  int clusteringcoauthornetworkoptions,
				  int mode3, int mode2, int mode, int spmethod, int sconly, int mptype, 
				  double mtie_tolerance, double mnpths, int bptype, double btie_tolerance, 
				  double bnpths, int byear, int eyear, 
				  int n_top_edges_local, int n_top_edges_global, double decay, wchar_t *clanfilename,
				  int Pexamined, int Pdiscussed, int Pcited, int Pmentioned, 
				  int Neg4star, int Neg3star, int Neg2star, int Neg1star, int take_weight, wchar_t *sversion)
{
	int i, ret, ret2;	
	int nlines;
	wchar_t niname[FNAME_SIZE], ninm[FNAME_SIZE], ptcname[FNAME_SIZE], tname[FNAME_SIZE], woscname[FNAME_SIZE];
	wchar_t dpath[FNAME_SIZE];
	wchar_t line[LBUF_SIZE+1], *lp, *tline;
	wchar_t dname[LBUF_SIZE+1], *dp;
	char tmstring[BLOCK_SIZE];
	char last_usage_time[BLOCK_SIZE];
	FILE *astream;	// for alias file
	SYSTEMTIME st;
	struct ATABLE *atable;

#ifdef MDEBUG
	Jmemory();	// allocate a big chunk of memory to trace memory allocations
#endif MDEBUG

	release_id = RELEASE_ID;	// general or DELTA release

	disable_assertion_message_window( );	// added 2011/06/11

	GetSystemTime(&st);
	sprintf(tmstring, "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);

	int first_usage = FALSE;
	read_last_usage_time(last_usage_time);
	for (i = 0; i < 8; i++)		// if it is used the first time, assume at least one of the entry will be non-digit
	{
		if (!isdigit(last_usage_time[i]))
			first_usage = TRUE;
	}

	if (!first_usage)	// want to know if someone have moved back the system time
	{
		int prevtime, curtime;
		read_last_usage_time(last_usage_time);
		prevtime = atoi(last_usage_time);
		curtime  = atoi(tmstring);
		if (curtime < prevtime)	// not reasonable
			exit(0);
	}

#ifdef PROFESSIONAL_TRIAL_VERSION
	// a weak protection for the software
	// valid until 2015/08
	if (st.wYear > 2015 || (st.wYear == 2015 && st.wMonth > 8))
		return MSG_SOFTWARE_EXPIRED;
		//exit(0);
#endif PROFESSIONAL_TRIAL_VERSION

#ifdef STUDENT_VERSION
	// a weak protection for the software
	// valid until 2016/07/31
	if (st.wYear > 2016 || (st.wYear == 2016 && st.wMonth > 8))
		return MSG_SOFTWARE_EXPIRED;
		//exit(0);
#endif STUDENT_VERSION

#ifdef LIGHT_VERSION
	// a weak protection for the software
	// valid until 2015/08
	if (st.wYear > 2015 || (st.wYear == 2015 && st.wMonth > 8))
		return MSG_SOFTWARE_EXPIRED;
		//exit(0);
#endif LIGHT_VERSION

#ifdef LAW_VERSION
	// a weak protection for the software
	// valid until 2015/08
	if (st.wYear > 2015 || (st.wYear == 2015 && st.wMonth > 8))
		return MSG_SOFTWARE_EXPIRED;
		//exit(0);
#endif LAW_VERSION

	write_last_usage_time(tmstring);	// keep it, to check when it is used the next time

	strip_name_extension(frecordname, tname, dpath);
	wchar_t logfname[FNAME_SIZE];
	swprintf(logfname, L"%sMainPath log.txt", dpath);
	_wfopen_s(&logstream, logfname, L"wt, ccs=UTF-8");	// modified 2016/01/18
#ifdef MDEBUG
	wchar_t mdebugfname[FNAME_SIZE];
	swprintf(mdebugfname, L"%sMemory log.txt", dpath);
	_wfopen_s(&mdebugstream, mdebugfname, L"w");
#endif MDEBUG

	GetLocalTime(&st);
	fwprintf(logstream, L"Execution start time: %d/%d/%d %d:%d:%d\n\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	fwprintf(logstream, L"MainPath version : %s\n", sversion);
	fwprintf(logstream, L"Full record file : %s\n", frecordname);
	fwprintf(logstream, L"Citation file    : %s\n", iname);
	fwprintf(logstream, L"Branch spec. file: %s\n", branchname); 
	fwprintf(logstream, L"Node scope: %d (0=SOURCE_COLUMN_NODES_ONLY, 1=NODES_IN_WOS_DATA, 2=ALL_NODES)\n", sconly);
	fwprintf(logstream, L"Traversal count algorithm: %d (0=SPC, 1=SPLC, 2=SPNP, 3=SPAD, 4=SPGD, 5=SPHD)\n", spmethod);
	if (spmethod == S_SPGD || spmethod == S_SPAD)
		fwprintf(logstream, L"    Decay = %.4f\n", decay);

//#######
	//test_eI();
	//exit(0);
//#######

	full_record_type = WOS_or_Others(frecordname);
#ifdef LAW_VERSION
	if (full_record_type !=  WESTLAW_DATA)
		return MSG_FILE_FORMAT_ERROR;
#endif LAW_VERSION
	if (full_record_type == MSG_WOSFILE_NOTFOUND) return MSG_WOSFILE_NOTFOUND;
	else if (full_record_type == WOS_DATA)
	{	
		no_wos_citation_file = 0;
		if (iname[0] == L'\0') 
		{
#ifdef PROFESSIONAL_TRIAL_VERSION
			return MSG_FILE_CITATION_FILE_NOT_PROVIDED;
#endif
#ifdef PROFESSIONAL_VERSION
			return MSG_FILE_CITATION_FILE_NOT_PROVIDED;
#endif
			no_wos_citation_file = 1;
			System::String^ mymsg = "WOS citation file is not provided.\nWill use CR field to generate citation relationships.";
			System::Windows::Forms::MessageBox::Show(mymsg);
			fwprintf(logstream, L"\nWOS citation file is not provided.\nWILL use CR field to generate citation relationships.\n\n");
		}
		ret = read_WOS(frecordname); // read WOS data file and create the wos[] arrary
		if (ret != 0) return ret;
		if (no_wos_citation_file) 	// make the citation file name
		{ ret = swprintf_s(woscname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG; }
		else
			wcscpy(woscname, oname);
	}
	else if (full_record_type == SCOPUS_DATA)	// this section is added 2014/04/23
	{
		no_wos_citation_file = 0;
		if (iname[0] == L'\0') 
			no_wos_citation_file = 1;
		ret = read_Scopus(frecordname); // read Scopus data file and create the wos[] arrary
		if (ret != 0) return ret;
		ret = swprintf_s(woscname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	}
	else if (full_record_type == USPTO_DATA)
	{	
		ret = read_USPTO(frecordname); // read USPTO data file and create the uspto[] and the wos[] arrary
		if (ret != 0) return ret;
		// make the citation file name
		ret = swprintf_s(ptcname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	}
	else if (full_record_type == TAIWAN_TD_DATA)
	{	
		ret = read_TaiwanTD(frecordname); 
		if (ret != 0) return ret;
		// make the citation file name
		ret = swprintf_s(ptcname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	}
	else if (full_record_type == THOMSON_INNOVATION_DATA)
	{	
		ret = read_TIP(frecordname); // read Thomson Innovation patent data file and create the uspto[] and the wos[] arrary
		if (ret != 0) return ret;
		// make the citation file name
		ret = swprintf_s(ptcname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	}
	else if (full_record_type == PGUIDER_DATA)
	{	
		ret = read_PGUIDER(frecordname); // read PGUIDER data file and create the uspto[] and the wos[] arrary
		if (ret != 0) return ret;
		// make the citation file name
		ret = swprintf_s(ptcname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	}
	else if (full_record_type == WUHAN_DATA)
	{	
		ret = read_WUHAN(frecordname); // read WUHAN data file and create the uspto[] and the wos[] arrary
		if (ret != 0) return ret;
		// make the citation file name
		ret = swprintf_s(ptcname, FNAME_SIZE, L"%s citation.txt", tname); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	}
	else if (full_record_type == WESTLAW_DATA)	// Westlaw data
	{	
		if (iname[0] == L'\0') // citation file is not provided
		{
			no_wos_citation_file = 1;
#ifdef OBSOLETE
			System::String^ mymsg = "Citation file is not provided.\nWill use KEYCITE information to generate citation relationships.";
			System::Windows::Forms::MessageBox::Show(mymsg);
#endif OBSOLETE
			fwprintf(logstream, L"\nCitation file is not provided.\nUse KEYCITE information to generate citation relationships.\n\n");
		}
		FILE *wstream;		// for source file
		if (_wfopen_s(&wstream, frecordname, L"rt") != 0)	
			return MSG_WOSFILE_NOTFOUND;
		fgetws(line, LBUF_SIZE, wstream);	// ignore the first line, it has been checked in the previous code
		if(fgetws(line, LBUF_SIZE, wstream) == NULL)	// the 2nd line should have the name of the directory that contains the Westlaw data
			return UNKNOWN_DATA;
		if (line[0] == '\n' || line[0] == '\r')
			return UNKNOWN_DATA;
		lp = line; dp = dname;
		while (*lp != '\0') *dp++ = *lp++; dp--; lp--; 
		if (*lp == '\r' || *lp == '\n') 
			*dp = '\0';	// remove the line feed at the end of the line
		else
		{
			dp++; *dp = '\0';
		}
		// read the options
		struct CITE_OPTIONS WL_options;
#ifdef XXX
		WL_options.include_negative_cases = 0;
		WL_options.include_examined = 0;
		WL_options.include_discussed= 0;
		WL_options.include_cited = 0;
		WL_options.include_mentioned = 0;
		while (TRUE)
		{
			if(fgetws(line, LBUF_SIZE, wstream) == NULL)	// starting from the 3rd line are options
				break;
			if (line[0] == '\n' || line[0] == '\r')	continue;	// skip empty lines
			if (wcsncmp(line, L"INCLUDE ", 8) == 0)
			{
				if (wcsncmp(&line[8], L"Negative Cases", 14) == 0)
					WL_options.include_negative_cases = 1;
				else if (wcsncmp(&line[8], L"Examined", 8) == 0)
					WL_options.include_examined = 1;
				else if (wcsncmp(&line[8], L"Discussed", 9) == 0)
					WL_options.include_discussed = 1;
				else if (wcsncmp(&line[8], L"Cited", 5) == 0)
					WL_options.include_cited = 1;
				else if (wcsncmp(&line[8], L"Mentioned", 9) == 0)
					WL_options.include_mentioned = 1;
			}
			else
				return UNKNOWN_DATA;
		}
#endif XXX
		fclose(wstream);	 
		WL_options.include_examined = Pexamined; 
		WL_options.include_discussed = Pdiscussed;
		WL_options.include_cited = Pcited;
		WL_options.include_mentioned = Pmentioned;
		WL_options.include_4star = Neg4star;
		WL_options.include_3star = Neg3star;
		WL_options.include_2star = Neg2star;
		WL_options.include_1star = Neg1star;
		if (no_wos_citation_file) 	// if citation file is not given
			fwprintf(logstream, L"WESTLAW Options:\nExamined: %d, Discussed: %d, Cited: %d, Mentioned: %d, Negative 4star: %d, Negative 3star: %d, Negative 2star: %d, Negative 1star: %d\n", 
				WL_options.include_examined, WL_options.include_discussed, WL_options.include_cited, WL_options.include_mentioned, 
				WL_options.include_4star, WL_options.include_3star, WL_options.include_2star, WL_options.include_1star);
		if (take_weight)
		{
			fwprintf(logstream, L"Take citation level into consideration: Yes\n");
			fwprintf(logstream, L"Method of handling citation level: ");
			if (take_weight == FLAT_IGNORE_SIGN)	fwprintf(logstream, L"FLAT_IGNORE_SIGN\n");
			else if (take_weight == LINEAR_IGNORE_SIGN)	fwprintf(logstream, L"LINEAR_IGNORE_SIGN\n");
			else if (take_weight == EXPONENTIAL_IGNORE_SIGN)	fwprintf(logstream, L"EXPONENTIAL_IGNORE_SIGN\n");
			else if (take_weight == FLAT_TAKE_SIGN)	fwprintf(logstream, L"FLAT_TAKE_SIGN\n");
			else if (take_weight == LINEAR_TAKE_SIGN)	fwprintf(logstream, L"LINEAR_TAKE_SIGN\n");
			else if (take_weight == EXPONENTIAL_TAKE_SIGN)	fwprintf(logstream, L"EXPONENTIAL_TAKE_SIGN\n");
			fwprintf(logstream, L"\n");
		}
		else
			fwprintf(logstream, L"Take citation level into consideration: No\n\n");
		ret = read_WESTLAW(dname, tname, &WL_options); // read WESTLAW data files from the given directory and create the westlaw[] and the wos[] arrary
		if (ret != 0) return ret;
		// make the citation file name if the file is not given
		if (no_wos_citation_file) 	// make the citation file name
		{	
			ret = swprintf_s(ptcname, FNAME_SIZE, L"%s citation.txt", tname); // 2013/02/19, changed from "dname" to "tname"
			if (ret == -1) return MSG_FILE_NAME_TOO_LONG; 
			replace_alias_case_id(ptcname);
		}
		else
		{
			swprintf_s(ptcname, FNAME_SIZE, L"%s.txt", oname);	// with .txt extension
			wcscpy(woscname, oname);	// no .txt extension
		}
	}
	else
		return MSG_WOSFILE_FORMAT_ERROR;

//#ifdef STUDENT_VERSION	// this limitation is removed on 2012/08/17
//	if (nwos > 5000)	// limit to 5,000 patents or papers for the student version
//		return MSG_EXCEEDING_LIMIT;
//#endif STUDENT_VERSION

	// if year range is given, 
	// create a new relationship-list file that cut out the data before and after the given year
	ret = swprintf_s(ninm, FNAME_SIZE, L"%sTMP%d-%d", dpath, byear, eyear); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	ret = swprintf_s(niname, FNAME_SIZE, L"%s.txt", ninm); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA || 
		full_record_type == PGUIDER_DATA || full_record_type == WUHAN_DATA || full_record_type == TAIWAN_TD_DATA)	// citation files for these data are created automatically, not given
		ret = create_new_relationship_list(ptcname, niname, separator, byear, eyear);
	else if (full_record_type == WESTLAW_DATA)
	{
		if (no_wos_citation_file)
			ret = create_new_relationship_list(ptcname, niname, separator, byear, eyear);
		else
			ret = create_new_relationship_list(ptcname, niname, separator, byear, eyear);
	}
	else if (no_wos_citation_file)
		ret = create_new_relationship_list(woscname, niname, separator, byear, eyear);
	else
		ret = create_new_relationship_list(iname, niname, separator, byear, eyear);
	if (ret != 0) return ret;

	// elimate the duplicate citations, added 2012/10/12
	ret = eliminate_duplicate_citations(niname);
	if (ret != 0) return ret;

	// create the "AliasTable.txt" file according to the wos[] array
	create_alias_table_file(dpath);

	atable = NULL;
	wchar_t atablefname[FNAME_SIZE];
	swprintf(atablefname, L"%sAliasTable.txt", dpath);
	if (_wfopen_s(&astream, atablefname, L"rt, ccs=UTF-8") == 0)	// alias table is provided, modified for UTF 2016/01/19
	{
		nlines = 0;
		while (TRUE)
		{		
			if(fgetws(line, LBUF_SIZE, astream) == NULL) break;
			if (line[0] == '\n' || line[0] == '\r') continue;
			nlines++;	
		}		
		atable = (struct ATABLE *)malloc(nlines * sizeof(struct ATABLE));
		if (atable == NULL) return MSG_NOT_ENOUGH_MEMORY;
		rewind(astream);	// point back to the begining of the file
		nlines = 0;
		while (TRUE)
		{		
			if(fgetws(line, LBUF_SIZE, astream) == NULL)
				break;
			if (line[0] == '\n' || line[0] == '\r')
				continue;	
			tline = line;
			if (nlines == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
			{
				if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
					tline = &line[1];	// skip the BOM
			}
			swscanf_s(tline, L"%s %s", atable[nlines].truename, MAX_NODE_NAME-1, atable[nlines].alias, MAX_NODE_NAME-1);
			nlines++;	
		}
	}
	nalias = nlines;
	qsort((void *)atable, (size_t)nlines, sizeof(struct ATABLE), compare_tname);

	// write two Pajek files, one in alias name, one in ISI ID	
	wchar_t oname2[FNAME_SIZE];
	if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA || 
		full_record_type == PGUIDER_DATA || full_record_type == WUHAN_DATA || full_record_type == TAIWAN_TD_DATA)	// citation files for these data are created automatically, not given
		ret = swprintf_s(oname2, FNAME_SIZE, L"%s citation.paj", tname);
	else if (full_record_type == WESTLAW_DATA)
	{
		if (no_wos_citation_file) 	
			ret = swprintf_s(oname2, FNAME_SIZE, L"%s citation.paj", tname);
		else		
			ret = swprintf_s(oname2, FNAME_SIZE, L"%s.paj", woscname);
	}
	else
		ret = swprintf_s(oname2, FNAME_SIZE, L"%s.paj", woscname);
	if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	wchar_t gfname[FNAME_SIZE];
	wcscpy(gfname, oname2);	// prepare the name to pass to GroupFinder
	ret = text2Pajek(ninm, oname2, atable, nalias, &nnodes, &nsn, separator, mode3, mode2, MFORWARD, spmethod, sconly, FALSE, WRITE_NODE_NAME);
	if (ret != MSG_FILE_CREATED) { fclose(logstream); return ret; }
	if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA || 
		full_record_type == PGUIDER_DATA || full_record_type == WUHAN_DATA || full_record_type == TAIWAN_TD_DATA)	// citation files for these data are created automatically, not given
		ret = swprintf_s(oname2, FNAME_SIZE, L"%s citation ID.paj", tname);
	else if (full_record_type == WESTLAW_DATA)
	{
		if (no_wos_citation_file) 	
			ret = swprintf_s(oname2, FNAME_SIZE, L"%s citation ID.paj", tname);
		else		
			ret = swprintf_s(oname2, FNAME_SIZE, L"%s ID.paj", woscname);
	}
	else
		ret = swprintf_s(oname2, FNAME_SIZE, L"%s ID.paj", woscname);
	if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	ret = text2Pajek(ninm, oname2, atable, nalias, &nnodes, &nsn, separator, mode3, mode2, MFORWARD, spmethod, sconly, FALSE, WRITE_NODE_ID);
	if (ret != MSG_FILE_CREATED) { fclose(logstream); return ret; }

	ret = establish_links(atable, nlines, separator, mode3, mode2, MFORWARD, sconly);	
	if (ret != 0) return ret;
	link_WOS_nw();	// establish the link between the two array wos[] and nw[]

	// write out data in the network (in the original WOS or Scopus format), added 2014/06/14
	wchar_t tmpname[FNAME_SIZE];
	if (full_record_type == WOS_DATA)
	{
		swprintf(tmpname, L"%s N%d.txt", tname, nnodes);
		readwrite_WOS(frecordname, tmpname, RW_PARTITION, 99999);	// 99999 indicates write all nodes
	}
	else if (full_record_type == SCOPUS_DATA)
	{
		swprintf(tmpname, L"%s N%d.csv", tname, nnodes);
		readwrite_Scopus(frecordname, tmpname, RW_PARTITION, 99999);
	}
	else if (full_record_type == THOMSON_INNOVATION_DATA)
	{
		swprintf(tmpname, L"%s N%d.csv", tname, nnodes);
		readwrite_TIP(frecordname, tmpname, RW_PARTITION, 99999);
	}
	else if (full_record_type == USPTO_DATA)
	{
		swprintf(tmpname, L"%s N%d.txt", tname, nnodes);
		readwrite_USPTO(frecordname, tmpname, RW_PARTITION, 99999);
	}

#ifdef DEA_APPLICATIONS	// output (partial) node degree information for DEA Applications
	int i, j, k, year, cnt1, cnt2, cnt3;
	fwprintf(logstream, L"\nNODES\tCITED_TOTAL\t1978-1990\t1991-2000\t2001-2010\n");
	for (i = 0; i < nnodes; i++)
	{
		if (wos[nw[i].ndx2wos].app_type == 0)	// for a purely theoretical DEA papers
		{
			cnt1 = cnt2 = cnt3 = 0;
			for (k = 0; k < nw[i].out_deg; k++)
			{
				j = nw[i].out_nbrs[k];
				year = wos[j].year;
				if (wos[j].app_type == 1)	// if its cited by an application paper
				{
					if (year <= 1990) cnt1++;	// 1978-1990
					else if (year >= 1991 && year <= 2000) cnt2++;	// 1991-2000
					else if (year >= 2001 && year <= 2010) cnt3++;	// 2001-2010
				}
			}
			fwprintf(logstream, L"%s\t%d\t%d\t%d\t%d\n", nw[i].alias, nw[i].out_deg, cnt1, cnt2, cnt3);
		}
	}
#endif DEA_APPLICATIONS

	Jfree(nodes, L"main_function: nodes");	// do not need this block of memory anymore, 2011/06/25
	nodes = NULL;	// added 2011/07/01

	if (full_record_type == USPTO_DATA) 
		create_assignee_network();	// create citation network among assignees, NOTE: providing "Assignee alias.txt" will help

//============

//	max_cliques(59, nnodes, nw);
//	exit(0);

//============

	//write_author_citations(tname, 0);	// added 2015/06/23, output author citation network that contains all authors
	//write_author_citations(tname, 1);	// added 2015/07/16, output author citation network that contains only authors that have been the 1st author 
	
	find_significant_country();	// moved to here 2015/10/18
	prepare_coword_data(tname, ntkeywords, tkeyword);	// added 2015/06/11

	wchar_t rpajekname[FNAME_SIZE];
	ret2 = mainpaths(frecordname, tname, branchname, gfname, spmethod, mptype, mtie_tolerance, mnpths, 
		bptype, btie_tolerance, bnpths, byear, eyear, 
		n_top_edges_local, n_top_edges_global, decay, clanfilename, dpath, take_weight, atable, rpajekname, gfname, 
		groupfinderoptions, clusteringcoauthornetworkoptions);
	fflush(logstream);

	// create a Pajek main path file with paritions determined by the GroupFinder program
	if (groupfinderoptions != GF_DISABLE)
		mainpaths_with_partition(rpajekname);	// added 2014/04/07

	// show patents (in the citation file) earlier than the earliest patent of the main paths, this function is requested by Louis
	// added 2013/01/23
	if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA || full_record_type == PGUIDER_DATA || full_record_type == WUHAN_DATA)
		find_earlier_patents(ptcname, separator);	// added 2003/01/25

	GetLocalTime(&st);
	fwprintf(logstream, L"\nExecution end time: %d/%d/%d %d:%d:%d\n\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	if (ret2 != 0)
	{
		fclose(logstream);
		_wunlink(niname);
		if (ret2 == -1)
			return MSG_ACYCLIC;
		else 
			return MSG_MEMORY;
	}

	_wunlink(niname);

#ifdef MDEBUG
	Jheap_check();
#endif MDEBUG

	return MSG_FILE_CREATED;
}

//
// turn a relationship-list into a Pajek file
//
int text2Pajek(wchar_t *iname, wchar_t *oname, struct ATABLE *atable, int nalias, int *nnodes, int *nsn, int separator, int mode3, int mode2, int mode, int spmethod, int sconly, int make_atable, int id_or_name)
{
	wchar_t line[LBUF_SIZE+1], *tline;
	wchar_t wline[LBUF_SIZE+1];
	wchar_t inpname[FNAME_SIZE];
	wchar_t mstr1[LBUF_SIZE+1], mstr3[LBUF_SIZE+1];
	int i;
	int nlines, cnt, ndx1, ndx2, nx1, nx2;
	int nnd, nsnd;
	int self_citation_count;

	self_citation_count = 0;
// 
// Open the source file (will fail if the file does not exist)
//	
	swprintf_s(inpname, FNAME_SIZE, L"%s.txt", iname);
	if (_wfopen_s(&istream, inpname, L"rt, ccs=UTF-8") != 0)	// modified 2016/01/19
		return MSG_IFILE_NOTFOUND;

	// 1st pass, count the toal number of input lines
	double weight;
	int wcnt;
	wchar_t name1[MAX_NODE_NAME+1], name2[MAX_NODE_NAME+1];
	wchar_t *n1p, *n2p;
	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nlines++;	
	}
	// allocate memory for the node array (with duplications)
	if (nodes == NULL)	// this is to avoid allocating the memory for "nodes" twice, 2011/03/10
		nodes = (struct NODE *)Jmalloc(nlines * 4 * sizeof(struct NODE), L"text2Pajek: nodes");
	if (nodes == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nlines*4; i++) { nodes[i].indx = i; nodes[i].type = NSOURCE; nodes[i].mainp = FALSE; nodes[i].special = FALSE; nodes[i].branch = FALSE; }	// initialize the strcuture

	// 2nd pass, find the total number of unique nodes
	cnt = 0; wcnt = 0; nlines = 0;
	rewind(istream);	// point back to the begining of the file
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		tline = line;
		if (nlines == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				tline = &line[1];	// skip the BOM
		}
		nlines++;
		weight = 0.0;
		parse_line(tline, separator, name1, name2, &weight);
		n1p = name1; n2p = name2;
		while (*n1p == '@' || *n1p == '*' || *n1p == '|') n1p++;	// ignore the leading specifier
		while (*n2p == '@' || *n2p == '*' || *n2p == '|') n2p++;
		nx1 = tname_search(atable, nalias, n1p);
		if (nx1 < 0)
		{
			if (sconly == NODES_IN_WOS_DATA)
				continue;
			wcscpy_s(nodes[cnt].id, MAX_NODE_NAME-1, n1p);	// added 2011/06/01
			wcscpy_s(nodes[cnt].name, MAX_NODE_NAME-1, n1p);
		}
		else
		{
			wcscpy_s(nodes[cnt].name, MAX_NODE_NAME-1, atable[nx1].alias); 
			wcscpy_s(nodes[cnt].id, MAX_NODE_NAME-1, atable[nx1].truename); 
			if (name1[0] == '*' || name1[1] == '*' || name1[2] == '*') nodes[cnt].mainp = TRUE;
			if (name1[0] == '@' || name1[1] == '@' || name1[2] == '@') nodes[cnt].special = TRUE;
			if (name1[0] == '|' || name1[1] == '|' || name1[2] == '|') nodes[cnt].branch = TRUE;
		}
		nodes[cnt].type = NSOURCE;
		cnt++;
		nx2 = tname_search(atable, nalias, n2p);
		if (nx2 < 0)
		{
			if (sconly == NODES_IN_WOS_DATA)
				continue;
			wcscpy_s(nodes[cnt].id, MAX_NODE_NAME-1, n2p);	// added 2011/06/01
			wcscpy_s(nodes[cnt].name, MAX_NODE_NAME-1, n2p);	// change the 3rd argument from n1p to n2p, that was a big typo error, 2013/03/17
		}
		else
		{
			wcscpy_s(nodes[cnt].name, MAX_NODE_NAME-1, atable[nx2].alias); 
			wcscpy_s(nodes[cnt].id, MAX_NODE_NAME-1, atable[nx2].truename); 
			if (name2[0] == '*' || name2[1] == '*' || name2[2] == '*') nodes[cnt].mainp = TRUE;
			if (name2[0] == '@' || name2[1] == '@' || name2[2] == '@') nodes[cnt].special = TRUE;
			if (name2[0] == '|' || name2[1] == '|' || name2[2] == '|') nodes[cnt].branch = TRUE;
		}
		nodes[cnt].type = NTARGET;
		cnt++;	
		if (weight == 0.0)
			wcnt++;
	}

	if (wcnt != cnt/2)
		weight_given = TRUE;

	qsort((void *)nodes, (size_t)cnt, sizeof(struct NODE), compare_id);

	// consolidate duplicate names
	wchar_t prev_name[MAX_NODE_NAME+1];
	prev_name[0] = '\0';
	nnd = 0; 
	for (i = 0; i < cnt; i++)
	{
		if (wcscmp(nodes[i].id, prev_name) != 0)
		{
			nodes[nnd] = nodes[i];
			nnd++;
			wcscpy_s(prev_name, MAX_NODE_NAME, nodes[i].id);
		}
		else	// overwrite the type
		{
			//if (nodes[i].type == NSOURCE)
			if (nodes[i].type != nodes[i-1].type)	// is both a source and a target
				nodes[nnd-1].type = NINTERMEDIATE;
		}
	}

	nsnd = 0;	// the number of the source node (1st column)
	for (i = 0; i < nnd; i++)
	{
		if (nodes[i].type == NSOURCE)
			nsnd++;
	}

	// handle the special order of names for 2-mode networks
	if (mode3 == M2_MODE || sconly == SOURCE_COLUMN_NODES_ONLY)
	{
		// sort by the type
		qsort((void *)nodes, (size_t)nnd, sizeof(struct NODE), compare_type);
		// sort the beginning source nodes
		qsort((void *)nodes, (size_t)nsnd, sizeof(struct NODE), compare_id);
		// sort the remaining nodes
		qsort((void *)&nodes[nsnd], (size_t)(nnd-nsnd), sizeof(struct NODE), compare_id);
	}

	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/19 
		return MSG_OFILE_CANNOTOPEN;

	// write out the "*Network" line
	if (mode3 == M2_MODE) 
		swprintf_s(mstr1, LBUF_SIZE, L"2-Mode");
	else
	{
		swprintf_s(mstr3, LBUF_SIZE, L"1-Mode"); 
		if (mode2 == MNONDIRECTED) 
			swprintf_s(mstr1, LBUF_SIZE, L"%s, Non-directed", mstr3); 
		else 
		{
			if (mode == MBACKWARD) 
				swprintf_s(mstr1, LBUF_SIZE, L"%s, Directed, Backward", mstr3); 
			else 
				swprintf_s(mstr1, LBUF_SIZE, L"%s, Directed, Forward", mstr3);
		}
	}

	swprintf_s(wline, LBUF_SIZE, L"*Network converted from \"%s\", Type: %s\n", inpname, mstr1);
	fwprintf(ostream, L"%s", wline); 
	// write out the "*vertice" line 
	if (mode3 == M1_MODE)
	{
		if (sconly == SOURCE_COLUMN_NODES_ONLY)		// take only the source column (1st column) nodes
			swprintf_s(line, LBUF_SIZE, L"*Vertices %d\n", nsnd);
		else
			swprintf_s(line, LBUF_SIZE, L"*Vertices %d\n", nnd);
	}
	else
		swprintf_s(line, LBUF_SIZE, L"*Vertices %d %d\n", nnd, nsnd);
	if (fputws(line, ostream) < 0) 
		return MSG_OFILE_CANNOTWRITE;
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	int n_to_write;
	wchar_t attr1[100], attr2[100];
	if (id_or_name == WRITE_NODE_NAME)
	{
		if (sconly == SOURCE_COLUMN_NODES_ONLY) n_to_write = nsnd; else n_to_write = nnd;
		for (i = 0; i < n_to_write; i++) 
		{
			if (nodes[i].special) swprintf(attr1, L"triangle"); else swprintf(attr1, L"ellipse");
			if (nodes[i].branch) swprintf(attr2, L"%s diamond", attr1); else swprintf(attr2, L"%s", attr1);
			//if (nodes[i].branch) swprintf(attr2, L"%s ic white", attr1); else swprintf(attr2, L"%s", attr1);
			fwprintf(ostream, L"%d \"%s\" %s\n", i+1, nodes[i].name, attr2); 
		} 
	}
	else
	{
		if (sconly == SOURCE_COLUMN_NODES_ONLY) n_to_write = nsnd; else n_to_write = nnd;
		for (i = 0; i < n_to_write; i++) 
		{
			if (nodes[i].special) swprintf(attr1, L"triangle"); else swprintf(attr1, L"ellipse");
			if (nodes[i].branch) swprintf(attr2, L"%s diamond", attr1); else swprintf(attr2, L"%s", attr1);
			//if (nodes[i].branch) swprintf(attr2, L"%s ic white", attr1); else swprintf(attr2, L"%s", attr1);
			fwprintf(ostream, L"%d \"%s\" %s\n", i+1, nodes[i].id, attr2); 
		} 
	}

	if (make_atable == TRUE)
	{
		// write also to alias table file
		wchar_t aliasname[FNAME_SIZE];
		FILE *aliasstream;
		swprintf_s(aliasname, FNAME_SIZE, L"%s alias table.txt", iname);
		if (_wfopen_s(&aliasstream, aliasname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/19
			return MSG_OFILE_CANNOTWRITE;
		for (i = 0; i < nnd; i++) 
			fwprintf(aliasstream, L"%s\t%s\n", nodes[i].id, nodes[i].name);
		fclose(aliasstream);
	}

	if (mode2 == MDIRECTED)
		swprintf_s(line, LBUF_SIZE, L"*Arcs\n");
	else
		swprintf_s(line, LBUF_SIZE, L"*Edges\n");
	if (fputws(line, ostream) < 0) 
		return MSG_OFILE_CANNOTWRITE;

	// 3rd pass, construct the link in Pajek format 
	nlines = 0;
	rewind(istream);	// point back to the begining of the file
	double wt; 
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		tline = line;
		if (nlines == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				tline = &line[1];	// skip the BOM
		}
		nlines++;
		weight = 0.0;
		parse_line(tline, separator, name1, name2, &weight);
		n1p = name1; n2p = name2;
		while (*n1p == '@' || *n1p == '*' || *n1p == '|') n1p++;
		while (*n2p == '@' || *n2p == '*' || *n2p == '|') n2p++;
#ifdef XXX
		nx1 = tname_search(atable, nalias, n1p);
		if (nx1 >= 0)
			wcscpy_s(n1p, MAX_NODE_NAME, atable[nx1].alias); // replace with alias if exist
		nx2 = tname_search(atable, nalias, n2p);
		if (nx2 >= 0)
			wcscpy_s(n2p, MAX_NODE_NAME, atable[nx2].alias); // replace with alias if exist
#endif XXX
		if (mode3 == M2_MODE) 
		{
			ndx1 = name_search(nodes, nsnd, n1p);
			ndx2 = nsnd + name_search(&nodes[nsnd], nnd-nsnd, n2p);
		}
		else
		{
			if (sconly == SOURCE_COLUMN_NODES_ONLY)		// "sconly" should happen only in 1-mode case
			{
				ndx2 = name_search(nodes, nsnd, n2p);
				if (ndx2 < 0 || ndx2 >= nsnd)
					continue;	// name2 is not a source column node
				ndx1 = name_search(nodes, nsnd, n1p);
			}
			else
			{
				ndx2 = name_search(nodes, nnd, n2p); if (ndx2 < 0) continue;
				ndx1 = name_search(nodes, nnd, n1p); if (ndx1 < 0) continue;
			}
		}
		if (weight_given)
		{
			if (weight == 0.0)
				wt = 1.0;
			else 
				wt = weight;
			if (mode2 == MNONDIRECTED)
				fwprintf(ostream, L"%d %d %.6f\n", ndx1+1, ndx2+1, wt);
			else 
			{				
				if (self_citation_check(n1p, n2p))
				{
					self_citation_count++;
#ifdef IGNORE_SELF_CITATION
					continue;		// ignore self-citation relationship
#endif IGNORE_SELF_CITATION
				}
				if (mode == MFORWARD)
					fwprintf(ostream, L"%d %d %.6f\n", ndx1+1, ndx2+1, wt);
				else
					fwprintf(ostream, L"%d %d %.6f\n", ndx2+1, ndx1+1, wt);
			}
		}
		else
		{
			if (mode2 == MNONDIRECTED)
				fwprintf(ostream, L"%d %d\n", ndx1+1, ndx2+1);
			else 
			{	
				if (self_citation_check(n1p, n2p))
				{
					self_citation_count++;
#ifdef IGNORE_SELF_CITATION
					continue;		// ignore self-citation relationship
#endif IGNORE_SELF_CITATION
				}
				if (mode == MFORWARD)
					fwprintf(ostream, L"%d %d\n", ndx1+1, ndx2+1);
				else
					fwprintf(ostream, L"%d %d\n", ndx2+1, ndx1+1);
			}
		}
	}

	fwprintf(logstream, L"Self-citation count (from text2Pajek) = %d\n", self_citation_count);

	if (sconly == SOURCE_COLUMN_NODES_ONLY)
		nnd = nsnd;

	fwprintf(ostream, L"*Partition created by MainPath program\n");
	fwprintf(ostream, L"*Vertices %d\n", nnd);
	for (i = 0; i < nnd; i++)
	{
		if (nodes[i].mainp == TRUE)
		{
			if (nodes[i].type == NSOURCE || nodes[i].type == NTARGET)
				fwprintf(ostream, L"%d\n", nodes[i].type);
			else
				fwprintf(ostream, L"%d\n", NMAINPATH);
		}
		else
			fwprintf(ostream, L"%d\n", nodes[i].type);
fflush(ostream);
	}
	
	// close the output file
	fclose(ostream);
	if (make_atable == TRUE)	// the case of main path and branch path
		fclose(istream);

	*nnodes = nnd; *nsn = nsnd;

	return MSG_FILE_CREATED;
}

//
// establish links in the nw[] network
// NOTE: assuming the input file is already opened
//
int establish_links(struct ATABLE *atable, int nalias, int separator, int mode3, int mode2, int mode, int sconly)
{
	wchar_t line[LBUF_SIZE+1];
	int ndx1, ndx2;
	double weight;
	wchar_t name1[MAX_NODE_NAME+1], name2[MAX_NODE_NAME+1];
	int ret;
	int self_citation_count;

	self_citation_count = 0;

	ret = create_network(nnodes);	// allocate memory and do initialization
	if (ret != 0) return ret;

	// 4th pass, establish the link
	rewind(istream);	// point back to the begining of the citation file
	double wt;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		weight = 0.0;
		parse_line(line, separator, name1, name2, &weight);
#ifdef XXX
		nx1 = tname_search(atable, nalias, name1);
		if (nx1 >= 0)
			wcscpy_s(name1, MAX_NODE_NAME, atable[nx1].alias); // replace with alias if exist
		nx2 = tname_search(atable, nalias, name2);
		if (nx2 >= 0)
			wcscpy_s(name2, MAX_NODE_NAME, atable[nx2].alias); // replace with alias if exist
#endif XXX
		if (mode3 == M2_MODE) 
		{
			ndx1 = name_search(nodes, nsn, name1);
			ndx2 = nsn + name_search(&nodes[nsn], nnodes-nsn, name2);
		}
		else
		{
			if (sconly == SOURCE_COLUMN_NODES_ONLY)		// "sconly" should happen only in 1-mode case
			{
				ndx2 = name_search(nodes, nsn, name2);
				if (ndx2 < 0 || ndx2 >= nsn)
					continue;	// name2 is not a source column node
				ndx1 = name_search(nodes, nsn, name1);
			}
			else
			{
				ndx2 = name_search(nodes, nnodes, name2); if (ndx2 < 0) continue;
				ndx1 = name_search(nodes, nnodes, name1); if (ndx1 < 0) continue;
			}
		}

		if (weight_given)
		{
			if (weight == 0.0)
				wt = 1.0;
			else 
				wt = weight;
			if (mode2 == MNONDIRECTED)
				link_nodes(ndx1, ndx2, wt, mode, mode2);
			else 
			{	
				if (self_citation_check(name1, name2))
				{
					self_citation_count++;
#ifdef IGNORE_SELF_CITATION
					continue;		// ignore self-citation relationship
#endif IGNORE_SELF_CITATION
				}
				if (mode == MFORWARD)
					link_nodes(ndx1, ndx2, wt, mode, mode2);
				else
					link_nodes(ndx2, ndx1, wt, mode, mode2);
			}
		}
		else
		{
			if (mode2 == MNONDIRECTED)
				link_nodes(ndx1, ndx2, wt, mode, mode2);
			else 
			{	
				if (self_citation_check(name1, name2))
				{
					self_citation_count++;
#ifdef IGNORE_SELF_CITATION
					continue;		// ignore self-citation relationship
#endif IGNORE_SELF_CITATION
				}
				if (mode == MFORWARD)
					link_nodes(ndx1, ndx2, 1.0, mode, mode2);
				else
					link_nodes(ndx2, ndx1, 1.0, mode, mode2);
			}
		}
	}

	fwprintf(logstream, L"Self-citation count (from establish_links) = %d\n", self_citation_count);
	// close the input file
	fclose(istream);

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_tname(const void *n1, const void *n2)
{
	struct ATABLE *t1, *t2;
	
	t1 = (struct ATABLE *)n1;
	t2 = (struct ATABLE *)n2;
	if (wcscmp(t2->truename, t1->truename) < 0)
		return 1;
	else if (wcscmp(t2->truename, t1->truename) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_id(const void *n1, const void *n2)
{
	struct NODE *t1, *t2;
	
	t1 = (struct NODE *)n1;
	t2 = (struct NODE *)n2;
	if (wcscmp(t2->id, t1->id) < 0)
		return 1;
	else if (wcscmp(t2->id, t1->id) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_aliasname(const void *n1, const void *n2)
{
	struct NODE *t1, *t2;
	
	t1 = (struct NODE *)n1;
	t2 = (struct NODE *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_type(const void *n1, const void *n2)
{
	struct NODE *t1, *t2;
	
	t1 = (struct NODE *)n1;
	t2 = (struct NODE *)n2;
	if (t2->type < t1->type)
		return 1;
	else if (t2->type == t1->type)
		return 0;
	else return -1;
}


//
// use binary search to find the proper position of a document id in a "NODE" array
//
int name_search(struct NODE d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].id) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].id) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].id) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].id) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// use binary search to find the proper position of a "truename" in a "ATABLE" array
//
int tname_search(struct ATABLE a[], int num, wchar_t *str)
{
	int low, high, cur;

	if (a == (struct ATABLE *)0) return -1;
	if (wcscmp(str, a[0].truename) < 0) return -1;

	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, a[cur].truename) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, a[high].truename) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, a[cur].truename) > 0)
			low = cur;
		else
			high = cur;
	}
}

// 
// parse one input line
//
#define WAIT_1ST_D 1
#define WAIT_2ND_D 2
#define WAIT_3RD_D 3
#define BYEBYE 4
int parse_line(wchar_t *line, int separator, wchar_t *nm1, wchar_t *nm2, double *weight)
{
	int i, state;
	wchar_t ch, *sp, *tp;
	wchar_t wt[LBUF_SIZE];

	sp = line;
	tp = nm1;
	// remove the leading spaces
	while (*sp == ' ') sp++;

	// start parsing
	nm1[0] = '\0'; nm2[0] = '\0'; wt[0] = '\0';
	tp = nm1;
	state = WAIT_1ST_D;
	while (*sp != '\0')
	{
		if (*sp == '\n' || *sp == '\r') 
		{ 
			*tp = '\0'; 
			if (state == WAIT_3RD_D)
				*weight = _wtof(wt);
			break; 
		}
		ch = *sp; 
		switch (state)
		{
		case WAIT_1ST_D:
			switch (separator)
			{
			case SP_TAB:
				if (ch == '\t') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			case SP_SPACE:
				if (ch == ' ') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			case SP_TAB_SPACE:
				if (ch == '\t' || ch == ' ') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			case SP_SEMIC:
				if (ch == ';') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			case SP_COLON:
				if (ch == ',') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			case SP_FSLASH:
				if (ch == '/') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			case SP_BSLASH:
				if (ch == '\\') { *tp = '\0'; tp = nm2; state = WAIT_2ND_D; } else *tp++ = *sp; break;
			default:
				break;
			}
			break;
		case WAIT_2ND_D:
			if (ch == ' ' && nm2[0] == '\0') break;	// ignore the leading spaces
			switch (separator)
			{
			case SP_TAB:
				if (ch == '\t') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			case SP_SPACE:
				if (ch == ' ') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			case SP_TAB_SPACE:
				if (ch == '\t' || ch == ' ') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			case SP_SEMIC:
				if (ch == ';') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			case SP_COLON:
				if (ch == ',') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			case SP_FSLASH:
				if (ch == '/') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			case SP_BSLASH:
				if (ch == '\\') { *tp = '\0'; tp = wt; state = WAIT_3RD_D; } else *tp++ = *sp; break;
			default:
				break;
			}
			break;
		case WAIT_3RD_D:
			if (ch == ' '&& wt[0] == '\0') break;	// ignore the leading spaces
			switch (separator)
			{
			case SP_TAB:
				if (ch == '\t') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			case SP_SPACE:
				if (ch == ' ') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			case SP_TAB_SPACE:
				if (ch == '\t' || ch == ' ') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			case SP_SEMIC:
				if (ch == ';') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			case SP_COLON:
				if (ch == ',') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			case SP_FSLASH:
				if (ch == '/') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			case SP_BSLASH:
				if (ch == '\\') { *tp = '\0'; *weight = _wtof(wt); state = BYEBYE; } else *tp++ = *sp; break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		if (state == BYEBYE) break;
		sp++;
	}

	// remove the spaces at the end of the names
	for (i = wcslen(nm1)-1; i >= 0; i--) 
		if (nm1[i] == ' ') nm1[i] = '\0'; else break;
	for (i = wcslen(nm2)-1; i >= 0; i--) if (nm2[i] == ' ') nm2[i] = '\0'; else break;

	return 0;
}

//
// eliminate duplicate citations
//
#define C_LEN 150	// extended to 150, 2014/02/04
struct CITATIONL
{
	int ind;
	wchar_t cline[C_LEN] ;	// a citation description line
};
int eliminate_duplicate_citations(wchar_t *fname)
{
	int k, nlines, nls;
	wchar_t line[LBUF_SIZE+1], *tline;
	wchar_t tname[FNAME_SIZE];
	FILE *fstream, *tstream; 
	struct CITATIONL *ct;
	int ndups;

// Open the given citation file (will fail if the file does not exist)
	if (_wfopen_s(&fstream, fname, L"rt, ccs=UTF-8") != 0)
		return MSG_IFILE_NOTFOUND;
	swprintf(tname, L"%sXXX", fname);
	if (_wfopen_s(&tstream, tname, L"wt, ccs=UTF-8") != 0)
		return MSG_IFILE_NOTFOUND;

	// first pass, count the number of lines
	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, fstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nlines++;
	}
	// allocate memory
	ct = (struct CITATIONL *)Jmalloc(nlines * sizeof(struct CITATIONL), L"eliminate_duplicate_citations: ct");
	if (ct == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// second pass, read in the citation lines
	rewind(fstream);	// point back to the begining of the file
	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, fstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		tline = line;
		if (nlines == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				tline = &line[1];	// skip the BOM
		}
		ct[nlines].ind = nlines;
		wcscpy(ct[nlines].cline, tline);
		nlines++;
	}

	// sort the citation information array
	qsort((void *)ct, (size_t)nlines, sizeof(struct CITATIONL), compare_citation);

	// find and eliminate duplicates
	fwprintf(logstream, L"\nChecking duplicate links (from the citation file) ...\n");
	wchar_t prev[C_LEN];
	ndups = 0; nls = 1;
	wcscpy(prev, ct[0].cline);
	for (k = 1; k < nlines; k++)
	{
		if (wcscmp(ct[k].cline, prev) == 0)	// repetition 
		{	
			fwprintf(logstream, L"Duplicate link: %s", ct[k].cline);
			ndups++;
		}
		else
			ct[nls++] = ct[k];	// move the data over
		wcscpy(prev, ct[k].cline);
	}
	fwprintf(logstream, L"Number of citation lines in file: %d\n", nlines);
	fwprintf(logstream, L"Number of duplicate lines: %d\n", ndups);
	fwprintf(logstream, L"Number of citation lines taken: %d\n\n", nls);

	// sort the citation information array back to original order
	qsort((void *)ct, (size_t)nls, sizeof(struct CITATIONL), compare_cind);

	for (k = 0; k < nls; k++)
		fwprintf(tstream, L"%s", ct[k].cline);	// write back to the file
	fclose(fstream); fclose(tstream);

	_wunlink(fname);
	_wrename(tname, fname);

	Jfree(ct, L"eliminate_duplicate_citations: ct");

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_citation(const void *n1, const void *n2)
{
	struct CITATIONL *t1, *t2;
	
	t1 = (struct CITATIONL *)n1;
	t2 = (struct CITATIONL *)n2;
	if (wcscmp(t2->cline, t1->cline) < 0)
		return 1;
	else if (wcscmp(t2->cline, t1->cline) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_cind(const void *n1, const void *n2)
{
	struct CITATIONL *t1, *t2;
	
	t1 = (struct CITATIONL *)n1;
	t2 = (struct CITATIONL *)n2;
	if (t2->ind < t1->ind)
		return 1;
	else if (t2->ind == t1->ind)
		return 0;
	else return -1;
}

#ifdef MOVED_TO_CLUSTERING.CPP	// moved 2015/10/07
//
// execute external executable program GroupFinder
//
int execute_GroupFinder(wchar_t *pname, int groupfinderoptions)
{
	int ret;
	wchar_t fname[FNAME_SIZE];
	wchar_t options[FNAME_SIZE];
	wchar_t cmd[50+FNAME_SIZE];

	if (groupfinderoptions == GF_UNDIRECTED_UNWEIGHTED)
		swprintf(options, L"Undirected Unweighted");
	else if (groupfinderoptions == GF_DIRECTED_UNWEIGHTED)
		swprintf(options, L"Unweighted");
	else if (groupfinderoptions == GF_UNDIRECTED_WEIGHTED)
	{
		swprintf(options, L"Undirected");
	}
	else if (groupfinderoptions == GF_DIRECTED_WEIGHTED)
	{
		swprintf(options, L"");
	}

	wcscpy(fname, pname);
    //wcstombs(fname, pname, FNAME_SIZE);
	swprintf(cmd, L"GroupFinder \"%s\" %s", fname, options);	// enforce "undirected" or "unweighted", i.e., ignore the direction or weight
	ret = _wsystem(cmd);

	//mbstowcs(tmps, cmd, FNAME_SIZE);
	if (ret != 0)
		fwprintf(logstream, L"Execution of [%s] failed.\n", cmd);
	else
		fwprintf(logstream, L"Execution of [%s] successfully returned.\n", cmd);

	return ret;
}

#define MAX_NNAME_LENGTH 500
struct NNAME {
	wchar_t name[MAX_NNAME_LENGTH];
	int partition;
};
//
// read the results of GroupFinder program
// GroupFinder results is in the partition section of the Pajek file
//
int read_GroupFinder_results(wchar_t *fname)
{
	int backslash;
	int nn;
	wchar_t oname[FNAME_SIZE];
	wchar_t *sp, *tp;
	FILE *fstream;
	wchar_t ln[SBUF_SIZE];
	wchar_t *line;
	wchar_t tmp[SBUF_SIZE];
	wchar_t nname[SBUF_SIZE];
	struct NNAME *node_name;
	int i, state, direction;

	// prepare the name for the Pajek file (a "GF " is added at the beginning of the file name)
	sp = fname; tp = oname; backslash = 0;
	while (*sp != L'\0') { if (*sp == '\\') backslash++; *tp++ = *sp++; }	// go to the end of the line, and check if there is backslashes in the name
	if (backslash == 0) // no backslash in name
		swprintf_s(oname, FNAME_SIZE, L"GF %s", fname);	// added "GF "
	else	// names in long format
	{
		*tp = '\0';
		while (*sp != L'\\') { sp--; tp--; }	// trace back to the last backslash
		tp++; sp++; *tp++ = L'G'; *tp++ = L'F'; *tp++ = ' ';	// pad "GF "
		while (*sp != L'\0') { *tp++ = *sp++; } // copy to the end of the line
		*tp++ = '\0';	
	}

	// open the Pajek file
	if (_wfopen_s(&fstream, oname, L"r") != 0)  { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }		
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);
		
	// first pass, get the number of nodes and check if the network is directed
	while (TRUE)
	{		
		if(fgetws(ln, SBUF_SIZE, fstream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0)
		{
			swscanf(line, L"%s %d", tmp, &nn);
			node_name = (struct NNAME *)malloc((int)nn * sizeof(struct NNAME));
			if (node_name == NULL) return -1;
		}
		if (wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*arcs", 5) == 0)
		{
			direction = 1;
			break;
		}
		if (wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*edges", 6) == 0)
		{
			direction = 0;
			break;
		}
	}

	rewind(fstream);
	i = 0; state = 0;
	while (TRUE)
	{		
		if(fgetws(ln, SBUF_SIZE, fstream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 0)
		{
			state = 1;
			continue;
		}
		else if ((wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*arcs", 5) == 0 || wcsncmp(line, L"*edges", 6) == 0) && state == 1)
		{
			state = 2; i = 0;
			continue;
		}
		else if ((wcsncmp(line, L"*Partition", 10) == 0 || wcsncmp(line, L"*partition", 10) == 0) && state == 2)
		{
			state = 3; i = 0;
			continue;
		}
		else if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 3)
		{
			state = 4; i = 0;
			continue;
		}
		if (state == 1)
		{
			parse_vertices(line, nname);
			wcscpy(node_name[i].name, nname);
			i++;
		}
		else if (state == 2)
			i++;	
		else if (state == 4)
		{
			swscanf(line, L"%d", &node_name[i].partition);
			i++;
			if (i >= nn) break;
		}
	}

	//fwprintf(logstream, L"GroupFinder results:\n");
	for (i = 0; i < nnodes; i++)
	{
		nw[i].partition = node_name[i].partition;	// the order in the Pajek file is the same as that in the nw[] array
		//fwprintf(logstream, L"%d %s\n", node_name[i].partition, node_name[i].name);
	}

	free(node_name);

	return 0;
}

//
// this function read a main path Pajek file and then replace the partition section with the results of the GroupFinder program
//
int mainpaths_with_partition(wchar_t *fname)
{
	int backslash;
	int nn;
	wchar_t oname[FNAME_SIZE];
	wchar_t *sp, *tp;
	FILE *fstream;
	wchar_t ln[SBUF_SIZE];
	wchar_t *line;
	wchar_t tmp[SBUF_SIZE];
	wchar_t nname[SBUF_SIZE];
	struct NNAME *node_name;
	int i, state, direction;
	int ndx;

	// open the original Pajek file
	if (_wfopen_s(&istream, fname, L"r") != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", fname); return -1; }		
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", fname);

	// prepare the name for the output Pajek file (a "GF " is added at the beginning of the file name)
	sp = fname; tp = oname; backslash = 0;
	while (*sp != L'\0') { if (*sp == '\\') backslash++; *tp++ = *sp++; }	// go to the end of the line, and check if there is backslashes in the name
	if (backslash == 0) // no backslash in name
		swprintf_s(oname, FNAME_SIZE, L"GF %s", fname);	// added "GF "
	else	// names in long format
	{
		*tp = '\0';
		while (*sp != L'\\') { sp--; tp--; }	// trace back to the last backslash
		tp++; sp++; *tp++ = L'G'; *tp++ = L'F'; *tp++ = ' ';	// pad "GF "
		while (*sp != L'\0') { *tp++ = *sp++; } // copy to the end of the line
		*tp++ = '\0';	
	}

	// create the new Pajek file 
	if ((_wfopen_s(&ostream, oname, L"w")) != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);

	// 1st pass, copy up to the "*Vertices" line (after the "*partition" line)
	i = 0; state = 0;
	while (TRUE) {		
		if(fgetws(ln, SBUF_SIZE, istream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 0)
		{
			swscanf(line, L"%s %d", tmp, &nn);
			node_name = (struct NNAME *)malloc((int)nn * sizeof(struct NNAME));
			if (node_name == NULL) return -1;
			state = 1;
		}
		else if ((wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*arcs", 5) == 0 || wcsncmp(line, L"*edges", 6) == 0) && state == 1)
			state = 2;
		else if (wcsncmp(line, L"*Partition", 10) == 0 && state == 2)
			break;
		if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }
	}

	// 2nd pass, obtain node names
	rewind(istream);
	i = 0; state = 0;
	while (TRUE)
	{		
		if(fgetws(ln, SBUF_SIZE, istream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 0)
		{
			state = 1;
			continue;
		}
		else if ((wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*arcs", 5) == 0 || wcsncmp(line, L"*edges", 6) == 0) && state == 1)
		{
			state = 2; i = 0;
			continue;
		}
		else if (wcsncmp(line, L"*", 1) == 0 && state == 2)
			break;
		if (state == 1)
		{
			parse_vertices(line, nname);
			wcscpy(node_name[i].name, nname);
			i++;
		}
		else if (state == 2)
			break;
	}

	// write the partition information
	fwprintf(ostream, L"*Partition created by the GroupFinder program\n");
	fwprintf(ostream, L"*Vertices %d\n", nn);
	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_pnaliasname);	// sort according to alias name
	for (i = 0; i < nn; i++)
	{
		ndx = aliasname_search(nw, nnodes, node_name[i].name);
		fwprintf(ostream, L"%d\n", nw[ndx].partition);
	}
	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_nwname);		// sort back to the original order

	// close both files 
	if (fclose(ostream)) { fwprintf(logstream, L"Can not close file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", oname);
	if (fclose(istream)) { fwprintf(logstream, L"Can not close file \"%s\".\n", fname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", fname);

	return 0;
}
#endif MOVED_TO_CLUSTERING.CPP	// moved 2015/10/07

//
// parse a Pajek vertice line
// rewrite on 2015/10/07
//
int parse_vertices(wchar_t *line, wchar_t *nname)
{
	int state;
	wchar_t *sp, *tp;

	sp = line; tp = nname;
	state = 1;
	while (*sp != '\0')
	{
		switch (state)
		{
		case 1:	// skip the ID, waiting for double quotes
			if (*sp == '"')
				state = 2;
			sp++;
			break;
		case 2:	// waiting for the 2nd double quote
			if (*sp == '"')
			{
				*tp = '\0';
				state = 3;
				break;
			}
			else
				*tp++ = *sp++;
		default:
			break;
		}
		if (state == 3)
			break;
	}

	return 0;
}

#ifdef OBSOLETE
int parse_vertices(wchar_t *line, wchar_t *nname)
{
	int i;
	int id;
	wchar_t *sp;

	swscanf(line, L"%d \"%s", &id, nname);
	// remove the double quote at the end of the line
	sp = nname;
	while (*sp != L'\"') sp++;
	*sp = '\0';

	return 0;
}
#endif OBSOLETE


