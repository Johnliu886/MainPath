//
// network_stats.cpp
//

//
// Revision History:
// 2012/01/11 Added to write the value of "total_paths_1" and "total_paths_2" to the basic statistics file
// 2012/07/08 Added to provide patent growth data by patent application date (in addition to that by issue date)
// 2013/04/18 Added to write a section of paper/patent information for each paper
// 2013/04/21 Fixed a bug in writing paper/patent information for each paper
// 2013/05/02 Improved the presentation of output of the SPX statistics
// 2013/07/15 Added a new argument "stype" to the function network_stats() and display paths_forward, nodes_backward, etc. information
// 2013/07/19 Added codes to list details of patent data, including countries, N_IPC4, N_IPC5, N_IPC6, IPC codes, etc.
// 2013/07/23 Added ISIC information to the patent data list
// 2013/11/03 Changed to display 5 (rather than 3) assignees for patent data
// 2014/01/07 Added codes to handle SPHD (harmonic decay)
// 2014/12/12 Added to print 1st author information
// 2014/12/12 Added to call function find_significant_country()
// 2015/05/18 Commented out the block of codes that calculate SPX histogram
// 2015/07/17 Added two functions count_single_author() and count_international_collaboration()
// 2015/07/19 Fixed a problem in the function count_international_collaboration(). In addition, it now provides accumulated number of international collaborations.
// 2015/07/27 Added to print document ID (WOS ID) in the article information section
// 2015/10/08 added to print research group information (a new block)
// 2015/10/18 moved the function find_significant_country() to the file "text2Pajek.xpp"
// 2015/10/29 added several more display entries to the research group information
// 2015/11/19 added code to display hm-index, which take multiple authorship into consideration
// 2016/01/19 Modification  : changed the file opening to Unicode mode (ccs=UTF-8)
// 2016/01/27 added a block of code to display Taiwan T&D information
// 2016/05/12 Fixed a problem in printing article information, when "Keyword report.txt" or "Author report.txt" are not provided.
// 2016/06/22 Commented out IPC-ISIC related code for WEBPAT data (because it uses CPC rather than IPC)
// 2016/10/13 Added codes to support WEBPAT3 data	
// 2016/12/23 Added function find_significant_country_assignee() and find_significant_area_assignee()
// 2016/12/27 Added a block of codes to display assignee information (for Thomson Innovation data)
// 2017/02/26 Fixed country display problem in the "Author Information" display for Scopus data
// 2017/03/02 Enable the "Article Information" display again for Scopus data (was disabled because that the codes to retrieve country info was not available)
// 2017/03/16 Fixed a problem in assignee end year display (for TIP data patent information), a typo error
// 2018/03/13 Commented out ISIC related code for TIP data 
// 2018/03/16 Added conditional compilation check for "UNIVERSITY_INDUSRY_PROJECT" (it cause problem for normal project)
// 2018/06/23 Added check to avoid errors from strnage assignee data (make sure wos[i].DE[k] >= 0)
// 2018/07/21 Added to call fflush(nstat_stream) in several places (for debugging purpose)
// 2018/07/24 Added to print InSPX and OutSPX information to paper information data block
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
#include "clustering.h"

int nslinks;
struct SLINK *slinks;	// holds all the links
double spx_cut;	// this is used in the function critical_transition(), added 2015/05/28

struct TEMPSTR { wchar_t str[MAX_ASSIGNEE_NAME];	};

extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern double scale;
extern int n_components;
extern struct COMP *cp;
extern int comp_largest;
extern int n_sources, n_sinks, n_isolates, n_intermediates;
extern double total_paths_1;
extern FILE *logstream;
extern int full_record_type;
extern int ncountries;				// added 2013/04/10
extern struct COUNTRIES *countries;// added 2013/04/10
extern struct LOCATION location[];
extern struct LOCATION location2[];
extern struct LOCATION location3[];	// added 2017/02/26
extern struct USSTATES usstates[];

extern int naus;
extern struct AUTHORS *authors;	// author name array
extern int nrgroups;
extern struct RESEARCH_GROUP *rgroups;
extern int nkreport;	// added 2016/01/21
extern struct TKEYWORDS *tword;	// added 2016/01/21
extern int ntkeywords;
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int nareport;	// added 2016/01/22
extern struct AUTHOR_REPORT *areport;	// added 2016/01/22
extern struct DEPARTMENTS *departments;	
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only

FILE *nstat_stream;

int SPX_statistics(FILE *);
int compare_spx(const void *, const void *);
int compare_cpsize(const void *, const void *);
int find_location_code(wchar_t *, int *, int *, int *);
int find_location_code2(wchar_t *, int *, int *, int *);
int find_location_code3(wchar_t *, int *, int *, int *);
int count_single_author(int, struct WOS *, int, AUTHORS *);
int count_international_collaboration(struct WOS *, int, AUTHORS *);
int compare_tempstr(const void *, const void *);

extern int compare_year(const void *, const void *);
extern int compare_a_year(const void *, const void *);
extern int find_significant_links(int, struct SLINK *);
extern int find_components(int nnodes, struct PN *nw);
extern int compare_docid(const void *, const void *);
extern int count_IPC_codes(int, int);
extern int replace_IPC_codes(int, int, struct WOS *, struct USPTO *);
extern int read_IPC_ISIC();
extern int find_ISIC(int, struct USPTO *);
extern int is_university(wchar_t *);

//
// calculate network statisics
//
int network_stats(wchar_t *tname, int nnodes, struct PN *nw, int nwos, struct WOS *wos, int nuspts, struct USPTO *uspto, int stype, int clusteringcoauthornetworkoptions)
{
	int i, j, k, m, ii, q, qq;
	int cnt, acnt, ret;
	wchar_t fname[FNAME_SIZE];
	double ndensity, cited_avg;
	wchar_t maincountry[50], mainarea[30];

	if (swprintf(fname, FNAME_SIZE, L"%s basic statistics.txt", tname) == -1)
		return MSG_FILE_NAME_TOO_LONG;

	_wfopen_s(&nstat_stream, fname, L"wt, ccs=UTF-8");	// modified 2016/01/19

//
// original data statistics
//
	int cnt_have_citation = 0;
	int cnt_citation_links = 0;
	for (i = 0; i < nwos; i++)
	{
		//fwprintf(logstream, L"@@@@@ %d\t%d\t[%s]", i, wos[i].tc, wos[i].alias); fflush(logstream);
		if (wos[i].tc != 0)
		{
			cnt_have_citation++;
			cnt_citation_links += wos[i].tc;
		}
	}
	fwprintf(nstat_stream, L"ORIGINAL DATA STATISITICS:\n\n");
	fwprintf(nstat_stream, L"Number of patents/papers = %d\n", nwos);
	fwprintf(nstat_stream, L"Number of patents/papers with citation data = %d\n", cnt_have_citation);
	fwprintf(nstat_stream, L"Total number of nominal citations = %d\n", cnt_citation_links);
	fwprintf(nstat_stream, L"Average citations per patents/papers = %f\n\n", (double)cnt_citation_links/nwos);
	fwprintf(nstat_stream, L"NOTE1: Those patents/papers that have no citation and are not citing patents/papers in the data set will not be included in the following citation network analysis.\n");
	fwprintf(nstat_stream, L"NOTE2: In the citation network, those nodes that are not cited by and not citing other nodes in the network becomes the isolated nodes.\n");
	fwprintf(nstat_stream, L"\n");

	// publication year statistics
	// count the number of documents for each year
	int prev_year;
	fwprintf(nstat_stream, L"Patent/paper growth trend:\n");
	fwprintf(nstat_stream, L"Year\tN. of Patents\tAccumulated Number\n");
	// sort by years
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_year);
	prev_year = 0;
	cnt = acnt = 0;
	for (i = 0; i < nwos; i++)
	{
		if (wos[i].year != prev_year)
		{
			if (cnt != 0)
				fwprintf(nstat_stream, L"%d\t%d\t%d\n", prev_year, cnt, acnt);
			prev_year = wos[i].year;
			cnt = 1; acnt++;
		}
		else	// same year
		{
			cnt++; acnt++;
		}
	}
	fwprintf(nstat_stream, L"%d\t%d\t%d\n", wos[i-1].year, cnt, acnt);
	fwprintf(nstat_stream, L"\n");

	// for WEBPAT data, also list patent growth by application date
	if (full_record_type == USPTO_DATA)
	{
		fwprintf(nstat_stream, L"Patent growth trend by application date:\n");
		fwprintf(nstat_stream, L"Year\tN. of Patents\tAccumulated Number\n");
		// sort by years
		qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_a_year);
		prev_year = 0;
		cnt = acnt = 0;
		for (i = 0; i < nwos; i++)
		{
			if (wos[i].a_year != prev_year)
			{
				if (cnt != 0)
					fwprintf(nstat_stream, L"%d\t%d\t%d\n", prev_year, cnt, acnt);
				prev_year = wos[i].a_year;
				cnt = 1; acnt++;
			}
			else	// same year
			{
				cnt++; acnt++;
			}
		}
		fwprintf(nstat_stream, L"%d\t%d\t%d\n", wos[i-1].a_year, cnt, acnt);
		fwprintf(nstat_stream, L"\n");
	}
	fflush(nstat_stream);	// added 2018/07/21

	// sort the data bak to the order in ISI document id and stay this way 
	qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);

//
// Network statistics
//
	fwprintf(nstat_stream, L"CIATION NETWORK STATISTICS:\n\n");
	fwprintf(nstat_stream, L"Network Size = %d nodes ", nnodes);
	fwprintf(nstat_stream, L"(sources: %d, sinks: %d, isolates: %d, intermediates: %d)\n", n_sources, n_sinks, n_isolates, n_intermediates);

	// find the number of outward links
	nslinks = 500000;	// the number is changed later
	slinks = (struct SLINK *)Jmalloc(nslinks * sizeof(struct SLINK), L"network_stats: slinks");
	if (slinks == NULL) return MSG_NOT_ENOUGH_MEMORY;
	nslinks = find_significant_links(nslinks, slinks);	
	qsort((void *)slinks, (size_t)nslinks, sizeof(struct SLINK), compare_spx);
	fwprintf(nstat_stream, L"Total number of citation links = %d\n", nslinks);
	spx_cut = slinks[nslinks/2].spx;	// SPX cut off value (top 20%), to be used in critical_transition(), added 2015/05/28

	cited_avg = (double)nslinks / nnodes;
	fwprintf(nstat_stream, L"Average cited count per node = %f\n", cited_avg);
	ndensity = (double)nslinks / (nnodes * (nnodes - 1));
	fwprintf(nstat_stream, L"Network density = %f\n", ndensity);

	ret = find_components(nnodes, nw);
	if (ret != 0) return ret;
	fwprintf(nstat_stream, L"Number of components = %d\n", n_components);
	// gather histogram of component size 
	int csize[11];
	for (i = 0; i < 11; i++) csize[i] = 0;
	for (i = 0; i < n_components; i++)
	{
		if (cp[i].nnodes > 10)
			csize[10]++;	/* size greater than 10 */
		else
			csize[cp[i].nnodes-1]++;	/* size 1 through 10 */
	}
	fwprintf(nstat_stream, L"ID of the largest component: %d (number of patents/papers: %d)\n", comp_largest+1, cp[comp_largest].nnodes);
	fwprintf(nstat_stream, L"Component size histogram: ");
	for (i = 0; i < 10; i++) 
		fwprintf(nstat_stream, L"%dx%d ", i+1, csize[i]);
	fwprintf(nstat_stream, L">10x%d\n", csize[10]);

	qsort((void *)cp, (size_t)n_components, sizeof(struct COMP), compare_cpsize);
	fwprintf(nstat_stream, L"Size of the top 10 components: ");
	for (i = 0; i < n_components; i++)
	{
		if (i >= 10) break;
		fwprintf(nstat_stream, L"%d ", cp[i].nnodes);
	}
	fwprintf(nstat_stream, L"\n");

	fprintf(nstat_stream, "Total number of paths (counted from sources) = %f\n", total_paths_1);
	fflush(nstat_stream);	// added 2018/07/21

	fwprintf(nstat_stream, L"\nTop 5000 Links:\n");
	if (stype == S_SPC)
		fwprintf(nstat_stream, L"Count\tSPC\tRelevancy\tFrom => To\tinflow paths\toutflow paths\n");
	else if (stype == S_SPLC)
		fwprintf(nstat_stream, L"Count\tSPLC\tRelevancy\tFrom => To\tinflow paths\toutflow paths\n");
	else if (stype == S_SPNP)
		fwprintf(nstat_stream, L"Count\tSPNP\tRelevancy\tFrom => To\tinflow paths\toutflow paths\n");
	else if (stype == S_SPGD)
		fwprintf(nstat_stream, L"Count\tSPGD\tRelevancy\tFrom => To\teffective inflow paths\tSPLC outflow paths\n");
	else if (stype == S_SPAD)
		fwprintf(nstat_stream, L"Count\tSPAD\tRelevancy\tFrom => To\teffective inflow paths\teffective outflow paths\n");
	else if (stype == S_SPHD)
		fwprintf(nstat_stream, L"Count\tSPHD\tRelevancy\tFrom => To\teffective inflow paths\tSPLC outflow paths\n");
	int ntodisplay;
	int sfrom, sto;

	if (nslinks >= 5000) ntodisplay = 5000; else ntodisplay = nslinks; 
	for (i = 0; i < ntodisplay; i++)
	{
		sfrom = slinks[i].from;	sto = slinks[i].to;
		fwprintf(nstat_stream, L"%d\t%.2f\t%.6f\t%s => %s\t", i+1, slinks[i].spx, slinks[i].relevancy, nw[sfrom].alias, nw[sto].alias);
		if (stype == S_SPC)
			fwprintf(nstat_stream, L"%.0f\t%.0f\n", (double)nw[sfrom].paths_backward, (double)nw[sto].paths_forward);
		else if (stype == S_SPLC)
			fwprintf(nstat_stream, L"%.0f\t%.0f\n", (double)nw[sfrom].nodes_backward, (double)nw[sto].paths_forward);
		else if (stype == S_SPNP)
			fwprintf(nstat_stream, L"%.0f\t%.0f\n", (double)nw[sfrom].nodes_backward, (double)nw[sto].nodes_forward);
		else if (stype == S_SPGD)
			fwprintf(nstat_stream, L"%.4f\t%.0f\n", (double)nw[sfrom].decay_backward, (double)nw[sto].paths_forward);
		else if (stype == S_SPAD )
			fwprintf(nstat_stream, L"%.4f\t%.4f\n", (double)slinks[i].spx/nw[sto].paths_forward, (double)nw[sto].paths_forward);
		else if (stype == S_SPHD)
			fwprintf(nstat_stream, L"%.4f\t%.0f\n", (double)slinks[i].spx/nw[sto].paths_forward, (double)nw[sto].paths_forward);
	}
	fflush(nstat_stream);	// added 2018/07/21

	//
	// write paper basic information, added 2013/04/18
	//
	if (full_record_type == WOS_DATA ||full_record_type == SCOPUS_DATA)	// Added to show Scopus info (again), 2017/03/02
	{
		fwprintf(nstat_stream, L"\n============= Article Information ================\n");
		int c1, c2, island;
		fwprintf(nstat_stream, L"\nDocuments\tDocID\tYear\tWOS_Citation\tSPX\tInSPX\tOutSPX\tInDeg\tOutDeg\tN_Authors\tCountry_RP\tArea\tSubArea\tIsland\tCountries\n");
		for (m = 0; m < nnodes; m++)
		{
			i = nw[m].ndx2wos;
			j = wos[i].countries_rp[0];
			wchar_t lcname[100];
			if (full_record_type == SCOPUS_DATA)	// added 2017/03/02
			{
				wcscpy(lcname, location3[j].name);
				find_location_code3(lcname, &c1, &c2, &island);
			}
			else
			{
				wcscpy(lcname, location[j].name);
				find_location_code(lcname, &c1, &c2, &island);
			}
			fwprintf(nstat_stream, L"%s\t%s\t%d\t%d\t%.1f\t%.1f\t%.1f\t%d\t%d\t%d\t%s\t%d\t%d\t%d", wos[i].alias, wos[i].docid, wos[i].year, wos[i].tc, 
				(double)0.5*(nw[m].total_in_spx+nw[m].total_out_spx), nw[m].total_in_spx, nw[m].total_out_spx,
				nw[m].in_deg, nw[m].out_deg, wos[i].nau, lcname, c1, c2, island);
			for (k = 0; k < wos[i].ncountries; k++)
			{
				j = wos[i].countries[k];
				if (full_record_type == SCOPUS_DATA)	// added 2017/03/02
					fwprintf(nstat_stream, L"\t%s", location3[j].name);
				else
					fwprintf(nstat_stream, L"\t%s", location[j].name);
			}
			fwprintf(nstat_stream, L"\n");
		}
	}
	fflush(nstat_stream);	// added 2018/07/21

	//
	// write paper basic information for Taiwan T&D data, added 2016/01/21
	//
	if (full_record_type == TAIWAN_TD_DATA)	
	{
		int *tndx, *andx;
		tndx = (int *)Jmalloc(nkreport * sizeof(int), L"network_stats: tndx");
		andx = (int *)Jmalloc(nareport * sizeof(int), L"network_stats: andx");
		fwprintf(nstat_stream, L"\n============= Article Information ================\n");
		int c1, c2, island;
		fwprintf(nstat_stream, L"\nDocuments\tDocID\tYear\tAuthor1\tAuthor2\tInDeg\tOutDeg\tN_Authors\tDepartment");
		for (m = 0; m < nkreport; m++)	// followed by keywords (defined in the file "Keyword report.txt"
			fwprintf(nstat_stream, L"\t%s", tword[m].name);
		for (m = 0; m < nareport; m++)	// followed by authors (defined in the file "Author report.txt"
			fwprintf(nstat_stream, L"\t%s", areport[m].name);
		fwprintf(nstat_stream, L"\n");
		for (m = 0; m < nnodes; m++)
		{
			i = nw[m].ndx2wos;
			fwprintf(nstat_stream, L"%s\t%s\t%d\t%s\t%s\t%d\t%d\t%d\t%s", wos[i].alias, wos[i].docid, wos[i].year,
				authors[wos[i].author[0]].name, authors[wos[i].author[1]].name,
				nw[m].in_deg, nw[m].out_deg, wos[i].nau, departments[wos[i].department].name);

			if (nkreport > 0)	// added 2016/05/12
			{
				for (qq = 0; qq < nkreport; qq++) tndx[qq] = 0;
				for (q = 0; q < wos[i].ntkws; q++)
				{
					qq = tkeyword[wos[i].tkws[q]].cross_ndx;
					if (qq != -1) tndx[qq] = 1;
				}
				for (qq = 0; qq < nkreport; qq++)	// print keyword status
					fwprintf(nstat_stream, L"\t%d", tndx[qq]);
			}

			if (nareport > 0)	// added 2016/05/12
			{
				for (qq = 0; qq < nareport; qq++) andx[qq] = 0;
				for (q = 0; q < wos[i].nau; q++)
				{
					qq = authors[wos[i].author[q]].ndx2;
					if (qq != -1) andx[qq] = 1;
				}
				for (qq = 0; qq < nareport; qq++)	// print author status
					fwprintf(nstat_stream, L"\t%d", andx[qq]);
			}
			fwprintf(nstat_stream, L"\n");
		}
		Jfree(tndx, L"network_stats: tndx");
		Jfree(andx, L"network_stats: andx");
	}
	fflush(nstat_stream);	// added 2018/07/21

	//
	// write author (1st author only) basic information, added 2014/10/31
	//
	if (full_record_type == WOS_DATA || full_record_type == SCOPUS_DATA)
	{
		// find_significant_country(); // added 2014/12/12, move to "text2Pajek.cpp", 2015/10/18
		count_single_author(nwos, wos, naus, authors);		// added 2015/07/17
		count_international_collaboration(wos, naus, authors);	// added 2015/07/17, modified 2015/07/19
		fwprintf(nstat_stream, L"\n============= Author Information (1st authors only) ================\n");
		int c1, c2, island;
		fwprintf(nstat_stream, L"\nAuthors\tGroupID\th-index\thm-index\tg-index\tNPapers\tN1st\tNSingle\tInter\tBYear\tEYear\tInSPX\tOutSPX\tTotalTC\tLocation\tArea\tSubArea\tIsland\tCountries\n");
		for (i = 0; i < naus; i++)
		{
			wchar_t lcname[100];
			if (authors[i].cnt1 > 0)
			{
				j = authors[i].location;
				if (full_record_type == SCOPUS_DATA)	// added 2017/02/26
				{
					wcscpy(lcname, location3[j].name);
					find_location_code3(lcname, &c1, &c2, &island);
				}
				else
				{
					wcscpy(lcname, location[j].name);
					find_location_code(lcname, &c1, &c2, &island);
				}
				fwprintf(nstat_stream, L"%s\t%d\t%d\t%.2f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.1f\t%.1f\t%d\t%s\t%d\t%d\t%d", 
					authors[i].name, authors[i].groupid, authors[i].h, authors[i].hm, authors[i].g, 
					authors[i].np, authors[i].cnt1, authors[i].nsingle, authors[i].inter,
					authors[i].byear, authors[i].eyear,
					authors[i].total_in_spx, authors[i].total_out_spx, authors[i].total_cites,
					lcname, c1, c2, island);
				for (k = 0; k < authors[i].ncountries; k++)
				{
					j = authors[i].countries[k];
					fwprintf(nstat_stream, L"\t%s\t%d", lcname, authors[i].country_cnt[k]);;
				}
				fwprintf(nstat_stream, L"\n");
			}
		}
	}
	fflush(nstat_stream);	// added 2018/07/21

#ifdef UNIVERSITY_INDUSRY_PROJECT
	//
	// write assignee information, added 2016/12/27
	//
	if (full_record_type == THOMSON_INNOVATION_DATA)
	{
		fwprintf(nstat_stream, L"\n============= Assignee Information ================\n");
		int c1, c2, island;
		int is_univ, scnt, acnt;
		int Ucoll, Icoll;
		int NCcoll;			// number of collaborating countries
		int NAcoll;			// number of collaborating US states
		int NPUI, NPUUI, NPUU;	// added 2017/03/17
		//wchar_t Ccoll[50];	// name of the collaborating country
		int citation_max, citation_min, citation_sum;
		double citation_mean;
		wchar_t prev[MAX_ASSIGNEE_NAME];
		wchar_t *patent_max, *patent_min;
		struct TEMPSTR *temps;
		fwprintf(nstat_stream, 
			L"\nAssignees\tUniversity\tNPatents\tNPUU\tNPUUI\tNPUI\tBYear\tEYear\tTotalColl\tUColl\tIColl\tNCountryColl\tNAreaColl\tCitationMean\tPatentMax\tMaxCitation\tPatentMin\tMinCitation\tMainCountry\tMainArea\tCollaborators\tCountryColl\tAreaColl\t...\n");
			
		temps = (struct TEMPSTR *)Jmalloc(1000 * sizeof(struct TEMPSTR), L"network_stats: temps");
		for (i = 0; i < nasgns; i++)	// find the number of collaborating firms and universities
		{	
			// find the patent spread, added 2017/03/17
			NPUI = NPUUI = NPUU = 0;
			for (k = 0; k < assignees[i].np; k++)
			{
				if (uspto[assignees[i].paper[k]].UI == UNIVERSITY_UNIVERSITY)
					NPUU++;
				else if (uspto[assignees[i].paper[k]].UI == UNIVERSITY_UNIVERSITY_INDUSTRY)
					NPUUI++;
				else if (uspto[assignees[i].paper[k]].UI == UNIVERSITY_INDUSTRY)
					NPUI++;
			}
			// find the number of collaborating non-universities
			Icoll = 0;
			for (k = 0; k < assignees[i].degree; k++)
			{
				if (is_university(assignees[assignees[i].nbrs[k]].name) == 0)	// not university
				{
					wcscpy(temps[Icoll].str, assignees[assignees[i].nbrs[k]].name);
					Icoll++;
				}
			}
			qsort((TEMPSTR *)temps, (size_t)Icoll, sizeof(struct TEMPSTR), compare_tempstr);
			scnt = 0; prev[0] = '\0';
			for (k = 0; k < Icoll; k++)
			{
				if (wcscmp(prev, temps[k].str) != 0)
				{
					wcscpy(prev, temps[k].str);
					scnt++;
				}
			}
			Icoll = scnt;	
			// find the number of collaborating universites
			Ucoll = 0;
			for (k = 0; k < assignees[i].degree; k++)
			{
				if (is_university(assignees[assignees[i].nbrs[k]].name))	// is a university
				{
					wcscpy(temps[Ucoll].str, assignees[assignees[i].nbrs[k]].name);
					Ucoll++;
				}
			}
			qsort((TEMPSTR *)temps, (size_t)Ucoll, sizeof(struct TEMPSTR), compare_tempstr);
			scnt = 0; prev[0] = '\0';
			for (k = 0; k < Ucoll; k++)
			{
				if (wcscmp(prev, temps[k].str) != 0)
				{
					wcscpy(prev, temps[k].str);
					scnt++;
				}
			}
			Ucoll = scnt;	
			// find the number of collaborating countries
			for (k = 0; k < assignees[i].degree; k++)
				wcscpy(temps[k].str, location2[assignees[assignees[i].nbrs[k]].location].name);
			qsort((TEMPSTR *)temps, (size_t)assignees[i].degree, sizeof(struct TEMPSTR), compare_tempstr);
			scnt = 0; prev[0] = '\0';
			for (k = 0; k < assignees[i].degree; k++)
			{
				if (wcscmp(prev, temps[k].str) != 0)
				{
					wcscpy(prev, temps[k].str);
					scnt++;
				}
			}
			NCcoll = scnt;	
			// find the number of collaborating US states
			acnt = 0;
			for (k = 0; k < assignees[i].degree; k++)
			{
				if (wcscmp(location2[assignees[assignees[i].nbrs[k]].location].name, L"US") == 0)
				{
					wcscpy(temps[acnt].str, usstates[assignees[assignees[i].nbrs[k]].location_area].abbr);
					acnt++;
				}
			}
			qsort((TEMPSTR *)temps, (size_t)acnt, sizeof(struct TEMPSTR), compare_tempstr);
			scnt = 0; prev[0] = '\0';
			for (k = 0; k < acnt; k++)
			{
				if (wcscmp(prev, temps[k].str) != 0)
				{
					wcscpy(prev, temps[k].str);
					scnt++;
				}
			}
			NAcoll = scnt;	
			if (assignees[i].np >= 0)
			{
				citation_max = -1; citation_min = 9999999; citation_sum = 0;
				for (k = 0; k < assignees[i].np; k++)	// find minimum and maximum citations for each assginee
				{
					citation_sum += wos[assignees[i].paper[k]].tc;
					if (wos[assignees[i].paper[k]].tc > citation_max) 
					{
						citation_max = wos[assignees[i].paper[k]].tc; 
						patent_max = wos[assignees[i].paper[k]].docid;
					}
					if (wos[assignees[i].paper[k]].tc < citation_min) 
					{
						citation_min = wos[assignees[i].paper[k]].tc;
						patent_min = wos[assignees[i].paper[k]].docid;
					}
				}
				citation_mean = (double)citation_sum / assignees[i].np;
				//j = assignees[i].location;
				//find_location_code(location[j].name, &c1, &c2, &island);
				if (assignees[i].location_status == 1)	// only one significant location
					wcscpy(maincountry, location2[assignees[i].location].name);
				else
					wcscpy(maincountry, L"VAGUE");
				if (assignees[i].nareas == 0)	// no meaningful area
					wcscpy(mainarea, L"~");
				else if (assignees[i].location_area_status == 1)	// only one significant area 
					wcscpy(mainarea, usstates[assignees[i].location_area].abbr);
				else
					wcscpy(mainarea, L"VAGUE");
				is_univ = is_university(assignees[i].name);
				fwprintf(nstat_stream, L"%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.2f\t%s\t%d\t%s\t%d\t%s\t%s", 
					assignees[i].name, is_univ,
					assignees[i].np, NPUU, NPUUI, NPUI,
					assignees[i].byear, assignees[i].eyear,
					assignees[i].degree, Ucoll, Icoll, NCcoll, NAcoll,
					citation_mean, patent_max, citation_max, patent_min, citation_min, 
					maincountry, mainarea
					);
				for (k = 0; k < assignees[i].degree; k++)
					fwprintf(nstat_stream, L"\t%s\t%s\t%s", assignees[assignees[i].nbrs[k]].name, 
					location2[assignees[assignees[i].nbrs[k]].location].name, usstates[assignees[assignees[i].nbrs[k]].location_area].abbr);
				fwprintf(nstat_stream, L"\n");
			}
		}
		Jfree(temps, L"network_stats: temps");
	}
#endif UNIVERSITY_INDUSRY_PROJECT

	//
	// write research group information, added 2015/10/08
	//
	if (clusteringcoauthornetworkoptions == CCoauthor_UNWEIGHTED && (full_record_type == WOS_DATA || full_record_type == SCOPUS_DATA))
	{
		double inward_citations, outward_citations, inward_citations_noselfcites, outward_citations_noselfcites, self_cites;
		fwprintf(nstat_stream, L"\n============= Research Group Information ================\n");
		fwprintf(nstat_stream, L"\nGroupID\tNScholars\tNPapers\tAvgAcademicAge\tBYear\tEYear\tSelfCites\tInwardCitation\tOutwardCitation\tWIntraLinks\tDegCentrality\tWDegCentrality\tDensity\tDegCentralization\tDegCentralization2\tTotalPaths\tm-index\tMultiplePaths\tScholars\t\t\t\t\tCountries\n");
		for (i = 0; i < nrgroups; i++)
		{
			inward_citations = outward_citations = inward_citations_noselfcites = outward_citations_noselfcites = 0.0;
			if (rgroups[i].nscholars >= 5)	// show only those groups with more than 5 scholars
			{
				for (k = 0; k < rgroups[i].ctn_indegree; k++) inward_citations += rgroups[i].ctn_inweight[k];		// include self-cites
				for (k = 0; k < rgroups[i].ctn_outdegree; k++) outward_citations += rgroups[i].ctn_outweight[k];	// include self-cites
				for (k = 0; k < rgroups[i].ctn_indegree; k++)	
				{
					if (rgroups[i].ctn_innbrs[k] != i)	// exclude self-cites
							inward_citations_noselfcites += rgroups[i].ctn_inweight[k];
				}
				for (k = 0; k < rgroups[i].ctn_outdegree; k++) 
				{
					if (rgroups[i].ctn_outnbrs[k] != i)	// exclude self-cites
						outward_citations_noselfcites += rgroups[i].ctn_outweight[k];
				}
				self_cites = inward_citations - inward_citations_noselfcites;
				fwprintf(nstat_stream, L"%d\t%d\t%d\t%.2f\t%d\t%d\t%.0f\t%.0f\t%.0f\t%d\t%d\t%d\t%.6f\t%.6f\t%.6f\t%.0f\t%.6f\t%.0f", 
					i, rgroups[i].nscholars, rgroups[i].np, rgroups[i].average_academic_age, rgroups[i].byear, rgroups[i].eyear,
					self_cites, inward_citations_noselfcites, outward_citations_noselfcites, 
					rgroups[i].weightedintralinks, rgroups[i].degcentrality, rgroups[i].wdegcentrality,
					rgroups[i].density, rgroups[i].centralization, rgroups[i].centralization2, 
					rgroups[i].total_paths, rgroups[i].mindex, rgroups[i].multiple_work_paths);
				//for (k = 0; k < rgroups[i].nscholars; k++)	
				for (k = 0; k < 5; k++)	// display only 5 scholars
				{
					fwprintf(nstat_stream, L"\t[%s]", authors[rgroups[i].scholars[k]].name);
				}
				for (k = 0; k < rgroups[i].ncountries; k++)
				{
					j = rgroups[i].countries[k];
					fwprintf(nstat_stream, L"\t%s\t%d", location[j].name, rgroups[i].country_cnt[k]);
				}
				fwprintf(nstat_stream, L"\n");
			}
		}
	}
	fflush(nstat_stream);	// added 2018/07/21

	static struct P_IPC ipc_s;	
	// 	
	// write patent basic information, for WEBPAT data, added 2013/07/19
	//
	if (full_record_type == USPTO_DATA || full_record_type == WEBPAT2_DATA || full_record_type == WEBPAT3_DATA)
	//if (full_record_type == USPTO_DATA)
	{
		fwprintf(nstat_stream, L"\n============= Patent Information ================\n");
		int c1, c2, island;
		if (read_IPC_ISIC() != 0)
			fwprintf(nstat_stream, L"WARNING: cannot open/read file \"IPC-ISIC.txt\"\n");
		else
		{
			fwprintf(nstat_stream, L"\n\tPatent_ID\tPatent_Alias\tYear\tSPX\tInDeg\tOutDeg\tN_Inventors\tCountry\tArea\tSubAread\tIsland\tN_Assignees\tAssignee1\tAssignee2\tAssignee3\tAssignee4\tAssignee5\tN_ISIC\tISIC1\tISIC1_P\tISIC2\tISIC2_P\tISIC3\tISIC3_P\tISIC4\tISIC4_P\tISIC5\tISIC5_P\tN_IPC4\tN_IPC5\tN_IPC6\tN_IPC\tIPCs\n");
			for (m = 0; m < nnodes; m++)
			{
				i = nw[m].ndx2wos;
				find_location_code2(uspto[i].country, &c1, &c2, &island);
				fwprintf(nstat_stream, L"%d\t%s\t%s\t%d\t%.1f\t%d\t%d\t%d\t%s\t%d\t%d\t%d", m+1, uspto[i].pid, wos[i].alias, wos[i].year,
					(double)0.5*(nw[m].total_in_spx+nw[m].total_out_spx),
					nw[m].in_deg, nw[m].out_deg, wos[i].nau, uspto[i].country, c1, c2, island);
				fwprintf(nstat_stream, L"\t%d", wos[i].nde);
				for (k = 0; k < 5; k++)	// changed from 3 to 5, 2013/11/03
				{
					if (k >= wos[i].nde)
						fwprintf(nstat_stream, L"\t");
					else
					{
						j = wos[i].DE[k];
						if (j < 0)	// added 2018/06/23
							fwprintf(nstat_stream, L"\t[unknown]");
						else
							fwprintf(nstat_stream, L"\t%s", assignees[j].name);
					}
				}
#ifdef NEED_TO_BE_CHANGED_FOR_CPC
				// save IPC info in wos[]
				ipc_s.nipc = wos[i].nipc;
				for (k = 0; k < wos[i].nipc; k++)	
					wcscpy_s(&ipc_s.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[i].ipc[MAX_IPC_CODE*k]); 

				int nipc4, nipc5, nipc6; 
				nipc4 = replace_IPC_codes(i, 4, wos, uspto);
				find_ISIC(i, uspto);	// find the correspond ISIC code using the 4-code IPC, added 2013/07/23
				fwprintf(nstat_stream, L"\t%d", uspto[i].nisic);
				for (k = 0; k < 5; k++)
				{
					if (k < uspto[i].nisic)
						fwprintf(nstat_stream, L"\t%s\t%.2f", &uspto[i].isic[k*MAX_ISIC_CODE], uspto[i].isic_percent[k]);
					else
						fwprintf(nstat_stream, L"\t\t");
				}
#ifdef DEBUG
				fwprintf(logstream, L"@@@ %s\t%d ==>", uspto[i].pid, uspto[i].nisic);
				for (k = 0; k < uspto[i].nisic; k++)
					fwprintf(logstream, L"\t[%s %.2f]", &uspto[i].isic[k*MAX_ISIC_CODE], uspto[i].isic_percent[k]);
				fwprintf(logstream, L"\n");
#endif DEBUG
				// restore IPC info to wos[]
				wos[i].nipc = ipc_s.nipc;
				for (k = 0; k < ipc_s.nipc; k++)
					wcscpy_s(&wos[i].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_s.ipc[MAX_IPC_CODE*k]);

				nipc5 = count_IPC_codes(i, 5);	// check only the 5 leading characters
				nipc6 = count_IPC_codes(i, 6);	// check only the 6 leading characters
				fwprintf(nstat_stream, L"\t%d\t%d\t%d\t%d", nipc4, nipc5, nipc6, wos[i].nipc);
				for (k = 0; k < wos[i].nipc; k++)
				{
					fwprintf(nstat_stream, L"\t%s", &wos[i].ipc[k*MAX_IPC_CODE]);
				}
#endif NEED_TO_BE_CHANGED_FOR_CPC
				fwprintf(nstat_stream, L"\n");
			}
		}
	}
	fflush(nstat_stream);	// added 2018/07/21

	// 	
	// write patent basic information, for THOMSON INNOVATION data, added 2013/07/27, massively modified, 2016/12/28
	//
	if (full_record_type == THOMSON_INNOVATION_DATA)
	{
		if (read_IPC_ISIC() != 0)
			fwprintf(nstat_stream, L"WARNING: cannot open/read file \"IPC-ISIC.txt\"\n");
		else
		{
			fwprintf(nstat_stream, L"\n============= Patent Information ================\n");
			fwprintf(nstat_stream, L"\n\tPatent_ID\tPatent_Alias\tPubYear\tNetwork\tCitation\tMultipleAssignees\tNAssignees\tUU\tUI\t\UUI\tNCountries\tMainCountry\tNAreas\tMainArea\tSPX\tInDeg\tOutDeg\tN_Inventors\tN_Assignees\tAssignee1\tAssignee2\tAssignee3\tAssignee4\tAssignee5\tN_ISIC\tISIC1\tISIC1_P\tISIC2\tISIC2_P\tISIC3\tISIC3_P\tISIC4\tISIC4_P\tISIC5\tISIC5_P\tN_IPC4\tN_IPC5\tN_IPC6\tN_IPC\tIPCs\n");
			//for (m = 0; m < nnodes; m++) { i = nw[m].ndx2wos;
			for (i = 0; i < nwos; i++)	// changed to display all patents, rather than only patents in the citation network
			{
				double spx;
				int indeg, outdeg, cnet;
				int UU, UI, UUI, massignees;
				m = wos[i].ndx2nw;
				if (m != -1) { cnet = 1; spx = (double)0.5*(nw[m].total_in_spx+nw[m].total_out_spx); indeg = nw[m].in_deg; outdeg = nw[m].out_deg; }
				else { cnet = 0; spx = 0.0; indeg = 0; outdeg = 0; }
				if (uspto[i].nassignee > 1) massignees = 1; else massignees = 0;
				UU = UI = UUI = 0;
				if (uspto[i].UI == UNIVERSITY_UNIVERSITY) UU = 1;
				else if (uspto[i].UI == UNIVERSITY_INDUSTRY) UI = 1;
				else if (uspto[i].UI == UNIVERSITY_UNIVERSITY_INDUSTRY) UUI = 1;
				if (uspto[i].location_status == 1)	// only one significant location
					wcscpy(maincountry, location2[uspto[i].location].name);
				else
					wcscpy(maincountry, L"VAGUE");
				if (uspto[i].nareas == 0)	// no meaningful area
					wcscpy(mainarea, L"~");
				else if (uspto[i].location_area_status == 1)	// only one significant area 
					wcscpy(mainarea, usstates[uspto[i].location_area].abbr);
				else
					wcscpy(mainarea, L"VAGUE");
				fwprintf(nstat_stream, L"%d\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\t%.1f\t%d\t%d\t%d", 
					i+1, wos[i].docid, wos[i].alias, wos[i].year,
					cnet, wos[i].tc, massignees, wos[i].nde, UU, UI, UUI, uspto[i].ncountries, maincountry, uspto[i].nareas, mainarea,
					spx, indeg, outdeg, wos[i].nau);
				fwprintf(nstat_stream, L"\t%d", wos[i].nde);	// wos.DE[] has the assignee information
				for (k = 0; k < 5; k++)
				{
					if (k >= wos[i].nde)
						fwprintf(nstat_stream, L"\t");
					else
					{
						j = wos[i].DE[k];
						if (j < 0)	// added 2018/06/23
							fwprintf(nstat_stream, L"\t[unknown]");
						else
							fwprintf(nstat_stream, L"\t%s", assignees[j].name);
					}
				}

#ifdef NEED_TO_BE_CHANGED_FOR_CPC	// added 2018/03/13
				// save IPC info in wos[]
				ipc_s.nipc = wos[i].nipc;
				//fwprintf(logstream, L"$$$$$\t%d\t%d\n", wos[i].nipc, i); fflush(logstream);

				for (k = 0; k < wos[i].nipc; k++)	
					wcscpy_s(&ipc_s.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[i].ipc[MAX_IPC_CODE*k]); 

				int nipc4, nipc5, nipc6; 
				nipc4 = replace_IPC_codes(i, 4, wos, uspto);
				find_ISIC(i, uspto);	// find the corresponding ISIC code using the 4-code IPC, added 2013/07/23
				fwprintf(nstat_stream, L"\t%d", uspto[i].nisic);
				for (k = 0; k < 5; k++)
				{
					if (k < uspto[i].nisic)
						fwprintf(nstat_stream, L"\t%s\t%.2f", &uspto[i].isic[k*MAX_ISIC_CODE], uspto[i].isic_percent[k]);
					else
						fwprintf(nstat_stream, L"\t\t");
				}
				// restore IPC info to wos[]
				wos[i].nipc = ipc_s.nipc;
				for (k = 0; k < ipc_s.nipc; k++)
					wcscpy_s(&wos[i].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_s.ipc[MAX_IPC_CODE*k]);

				nipc5 = count_IPC_codes(i, 5);	// check only the 5 leading characters
				nipc6 = count_IPC_codes(i, 6);	// check only the 6 leading characters
				fwprintf(nstat_stream, L"\t%d\t%d\t%d\t%d", nipc4, nipc5, nipc6, wos[i].nipc);
				for (k = 0; k < wos[i].nipc; k++)
				{
					fwprintf(nstat_stream, L"\t%s", &wos[i].ipc[k*MAX_IPC_CODE]);
				}
#endif NEED_TO_BE_CHANGED_FOR_CPC
				fwprintf(nstat_stream, L"\n"); fflush(nstat_stream);
			}
		}
	}
	fflush(nstat_stream);	// added 2018/07/21

	//
	// statistics of link SPxs
	//	
	#define N_YEARS 2200
	double spx_avg[N_YEARS];	// avefage SPx for each year (year is the year of the eminating node)
	int lnk_counts[N_YEARS];
	for (i = 0; i < N_YEARS; i++) { lnk_counts[i] = 0; spx_avg[i] = 0.0; }
	// average SPx variations in years
	for (i = 0; i < nslinks; i++)
	{
		j = slinks[i].year_s;
		spx_avg[j] += slinks[i].spx;
		lnk_counts[j]++;
	}

	int ybegin;
	SYSTEMTIME st;	
	GetSystemTime(&st);
	for (i = 0; i < N_YEARS; i++) { if (lnk_counts[i] != 0) { ybegin = i; break; }}
	fwprintf(nstat_stream, L"\nAverage SPx in years %d ~ %d:\n", ybegin, st.wYear);
	fwprintf(nstat_stream, L"\nYear\tNLinks\tAvgSPX\tln(AvgSPX)\n");
	for (i = ybegin; i <= st.wYear; i++) 
	{
		if (lnk_counts[i] != 0)
			spx_avg[i] = spx_avg[i] / lnk_counts[i];
		else
			spx_avg[i] = 0.0;
		if (spx_avg[i] != 0.0)
			fwprintf(nstat_stream, L"%d\t%d\t%.2f\t%.2f\n", i, lnk_counts[i], spx_avg[i], log(spx_avg[i]));
		else
			fwprintf(nstat_stream, L"%d\t%d\t%.2f\t%.2f\n", i, lnk_counts[i], spx_avg[i], 0.0);
	}

	// histogram of link SPxs
	#define N_SLOTS 25
	int spx_hist[N_SLOTS];
	double max_spx = 0.0;
	double tmp;
	for (i = 0; i < N_SLOTS; i++) spx_hist[i] = 0;
	// find the maximum spx value
	for (i = 0; i < nslinks; i++) { if (slinks[i].spx > max_spx) max_spx = slinks[i].spx; }
	max_spx = log10(max_spx);
	for (i = 0; i < nslinks; i++)
	{
		j = (int)(N_SLOTS * log10(slinks[i].spx) / max_spx);
		//fwprintf(logstream, L"%d ", j); fflush(logstream);
#ifdef THERE_IS_A_BUG_IN_THIS_BLOCK
		if (j == N_SLOTS) 
			spx_hist[N_SLOTS-1]++;	// special case
		else
			spx_hist[j]++;	
#endif THERE_IS_A_BUG_IN_THIS_BLOCK
	}
	fflush(nstat_stream);	// added 2018/07/21

	fwprintf(nstat_stream, L"\nScaling factor for SPx = %d, Maximum SPx (log10) = %.4f\n", (int)scale, max_spx);
	fwprintf(nstat_stream, L"SPx histogram:\n");
	fwprintf(nstat_stream, L"ID\tSPX\tLog10(SPX)\tCount\n");
	for (i = 0; i < N_SLOTS; i++) 
	{
		tmp = i * max_spx / N_SLOTS;
		fwprintf(nstat_stream, L"%d\t%.2f\t%.4f\t%d\n", i+1, pow(10, tmp), tmp, spx_hist[i]);
	}

#ifdef PARTNER_VERSION
	SPX_statistics(nstat_stream);
#endif PARTNER_VERSION

	fclose(nstat_stream);

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_spx(const void *n1, const void *n2)
{
	struct SLINK *t1, *t2;
	
	t1 = (struct SLINK *)n1;
	t2 = (struct SLINK *)n2;
	if (t2->spx > t1->spx)
		return 1;
	else if (t2->spx == t1->spx)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_cpsize(const void *n1, const void *n2)
{
	struct COMP *t1, *t2;
	
	t1 = (struct COMP *)n1;
	t2 = (struct COMP *)n2;
	if (t1->nnodes < t2->nnodes)
		return 1;
	else if (t2->nnodes == t1->nnodes)
		return 0;
	else return -1;
}

//
// find the most significant country for each author
// added 2014/12/12, modified again 2015/10/16
//
int find_significant_country()
{
	int i, k, max, kmax, max_cnt;

	for (i = 0; i < naus; i++)
	{	
		// 1st pass
		max = 0; kmax = -1; 
		for (k = 0; k < authors[i].ncountries; k++)
		{
			if (authors[i].country_cnt[k] >= max)
			{
				max = authors[i].country_cnt[k];
				kmax = k;
			}
		}
		max_cnt = 0;
		// find out the number of countries that have the same miximum count
		for (k = 0; k < authors[i].ncountries; k++)		
			if (authors[i].country_cnt[k] == max) max_cnt++;
		authors[i].location_status = max_cnt;
		if (kmax == -1)
		{
			authors[i].location = -1;
			continue;
		}

		// 2nd pass, make the decision, ignore [VAGUE] and [unknown country] unless they are the only information
		if (authors[i].ncountries == 1 && (authors[i].countries[kmax] == 0 || authors[i].countries[kmax] == 1)) // 0=>[VAGUE], 1=>[unknown country]
		{
			authors[i].location = authors[i].countries[kmax];
			continue;
		}
		max = 0; 
		for (k = 0; k < authors[i].ncountries; k++)
		{
			if (authors[i].countries[k] == 0 || authors[i].countries[k] == 1)	// ignore [VAGUE] and [unknown country]
				continue;
			if (authors[i].country_cnt[k] >= max)
			{
				max = authors[i].country_cnt[k];
				kmax = k;
			}
		}
		authors[i].location = authors[i].countries[kmax];
	}

	return 0;
}

//
// for all authors, find their number of times as single author
//
int count_single_author(int nwos, struct WOS *wos, int naus, AUTHORS *authors)
{
	int i;

	for (i = 0; i < naus; i++) authors[i].nsingle = 0;	// initialization
	for (i = 0; i < nwos; i++) 
	{
		if (wos[i].nau == 1)	// only one author
			authors[wos[i].author[0]].nsingle++;
	}

	return 0;
}

//
// for all authors, find whether the authors have international collaboration in his papers
//
int count_international_collaboration(struct WOS *wos, int naus, AUTHORS *authors)
{
	int i, k;
	int iw;
	int prev;

	for (i = 0; i < naus; i++) 
	{
		authors[i].inter = 0;
		for (k = 0; k < authors[i].np; k++)
		{
			iw = authors[i].paper[k];
			if (wos[iw].ncountries > 1)
				authors[i].inter++;
		}
	}

	return 0;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_tempstr(const void *n1, const void *n2)
{
	struct TEMPSTR *t1, *t2;
	
	t1 = (struct TEMPSTR *)n1;
	t2 = (struct TEMPSTR *)n2;
	if (wcscmp(t2->str, t1->str) < 0)
		return 1;
	else if (wcscmp(t2->str, t1->str) == 0)
		return 0;
	else return -1;
}