//
// searchpath.cpp
//

//
// Revision History:
// 2012/01/11 Modification  : added two global variables "total_paths_1" and "total_paths_2"
// 2012/02/05 Modification  : added to call function find_author_outward_paths()
// 2012/02/07 Modification  : added to call function find_author_outward_nodes()
// 2012/05/20 Modification  : added codes to amplify SPX if the weight is given
//                          : added codes for PATENT_SPX_COUNTRY_IPC_SUMMARY
// 2012/06/12 Added function: added function count_IPC_codes() (for patent analysis purpose)
// 2012/06/23 Modification  : modified the weighted exponential multiplier from 10 to 2.718282 (for Westlaw data)
// 2012/06/26 Modification  : added codes to calculate nw[i].total_out_spx and nw[i].total_in_spx
// 2012/09/24 Fixed problem : changed the expression of the huge number to 9999999999E+30 (since the largest number for mantissa is only 15~16 digits)	
// 2012/09/24 Fixed problem : changed the display of the "scale" from integer to double, so that no more negative value is displayed
// 2012/10/18 Modification  : added code to handle new clan type: Country
// 2012/10/18 Fixed problem : changed to call output_keyword_info() only when the data is WOS data
// 2013/01/14 Added function: added codes to handle "WOS" and "Author" clan codes (for WOS data)
// 2013/01/14 Modification  : changed to handle multiple patent assignees in the clan analysis (was searching for only the first assignees)
// 2013/01/25 Added function: added to show the earliest patent on the main path, and patents earlier than this patent (top 20 cited)
//                            function added: find_earlier_patents()
// 2013/01/27 Modification  : changed to allocate memory for "q", it was using global space
// 2013/05/02 Modification  : added a warning message in the case that the number of links exceed expectation
// 2013/05/03 Modification  : added code to write out SPX and decay (for SPDC) information to the log file
// 2013/05/03 Added function: added function: added code to accept "decay factor" passed from the user interface
// 2013/07/20 Added function: added a new function replace_IPC_codes()
// 2013/11/09 Modification  : reflect the modification on index_of() function (added a new argument) 
// 2014/04/13 Added function: added codes to do clustering (Newman's edge-betweenness method). this is done by calling another program "GroupFinder"
// 2014/05/15 Added funciton: added codes to write clustering results in the log file and to seperate cluster files
// 2014/05/27 Modification  : changed to write out data for the top 10 clusters, but these clusterse has to be greater than 10 articles
// 2014/05/28 Modification  : changed the output file names for cluster data, added cluster size information
// 2014/07/11 Added function: changed to write out data on the main paths (in the original WOS or Scopus format)
// 2014/07/14 Fixed problem : fixed a stack problem in replace_IPC_codes() and count_IPC_codes()
// 2014/07/14 Added function: changed to write out data on the main paths (in the original Thomson Innovation Patent format)
// 2014/10/29 Fixed problem : changed to add a check for inclusion in the network before proceeding to output_subnet() in the branch process 
// 2014/12/21 Added function: added a function combine_local_keyroute_path_queues(),
//                            the purpose is to make the key-route search also returns queues that contain paths from begin the end
// 2014/12/23 Added function: added a function combine_global_keyroute_path_queues(),
//                            the purpose is to make the key-route search also returns queues that contain paths from begin the end
// 2014/12/26 Fixed problem : in key-route global search, the cases when the tail of the key-route is a source in backward search
//                                or the head of the key-route is a sink in forward search are not handled, they are handled now!
//                                this problem does not affect the results of the key-route main paths, but the 'qsize' is wrong.
// 2014/12/26 Added function: added to call critical_transition() function
// 2015/02/02 Fixed problem : increase the memory alloction size for "q_b"
// 2015/02/04 Fixed problem : in prepare_for_global_search(), allocate memory using int64 integer so that memory size will not overflow
// 2015/07/18 Fixed problem : the problem of reversed sign in citation weight for 'flat' method is fixed 
//                                 (the 'flat' results in JASIST paper was wrong, but that did not make much difference on the conclusion) 
// 2015/08/20 Fixed problem : the function consolidate_relationship_list() creates and deletes a temporary file. it is called back-to-back. 
//                            the file system may not be quick enough to delete the temporary file before the next fopen() is called. 
//                            to avoid the problem, change file name every time the function is called. use 'tmpname_counter' to chane file name.
// 2015/08/20 Added function: added to make directories for each cluster and copied cluster data and citation file to the cluster directories
// 2015/10/07 Modification  : moved all codes related to clustering the citation network to the file "clustering.cpp"
// 2016/01/19 Modification  : changed the file opening to Unicode mode (ccs=UTF-8)
// 2016/02/01 Modification  : added codes to check if there is byte-order-mark, this is necessary after the modification of openning file in UTF-8 type
// 2016/03/13 Fixed problem : extend the memory allocated for q_f (10 to 20). there is no problem for q_b (it is 20).
// 2016/03/19 Added function: Added codes to call readwrite_TaiwanTD().
// 2016/04/23 Added to call readwrite_TCI()
// 2016/07/05 Added to call cluster_coword_doc_network()
// 2016/10/13 Added codes to support WEBPAT3 data	
// 2016/12/07 Added more restriction in calling the function write_author_citations()
// 2017/01/12 Correct an error in the function  nw2Pajek(), which was writing to nbrs[] rather than the correct out_nbrs[]
// 2017/01/15 Fixed problem : the problem happens in key-route global search. when the link with the highest SPX is pointing to a sink (rarely happen though), 
//                               a check that relies on the assumption that it is not pointing to a sink is violeted. this causes the relationship list file 
//                               opens at 'append' mode rather than the 'new' mode. this eventually results in wrong main paths in the Pajek file.
// 2017/02/07 Fixed problem : the problem is that in global key-route search, mainp flag is not set properly when
//                               the head end of the specified key-route is a sink or the tail end of the specified key-route is a source. 
//                               this causes main path data not written properly into the file.
// 2017/02/07 Fixed problem : added codes to overwrite the number of top edges provided by the user if there is SPX tie at the end of the list
//                               That is, user specifies key-route 10 and that the 10th, 11th, and the 12th largest link have the same SPX, 
//                               then the program will assume the key-route is 12.
// 2017/03/02 Modification  : enabled clan analysis for all MainPath releases
// 2017/11/02 Added function: for WEBPAT data, added to call find_assignee_total_paths(), find_assignee_outward_nodes(), and find_assignee_total_spx()
// 2018/01/19 Modification  : added conditional compilation check for critical transition codes (so that they do not cause trouble when not doing critical transition check)
// 2018/05/09 Modification  : changed all output file to UTF-8 coding (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8"))
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

extern int nnodes;
extern struct PN *nw;
extern int nwos;
extern struct WOS *wos;
extern int nuspto;
extern struct USPTO *uspto;
extern int nkwde;
extern struct KWORDS *kwde;	// author keywords
extern int nkwid;
extern struct KWORDS *kwid;	// Keyword Plus keywords
extern int ntkeywords;
extern struct TKEYWORDS *tkeyword;	// title keywords
extern int nalias;
extern struct ATABLE *atable;
extern int full_record_type;
extern struct NODE *nodes;
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern wchar_t coauthorfilename[];	// added 2015/10/07
extern wchar_t coworddocfilename[];	// added 2016/07/05
extern int nrgroups;
extern struct RESEARCH_GROUP *rgroups;

extern FILE *logstream;


int find_sources_sinks(int, int);
int global_path_search(double);
int global_path_search_branch(double, int);
int local_path_search_forward(double, int);
int local_path_search_backward(double, int);
int select_starting_edges_forward(int);
int select_starting_edges_backward(int);
int select_starting_edges_keyroute(int, int, struct SLINK *, int, int, int);
int select_starting_edges_forward_branches(int);
int select_starting_edges_backward_branches(int);
int text2Pajek(wchar_t*, wchar_t*, struct ATABLE *, int, int *, int *, int, int, int, int, int, int, int, int);
int replace_init_path(int, int, double);
int add_init_path(int, int, double);
int find_min_acnt(int, int, int);
int fill_the_hole(int, int, int, int);
int to_relationship_list(wchar_t *, int, int, int, int);
int nname_search(struct PN *, int, wchar_t *);
int output_keyword_info(wchar_t *);
int compare_dndx(const void *, const void *);
int compare_kndx(const void *, const void *);
int compare_kcnt(const void *, const void *);
int compare_rls(const void *, const void *);
int prepare_for_global_search();
int to_path_nodes_alias_table(wchar_t *);
int consolidate_relationship_list(wchar_t *);
int find_links(int, struct PN *, int, struct SLINK *);
int key_routes_relationships(wchar_t *, int, struct SLINK *);
int count_IPC_codes(int, int);
int compare_ipccode(const void *, const void *);
int compare_patid(const void *, const void *);
int compare_pattc(const void *, const void *);
int combine_local_keyroute_path_queues(int, SPATH *q, int *, SPATH *);
int combine_global_keyroute_path_queues(int, SPATH *, int, SPATH *, int *, SPATH *);
int find_actual_top_edges(int, int, struct SLINK *);	// added 2017/02/07

extern int floyd_warshall(int, double *t, struct FWDIST *, short *);
extern int find_shortest_paths(int, struct FWDIST *, short *, int, int *, int, int *, int *, struct SPATH *);
extern int compare_alias(const void *, const void *);
extern int compare_docid(const void *, const void *);
extern int alias_search(struct WOS *, int, wchar_t *);
extern int find_author_total_spx();
extern int find_author_total_paths(wchar_t *, int);	// added 2012/01/12
extern int find_assignee_total_paths(wchar_t *);	// added 2017/11/02
extern int find_assignee_outward_nodes();			// added 2017/11/02
extern int find_assignee_total_spx();				// added 2017/11/02
extern int find_author_outward_paths();	// added 2012/02/05
extern int find_author_outward_nodes();	// added 2012/02/07
extern int SPx(int, int, struct PN *, double);
extern int output_subnet(struct PN *, int, int, int, int, wchar_t *);
extern int index_of(int, wchar_t *, wchar_t *);
extern int parse_line(wchar_t *, int, wchar_t *, wchar_t *, double *);
extern int critical_transition(int, struct SPATH *, int, int, struct PN *, int, struct WOS *, int, struct TKEYWORDS *);
extern int read_GroupFinder_results(wchar_t *);
extern int nw2Pajek(int, wchar_t *);
extern int readwrite_WOS(wchar_t *, wchar_t *, int, int);
extern int readwrite_Scopus(wchar_t *, wchar_t *, int, int);
extern int readwrite_TIP(wchar_t *, wchar_t *, int, int);
extern int readwrite_WEBPAT2(wchar_t *, wchar_t *, int, int);
extern int readwrite_WEBPAT3(wchar_t *, wchar_t *, int, int);
extern int readwrite_USPTO(wchar_t *, wchar_t *, int, int);
extern int cluster_citation_network(wchar_t *, int, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
extern int cluster_coauthor_network(wchar_t *, int, wchar_t *, wchar_t *, wchar_t *);
extern int cluster_coassignee_network(wchar_t *, int, wchar_t *, wchar_t *, wchar_t *);
extern int cluster_coword_doc_network(wchar_t *, int, wchar_t *, wchar_t *, wchar_t *);
extern int shrink_author_citation_network(int, struct AUTHORS *, int, struct RESEARCH_GROUP *);
extern int write_author_citations(wchar_t *, int);	// added 2015/06/23, 2015/07/16, moved to this file from "text2Pajek.cpp" 2015/10/21
extern int rgroup_citation_to_Pajek_file(wchar_t *, int, struct RESEARCH_GROUP *, wchar_t *);	// added 2015/10/21
extern int readwrite_TaiwanTD(wchar_t *, wchar_t *, int, int);	// added 2016/03/19
extern int readwrite_TCI(wchar_t *, wchar_t *, int, int);	// added 2016/04/23
extern int apply_relevancy_strategy(int);

int nsources;
int *sources;	// memory to save index of all sources
int nsinks;
int *sinks;
struct FWDIST *fw;	// the result of the FLoyd-Warshall algorithm
short *fw_midpt;	// 2 bytes
double scale = 1.0;
int n_sources, n_sinks, n_isolates, n_intermediates;
double total_paths_1;	// counted from the source side
double total_paths_2;	// counted from the sind side
int tmpname_counter = 0;	// this is used to provide different temporary file names

#define TOLERANCE_GRADIENT 0.0
//#define TOLERANCE_GRADIENT 0.05

#define F_NEW 1
#define F_APPEND 2

int ndataq;
int qsize;	// qsize varies, but can not be larger than MAX_PATH_QUEUE+EXT_PATH_QUEUE
//struct SPATH q[MAX_PATH_QUEUE+EXT_PATH_QUEUE];	// removed 2013/01/27
struct SPATH *q;									// added 2012/01/27
int ind_min;	// the index of path which has minimum average count
double acnt_min;	// minimum average SPx count among all paths in the queue

extern int nslinks;
extern struct SLINK *slinks;	// holds all the links
extern int release_id;
extern int naus;
extern struct AUTHORS *authors;	// author name array

extern int acyclic_check();
extern int dissimilarity(int, struct PN *, int, int, double *);
extern int network_stats(wchar_t *, int, struct PN *, int, struct WOS *, int, struct USPTO *, int, int);
extern int link_importance();
extern int parse_clan_spec(wchar_t *, struct CLAN_PARAMS *pm, int *);

#define MAX_RELATIONS 400
struct RELATIONSHIPS { 	wchar_t r[MAX_RELATIONS]; };

//#define DEBUG

#define NPARAMS 5

wchar_t pt_typestr[5][20] = { L"IPC", L"UPC", L"Assignee", L"Country", L"?????" };
wchar_t pp_typestr[3][20] = { L"WOS", L"Author", L"?????" };

//
// find main paths
//
int mainpaths(wchar_t *frecordname, wchar_t *tname, wchar_t *branchname, wchar_t *citationname, int stype, int mptype, double mtie_tolerance, double mnpths,
			  int bptype, double btie_tolerance, double bnpths, int byear, int eyear,
			  int n_top_edges_local, int n_top_edges_global, double decay, wchar_t *clanfilename, wchar_t *dpath, int take_weight, struct ATABLE *atable, 
			  wchar_t *rpajekname, wchar_t *gfname, int groupfinderoptions, int clusteringcoauthornetworkoptions, int relevancystrategy)
{
	int i, j, k, m, p, kk, ai;
	int first_time_flag;	// added 2017/01/15
	int ndx, ndx2, ret;
	int nnd_mainpath, nsn_mainpath;		
	wchar_t tmain[30], tbranch[30];
	struct CLAN_PARAMS pm[NPARAMS];
	int nclan;
	wchar_t types[50];
	int at;
	wchar_t spxname[FNAME_SIZE];

	q = (struct SPATH *)Jmalloc((MAX_PATH_QUEUE+EXT_PATH_QUEUE) * sizeof(struct SPATH), L"mainpaths: q");	// added 2013/01/27
	if (q == NULL) return MSG_NOT_ENOUGH_MEMORY;

	for (i = 0; i < NPARAMS; i++) { pm[i].bratio = 1.0; pm[i].ctype = 0; }
	if (clanfilename[0] == '\0')
		nclan = 0;
	else
	{
		ret = parse_clan_spec(clanfilename, pm, &nclan);
		if (ret != 0)
			return ret;
	}

	ret = find_sources_sinks(byear, eyear);
	if (ret != 0) return ret;

	ret = acyclic_check();
	if (ret != 0) return ret;

#ifdef DISSIMLARITY
	// calculate dissimilarity of inward and outward connenctions for all nodes
	double dissim;
	fwprintf(logstream, L"\nDissimilarity\n");
	for (i = 0; i < nnodes; i++)
	{
		fwprintf(logstream, L"%s[%d]: ", nw[i].alias, nw[i].out_deg);
		for (k = 0; k < nw[i].out_deg; k++)
		{
			ret = dissimilarity(nnodes, nw, i, nw[i].out_nbrs[k], &dissim);
			if (ret != 0) return ret;
			if (dissim < 5) fwprintf(logstream, L"%.2f\t", dissim);
			nw[i].out_dissim[k] = dissim;
		}
		fwprintf(logstream, L"\n");
	}
	fwprintf(logstream, L"\n");
	for (i = 0; i < nnodes; i++)
	{
		fwprintf(logstream, L"%s[%d]: ", nw[i].alias, nw[i].in_deg);
		for (k = 0; k < nw[i].in_deg; k++)
		{
			ret = dissimilarity(nnodes, nw, i, nw[i].in_nbrs[k], &dissim);
			if (ret != 0) return ret;
			if (dissim < 5) fwprintf(logstream, L"%.2f\t", dissim);
			nw[i].in_dissim[k] = dissim;
		}
		fwprintf(logstream, L"\n");
	}
#endif DISSIMLARITY

	//link_importance();
	wchar_t spx_type[20], tmps[FNAME_SIZE], *sp;
	if (stype == S_SPC)		// added 2013/05/03
	{
		fwprintf(logstream, L"\nTraversal count algorithm: SPC\n"); 
		wcscpy(spx_type, L"SPC");
	}
	else if (stype == S_SPLC)
	{
		fwprintf(logstream, L"\nTraversal count algorithm: SPLC\n");
		wcscpy(spx_type, L"SPLC");
	} 
	else if (stype == S_SPNP)
	{
		fwprintf(logstream, L"\nTraversal count algorithm: SPNP\n"); 
		wcscpy(spx_type, L"SPNP");
	}
	else if (stype == S_SPAD)
	{
		fwprintf(logstream, L"\nTraversal count algorithm: SPAD\n");
		fwprintf(logstream, L"Decay = %.4f\n", decay);
		swprintf(spx_type, L"SPAD %.4f", decay);
	}
	else if (stype == S_SPGD)
	{
		fwprintf(logstream, L"\nTraversal count algorithm: SPGD\n");
		fwprintf(logstream, L"Decay = %.4f\n", decay);
		swprintf(spx_type, L"SPGD %.4f", decay);
	}
	else if (stype == S_SPHD)
	{
		fwprintf(logstream, L"\nTraversal count algorithm: SPHD\n");
		swprintf(spx_type, L"SPHD", decay);
	}
	ret = SPx(stype, nnodes, nw, decay);
	if (ret != 0) return ret;

#ifdef XXX
	//========= temporary codes
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_spx[k]  /= 10000000000.0;
		for (k = 0; k < nw[i].out_deg; k++)
			nw[i].out_spx[k] /= 10000000000.0;
	}
	//=========
#endif XXX

	wcscpy(tmps, citationname);
	sp = tmps; while (*sp != '\0') sp++;	// move to the end of the file name
	sp -= 4; *sp ='\0';	// ignore the ".paj." at the end
	swprintf(spxname, L"%s %s.paj", tmps, spx_type);
	nw2Pajek(1, spxname);	// write out the citation network with SPX in weight

	// now that SPX has been calculated, we want to apply the relevancy strategy
	// that is, adjust the SPX values with the relevancy for each link
	// the code for relevancy strategy is added 2016/05/07
	apply_relevancy_strategy(relevancystrategy);

	wcscpy(tmps, citationname);
	sp = tmps; while (*sp != '\0') sp++;	// move to the end of the file name
	sp -= 4; *sp ='\0';	// ignore the ".paj." at the end
	swprintf(spxname, L"%s Relevancy.paj", tmps);
	nw2Pajek(2, spxname);	// write out the citation network with relevancy in weight

	// we want to cluster the citation network here (calling the GroupFinder program), the results are stored in the nw[] array
	// the clustering codes are added 2014/04/07
	wchar_t tmpname[FNAME_SIZE];
	if (groupfinderoptions != GF_DISABLE)
	{
		if (groupfinderoptions == GF_UNDIRECTED_WEIGHTED || groupfinderoptions == GF_DIRECTED_WEIGHTED)	// NOTE: this will not happen because there is no such option in the main menu
			wcscpy(tmpname, spxname);	// for weighted option, use the file with SPX values
		else if (groupfinderoptions == GF_UNDIRECTED_UNWEIGHTED || groupfinderoptions == GF_DIRECTED_UNWEIGHTED)	
		{
			wcscpy(tmpname, gfname);
			cluster_citation_network(tmpname, groupfinderoptions, dpath, frecordname, tname, citationname);	// added 2015/10/07
		}
		else if (groupfinderoptions == CCoword_UNWEIGHTED)	
		{
			wcscpy(tmpname, gfname);
			cluster_coword_doc_network(coworddocfilename, GF_UNDIRECTED_UNWEIGHTED, dpath, frecordname, tname);	// added 2016/07/05
		}
	}
#ifdef MOVED_TO_CLUSTERING.CPP	// 2015/10/07
	wchar_t tmpname[FNAME_SIZE];
	wchar_t dirname[FNAME_SIZE];
	wchar_t citname[FNAME_SIZE];
	wchar_t destname[FNAME_SIZE];
	if (groupfinderoptions != GF_DISABLE)
	{
		if (groupfinderoptions == GF_UNDIRECTED_WEIGHTED || groupfinderoptions == GF_DIRECTED_WEIGHTED)	// NOTE: this will not happen because there is no such option in the main menu
			wcscpy(tmpname, spxname);	// for weighted option, use the file with SPX values
		else
			wcscpy(tmpname, gfname);
		execute_GroupFinder(tmpname, groupfinderoptions);
		read_GroupFinder_results(tmpname);
		fwprintf(logstream, L"\nClustering results:\n");
		fwprintf(logstream, L"\Partition\tIndex\tID\tAlias\n");
		for (i = 0; i < nnodes; i++)	// printf the clustering results, added 2014/05/15
			fwprintf(logstream, L"%d\t%d\t%s\t%s\n", nw[i].partition, i, nw[i].name, nw[i].alias);
		fwprintf(logstream, L"\n");
		struct CMEMBER *memb;	//  following codes are added 2014/05/14
		memb = (struct CMEMBER *)Jmalloc(nnodes * sizeof(struct CMEMBER), L"mainpath: memb");
		for (i = 0; i < nnodes; i++) { memb[i].id = i; memb[i].mcnt = 0; }
		for (i = 0; i < nnodes; i++) memb[nw[i].partition].mcnt++;
		qsort((void *)memb, (size_t)nnodes, sizeof(struct CMEMBER), compare_mcnt);
		//for (i = 0; i < nnodes; i++)
		//	fwprintf(logstream, L"%d %d\n", i, memb[i].mcnt);
		//for (i = 0; i < nwos; i++) fwprintf(logstream, L"%s\n", wos[i].docid);
		for (i = 0; i < 20; i++)	// write out data for the top 10 clusters, changed to top 20 clusters, 2015/08/20
		{
			if (memb[i].mcnt < 10) continue;	// ignore small clusters
			// make a cluster directory, added 2015/08/20		
			swprintf(dirname, L"%s\Cluster%d-%d", dpath, memb[i].id, memb[i].mcnt);
			if (CreateDirectory(dirname, NULL) != 0)	// the directory does not exist
				fwprintf(logstream, L"Directory \"%s\" created successfully.\n\n", dirname);
			else
				fwprintf(logstream, L"Directory \"%s\" already exist.\n\n", dirname);
			if (full_record_type == WOS_DATA)
			{
				swprintf(tmpname, L"%s\\Cluster%d-%d.txt", dirname, memb[i].id, memb[i].mcnt);
				readwrite_WOS(frecordname, tmpname, RW_PARTITION, memb[i].id);
			}
			else if (full_record_type == SCOPUS_DATA)
			{
				swprintf(tmpname, L"%s Cluster%d-%d.csv", tname, memb[i].id, memb[i].mcnt);
				readwrite_Scopus(frecordname, tmpname, RW_PARTITION, memb[i].id);
			}
			else if (full_record_type == THOMSON_INNOVATION_DATA)
			{
				swprintf(tmpname, L"%s Cluster%d-%d.csv", tname, memb[i].id, memb[i].mcnt);
				readwrite_TIP(frecordname, tmpname, RW_PARTITION, memb[i].id);
			}
			else if (full_record_type == USPTO_DATA)
			{
				swprintf(tmpname, L"%s Cluster%d-%d.txt", tname, memb[i].id, memb[i].mcnt);
				readwrite_USPTO(frecordname, tmpname, RW_PARTITION, memb[i].id);
			}
			// also copy the citation file to the directory, added 2015/08/20
			wcscpy(tmps, citationname);
			sp = tmps; while (*sp != '\0') sp++;	// move to the end of the file name
			sp -= 4; *sp ='\0';	// ignore the ".paj." at the end
			swprintf(citname, L"%s.txt", tmps);	// restore the original citation file name
			wcscpy(tmps, citname);
			sp = tmps; while (*sp != '\0') sp++;	// move to the end of the file name
			while (*sp != '\\') sp--;				// move back to the begining of the citation file name
			swprintf(destname, L"%s%s", dirname, sp);	// restore the original citation file name
			ret = CopyFile(citname, destname, FALSE);
			if (ret == 0) fwprintf(logstream, L"Failed to copy file [%s]==>[%s].\n\n", citname, destname);
			else fwprintf(logstream, L"Successfully copied file [%s]==>[]%s.\n\n", citname, destname);
		}
	}
#endif MOVED_TO_CLUSTERING.CPP	// 2015/10/07

	// find the total_in_spx and total_out_spx for each node, 
	// in theory, these two variables should be exactly the same, added 2012/06/26
	for (i = 0; i < nnodes; i++)
	{
		nw[i].total_out_spx = 0.0;
		if (nw[i].out_deg != 0)	// not sinks
		{
			for (k = 0; k < nw[i].out_deg; k++)
				nw[i].total_out_spx += nw[i].out_spx[k];
		}
		else // for sinks, take their in_spx as the out_spx
		{
			for (k = 0; k < nw[i].in_deg; k++)
				nw[i].total_out_spx += nw[i].in_spx[k];
		}
		nw[i].total_in_spx = 0.0;
		if (nw[i].in_deg != 0)	// not sources
		{
			for (k = 0; k < nw[i].in_deg; k++)
				nw[i].total_in_spx += nw[i].in_spx[k];
		}
		else
		{
			for (k = 0; k < nw[i].out_deg; k++)
				nw[i].total_in_spx += nw[i].out_spx[k];
		}
		//fwprintf(logstream, L"%d[%f, %f]\n", i+1, nw[i].total_in_spx, nw[i].total_out_spx);
	}

	// find the total number of paths, added 2012/01/11
	total_paths_1 = 0.0;
	for (i = 0; i < nnodes; i++)
	{
		if (nw[i].type == T_SOURCE)
		{
			for (k = 0; k < nw[i].out_deg; k++)
				total_paths_1 += nw[i].out_spx[k];
		}
	}
	fprintf(logstream, "Total number of paths (counted from sources) = %f\n", total_paths_1);
	total_paths_2 = 0.0;
	for (i = 0; i < nnodes; i++)
	{
		if (nw[i].type == T_SINK)
		{
			for (k = 0; k < nw[i].in_deg; k++)
				total_paths_2 += nw[i].in_spx[k];
		}
	}
	fprintf(logstream, "Total number of paths (counted from sinks)   = %f\n", total_paths_2);


	// The Clan Analysis requires adjusting SPx values
	for (i = 0; i < nnodes; i++) nw[i].special = 0;
	int cslen, mcnt;
	// DELTA project has a Clan Analysis fuction that boost the SPx value for specified type of nodes
	//if (release_id == 2 && nclan != 0 && (full_record_type == WOS_DATA || full_record_type == USPTO_DATA || 
	//	full_record_type == THOMSON_INNOVATION_DATA || full_record_type == WEBPAT2_DATA  || full_record_type == WEBPAT3_DATA || 
	//	full_record_type == PGUIDER_DATA))	
	if (nclan != 0)	// enabled clan analysis for all releases, 2017/03/02
	{
		fwprintf(logstream, L"\nNodes that meet the specified clan codes ");
		for (i = 0; i < nclan; i++)
		{ 
			if (full_record_type == WOS_DATA)
				fwprintf(logstream, L"{%s, %f, %s} ", pp_typestr[pm[i].ctype-1], pm[i].bratio, pm[i].ccode);
			else
				fwprintf(logstream, L"{%s, %f, %s} ", pt_typestr[pm[i].ctype-1], pm[i].bratio, pm[i].ccode);
		}
		fwprintf(logstream, L":\n");
		// mark the nodes that meet the code specification
		mcnt = 0;
		for (i = 0; i < nnodes; i++)	// for each node
		{
			m = nw[i].ndx2wos;	// get the node's index in the wos[] and uspto[] array
			for (kk = 0; kk < nclan; kk++)	// for each clan specification
			{
				cslen = (int)wcslen(pm[kk].ccode);
				if (pm[kk].ctype == CTYPE_UPC && full_record_type != WOS_DATA)
				{
					for (j = 0; j < uspto[m].nusc; j++)
					{
						if (wcsncmp(pm[kk].ccode, &uspto[m].usc[MAX_USC_CODE*j], cslen) == 0)
						{
							if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
							else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
							fwprintf(logstream, L"%s\t%s\tUPC: %s\n",nw[i].alias, uspto[m].pid, &uspto[m].usc[MAX_USC_CODE*j]);
							//break;	// only need to know that one of the code meets the specification, removed 2013/01/14
						}
					}
				}
				else if (pm[kk].ctype == CTYPE_IPC && full_record_type != WOS_DATA)	
				{
					for (j = 0; j < uspto[m].nipc; j++)
					{
						if (wcsncmp(pm[kk].ccode, &uspto[m].ipc[MAX_IPC_CODE*j], cslen) == 0)
						{
							if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
							else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
							fwprintf(logstream, L"%s\tIPC: %s\n",nw[i].alias, &uspto[m].ipc[MAX_IPC_CODE*j]);
							//break;	// only need to know that one of the code meets the specification, removed 2013/01/14
						}
					}
				}
				else if (pm[kk].ctype == CTYPE_ASSIGNEE && full_record_type != WOS_DATA)	// modified 2017/06/25
				{
					for (ai = 0; ai < uspto[m].nassignee; ai++)
					{
						at = index_of(0, assignees[uspto[m].assignee[ai]].name, pm[kk].ccode);	// modified 2012/06/09, verified 2012/09/11
						if (at != -1)
						{
							if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
							else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
							fwprintf(logstream, L"%s\tASSIGNEE: %s\n", nw[i].alias, assignees[uspto[m].assignee[ai]].name); // modified 2012/06/09, verified 2012/09/11
						}
					}
				}
				else if (pm[kk].ctype == CTYPE_COUNTRY && full_record_type != WOS_DATA)
				{
					if (wcsncmp(pm[kk].ccode, uspto[m].country, cslen) == 0)
					{
						if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
						else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
						fwprintf(logstream, L"%s\tCOUNTRY: %s\n",nw[i].alias, uspto[m].country);
						//break;	// only need to know that one of the code meets the specification, removed 2013/01/14
					}
				}
				else if (pm[kk].ctype == PTYPE_WOS && full_record_type != WOS_DATA)	
				{
					if (pm[kk].ccode[0] == L'@')	// indicates that the WOS ID list is in the file with the name after the '@' sign
					{
						FILE *sstream;	
						int idlen;
						wchar_t wosname[FNAME_SIZE];	
						wchar_t wosid[LBUF_SIZE+1];
						swprintf(wosname, L"%s.txt", &pm[kk].ccode[1]);
						if (_wfopen_s(&sstream, wosname, L"rt") != 0)	// open the WOS ID list file, but failed	
							fwprintf(logstream, L"Can not open WOS list file: %s\n",wosname);	// print the error message and do nothing
						else
						{
							while (TRUE)	// read the WOS IDs
							{		
								if(fgetws(wosid, LBUF_SIZE, sstream) == NULL)
									break;
								if (wosid[0] == '\n' || wosid[0] == '\r' || wosid[0] == ' ' || wosid[0] == '\t')
									continue;
								idlen = (int)wcslen(wosid) - 1;	// less the line feed at the end
								if (wcsncmp(wosid, wos[m].docid, idlen) == 0)
								{
									if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
									else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
									fwprintf(logstream, L"%s\tWOS: %s\n",nw[i].alias, wos[m].docid);
								}
							}
							fclose(sstream);	// here, file will be opened and closed nnodes times!! can optimized the code later!
						}
					}
					else
					{
						if (wcsncmp(pm[kk].ccode, wos[m].docid, cslen) == 0)	// the WOS ID is given directly
						{
							if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
							else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
							fwprintf(logstream, L"%s\tWOS: %s\n",nw[i].alias, wos[m].docid);
						}
					}
				}
				else if (pm[kk].ctype == PTYPE_AUTHOR && full_record_type != WOS_DATA)	
				{
					for (ai = 0; ai < wos[m].nau; ai++)
					{
						at = index_of(0, authors[wos[m].author[ai]].name, pm[kk].ccode);
						if (at != -1)	// found the matching author
						{
							if (nw[i].special != 1) { nw[i].special = 1; nw[i].bratio = pm[kk].bratio; mcnt++; }
							else if (nw[i].bratio < pm[kk].bratio) nw[i].bratio = pm[kk].bratio;	// assign the larger bratio to this node
							fwprintf(logstream, L"%s\tAUTHOR: %s\n", nw[i].alias, authors[wos[m].author[ai]].name); 
						}
					}
				}
			}
		}
		if (mcnt == 0) // notify users if the specified clan node is not found	// added 2011/06/11
		{
			//System::String^ mymsg1 = "";		 
			//for (p = 0; p < (int)wcslen(clan_code); p++) 
			//	mymsg1 = System::String::Concat(mymsg1, System::Convert::ToString(clan_code[p]));
			if (full_record_type == WOS_DATA)
			{
				System::String^ mymsgpp = "No paperss matching the specificed clan codes.";
				System::Windows::Forms::MessageBox::Show(mymsgpp);
			}
			else	
			{
				System::String^ mymsgpt = "No patents matching the specificed clan codes.";
				System::Windows::Forms::MessageBox::Show(mymsgpt);
			}
		}
		// boost the SPx according to the status of the node pairs
		fwprintf(logstream, L"\n");
		double br1, br2;
		for (i = 0; i < nnodes; i++)
		{
			for (k = 0; k < nw[i].out_deg; k++)
			{
				j = nw[i].out_nbrs[k];
				if (nw[i].special == 1) br1 = nw[i].bratio; else br1 = 1.0;
				if (nw[j].special == 1) br2 = nw[j].bratio; else br2 = 1.0;
				if (nw[i].special == 1 || nw[j].special == 1)
					fwprintf(logstream, L"%s=>%s boosted from %.0f to %.0f.\n", nw[i].alias, nw[j].alias, nw[i].out_spx[k], (br1 + br2)*0.5*nw[i].out_spx[k]); 
				nw[i].out_spx[k] = (br1 + br2) * 0.5 * nw[i].out_spx[k];
			}
			for (k = 0; k < nw[i].in_deg; k++)
			{
				j = nw[i].in_nbrs[k];
				if (nw[i].special == 1) br1 = nw[i].bratio; else br1 = 1.0;
				if (nw[j].special == 1) br2 = nw[j].bratio; else br2 = 1.0;
				if (nw[i].special == 1 || nw[j].special == 1)
					fwprintf(logstream, L"%s=>%s boosted from %.0f to %.0f.\n", nw[j].alias, nw[i].alias, nw[i].in_spx[k], (br1+br2)*0.5*nw[i].in_spx[k]); 
				nw[i].in_spx[k] = (br1 + br2) * 0.5 * nw[i].in_spx[k];	
			}
		}
	}
	
#ifdef PATENT_SPX_COUNTRY_IPC_SUMMARY
	double total_in_spx, total_out_spx;
	fwprintf(logstream, L"\nPatent\tCOUNTRY\tSPC\tNIPC4\tNIPC7\tASSIGNEE1\tASSIGNEE2\tASSIGNEE3\tIPC\n");
	for (i = 0; i < nnodes; i++)
	{
		total_out_spx = 0.0;
		for (k = 0; k < nw[i].out_deg; k++)
			total_out_spx += nw[i].out_spx[k];
		total_in_spx = 0.0;
		for (k = 0; k < nw[i].in_deg; k++)
			total_in_spx += nw[i].in_spx[k];
		m = nw[i].ndx2wos;
		if (total_out_spx != 0)
			fwprintf(logstream, L"%s\t%s\t%.0f\t", nw[i].name, wos[m].country, total_out_spx);
		else
			fwprintf(logstream, L"%s\t%s\t%.0f\t", nw[i].name, wos[m].country, total_in_spx);
		int nipc; 
		nipc = count_IPC_codes(m, 4);	// check only the 4 leading characters
		fwprintf(logstream, L"%d\t", nipc);
		nipc = count_IPC_codes(m, 7);	// check only the 7 leading characters
		fwprintf(logstream, L"%d\t", nipc);
		for (k = 0; k < wos[m].nde; k++)	// write assignee inofrmation
			fwprintf(logstream, L"%s\t", assignees[wos[m].DE[k]].name);
			//fwprintf(logstream, L"%d\t", wos[m].DE[k]);
		while (k < 3) { k++; fwprintf(logstream, L"\t");	}	// assuminng that the maximum number of asssignees of a patent is 3
		for (k = 0; k < wos[m].nipc; k++)	// write IPC inofrmation
			fwprintf(logstream, L"%s\t", &wos[m].ipc[MAX_IPC_CODE*k]);
		fwprintf(logstream, L"\n");
	}
#endif PATENT_SPX_COUNTRY_IPC_SUMMARY

	if (clusteringcoauthornetworkoptions == CCoauthor_UNWEIGHTED)
		cluster_coauthor_network(coauthorfilename, GF_UNDIRECTED_UNWEIGHTED, dpath, frecordname, tname);	// added 2015/10/08

#ifdef NOT_YET_READY
	if (groupfinderoptions == CCoassignee_UNWEIGHTED)
		cluster_coassignee_network(coassigneefilename, GF_UNDIRECTED_UNWEIGHTED, dpath, frecordname, tname);	// added 2017/03/26
#endif NOT_YET_READY

#ifdef DEBUG
	for (i = 0; i < naus; i++)
	{
		if (authors[i].ndx1 == -1) continue;	// check only 1st authors
		//fwprintf(logstream, L"###%d %d ==> degree=%d\n", i+1, authors[i].ndx1+1, authors[i].degree);
		for (k = 0; k < authors[i].degree; k++)
		{
			if (authors[i].ndx1+1 < authors[authors[i].nbrs[k]].ndx1)	// display only one direction
			fwprintf(logstream, L"###\t%05d\t%05d\t%.2f\n", authors[i].ndx1+1, authors[authors[i].nbrs[k]].ndx1+1, authors[i].weight[k]);
		}
	}
#endif DEBUG

	if (full_record_type != TAIWAN_TD_DATA && full_record_type != WESTLAW_DATA && full_record_type != WESTLAW_DATA2 &&
		full_record_type != USPTO_DATA && full_record_type != WEBPAT2_DATA && full_record_type != WEBPAT3_DATA)	// added 2016/01/18, 2016/12/07
	{
		write_author_citations(tname, 0);	// added 2015/06/23, output author citation network that contains all authors
		write_author_citations(tname, 1);	// added 2015/07/16, output author citation network that contains only authors that have been the 1st author 
	}

#ifdef DEBUG	// check the citation linkages
	for (i = 0; i < naus; i++)
	{
		if (authors[i].ndx1 == -1) continue;	// check only 1st authors
		fwprintf(logstream, L"%d %d ==> ctn_outdegree=%d\n", i+1, authors[i].ndx1+1, authors[i].ctn_outdegree);
		for (k = 0; k < authors[i].ctn_outdegree; k++)
			fwprintf(logstream, L"#\t%05d\t%05d\t%.2f\n", authors[i].ndx1+1, authors[authors[i].ctn_outnbrs[k]].ndx1+1, authors[i].ctn_outweight[k]);
	}
#endif DEBUG

	shrink_author_citation_network(naus, authors, nrgroups, rgroups);		// added 2015/10/21
	
	wchar_t dirname[FNAME_SIZE];
	swprintf(tmps, L"research groups");
	swprintf(dirname, L"%s\Author Subnetworks", dpath);
	swprintf(tmpname, L"%s\\Research Group Citation.paj", dirname);
	rgroup_citation_to_Pajek_file(tmpname, nrgroups, rgroups, tmps);	// added 2015/10/21, write out the group citation network to a Pajek file

	find_author_total_spx();	// to obtain an author's total_in_spx and total_out_spx, added the call back on 2014/12/11

#ifdef AUTHOR_TOTAL_SPX			// m-index calculation
	if (clusteringcoauthornetworkoptions)	// this check is added 2015/11/03
	{
		find_author_outward_nodes();// added 2012/02/07
		find_author_total_paths(tname, clusteringcoauthornetworkoptions);	// added 2012/01/12 to replace find_author_total_spx(), added the 2nd parameter, 2015/10/19
	//find_author_outward_paths();// added 2012/02/05 to replace find_author_total_paths() and the even older find_author_total_spx()
	}
#endif AUTHOR_TOTAL_SPX

#ifdef ASSIGNEE_TOTAL_SPX			// m-index calculation
	if (full_record_type == WEBPAT3_DATA)	
	{
		find_assignee_total_spx();			// added 2017/1102
		find_assignee_outward_nodes();		// added 2017/11/02
		find_assignee_total_paths(tname);	// added 2017/11/02
	}
#endif ASSIGNEE_TOTAL_SPX

#ifdef DISSIMLARITY
	// adjust spx by the dissimilarity scores
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_spx[k] = nw[i].in_spx[k] * (1.0 - nw[i].in_dissim[k]);
		for (k = 0; k < nw[i].out_deg; k++)
			nw[i].out_spx[k] = nw[i].out_spx[k] * (1.0 - nw[i].out_dissim[k]);
	}
#endif DISSIMLARITY

	// adjust spx according to the "Citation level consideration" options for Westlaw data
	double multiplier, pp, wght;
	if (full_record_type == WESTLAW_DATA || full_record_type == WESTLAW_DATA2)
	{
		for (i = 0; i < nnodes; i++)
		{
			for (k = 0; k < nw[i].in_deg; k++)
			{
				if (take_weight == LINEAR_IGNORE_SIGN || take_weight == LINEAR_TAKE_SIGN)	// handles "linear"
					nw[i].in_spx[k] = nw[i].in_spx[k] * nw[i].in_weight[k];
				else if (take_weight == EXPONENTIAL_IGNORE_SIGN || take_weight == EXPONENTIAL_TAKE_SIGN)	// handles "exponential"
				{
					multiplier = 1.0; 
					if (nw[i].in_weight[k] >= 0.0) wght = (int)nw[i].in_weight[k]; else wght = -1 * (int)nw[i].in_weight[k];
					for (pp = 1; pp < wght; pp++) multiplier *= 2.718282;
					if (nw[i].in_weight[k] >= 0.0)
						nw[i].in_spx[k] = nw[i].in_spx[k] * multiplier;
					else
						nw[i].in_spx[k] = -1 * nw[i].in_spx[k] * multiplier;
				}
				// change the sign of SPX but not the weight
				//if (take_weight == FLAT_TAKE_SIGN || take_weight == LINEAR_TAKE_SIGN || take_weight == EXPONENTIAL_TAKE_SIGN)
				if (take_weight == FLAT_IGNORE_SIGN || take_weight == LINEAR_TAKE_SIGN || take_weight == EXPONENTIAL_TAKE_SIGN)	// 2015/07/18, this fixed a bug in 'flat' method
					nw[i].in_spx[k] = nw[i].in_spx[k];	// keep the sign (this line really does nothing)
				else
				{
					if (nw[i].in_weight[k] < 0.0)	// enforce the negative citation to positive
						nw[i].in_spx[k] = -1 * nw[i].in_spx[k];	// change to positive
				}
				//fwprintf(logstream, L"[%f] ", nw[i].in_weight[k]);
			}
			for (k = 0; k < nw[i].out_deg; k++)
			{
				if (take_weight == LINEAR_IGNORE_SIGN || take_weight == LINEAR_TAKE_SIGN)	// handles "linear"
					nw[i].out_spx[k] = nw[i].out_spx[k] * nw[i].out_weight[k];
				else if (take_weight == EXPONENTIAL_IGNORE_SIGN || take_weight == EXPONENTIAL_TAKE_SIGN)	// handles "exponential"
				{
					multiplier = 1.0; 
					if (nw[i].out_weight[k] >= 0.0) wght = (int)nw[i].out_weight[k]; else wght = -1 * (int)nw[i].out_weight[k];
					for (pp = 1; pp < wght; pp++) multiplier *= 2.718282;
					if (nw[i].out_weight[k] >= 0.0)
						nw[i].out_spx[k] = nw[i].out_spx[k] * multiplier;
					else
						nw[i].out_spx[k] = -1 * nw[i].out_spx[k] * multiplier;
				}
				// change the sign of SPX but not the weight
				//if (take_weight == FLAT_TAKE_SIGN || take_weight == LINEAR_TAKE_SIGN || take_weight == EXPONENTIAL_TAKE_SIGN)
				if (take_weight == FLAT_IGNORE_SIGN || take_weight == LINEAR_TAKE_SIGN || take_weight == EXPONENTIAL_TAKE_SIGN)	// 2015/07/18, this fixed a bug in 'flat' method
					nw[i].out_spx[k] = nw[i].out_spx[k];	// keep the sign (this line really does nothing)
				else
				{
					if (nw[i].out_weight[k] < 0.0)	// enforce the negative citation to positive
						nw[i].out_spx[k] = -1 * nw[i].out_spx[k];	// change to positive
				}
				//fwprintf(logstream, L"[%f] ", nw[i].out_weight[k]);
			}
		}
	}

	ret = network_stats(tname, nnodes, nw, nwos, wos, nuspto, uspto, stype, clusteringcoauthornetworkoptions);
	if (ret != 0) return ret;

#ifdef PATENT_SPX_COUNTRY_IPC_SUMMARY
	exit(0);
#endif PATENT_SPX_COUNTRY_IPC_SUMMARY

//#ifdef AUTHOR_TOTAL_SPX
//	exit(0);
//#endif AUTHOR_TOTAL_SPX

	// find a proper scaling factor for the link weight (otherwise the link weight too big will cause trouble in drawing the network)
	int cnt;
	double sum, avg;
	sum = 0.0; cnt = 0;
	for (i = 0; i < 100; i++) { cnt++; sum += slinks[i].spx; }	// use the average value of top 100 SPxs
	avg = sum / cnt;
	scale = 1.0;
	while ((avg / 10.0) >= 1.0) { avg = avg / 10.0; scale = scale * 10.0; }
	fwprintf(logstream, L"\nScale for the link weight = %.0f\n\n", scale);	// modified 2012/09/24

	// prepare file names for the output files
	wchar_t mnm[FNAME_SIZE], mname[FNAME_SIZE], mnamep[FNAME_SIZE], anm[FNAME_SIZE], aname[FNAME_SIZE];
	if (mptype == P_LOCAL_F)
		swprintf_s(tmain, 30, L"Local %.2f Forward", mtie_tolerance); 
	else if (mptype == P_LOCAL_B)
		swprintf_s(tmain, 30, L"Local %.2f Backward", mtie_tolerance);
	else if (mptype == P_LOCAL_KEY_ROUTE)
		swprintf_s(tmain, 30, L"Local %.2f Key-Route %d", mtie_tolerance, n_top_edges_local);
	else if (mptype == P_GLOBAL_KEY_ROUTE)
		swprintf_s(tmain, 30, L"Global %.0f Key-Route %d", mnpths, n_top_edges_global);	
	else
		swprintf_s(tmain, 30, L"Global %.0f", mnpths);	
	if (bptype == P_LOCAL_F)
		swprintf_s(tbranch, 30, L"Local %.2f Forward", btie_tolerance); 
	else if (bptype == P_LOCAL_B)	
		swprintf_s(tbranch, 30, L"Local %.2f Backward", btie_tolerance); 
	else if (bptype == P_LOCAL_FB)	
		swprintf_s(tbranch, 30, L"Local %.2f Forward+Backward", btie_tolerance); 
	else 
		swprintf_s(tbranch, 30, L"Global %.0f", bnpths);
	ret = swprintf_s(mnm, FNAME_SIZE, L"%sMainpaths %s %d-%d", dpath, tmain, byear, eyear); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	ret = swprintf_s(mname, FNAME_SIZE, L"%s.txt", mnm); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	ret = swprintf_s(anm, FNAME_SIZE, L"%sAllpaths %s-%s %d-%d", dpath, tmain, tbranch, byear, eyear); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	ret = swprintf_s(aname, FNAME_SIZE, L"%s.txt", anm); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
	ret = swprintf_s(mnamep, FNAME_SIZE, L"%s.paj", mnm); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;

	int *sources_save, *sinks_save;
	int nsources_save, nsinks_save, need_new_file;
	int type_save;
	// initialize the queue
	for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_sink = FALSE; q[i].hit_a_source = FALSE; q[i].kroute_id = -1;}
	int direction;
	if (mptype == P_LOCAL_F || mptype == P_LOCAL_B)
	{
		qsize = 1;		// set initial size of the queue	
		if (mptype == P_LOCAL_F)
		{
			select_starting_edges_forward(qsize);	// prepare the starting queue
			local_path_search_forward(mtie_tolerance, TRUE);
			direction = MFORWARD;
		}
		else
		{
			select_starting_edges_backward(qsize);	// prepare the starting queue
			local_path_search_backward(mtie_tolerance, TRUE);
			direction = MBACKWARD;
		}	

		to_relationship_list(mname, TRUE, F_NEW, mptype, direction);
		ret = consolidate_relationship_list(mname); if (ret != 0) return ret;
		to_relationship_list(aname, TRUE, F_NEW, mptype, direction);	// to a relationship file first, then to Pajek file
		ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
		text2Pajek(mnm, mnamep, atable, nalias, &nnd_mainpath, &nsn_mainpath, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);
		Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
	}
	else if (mptype == P_LOCAL_KEY_ROUTE)
	{
		// search forward
		int actual_n_top_edges_local;
		actual_n_top_edges_local = find_actual_top_edges(n_top_edges_local, nslinks, slinks);	// added 2017/02/07
		fwprintf(logstream, L"User specified key-route number = %d, Actual key-route number applied = %d\n", n_top_edges_local, actual_n_top_edges_local);	
		n_top_edges_local = actual_n_top_edges_local;	// overwrite the number provided by the user, added 2017/02/07
		qsize = n_top_edges_local;		// set initial size of the queue, here is basically the same as the number of "edges" to start	
		select_starting_edges_keyroute(qsize, nslinks, slinks, MFORWARD, byear, eyear);	// prepare the starting queue
		local_path_search_forward(mtie_tolerance, TRUE);
		to_relationship_list(mname, TRUE, F_NEW, mptype, MFORWARD);
		ret = consolidate_relationship_list(mname); if (ret != 0) return ret;
		to_relationship_list(aname, TRUE, F_NEW, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
		ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
		// -------- following code block was added 2014/12/21, this is for the purpose of checking critical transition
		int qsize_save; SPATH *q_save;
		q_save = (struct SPATH *)Jmalloc(qsize * sizeof(struct SPATH), L"mainpaths: q_save");
		if (q_save == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (i = 0; i < qsize; i++) q_save[i] = q[i];
		qsize_save = qsize;
		// -------- above code block was added 2014/12/21
		// then, search backward
		qsize = n_top_edges_local;		
		for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_source = FALSE; q[i].kroute_id = -1;}	
		select_starting_edges_keyroute(qsize, nslinks, slinks, MBACKWARD, byear, eyear);	// prepare the starting queue
		local_path_search_backward(mtie_tolerance, TRUE);
		key_routes_relationships(mname, n_top_edges_local, slinks);
		key_routes_relationships(aname, n_top_edges_local, slinks);
		to_relationship_list(mname, TRUE, F_APPEND, mptype, MBACKWARD);	// append to the end of the relationship file
		to_relationship_list(aname, TRUE, F_APPEND, mptype, MBACKWARD);	
		ret = consolidate_relationship_list(mname); if (ret != 0) return ret;
		ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
		text2Pajek(mnm, mnamep, atable, nalias, &nnd_mainpath, &nsn_mainpath, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);
		Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
		// -------- following code block was added 2014/12/21, this is for the purpose of checking critical transition
		ret = combine_local_keyroute_path_queues(qsize_save, q_save, &qsize, q);
		if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;
		Jfree(q_save, L"mainpaths: q_save"); 
		// -------- above code block was added 2014/12/21
	}
	else if (mptype == P_GLOBAL)
	{	
		fw = NULL;
		ret = prepare_for_global_search(); 		
		if (ret != 0) return ret;
		ret = global_path_search(mnpths);
		if (ret != 0) return ret;		
		to_relationship_list(mname, TRUE, F_NEW, mptype, MFORWARD);
		ret = consolidate_relationship_list(mname); if (ret != 0) return ret;
		to_relationship_list(aname, TRUE, F_NEW, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
		ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
		text2Pajek(mnm, mnamep, atable, nalias, &nnd_mainpath, &nsn_mainpath, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);
		Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
	}	
	else if (mptype == P_GLOBAL_KEY_ROUTE)
	{
		int actual_n_top_edges_global;
		actual_n_top_edges_global = find_actual_top_edges(n_top_edges_global, nslinks, slinks);	// added 2017/02/07
		fwprintf(logstream, L"User specified key-route number = %d, Actual key-route number applied = %d\n", n_top_edges_global, actual_n_top_edges_global);	
		n_top_edges_global = actual_n_top_edges_global;		// overwrite the number provided by the user, added 2017/02/07
		// save the sources[] and sinks[]
		sources_save = (int *)Jmalloc(nsources * sizeof(int), L"mainpaths: sources_save");
		if (sources_save == NULL) return MSG_NOT_ENOUGH_MEMORY;
		sinks_save = (int *)Jmalloc(nsinks * sizeof(int), L"mainpaths: sinks_save");
		if (sinks_save == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (i = 0; i < nsources; i++) sources_save[i] = sources[i];
		for (i = 0; i < nsinks; i++) sinks_save[i] = sinks[i];
		nsinks_save = nsinks; nsources_save = nsources;		
		int qsize_f = 0; SPATH *q_f;	// added 2014/12/23
		q_f = (struct SPATH *)Jmalloc(n_top_edges_global * mnpths * 20 * sizeof(struct SPATH), L"mainpaths: q_f");	// added 2014/12/22, extend from 10 to 20, 2016/03/13
		if (q_f == NULL) return MSG_NOT_ENOUGH_MEMORY;
		// find the global path toward the sinks
		first_time_flag = TRUE;
		for (j = 0; j < n_top_edges_global; j++)
		{			
			//fwprintf(logstream, L"----- %d %s=>%s %f\n", j, nw[slinks[j].from].alias,  nw[slinks[j].to].alias, slinks[j].spx);
			if (nw[slinks[j].to].type == T_SINK)	// head end of the key-route is a sink, this check is added 2014/12/26
			{
					qsize = 1;	// let there be only on path, and the path contains only the sink node
					q[0].len = 1; q[0].seq[0] = slinks[j].to;
					nw[q[0].seq[0]].mainp = TRUE;	// this and the following 6 lines are added 2017/02/07
					if (first_time_flag == TRUE)	
					{ need_new_file = F_NEW; first_time_flag = FALSE; }
					else 
						need_new_file = F_APPEND;
					to_relationship_list(mname, TRUE, need_new_file, mptype, MFORWARD);
					to_relationship_list(aname, TRUE, need_new_file, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
			}
			else	// normal case
			{
				sources[0] = slinks[j].to; nsources = 1; 
				for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_sink = FALSE; }
				type_save = nw[sources[0]].type;	// save, added 2011/04/17
				nw[sources[0]].type = T_SOURCE;	 
				if (fw == NULL) 
				{
					ret = prepare_for_global_search();
					if (ret != 0) return ret;
				}
				ret = global_path_search(mnpths);
				if (ret != 0) return ret;		
				nw[sources[0]].type = type_save;	// restore, added 2011/04/17
				if (first_time_flag == TRUE)	// this check is modified 2017/01/15, it was checking if j is 0, but that does not always work
				{ need_new_file = F_NEW; first_time_flag = FALSE; }
				else 
					need_new_file = F_APPEND;
				to_relationship_list(mname, TRUE, need_new_file, mptype, MFORWARD);
				to_relationship_list(aname, TRUE, need_new_file, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
			}
#ifdef CRITICAL_TRANSITION	// theck check is added 2018/01/19
			// -------- following code block was added 2014/12/23, this is for the purpose of checking critical transition
			for (i = 0; i < qsize; i++) { q_f[qsize_f+i] = q[i]; q_f[qsize_f+i].kroute_id = j; }
			qsize_f += qsize;
			//fwprintf(logstream, L"++++++ j=%d, qsize=%d ++++++\n", j, qsize); fflush(logstream);
			// -------- above code block was added 2014/12/23
#endif CRITICAL_TRANSITION
		}
		// then, find the global path toward the sources	
		nsources = nsources_save;										// recover nsources	
		for (i = 0; i < nsources; i++) sources[i] = sources_save[i];	// recover sources[] 	
		int qsize_b = 0; SPATH *q_b;	// added 2014/12/23
		q_b = (struct SPATH *)Jmalloc(n_top_edges_global * mnpths * 20 * sizeof(struct SPATH), L"mainpaths: q_b");	// added 2014/12/22. Changed to 20 (was 10), 2015/02/02.
		if (q_b == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (j = 0; j < n_top_edges_global; j++)
		{		
			//fwprintf(logstream, L"===== %d %s=>%s %f\n", j, nw[slinks[j].from].alias,  nw[slinks[j].to].alias, slinks[j].spx);
			if (nw[slinks[j].from].type == T_SOURCE)	// tail end of the key-route is a source, this check is added 2014/12/26
			{
					qsize = 1;	// let there be only on path, and the path contains only the source node
					q[0].len = 1; q[0].seq[0] = slinks[j].from;
					nw[q[0].seq[0]].mainp = TRUE;	// this and the following 6 lines are added 2017/02/07
					if (first_time_flag == TRUE)	
					{ need_new_file = F_NEW; first_time_flag = FALSE; }
					else 
						need_new_file = F_APPEND;
					to_relationship_list(mname, TRUE, need_new_file, mptype, MFORWARD);
					to_relationship_list(aname, TRUE, need_new_file, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
			}
			else	// normal case
			{
				sinks[0] = slinks[j].from; nsinks = 1; 
				for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_source = FALSE; }
				type_save = nw[sinks[0]].type;	// save, added 2011/04/17
				nw[sinks[0]].type = T_SINK;
				if (fw == NULL) 
				{
					ret = prepare_for_global_search();
					if (ret != 0) return ret;
				}
				ret = global_path_search(mnpths);
				if (ret != 0) return ret;	
				nw[sinks[0]].type = type_save;	// restore, added 2011/04/17
				to_relationship_list(mname, TRUE, F_APPEND, mptype, MFORWARD);
				to_relationship_list(aname, TRUE, F_APPEND, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
			}
#ifdef CRITICAL_TRANSITION	// theck check is added 2018/01/19
			// -------- following code block was added 2014/12/23, this is for the purpose of checking critical transition
			for (i = 0; i < qsize; i++) { q_b[qsize_b+i] = q[i]; q_b[qsize_b+i].kroute_id = j; }
			qsize_b += qsize;
			//fwprintf(logstream, L"====== j=%d, qsize=%d ======\n", j, qsize); fflush(logstream);
			// -------- above code block was added 2014/12/23
#endif CRITICAL_TRANSITION
		}
#ifdef CRITICAL_TRANSITION	// theck check is added 2018/01/19
		// -------- following code block was added 2014/12/23, this is for the purpose of checking critical transition
		ret = combine_global_keyroute_path_queues(qsize_f, q_f, qsize_b, q_b, &qsize, q);
		if (ret == MSG_NOT_ENOUGH_MEMORY) return MSG_NOT_ENOUGH_MEMORY;
		Jfree(q_f, L"mainpaths: q_f"); Jfree(q_b, L"mainpaths: q_b"); 
		// -------- above code block was added 2014/12/23
#endif CRITICAL_TRANSITION
		nsinks = nsinks_save;									// recover nsinks	
		for (i = 0; i < nsinks; i++) sinks[i] = sinks_save[i];	// recover sinks[]
		key_routes_relationships(mname, n_top_edges_global, slinks);
		key_routes_relationships(aname, n_top_edges_global, slinks);
		ret = consolidate_relationship_list(mname);	if (ret != 0) return ret;
		ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
		text2Pajek(mnm, mnamep, atable, nalias, &nnd_mainpath, &nsn_mainpath, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);	
		Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
		Jfree(sources_save, L"mainpaths: sources_save"); Jfree(sinks_save, L"mainpaths: sinks_save");
	}
	wcscpy(rpajekname, mnamep);	// return the Pajek file name that contains the main path results

	// write out data on the main paths (in the original WOS or Scopus format), added 2014/07/11
	wchar_t tempname[FNAME_SIZE];
	if (full_record_type == WOS_DATA)
	{
		swprintf(tempname, L"%s data.txt", mnm);
		readwrite_WOS(frecordname, tempname, RW_MAINPATH, -1);	
	}
	else if (full_record_type == SCOPUS_DATA)
	{
		swprintf(tempname, L"%s data.csv", mnm);
		readwrite_Scopus(frecordname, tempname, RW_MAINPATH, -1);
	}
	else if (full_record_type == THOMSON_INNOVATION_DATA)	// added 2014/07/14
	{
		swprintf(tempname, L"%s data.csv", mnm);
		readwrite_TIP(frecordname, tempname, RW_MAINPATH, -1);
	}
	else if (full_record_type == WEBPAT2_DATA)	// added 2016/05/13
	{
		swprintf(tempname, L"%s data.csv", mnm);
		readwrite_WEBPAT2(frecordname, tempname, RW_MAINPATH, -1);
	}
	else if (full_record_type == WEBPAT3_DATA)	// added 2016/05/13
	{
		swprintf(tempname, L"%s data.csv", mnm);
		readwrite_WEBPAT3(frecordname, tempname, RW_MAINPATH, -1);
	}
	else if (full_record_type == USPTO_DATA)	// added 2014/07/14
	{
		swprintf(tempname, L"%s data.txt", mnm);			// added 2014/07/22
		readwrite_USPTO(frecordname, tempname, RW_MAINPATH, -1);
	}
	else if (full_record_type == TAIWAN_TD_DATA)	// added 2016/03/19
	{
		swprintf(tempname, L"%s data.txt", mnm);			
		readwrite_TaiwanTD(frecordname, tempname, RW_MAINPATH, -1);
	}
	else if (full_record_type == TCI_DATA)	// added 2016/04/23
	{
		swprintf(tempname, L"%s data.csv", mnm);			
		readwrite_TCI(frecordname, tempname, RW_MAINPATH, -1);
	}

	fwprintf(logstream, L"\nFinal paths:\n");
	for (i = 0; i < qsize; i++)
	{ 
		if (mptype == P_LOCAL_F && (q[i].tcnt == 0 || q[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (q[i].tcnt == 0 || q[i].hit_a_source == FALSE)) continue;
		if (q[i].kroute_id == -1)
			fwprintf(logstream, L"Paths %d, Count=%.2f %.2f\n", i, q[i].tcnt, q[i].acnt);
		else
			fwprintf(logstream, L"Paths %d, Key-route [%d], Count=%.2f %.2f\n", i, q[i].kroute_id, q[i].tcnt, q[i].acnt);
		for (k = 0; k < q[i].len; k++)
			fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
		fwprintf(logstream, L"\n");
	}
#ifdef CRITICAL_TRANSITION
#ifndef DELTA_RELEASE
	critical_transition(qsize, q, mptype, nnodes, nw, nwos, wos, ntkeywords, tkeyword);
#endif DELTA_RELEASE
#endif CRITICAL_TRANSITION

	// set the nodes in the mainpath to sinks or sources
	for (i = 0; i < qsize; i++)
	{ 
		if (mptype == P_LOCAL_F)
		{
			if (q[i].tcnt == 0 || q[i].hit_a_sink == FALSE) continue;
			for (k = 0; k < (q[i].len-1); k++) 
			{
				if (nw[q[i].seq[k]].mainp == TRUE)
				{
					if (bptype == P_LOCAL_F)
						nw[q[i].seq[k]].type = T_SINK;
					else
						nw[q[i].seq[k]].type = T_SOURCE;
				}
			}
		}
		if (mptype == P_LOCAL_B)
		{
			if (q[i].tcnt == 0 || q[i].hit_a_source == FALSE) continue;
			for (k = 0; k < (q[i].len-1); k++) 
			{
				if (nw[q[i].seq[k]].mainp == TRUE)
				{
					if (bptype == P_LOCAL_F)
						nw[q[i].seq[k]].type = T_SINK;
					else
						nw[q[i].seq[k]].type = T_SOURCE;
				}
			}
		}
	}	

	// check if braches are requested
	FILE *bstream;
	int nlines;
	wchar_t line[LBUF_SIZE+1], *tline;
	wchar_t bname[LBUF_SIZE+1];
	wchar_t bpname[LBUF_SIZE+1];
	wchar_t apname[LBUF_SIZE+1];
	wchar_t bnm[LBUF_SIZE+1];
	wchar_t *bnds;
	if (branchname[0] == L'\0') return 0;	// branch specification file not given
	if (_wfopen_s(&bstream, branchname, L"rt, ccs=UTF-8") != 0)	// check if brach specification file can be opened
		return 0;	// that's it

	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, bstream) == NULL) break;
		if (line[0] == '\n' || line[0] == '\r') continue;
		nlines++;	
	}		
	bnds = (wchar_t *)Jmalloc(nlines * MAX_NODE_NAME * sizeof(wchar_t), L"mainpaths: bnds");
	if (bnds == NULL) return MSG_NOT_ENOUGH_MEMORY;
	rewind(bstream);	// point back to the beginning of the file
	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, bstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		tline = line;
		if (nlines == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				tline = &line[1];	// skip the BOM
		}
		swscanf_s(tline, L"%s", &bnds[nlines*MAX_NODE_NAME], MAX_NODE_NAME);
		nlines++;	
	}
	fclose(bstream);

	// save the sources[] and sinks[]
	sources_save = (int *)Jmalloc(nsources * sizeof(int), L"mainpaths: sources_save");
	if (sources_save == NULL) return MSG_NOT_ENOUGH_MEMORY;
	sinks_save = (int *)Jmalloc(nsinks * sizeof(int), L"mainpaths: sinks_save");
	if (sinks_save == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nsources; i++) sources_save[i] = sources[i];
	for (i = 0; i < nsinks; i++) sinks_save[i] = sinks[i];
	nsinks_save = nsinks; nsources_save = nsources;
	for (j = 0; j < nlines; j++)
	{
		qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_alias);
		ndx2 = alias_search(wos, nwos, &bnds[j*MAX_NODE_NAME]);
		if (ndx2 >= 0 && wos[ndx2].ndx2nw != -1)	// changed to add the 2nd check, modified 2014/10/29
			ndx = wos[ndx2].ndx2nw;
		else	// notify users if the specified branch node is not found	// added 2011/06/11
		{ 
			System::String^ mymsg1 = "";		 
			for (p = 0; p < (int)wcslen(&bnds[j*MAX_NODE_NAME]); p++) 
				mymsg1 = System::String::Concat(mymsg1, System::Convert::ToString(bnds[j*MAX_NODE_NAME+p]));
			System::String^ mymsg = "Branch paper/patent: \"";
			mymsg = System::String::Concat(mymsg, mymsg1, "\" not found.");
			System::Windows::Forms::MessageBox::Show(mymsg);
		}
		// output the Pajek file of the subnetwork centered on the geiven documents 
		ret = output_subnet(nw, nnodes, ndx, 2, DIRECT_BOTH, dpath);	// currently, hard coded the distance to 2
		if (ret != 0) return ret;
		// work on search for branches
		qsort((void *)wos, (size_t)nwos, sizeof(struct WOS), compare_docid);
		if (ndx2 >= 0)	// ndx2 is the index when sorted by alias, ndx is the index when sorted by document id
		{	
			// re-initialize the queue
			if (bptype == P_LOCAL_F || bptype == P_LOCAL_B || bptype == P_LOCAL_FB)
			{
				swprintf_s(bnm, LBUF_SIZE, L"%sBranch %s %s %d-%d", dpath, tbranch, &bnds[j*MAX_NODE_NAME], byear, eyear);
				ret = swprintf_s(bname, FNAME_SIZE, L"%s.txt", bnm); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
				if (bptype == P_LOCAL_F)
				{
					if (nw[ndx].out_deg == 0) continue; 	// ignore if can't go further
					qsize = 1;		// set initial size of the queue
					sources[0] = ndx; nsources = 1; 
					for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_sink = FALSE; }
					nw[ndx].type = T_SOURCE;
					select_starting_edges_forward_branches(qsize);	// prepare the starting queue
					local_path_search_forward(btie_tolerance, FALSE);
					direction = MFORWARD;
					if (full_record_type == WOS_DATA) output_keyword_info(bname);
					to_relationship_list(bname, TRUE, F_NEW, mptype, direction);	// to a relationship file first, then to Pajek file
					to_relationship_list(aname, TRUE, F_APPEND, mptype, direction);	// also append to the "Allpaths" file
				}
				else if (bptype == P_LOCAL_B)
				{			
					if (nw[ndx].in_deg == 0) continue;	// ignore if can't go further
					qsize = 1;		// set initial size of the queue
					sinks[0] = ndx; nsinks = 1; 
					for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_source = FALSE; }
					nw[ndx].type = T_SINK;
					select_starting_edges_backward_branches(qsize);	// prepare the starting queue
					local_path_search_backward(btie_tolerance, FALSE);
					direction = MBACKWARD;
					if (full_record_type == WOS_DATA) output_keyword_info(bname);
					to_relationship_list(bname, TRUE, F_NEW, mptype, direction);	// to a relationship file first, then to Pajek file
					to_relationship_list(aname, TRUE, F_APPEND, mptype, direction);	// also append to the "Allpaths" file
				} 
				else if (bptype == P_LOCAL_FB)
				{
					if (nw[ndx].out_deg == 0 && nw[ndx].in_deg == 0) continue;	// ignore if can't go both way 
					qsize = 1;		// set initial size of the queue
					sources[0] = ndx; nsources = 1; 
					for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_sink = FALSE; }
					type_save = nw[ndx].type;	
					nw[ndx].type = T_SOURCE;
					select_starting_edges_forward_branches(qsize);	// prepare the starting queue
					local_path_search_forward(btie_tolerance, FALSE);
					nw[ndx].type = type_save;	
					direction = MFORWARD;
					if (full_record_type == WOS_DATA) output_keyword_info(bname);
					to_relationship_list(bname, TRUE, F_NEW, mptype, direction);	// to a relationship file first, then to Pajek file
					to_relationship_list(aname, TRUE, F_APPEND, mptype, direction);	// also append to the "Allpaths" file
					nsources = nsources_save;										// recover nsources
					for (i = 0; i < nsources; i++) sources[i] = sources_save[i];	// recover sources[] 
					qsize = 1;		// set initial size of the queue
					sinks[0] = ndx; nsinks = 1; 
					for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_source = FALSE; }
					type_save = nw[ndx].type;	
					nw[ndx].type = T_SINK;
					select_starting_edges_backward_branches(qsize);	// prepare the starting queue
					local_path_search_backward(btie_tolerance, FALSE);
					nw[ndx].type = type_save;	
					direction = MBACKWARD;
					if (full_record_type == WOS_DATA) output_keyword_info(bname);
					to_relationship_list(bname, TRUE, F_APPEND, mptype, direction);	// to a relationship file first, then to Pajek file
					to_relationship_list(aname, TRUE, F_APPEND, mptype, direction);	// also append to the "Allpaths" file
					nsinks = nsinks_save;									// recover nsinks
					for (i = 0; i < nsinks; i++) sinks[i] = sinks_save[i];	// recover sinks[]
				}
				// write the resulting paths to Pajek file
				ret = consolidate_relationship_list(bname); if (ret != 0) return ret;
				ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
				int nnd_branch, nsn_branch;
				swprintf_s(bpname, LBUF_SIZE, L"%sBranch %s %s %d-%d.paj", dpath, tbranch, &bnds[j*MAX_NODE_NAME], byear, eyear);
				text2Pajek(bnm, bpname, atable, nalias, &nnd_branch, &nsn_branch, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);
				Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
				_wunlink(bname);	// delete the temporary relationship-list file
			}
			else	// branch applying global search (both forward and backward)
			{
				// find the global path from the current node toward the sinks
				sources[0] = ndx; nsources = 1; 
				for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_sink = FALSE; }
				type_save = nw[sources[0]].type;	
				nw[sources[0]].type = T_SOURCE;	 
				if (fw == NULL) 
				{
					ret = prepare_for_global_search();
					if (ret != 0) return ret;
				}
				ret = global_path_search_branch(bnpths, MFORWARD);
				nw[sources[0]].type = type_save;	
				if (ret < 0) return -2;	
				// write the resulting paths to Pajek file
				swprintf_s(bnm, LBUF_SIZE, L"%sBranch %s %s %d-%d", dpath, tbranch, &bnds[j*MAX_NODE_NAME], byear, eyear);
				ret = swprintf_s(bname, FNAME_SIZE, L"%s.txt", bnm); if (ret == -1) return MSG_FILE_NAME_TOO_LONG;
				if (full_record_type == WOS_DATA) output_keyword_info(bname);
				to_relationship_list(bname, TRUE, F_NEW, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
				to_relationship_list(aname, TRUE, F_APPEND, mptype, MFORWARD);	// also append to the "Allpaths" file
				// then, find the global path from the sources to the current node		
				nsources = nsources_save;										// recover nsources
				for (i = 0; i < nsources; i++) sources[i] = sources_save[i];	// recover sources[] 
				sinks[0] = ndx; nsinks = 1; 
				for (i = 0; i < MAX_PATH_QUEUE+EXT_PATH_QUEUE; i++) { q[i].tcnt = 0; q[i].hit_a_sink = FALSE; }
				type_save = nw[sinks[0]].type;	
				nw[sinks[0]].type = T_SINK;
				if (fw == NULL) 
				{
					ret = prepare_for_global_search();
					if (ret != 0) return ret;
				}
				ret = global_path_search_branch(bnpths, MBACKWARD);
				nw[sinks[0]].type = type_save;	
				if (ret < 0) return -2;	
				to_relationship_list(bname, TRUE, F_APPEND, mptype, MFORWARD);
				to_relationship_list(aname, TRUE, F_APPEND, mptype, MFORWARD);	// to a relationship file first, then to Pajek file
				ret = consolidate_relationship_list(bname);	if (ret != 0) return ret;	
				ret = consolidate_relationship_list(aname); if (ret != 0) return ret;
				int nnd_branch, nsn_branch;
				swprintf_s(bpname, LBUF_SIZE, L"%sBranch %s %s %d-%d.paj", dpath, tbranch, &bnds[j*MAX_NODE_NAME], byear, eyear);
				text2Pajek(bnm, bpname, atable, nalias, &nnd_branch, &nsn_branch, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);
				Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
				_wunlink(bname);	// delete the temporary relationship-list file
				nsinks = nsinks_save;									// recover nsinks
				for (i = 0; i < nsinks; i++) sinks[i] = sinks_save[i];	// recover sinks[]
			}
		}
	}
	Jfree(sources_save, L"mainpaths: sources_save"); Jfree(sinks_save, L"mainpaths: sinks_save");

	int nnd_allpath, nsn_allpath;
	swprintf_s(apname, LBUF_SIZE, L"%sAllpaths %s-%s %d-%d.paj", dpath, tmain, tbranch, byear, eyear);
	text2Pajek(anm, apname, atable, nalias, &nnd_allpath, &nsn_allpath, SP_TAB_SPACE, M1_MODE, MDIRECTED, MFORWARD, stype, ALL_NODES, TRUE, WRITE_NODE_NAME);
	wcscpy(rpajekname, apname);	// return the Pajek file name that contains the main path results
	Jfree(nodes, L"mainpaths: nodes"); nodes = NULL;	// added 2011/12/11
	_wunlink(aname);	// delete the temporary relationship-list file

	return 0;
}

//
// search main path (forward)
// following the algorithm specified in 
//    Bart Verspagen (2007) "Mapping technological trajectories as patent citation networks: 
//    a study on the history of technological trajectories of fuel cell research", 
//    Advances in Complex Systems, Vol. 10, No. 1, p. 93¡V115.
// this is actually a "local" search, it selects only the "current" maximum path
// NOTE: but the meaning of "tie" on SPx is extended, 
//       a "tie" means roughly equal (within a certain range of tolerance)
//
int local_path_search_forward(double tie_tolerance, int mark_mp)
{
	int iput, iget;		// index to the queue 
	int i, j, k, m, icur, iprev;
	struct SPATH curp, sp;
	double ccnt, maxc;
	int all_hit_sink, nmax;
	double cur_tolerance;

//#ifdef DEBUG
	fwprintf(logstream, L"\nStarting paths (local search forward):\n");
	for (i = 0; i < ndataq; i++)
	{ 
		if (q[i].tcnt == 0)
			continue;
		fwprintf(logstream, L"Count=%.2f %.2f ", q[i].tcnt, q[i].acnt);
		for (k = 0; k < q[i].len; k++)
			fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
		fwprintf(logstream, L"\n");
	}
	fflush(logstream);
//#endif DEBUG

	// mark the starting nodes as main path points
	for (i = 0; i < ndataq; i++) 
	{ 
		if (q[i].tcnt == 0) continue; 
		if (mark_mp == TRUE) { nw[q[i].seq[0]].mainp = TRUE; nw[q[i].seq[1]].mainp = TRUE; } // main path
		else { nw[q[i].seq[0]].branch = TRUE; nw[q[i].seq[1]].branch = TRUE; }	// branches
	}

	icur = q[0].seq[0];
	iput = ndataq;
	if (iput == qsize) iput = 0;
	iget = 0;
	int loopcnt = 0;
	while (TRUE)	// until all paths hit a sink
	{
		if (loopcnt > 1000) exit(0);
		else loopcnt++;

		all_hit_sink = TRUE;
		for (i = 0; i < ndataq; i++) { if (q[i].hit_a_sink == FALSE) { all_hit_sink = FALSE; break; } }
		if (all_hit_sink == TRUE) break;	// leave the loop
		// get from the queue, first-in-first out
		iprev = icur;
		curp = q[iget];
		icur = curp.seq[curp.len-1];

		//fwprintf(logstream, L"$$$$$ %04d %d %d %d %s\n", loopcnt, qsize, iget, icur, nw[icur].alias); fflush(logstream);

		// mark the path, if hit a sink or an isolate (why isolate? because when a node is cut-out by a time window, an "isolate" is the same as a "source"
		if (nw[icur].type == T_SINK || nw[icur].type == T_ISOLATE) q[iget].hit_a_sink = TRUE;
		iget++; if (iget == qsize)	iget = 0;	// round back
		if (nw[icur].type == T_SINK || nw[icur].type == T_ISOLATE) 
		{ 
			iput++; if (iput == qsize) iput = 0;	// iput still need to roll over
			continue;
		}
		// find the arcs with the largest SPx count
		maxc = 0.0;
		for (k = 0; k < nw[icur].out_deg; k++)
		{
			j = nw[icur].out_nbrs[k];	// index of the neighbors
			if (nw[icur].out_spx[k] > maxc) maxc = nw[icur].out_spx[k]; 
		}
		nmax = 0;
		for (k = 0; k < nw[icur].out_deg; k++)
		{
			j = nw[icur].out_nbrs[k];	// index of the neighbors
			// adjust the tie_tolerance dynamically, reducing with the step to the starting node
			cur_tolerance = tie_tolerance - (curp.len * TOLERANCE_GRADIENT);
			if (cur_tolerance < 0.0) cur_tolerance = 0.0;
			if (nw[icur].out_spx[k] >= (maxc * (1 - cur_tolerance))) // arcs with SPx within the tolerance are accepted
			{
				if (nw[icur].out_spx[k] == maxc && mark_mp == TRUE && nw[icur].mainp == TRUE) nw[j].mainp = TRUE; // mark the node which is maximum and is along the main path
				if (nw[icur].out_spx[k] == maxc && mark_mp == FALSE && nw[icur].branch == TRUE && nw[j].mainp == FALSE) nw[j].branch = TRUE; // mark the node which is maximum and is along the main path
				nmax++;
				ccnt = nw[icur].out_spx[k];	// from icur to j
				for (m = 0; m < curp.len; m++) sp.seq[m] = curp.seq[m];
				for (m = 0; m < curp.len-1; m++) sp.spx[m] = curp.spx[m];
				sp.seq[curp.len] = j;
				sp.spx[curp.len-1] = nw[icur].out_spx[k];
				sp.len = curp.len + 1;
				sp.tcnt = ccnt;
				sp.acnt = ccnt;
				sp.hit_a_sink = FALSE;
				if (nmax == 1)	// just update the path
				{
					q[iput] = sp;
					iput++; if (iput == qsize) iput = 0;
				}
				else	// there are ties in the SPx, add an new entry in the path queue
				{
					if (qsize >= (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
						continue;	// no more space to extend
					// extend the queue by one unit
					qsize++;
					for (i = qsize-1; i > iput; i--) q[i] = q[i-1]; 
					q[iput] = sp; 
					iput++; if (iput == qsize) iput = 0;
					ndataq++;
					iget++; if (iget == qsize) iget = 0;
				}
			}
		}
//#ifdef DEBUG
		fwprintf(logstream, L"\n\nCurrent paths from %s:\n", nw[icur].alias);
		fwprintf(logstream, L"ndataq=%d, qsize=%d, iget=%d, iput=%d, ind_min=%d, acnt_min=%.2f\n", ndataq, qsize, iget, iput, ind_min, acnt_min);
		if (iput > iget)
		{
			for (i = iget; i < iput; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
				fwprintf(logstream, L"\n");
			}
		}
		else
		{
			for (i = iget; i < qsize; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
				fwprintf(logstream, L"\n");
			}
			for (i = 0; i < iput; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
				fwprintf(logstream, L"\n");
			}
		}
		fflush(logstream);
//#endif DEBUG
	}

	return 0;
}

//
// search main path (backward)
// following the algorithm specified in 
//    Bart Verspagen (2007) "Mapping technological trajectories as patent citation networks: 
//    a study on the history of technological trajectories of fuel cell research", 
//    Advances in Complex Systems, Vol. 10, No. 1, p. 93¡V115.
// this is actually a "local" search, it selects only the "current" maximum path
// NOTE: but the meaning of "tie" on SPx is extended, 
//       a "tie" means roughly equal (within a certain range of tolerance)
//
int local_path_search_backward(double tie_tolerance, int mark_mp)
{
	int iput, iget;		// index to the queue 
	int i, j, k, m, icur, iprev;
	struct SPATH curp, sp;
	double ccnt, maxc;
	int all_hit_source, nmax;
	double cur_tolerance;

//#ifdef DEBUG
	fwprintf(logstream, L"\nStarting paths (local search backward):\n");
	for (i = 0; i < ndataq; i++)
	{ 
		if (q[i].tcnt == 0)
			continue;
		fwprintf(logstream, L"Count=%.2f %.2f ", q[i].tcnt, q[i].acnt);
		for (k = 0; k < q[i].len; k++)
			fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
		fwprintf(logstream, L"\n");
	}
	fflush(logstream);
//#endif DEBUG

	// mark the starting nodes as main path points
	for (i = 0; i < ndataq; i++) 
	{ 
		if (q[i].tcnt == 0) continue; 
		if (mark_mp == TRUE) { nw[q[i].seq[0]].mainp = TRUE; nw[q[i].seq[1]].mainp = TRUE; } // main path
		else { nw[q[i].seq[0]].branch = TRUE; nw[q[i].seq[1]].branch = TRUE; }	// branches
	}

	icur = q[0].seq[0];
	iput = ndataq;
	if (iput == qsize) iput = 0;
	iget = 0;
	while (TRUE)	// until all paths hit a source
	{
		all_hit_source = TRUE;
		for (i = 0; i < ndataq; i++) { if (q[i].hit_a_source == FALSE) { all_hit_source = FALSE; break; } }
		if (all_hit_source == TRUE) break;	// leave the loop
		// get from the queue, first-in-first out
		iprev = icur;
		curp = q[iget];
		icur = curp.seq[curp.len-1];
		// mark the path, if hit a source or an isolate (why isolate? because when a node is cut-out by a time window, an "isolate" is the same as a "source"
		if (nw[icur].type == T_SOURCE || nw[icur].type == T_ISOLATE) q[iget].hit_a_source = TRUE;
		iget++; if (iget == qsize)	iget = 0;	// round back
		if (nw[icur].type == T_SOURCE || nw[icur].type == T_ISOLATE) 
		{ 
			iput++; if (iput == qsize) iput = 0;	// iput still need to roll over
			continue;
		}
		// find the arcs with the largest SPx count
		maxc = 0.0;
		for (k = 0; k < nw[icur].in_deg; k++)
		{
			j = nw[icur].in_nbrs[k];	// index of the neighbors
			if (nw[icur].in_spx[k] > maxc) maxc = nw[icur].in_spx[k]; 
		}
		nmax = 0;
		for (k = 0; k < nw[icur].in_deg; k++)
		{
			j = nw[icur].in_nbrs[k];	// index of the neighbors
			// adjust the tie_tolerance dynamically, reducing with the step to the starting node
			cur_tolerance = tie_tolerance - (curp.len * TOLERANCE_GRADIENT);
			if (cur_tolerance < 0.0) cur_tolerance = 0.0;
			if (nw[icur].in_spx[k] >= (maxc * (1 - cur_tolerance))) // arcs with SPx within the tolerance are accepted
			{				
				if (nw[icur].in_spx[k] == maxc && mark_mp == TRUE && nw[icur].mainp == TRUE) nw[j].mainp = TRUE; // mark the node which is maximum and is along the main path
				if (nw[icur].in_spx[k] == maxc && mark_mp == FALSE && nw[icur].branch == TRUE) nw[j].branch = TRUE; // mark the node which is maximum and is along the main path
				nmax++;
				ccnt = nw[icur].in_spx[k];	// from icur to j
				for (m = 0; m < curp.len; m++) sp.seq[m] = curp.seq[m];
				for (m = 0; m < curp.len-1; m++) sp.spx[m] = curp.spx[m];
				sp.seq[curp.len] = j;
				sp.spx[curp.len-1] = nw[icur].in_spx[k];
				sp.len = curp.len + 1;
				sp.tcnt = ccnt;
				sp.acnt = ccnt;
				sp.hit_a_source = FALSE;
				if (nmax == 1)	// just update the path
				{
					q[iput] = sp;
					iput++; if (iput == qsize) iput = 0;
				}
				else	// there are ties in the SPx, add an new entry in the path queue
				{
					if (qsize >= (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
						continue;	// no more space to extend
					// extend the queue by one unit
					qsize++;
					for (i = qsize-1; i > iput; i--) q[i] = q[i-1]; 
					q[iput] = sp; 
					iput++; if (iput == qsize) iput = 0;
					ndataq++;
					iget++; if (iget == qsize) iget = 0;
				}
			}
		}
#ifdef DEBUG
		fwprintf(logstream, L"\n\nCurrent paths from %s:\n", nw[icur].name);
		fwprintf(logstream, L"ndataq=%d, qsize=%d, iget=%d, iput=%d, ind_min=%d, acnt_min=%.2f\n", ndataq, qsize, iget, iput, ind_min, acnt_min);
		if (iput > iget)
		{
			for (i = iget; i < iput; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
				fwprintf(logstream, L"\n");
			}
		}
		else
		{
			for (i = iget; i < qsize; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
				fwprintf(logstream, L"\n");
			}
			for (i = 0; i < iput; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].alias);
				fwprintf(logstream, L"\n");
			}
		}
		fflush(logstream);
#endif DEBUG
	}

	return 0;
}

//
// apply Floyd-Warshall algorithm
// have the table fw[] ready
//
#define SPX_FACTOR 1000000.0
int prepare_for_global_search()
{
	int i, j, k;
	double *netx;
	__int64 msize;	// added 2015/02/04

	msize = nnodes * nnodes * sizeof(double);	// added 2015/02/04
	netx = (double *)Jmalloc(msize, L"prepare_for_global_search: netx");
	if (netx == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nnodes*nnodes; i++) netx[i] = 0.0;	// initialize
	for (i = 0; i < nnodes; i++)	// assign values to the matrix
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			// negate the SPx value because Floyd-Warshall looks for paths with smallest weight
			//netx[i*nnodes+j] = SPX_FACTOR / nw[i].out_spx[k];	
			netx[i*nnodes+j] = -1.0 * nw[i].out_spx[k];	
		}
	}

	msize = nnodes * nnodes * sizeof(struct FWDIST);	// added 2015/02/04
	fw = (struct FWDIST *)Jmalloc(msize, L"prepare_for_global_search: fw");
	if (fw == NULL) return MSG_NOT_ENOUGH_MEMORY;
	msize = nnodes * nnodes * sizeof(short);	// added 2015/02/04
	fw_midpt = (short *)Jmalloc(msize, L"prepare_for_global_search: fw_midpt");
	if (fw_midpt == NULL) return MSG_NOT_ENOUGH_MEMORY;

	floyd_warshall(nnodes, netx, fw, fw_midpt);
	Jfree(netx, L"prepare_for_global_search: netx");

#ifdef DEBUG
	// print out the distance results
	//for (i = 0; i < nn; i++) 
	//{
	i = 1089;
		for (j = 0; j < nnodes; j++)
		fwprintf(logstream, L"(%d)%.2f[%d]\n", j, fw[i*nnodes+j].dist, fw_midpt[i*nnodes+j]);
		fwprintf(logstream, L"\n");
	//}
#endif DEBUG

	return 0;
}

//
// conduct global path search by applying Floyd-Warshall algorithm
// select paths with larger average SPx in a path
//
int global_path_search(double npths)
{
	int i, j, k;
	int npaths;
	int ret;

	npaths = (int)npths;

	ret = find_shortest_paths(nnodes, fw, fw_midpt, nsources, sources, nsinks, sinks, &npaths, q);	
	if (ret != 0) return ret;
	qsize = npaths;	// "npaths" may be changed inside find_shortest_paths()

	// recover the original SPx value
	for (i = 0; i < qsize; i++)
	{
		for (k = 0; k < q[i].len; k++)
			q[i].spx[k] = -1.0 * q[i].spx[k];
			//q[i].spx[k] = SPX_FACTOR / q[i].spx[k];
	}

	// mark the main path, there may be multiple paths with the same overall SPx
	for (i = 0; i < qsize; i++)
	{
		if (q[i].tcnt != q[0].tcnt)
			break;
		for (k = 0; k < q[i].len; k++)
			nw[q[i].seq[k]].mainp = TRUE;
	}

	return 0;
}

//
// conduct global path search by applying Floyd-Warshall algorithm
// select paths with larger average SPx in a path
// this function is designed for branch search only
//
int global_path_search_branch(double npths, int direction)
{
	int i, j, k, m, p;
	int npaths;
	int ret;

	npaths = (int)npths;

	ret = find_shortest_paths(nnodes, fw, fw_midpt, nsources, sources, nsinks, sinks, &npaths, q);	
	if (ret != 0) return ret;
	qsize = npaths;	// "npaths" may be changed inside find_shortest_paths()

	// recover the original SPx value
	for (i = 0; i < qsize; i++)
	{
		for (k = 0; k < q[i].len; k++)
			q[i].spx[k] = -1.0 * q[i].spx[k];
			//q[i].spx[k] = SPX_FACTOR / q[i].spx[k];
	}

	j = 0;
	for (i = 0; i < qsize; i++)
	{
		if (q[i].len == 0) continue;
		q[j] = q[i];
		for (k = 0; k < q[j].len; k++)
		{
			if (nw[q[j].seq[k]].mainp == FALSE)	
				nw[q[j].seq[k]].branch = TRUE;	// mark as a brach node
		}
		j++;
	}
	qsize = j;

	return 0;
}

#ifdef REPLACED_BY_FLOYD_WARSHALL_20100622
//
// NOTE: this functin becomes obsolete after 20100622 (replaced by Floyd-Warshall algorithm)
//
// conduct global path search
// select paths with larger average SPx in a path
//
int global_path_search(double cutoff)
{
	int iput, iget;		// index to the queue 
	int i, j, k, m, icur;
	struct SPATH curp, sp;
	double tcnt, acnt;
	int all_hit_sink;

//#ifdef DEBUG
	fwprintf(logstream, L"\nStarting paths (global search):\n");
	for (i = 0; i < ndataq; i++)
	{ 
		if (q[i].tcnt == 0)
			continue;
		fwprintf(logstream, L"Count=%.2f %.2f ", q[i].tcnt, q[i].acnt);
		for (k = 0; k < q[i].len; k++)
			fwprintf(logstream, L"%s->", nw[q[i].seq[k]].name);
		fwprintf(logstream, L"\n");
	}
//#endif DEBUG

	// following code is not as efficient as Floyd-Warshall algorithm
	// it also may not find the path with the largest SPx
	iput = ndataq;
	if (iput == qsize) iput = 0;
	iget = 0;
	while (TRUE)	// forever, until all paths end at sinks
	{
		all_hit_sink = TRUE;
		for (i = 0; i < ndataq; i++) { if (q[i].hit_a_sink == FALSE) { all_hit_sink = FALSE; break; } }
		if (all_hit_sink == TRUE) break;	// leave the loop
		// get from the queue, first-in-first out
		curp = q[iget];
		icur = curp.seq[curp.len-1];
		// mark the path, if hit a sink
		if (nw[icur].type == T_SINK)
			q[iget].hit_a_sink = TRUE;
		iget++; if (iget == qsize)	iget = 0;	// round back
		ndataq--;
		if (nw[icur].type == T_SINK) continue;
		for (k = 0; k < nw[icur].out_deg; k++)
		{
			j = nw[icur].out_nbrs[k];	// index of the neighbors
			if (nw[icur].out_spx[k] < (cutoff * acnt_min))	// small branch, ignore
				continue;
			// find the average SPx count of the path
			tcnt =  curp.tcnt + nw[icur].out_spx[k];	// from icur to j
			acnt = tcnt / curp.len;
			for (m = 0; m < curp.len; m++) sp.seq[m] = curp.seq[m];
			for (m = 0; m < curp.len-1; m++) sp.spx[m] = curp.spx[m];
			sp.seq[curp.len] = j;
			sp.spx[curp.len-1] = nw[icur].out_spx[k];
			sp.len = curp.len + 1;
			sp.tcnt = tcnt;
			sp.acnt = acnt;
			sp.hit_a_sink = FALSE;
			if (ndataq < qsize)	// put at the end of the path queue
			{
				q[iput] = sp;
				iput++; if (iput == qsize) iput = 0;
				ndataq++;
				ind_min = find_min_acnt(iput, iget, qsize);
				acnt_min = q[ind_min].acnt;
			}
			else	// the queue is full, 
			{	 
				if (acnt < acnt_min)
					continue;	// smaller than the minimum, not interesting
				else if (acnt == acnt_min)	
				{
					if (qsize >= (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
						continue;	// no more space to extend
					// extend the queue by one unit
					qsize++;	// when the queue is full, iget=iput
					for (i = qsize-1; i > iput; i--) q[i] = q[i-1]; 
					q[iput] = sp; 
					iput++; if (iput == qsize) iput = 0;
					ndataq++;
					iget++; if (iget == qsize) iget = 0;
				}
				else // remove the path with minimum SPx count and put the given data at the end of the queue
				{	 // rearrange the queue	
					fill_the_hole(iput, iget, qsize, ind_min);
					if (iput == 0) q[qsize-1] = sp; else q[iput-1] = sp;
					ind_min = find_min_acnt(iput, iget, qsize);
					acnt_min = q[ind_min].acnt;
				}
			}
			//fwprintf(logstream, L"%.2f ", acnt_min);
		}

#ifdef DEBUG
		fwprintf(logstream, L"\nCurrent paths from %s:\n", nw[icur].name);
		fwprintf(logstream, L"ndataq=%d, qsize=%d, iget=%d, iput=%d, ind_min=%d, acnt_min=%.2f\n", ndataq, qsize, iget, iput, ind_min, acnt_min);
		if (iput > iget)
		{
			for (i = iget; i < iput; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].name);
				fwprintf(logstream, L"\n");
			}
		}
		else
		{
			for (i = iget; i < qsize; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].name);
				fwprintf(logstream, L"\n");
			}
			for (i = 0; i < iput; i++)
			{ 
				if (q[i].tcnt == 0) continue;
				fwprintf(logstream, L"qind=%d, Count=%.2f %.2f ", i, q[i].tcnt, q[i].acnt);
				for (k = 0; k < q[i].len; k++) fwprintf(logstream, L"%s->", nw[q[i].seq[k]].name);
				fwprintf(logstream, L"\n");
			}
		}
		fflush(logstream);
#endif DEBUG
	}

	return 0;
}
#endif REPLACED_BY_FLOYD_WARSHALL_20100622

//
// given an index, remove it by shift all data after it one step backward,
// after the operation, the position iput is open 
// when the function is called, iput=iget, always
//
int fill_the_hole(int ip, int ig, int qsize, int ind)
{
	int i;

	if (ind >= ig && ind < qsize)	// ind is between iget and qsize
	{
		for (i = ind; i < qsize-1; i++) q[i] = q[i+1];
		q[qsize-1] = q[0];
		for (i = 0; i < ip; i++) q[i] = q[i+1];
	}
	else							// ind is between 0 and iget
		for (i = ind; i < ip; i++) q[i] = q[i+1];

	return 0;
}


//
// find the index of the path in queue with minimum average SPx count
// return the index
//
int find_min_acnt(int ip, int ig, int qs)
{
	int i, ind;
	double min;

	min = 9999999999E+30;	// modifided 2012/09/24
	if (ip > ig)
	{
		for (i = ig; i < ip; i++) { if (q[i].acnt < min) { min = q[i].acnt; ind = i; } }
	}
	else
	{
		for (i = ig; i < qs; i++) { if (q[i].acnt < min) { min = q[i].acnt; ind = i; } }
		for (i = 0; i < ip; i++) { if (q[i].acnt < min) { min = q[i].acnt; ind = i; } }
	}

	return ind;
}

//
// select starting edges from the edges eminating from the sources
// put them into the path queue
//
int select_starting_edges_forward(int ste_init)
{
	int i, j, k;
	double acnt_max;

	for (i = 0; i < qsize; i++) { q[i].tcnt = 0; q[i].len = 0; }

	// initial selection of the starting edges	
	ndataq = 0; 
	for (i = 0; i < nsources; i++)
	{
		j = sources[i];
		for (k = 0; k < nw[j].out_deg; k++)
		{
			//if (nw[nw[j].out_nbrs[k]].mainp == TRUE) continue;	// avoid edges that is already on the main path (when working on branches)
			if (ndataq >= qsize)	// path queue full, replace the path which has minimum count with a new path
			{
				if (nw[j].out_spx[k] == acnt_min)	// equal the minimum average count
				{
					if (qsize < (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
					{
						add_init_path(j, nw[j].out_nbrs[k], nw[j].out_spx[k]);
						qsize++;
					}
				}
				else if (nw[j].out_spx[k] > acnt_min)
					replace_init_path(j, nw[j].out_nbrs[k], nw[j].out_spx[k]);
			}
			else
				add_init_path(j, nw[j].out_nbrs[k], nw[j].out_spx[k]);
		}
	}

	// check the rationality of the starting edges, for the case of ste_init=1, a special case for one path search
	// may get unreasonable result, for example, ask for 1 but given 3, but there is only one which is the largest
	int nsmax; 
	if (ste_init == 1 && ndataq != 1)
	{
		acnt_max = 0.0; 
		for (i = 0; i < ndataq; i++)	// find the maximum average count
		{
			if (q[i].acnt > acnt_max)
				acnt_max = q[i].acnt;
		}
		nsmax = 0;
		for (i = 0; i < ndataq; i++)	// find the number of maximum average count
		{
			if (q[i].acnt == acnt_max)
				q[nsmax++] = q[i];
		}
		qsize = ndataq = nsmax;
	}

	return 0;
}

//
// select starting edges from the edges eminating from the sinks
// put them into the path queue
//
int select_starting_edges_backward(int ste_init)
{
	int i, j, k;
	double acnt_max;

	for (i = 0; i < qsize; i++) { q[i].tcnt = 0; q[i].len = 0; }

	// initial selection of the starting edges	
	ndataq = 0; 
	for (i = 0; i < nsinks; i++)
	{
		j = sinks[i];
		for (k = 0; k < nw[j].in_deg; k++)
		{
			//if (nw[nw[j].in_nbrs[k]].mainp == TRUE) continue;	// avoid edges that is already on the main path (when working on branches)
			if (ndataq >= qsize)	// path queue full, replace the path which has minimum count with a new path
			{
				if (nw[j].in_spx[k] == acnt_min)	// equal the minimum average count
				{
					if (qsize < (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
					{
						add_init_path(j, nw[j].in_nbrs[k], nw[j].in_spx[k]);
						qsize++;
					}
				}
				else if (nw[j].in_spx[k] > acnt_min)
					replace_init_path(j, nw[j].in_nbrs[k], nw[j].in_spx[k]);
			}
			else
				add_init_path(j, nw[j].in_nbrs[k], nw[j].in_spx[k]);
		}
	}

	// check the rationality of the starting edges, for the case of ste_init=1, a special case for one path search
	// may get unreasonable result, for example, ask for 1 but given 3, but there is only one which is the largest
	int nsmax; 
	if (ste_init == 1 && ndataq != 1)
	{
		acnt_max = 0.0; 
		for (i = 0; i < ndataq; i++)	// find the maximum average count
		{
			if (q[i].acnt > acnt_max)
				acnt_max = q[i].acnt;
		}
		nsmax = 0;
		for (i = 0; i < ndataq; i++)	// find the number of maximum average count
		{
			if (q[i].acnt == acnt_max)
				q[nsmax++] = q[i];
		}
		qsize = ndataq = nsmax;
	}

	return 0;
}

//
// select starting edges from the list of the edges with the largest SPx
// put them into the path queue
//
int select_starting_edges_keyroute(int nedges, int nslinks, struct SLINK *slinks, int direction, int byear, int eyear)
{
	int i, j, k;
	int cnt;
	double acnt_max;

	// note that qsize is the same the nedges
	for (i = 0; i < qsize; i++) { q[i].tcnt = 0; q[i].len = 0; }

	// initial selection of the starting edges	
	ndataq = 0;		// this number is incremented in the function add_init_path()
	cnt = 0;
	for (i = 0; i < nslinks; i++)
	{
		if (wos[nw[slinks[i].from].ndx2wos].year < byear || 
			wos[nw[slinks[i].from].ndx2wos].year > eyear || 
			wos[nw[slinks[i].to].ndx2wos].year < byear || 
			wos[nw[slinks[i].to].ndx2wos].year > eyear)
			continue;
		if (direction == MFORWARD)
			add_init_path(slinks[i].from, slinks[i].to, slinks[i].spx);
		else
			add_init_path(slinks[i].to, slinks[i].from, slinks[i].spx);
		cnt++;
		if (cnt >= nedges)
			break;
	}

	return 0;
}

//
// select starting edges from the edges eminating from the sources
// this function is designed for searching brahches
// ==> ignore the edges on the main path (flagged with "mainp")
// put the results into the path queue
//
int select_starting_edges_forward_branches(int ste_init)
{
	int i, j, k;
	double acnt_max;

	for (i = 0; i < qsize; i++) { q[i].tcnt = 0; q[i].len = 0; }

	// initial selection of the starting edges	
	ndataq = 0; 
	for (i = 0; i < nsources; i++)
	{
		j = sources[i];
		for (k = 0; k < nw[j].out_deg; k++)
		{
			if (nw[nw[j].out_nbrs[k]].mainp == TRUE) continue;	// avoid edges that is already on the main path (when working on branches)
			if (ndataq >= qsize)	// path queue full, replace the path which has minimum count with a new path
			{
				if (nw[j].out_spx[k] == acnt_min)	// equal the minimum average count
				{
					if (qsize < (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
					{
						add_init_path(j, nw[j].out_nbrs[k], nw[j].out_spx[k]);
						qsize++;
					}
				}
				else if (nw[j].out_spx[k] > acnt_min)
					replace_init_path(j, nw[j].out_nbrs[k], nw[j].out_spx[k]);
			}
			else
				add_init_path(j, nw[j].out_nbrs[k], nw[j].out_spx[k]);
		}
	}

	// check the rationality of the starting edges, for the case of ste_init=1, a special case for one path search
	// may get unreasonable result, for example, ask for 1 but given 3, but there is only one which is the largest
	int nsmax; 
	if (ste_init == 1 && ndataq != 1)
	{
		acnt_max = 0.0; 
		for (i = 0; i < ndataq; i++)	// find the maximum average count
		{
			if (q[i].acnt > acnt_max)
				acnt_max = q[i].acnt;
		}
		nsmax = 0;
		for (i = 0; i < ndataq; i++)	// find the number of maximum average count
		{
			if (q[i].acnt == acnt_max)
				q[nsmax++] = q[i];
		}
		qsize = ndataq = nsmax;
	}

	return 0;
}

//
// select starting edges from the edges eminating from the sinks
// this function is designed for searching brahches
// ==> ignore the edges on the main path (flagged with "mainp")
// put the results into the path queue
//
int select_starting_edges_backward_branches(int ste_init)
{
	int i, j, k;
	double acnt_max;

	for (i = 0; i < qsize; i++) { q[i].tcnt = 0; q[i].len = 0; }

	// initial selection of the starting edges	
	ndataq = 0; 
	for (i = 0; i < nsinks; i++)
	{
		j = sinks[i];
		for (k = 0; k < nw[j].in_deg; k++)
		{
			if (nw[nw[j].in_nbrs[k]].mainp == TRUE) continue;	// avoid edges that is already on the main path (when working on branches)
			if (ndataq >= qsize)	// path queue full, replace the path which has minimum count with a new path
			{
				if (nw[j].in_spx[k] == acnt_min)	// equal the minimum average count
				{
					if (qsize < (MAX_PATH_QUEUE+EXT_PATH_QUEUE))
					{
						add_init_path(j, nw[j].in_nbrs[k], nw[j].in_spx[k]);
						qsize++;
					}
				}
				else if (nw[j].in_spx[k] > acnt_min)
					replace_init_path(j, nw[j].in_nbrs[k], nw[j].in_spx[k]);
			}
			else
				add_init_path(j, nw[j].in_nbrs[k], nw[j].in_spx[k]);
		}
	}

	// check the rationality of the starting edges, for the case of ste_init=1, a special case for one path search
	// may get unreasonable result, for example, ask for 1 but given 3, but there is only one which is the largest
	int nsmax; 
	if (ste_init == 1 && ndataq != 1)
	{
		acnt_max = 0.0; 
		for (i = 0; i < ndataq; i++)	// find the maximum average count
		{
			if (q[i].acnt > acnt_max)
				acnt_max = q[i].acnt;
		}
		nsmax = 0;
		for (i = 0; i < ndataq; i++)	// find the number of maximum average count
		{
			if (q[i].acnt == acnt_max)
				q[nsmax++] = q[i];
		}
		qsize = ndataq = nsmax;
	}

	return 0;
}

//
// replace the path with minimum SPx count with the given new path
//
int replace_init_path(int s, int t, double cnt)
{
	int i, ind, pind;
	double min;

	pind = ind_min;
	q[pind].seq[0] = s;
	q[pind].seq[1] = t;
	q[pind].spx[0] = cnt;
	q[pind].len = 2;
	q[pind].tcnt = cnt;
	q[pind].acnt = cnt / (q[pind].len - 1);
	q[ndataq].kroute_id = -1;	// added 2014/12/26
	min = 9999999999E+30;	// modifided 2012/09/24
	for (i = 0; i < ndataq; i++) { if (q[i].acnt < min) { min = q[i].acnt; ind = i; } }
	ind_min = ind;
	acnt_min = min;

	return 0;
}

//
// add a new path to the path queue
//
int add_init_path(int s, int t, double cnt)
{
	int i, ind;
	double min;

	if (ndataq >= (MAX_PATH_QUEUE+EXT_PATH_QUEUE)) return -1;

	q[ndataq].seq[0] = s;
	q[ndataq].seq[1] = t;
	q[ndataq].spx[0] = cnt;
	q[ndataq].len = 2;
	q[ndataq].tcnt = cnt;
	q[ndataq].acnt = cnt / (q[ndataq].len - 1);
	q[ndataq].kroute_id = -1;	// added 2014/12/26
	ndataq++; 
	min = 9999999999E+30;	// modifided 2012/09/24
	for (i = 0; i < ndataq; i++) { if (q[i].acnt < min) { min = q[i].acnt; ind = i; } }
	ind_min = ind;
	acnt_min = min;

	return 0;
}

//
// establish the array of outward edges in the network
// return the number of outward links found
//
static int lcnt;
static int slevel;
int find_significant_links(int nsl, struct SLINK *sl)
{
	int i, j, k;

	for (i = 0; i < nnodes; i++)	// clear the "was_here" flag 
		nw[i].was_here = FALSE;

	lcnt = 0;
	for (i = 0; i < nnodes; i++)
	{
		if (nw[i].was_here == TRUE)
			continue;
		nw[i].was_here = TRUE;	// mark node, 2011/06/29
		for (k = 0; k < nw[i].out_deg; k++)
		{
			slevel = 0;	
			j = nw[i].out_nbrs[k];
			sl[lcnt].from = i;
			sl[lcnt].to = j;
			sl[lcnt].spx = nw[i].out_spx[k];
			sl[lcnt].relevancy = nw[i].out_relevancy[k];	// added 2017/01/17
			lcnt++;
			if (lcnt == nsl)	// break to avoid memory problem
				break;
			if (nw[j].was_here == TRUE) 
				continue;
			nw[j].was_here = TRUE;
			find_links(j, nw, nsl, sl);	
		}
		if (lcnt == nsl)	// break to avoid memory problem
		{
			fwprintf(logstream, L"\nWARNING: number of links exceed expectation.\n");	// added 2013/05/02
			break;
		}
	}

	// set the year_s and year_t of each link
	for (i = 0; i < lcnt; i++)
	{
		j = nw[sl[i].from].ndx2wos;
		sl[i].year_s = wos[j].year;
		k = nw[sl[i].to].ndx2wos;
		sl[i].year_t = wos[k].year;
		sl[i].year_avg = ((double)sl[i].year_s + (double)sl[i].year_t) / 2.0;
	}

	return lcnt;
}

// 
// recursive, depth-first algorithm
//
int find_links(int nid, struct PN *nwk, int nsl, struct SLINK *sl)
{
	int j, k;
	int new_nd;

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		j = nwk[nid].out_nbrs[k];
		sl[lcnt].from = nid;
		sl[lcnt].to = j;
		sl[lcnt].spx = nw[nid].out_spx[k];
		sl[lcnt].relevancy = nw[nid].out_relevancy[k];	// added 2017/01/17
		lcnt++;
		if (lcnt == nsl)	// break to avoid memory problem
			break;
		if (nwk[j].was_here == TRUE) 
			continue;
		nwk[j].was_here = TRUE;
		find_links(j, nwk, nsl, sl);
	}
	slevel--;

	return 0;
}


//
// find sources and sinks
//
int find_sources_sinks(int byr, int eyr)
{
	int i, j, k, cnt, ndx;
	int indg, outdg;

#ifdef CUT_OFF_YEAR_DATA
	// year data is cutted in the create_new_relationship_list() function, do not handle year winodw here	
	for (i = 0; i < nnodes; i++)
	{	
		if (nw[i].out_deg == 0 && nw[i].in_deg == 0)
			nw[i].type = T_ISOLATE;
		else if (nw[i].out_deg != 0 && nw[i].in_deg == 0)
			nw[i].type = T_SOURCE;
		else if (nw[i].out_deg != 0 && nw[i].in_deg != 0)
			nw[i].type = T_INTERMEDIATE;
		else if (nw[i].out_deg == 0 && nw[i].in_deg != 0)
			nw[i].type = T_SINK;
	}
#else
	for (i = 0; i < nnodes; i++)
	{
		ndx = nw[i].ndx2wos;
		if (wos[ndx].year < byr || wos[ndx].year > eyr)	// outside the spedified window
		{
			outdg = 0; indg = 0;
		}
		else	// inside the specified window
		{
			if (byr == 0)
				outdg = nw[i].out_deg;
			else
			{
				outdg = 0;
				for (k = 0; k < nw[i].out_deg; k++)
				{
					j = nw[i].out_nbrs[k];
					ndx = nw[j].ndx2wos;
					if (wos[ndx].year <= eyr)
						outdg++;
				}	
			}
			if (byr == 0) 
				indg = nw[i].in_deg;
			else
			{
				indg = 0;
				for (k = 0; k < nw[i].in_deg; k++)
				{
					j = nw[i].in_nbrs[k];
					ndx = nw[j].ndx2wos;
					if (wos[ndx].year >= byr)
						indg++;
				}
			}
		}
		if (outdg == 0 && indg == 0)
			nw[i].type = T_ISOLATE;
		else if (outdg != 0 && indg == 0)
			nw[i].type = T_SOURCE;
		else if (outdg != 0 && indg != 0)
			nw[i].type = T_INTERMEDIATE;
		else if (outdg == 0 && indg != 0)
			nw[i].type = T_SINK;
	}
#endif CUT_OFF_YEAR_DATA

	// initialize the sources[] array
	cnt = 0;
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_SOURCE) cnt++; }
	nsources = cnt;
	sources = (int *)Jmalloc(nsources * sizeof(int), L"find_sources_sinks: sources");
	if (sources == NULL) return MSG_NOT_ENOUGH_MEMORY;
	cnt = 0;
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_SOURCE)  sources[cnt++] = i; }
	// initialize the sinks[] array
	cnt = 0;
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_SINK) cnt++; }
	nsinks = cnt;
	sinks = (int *)Jmalloc(nsinks * sizeof(int), L"find_sources_sinks: sinks");
	if (sinks == NULL) return MSG_NOT_ENOUGH_MEMORY;
	cnt = 0;
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_SINK)  sinks[cnt++] = i; }

//#ifdef DEBUG1
	cnt = 0;
	fwprintf(logstream, L"Isolates:\n");
	fwprintf(logstream, L"Count\tCited\tInDeg\tOutDeg\tNodeName\n");
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_ISOLATE) { cnt++; fwprintf(logstream, L"%d\t%d\t%d\t%d\t%s\n", cnt, wos[nw[i].ndx2wos].tc, nw[i].in_deg, nw[i].out_deg, nw[i].alias); } }
	fwprintf(logstream, L"\n");
	n_isolates = cnt;

	cnt = 0;
	fwprintf(logstream, L"Sources:\n");
	fwprintf(logstream, L"Count\tCited\tInDeg\tOutDeg\tNodeName\n");
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_SOURCE) { cnt++; fwprintf(logstream, L"%d\t%d\t%d\t%d\t%s\n", cnt, wos[nw[i].ndx2wos].tc, nw[i].in_deg, nw[i].out_deg, nw[i].alias); } }
	fwprintf(logstream, L"\n");
	n_sources = cnt;

	cnt = 0;
	fwprintf(logstream, L"Sinks:\n");
	fwprintf(logstream, L"Count\tCited\tInDeg\tOutDeg\tNodeName\n");
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_SINK) { cnt++; fwprintf(logstream, L"%d\t%d\t%d\t%d\t%s\n", cnt, wos[nw[i].ndx2wos].tc, nw[i].in_deg, nw[i].out_deg, nw[i].alias); } }
	fwprintf(logstream, L"\n");
	n_sinks = cnt;

	cnt = 0;
	fwprintf(logstream, L"Intermediates:\n");
	fwprintf(logstream, L"Count\tCited\tInDeg\tOutDeg\tNodeName\n");
	for (i = 0; i < nnodes; i++) { if (nw[i].type == T_INTERMEDIATE) { cnt++; fwprintf(logstream, L"%d\t%d\t%d\t%d\t%s\n", cnt, wos[nw[i].ndx2wos].tc, nw[i].in_deg, nw[i].out_deg, nw[i].alias); } }
	fwprintf(logstream, L"\n");
	n_intermediates = cnt;
//#endif DEBUG1

	return 0;
}

//
// write the keywords statistics to the file
//
#define KBUFSIZE 1000
#define DBUFSIZE 1000
struct KWT { int ndx; int cnt; };
int output_keyword_info(wchar_t *fname)
{
	int i, k, kk, ndx, cnt;
	int nkws, ndocs;
	struct KWT kwt[KBUFSIZE];
	int doct[DBUFSIZE];
	int prev;

	// put all nodes in the path into the buffer doct[]
	cnt = 0;
	for (i = 0; i < qsize; i++)
	{ 
		if (q[i].tcnt == 0 || q[i].hit_a_sink == FALSE)
			continue;
		for (k = 0; k < q[i].len; k++)
			doct[cnt++] = q[i].seq[k];
	}
	// sort documents by the index
	qsort((void *)doct, (size_t)cnt, sizeof(int), compare_dndx);

	// consolidate duplicate documents (nodes)
	prev = -1;
	k = 0;
	for (i = 0; i < cnt; i++)
	{
		if (doct[i] != prev)	// hit a new index
		{
			doct[k] = doct[i]; 
			prev = doct[i];
			k++;
		}
	}
	ndocs = k;

// find DE keywords from all documents
	nkws = 0; 
	for (i = 0; i < ndocs; i++)
	{ 
		//fwprintf(logstream, L"%s ", nw[doct[i]].name);
		ndx = nw[doct[i]].ndx2wos;
		for (kk = 0; kk < wos[ndx].nde; kk++)
			kwt[nkws++].ndx = wos[ndx].DE[kk];
	}

	// sort keywords by the index
	qsort((void *)kwt, (size_t)nkws, sizeof(struct KWT), compare_kndx);

	// consolidate duplicate keywords
	prev = -1;
	k = 0; kwt[0].cnt = 1;
	for (i = 0; i < nkws; i++)
	{
		if (kwt[i].ndx != prev)	// hit a new name
		{
			kwt[k].ndx = kwt[i].ndx; 
			prev = kwt[i].ndx;
			k++; kwt[k].cnt = 1;
		}
		else	// same name
			kwt[k].cnt++;
	}
	nkws = k;

	// sort by the count
	qsort((void *)kwt, (size_t)nkws, sizeof(struct KWT), compare_kcnt);
	fwprintf(logstream, L"\n%s Author Keywords (from %d documents):\n", fname, ndocs);
	for (i = 0; i < nkws; i++)
	{
		kk = kwt[i].ndx;
		fwprintf(logstream, L"%03d %s\n", kwt[i].cnt, kwde[kk].name);
	}
	fwprintf(logstream, L"\n");

// find ID keywords from all documents
	nkws = 0; 
	for (i = 0; i < ndocs; i++)
	{ 
		//fwprintf(logstream, L"%s ", nw[doct[i]].name);
		ndx = nw[doct[i]].ndx2wos;
		for (kk = 0; kk < wos[ndx].nid; kk++)
			kwt[nkws++].ndx = wos[ndx].ID[kk];
	}

	// sort keywords by the index
	qsort((void *)kwt, (size_t)nkws, sizeof(struct KWT), compare_kndx);

	// consolidate duplicate keywords
	prev = -1;
	k = 0; kwt[0].cnt = 1;
	for (i = 0; i < nkws; i++)
	{
		if (kwt[i].ndx != prev)	// hit a new name
		{
			kwt[k].ndx = kwt[i].ndx; 
			prev = kwt[i].ndx;
			k++; kwt[k].cnt = 1;
		}
		else	// same name
			kwt[k].cnt++;
	}
	nkws = k;

	// sort by the count
	qsort((void *)kwt, (size_t)nkws, sizeof(struct KWT), compare_kcnt);
	fwprintf(logstream, L"\n%s Keyword Plus Keywords (from %d documents):\n", fname, ndocs);
	for (i = 0; i < nkws; i++)
	{
		kk = kwt[i].ndx;
		fwprintf(logstream, L"%03d %s\n", kwt[i].cnt, kwid[kk].name);
	}
	fwprintf(logstream, L"\n");

	return 0;
}

//
// write the resulting paths to a relationship list file
//
#define BUFSIZE 1000
int to_relationship_list(wchar_t *fname, int mark_mp, int mode, int mptype, int direction)
{
	int i, k;
	FILE *ostream;
	wchar_t buf1[BUFSIZE], buf2[BUFSIZE];
	wchar_t buf3[BUFSIZE], buf4[BUFSIZE];
	wchar_t buf5[BUFSIZE], buf6[BUFSIZE];
	int cnt;

#ifdef NOT_ANYMORE	// these code become obsolete on 2011/02/08
	double sum, avg;
	// find a proper scaling factor for the link weight (otherwise the link weight too big will cause trouble in drawing the network)
	sum = 0.0; cnt = 0;
	for (i = 0; i < qsize; i++)
	{ 
		if (mptype == P_LOCAL_F && (q[i].tcnt == 0 || q[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (q[i].tcnt == 0 || q[i].hit_a_source == FALSE)) continue;
		for (k = 0; k < (q[i].len-1); k++) 
		{ 
			if (nw[q[i].seq[k]].type == T_ISOLATE) continue;	// there can be "isolate" nodes that are cut-off by the time window, ignore them
			if (nw[q[i].seq[k+1]].type == T_ISOLATE) continue;	// there can be "isolate" nodes that are cut-off by the time window, ignore them
			cnt++; sum += q[i].spx[k]; 
		}
	}
	avg = sum / cnt;
	if (scale == -1.0)	// set scale only once
	{
		scale = 1;
		while ((avg / 10.0) >= 1.0) { avg = avg / 10.0; scale = scale * 10.0; }
		fwprintf(logstream, L"\nScale for \"%s\" = %d\n\n", fname, (int)scale);
	}
#endif NOT_ANYMORE 

	if (mode == F_NEW)
		_wfopen_s(&ostream, fname, L"wt, ccs=UTF-8");	// modified 2016/01/19
	else
		_wfopen_s(&ostream, fname, L"at, ccs=UTF-8");	// modified 2016/01/19

	// NOTE: '*' denotes that the node is on the main path, '@' denotes that the node is a special node, '|' denotes that the node is a brach node 
	for (i = 0; i < qsize; i++)
	{ 
		if (mptype == P_LOCAL_F && (q[i].tcnt == 0 || q[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (q[i].tcnt == 0 || q[i].hit_a_source == FALSE)) continue;
		for (k = 0; k < (q[i].len-1); k++)
		{
			if (nw[q[i].seq[k]].type == T_ISOLATE) continue;	// there can be "isolate" nodes that are cut-off by the time window, ignore them
			if (nw[q[i].seq[k+1]].type == T_ISOLATE) continue;	// there can be "isolate" nodes that are cut-off by the time window, ignore them
			if (nw[q[i].seq[k]].mainp == TRUE && mark_mp) swprintf_s(buf5, BUFSIZE-1, L"*%s", nw[q[i].seq[k]].name); else wcscpy_s(buf5, BUFSIZE-1, nw[q[i].seq[k]].name);
			if (nw[q[i].seq[k+1]].mainp == TRUE && mark_mp) swprintf_s(buf6, BUFSIZE-1, L"*%s", nw[q[i].seq[k+1]].name); else wcscpy_s(buf6, BUFSIZE-1, nw[q[i].seq[k+1]].name);
			if (nw[q[i].seq[k]].special == TRUE && mark_mp) swprintf_s(buf3, BUFSIZE-1, L"@%s", buf5); else wcscpy_s(buf3, BUFSIZE-1, buf5);	// added 2011/06/03
			if (nw[q[i].seq[k+1]].special == TRUE && mark_mp) swprintf_s(buf4, BUFSIZE-1, L"@%s", buf6); else wcscpy_s(buf4, BUFSIZE-1, buf6);	// added 2011/06/03
			if (nw[q[i].seq[k]].branch == TRUE && mark_mp) swprintf_s(buf1, BUFSIZE-1, L"|%s", buf3); else wcscpy_s(buf1, BUFSIZE-1, buf3);	// added 2011/06/04
			if (nw[q[i].seq[k+1]].branch == TRUE && mark_mp) swprintf_s(buf2, BUFSIZE-1, L"|%s", buf4); else wcscpy_s(buf2, BUFSIZE-1, buf4);	// added 2011/06/04
			if (direction == MFORWARD)
				fwprintf(ostream, L"%s %s %.2f\n", buf1, buf2, (q[i].spx[k] * 10 / scale));
			else
				fwprintf(ostream, L"%s %s %.2f\n", buf2, buf1, (q[i].spx[k] * 10 / scale));
		}
	}
	fclose(ostream);

	return 0;
}

//
// write the key-routes relationships to the file
//
int key_routes_relationships(wchar_t *fname, int nsl, struct SLINK *sl)
{
	int i;
	wchar_t buf1[BUFSIZE], buf2[BUFSIZE];
	wchar_t buf3[BUFSIZE], buf4[BUFSIZE];
	wchar_t buf5[BUFSIZE], buf6[BUFSIZE];
	FILE *ostream;	

	_wfopen_s(&ostream, fname, L"at, ccs=UTF-8");	// modified 2016/01/19

	for (i = 0; i < nsl; i++)
	{
		if (nw[sl[i].from].type == T_ISOLATE) continue;	// there can be "isolate" nodes that are cut-off by the time window, ignore them
		if (nw[sl[i].to].type == T_ISOLATE) continue;	// there can be "isolate" nodes that are cut-off by the time window, ignore them
		if (nw[sl[i].from].mainp == TRUE) swprintf_s(buf5, BUFSIZE-1, L"*%s", nw[sl[i].from].name); else wcscpy_s(buf5, BUFSIZE-1, nw[sl[i].from].name);
		if (nw[sl[i].to].mainp == TRUE) swprintf_s(buf6, BUFSIZE-1, L"*%s", nw[sl[i].to].name); else wcscpy_s(buf6, BUFSIZE-1, nw[sl[i].to].name);
		if (nw[sl[i].from].special == TRUE) swprintf_s(buf3, BUFSIZE-1, L"@%s", buf5); else wcscpy_s(buf3, BUFSIZE-1, buf5);	// added 2011/06/03
		if (nw[sl[i].to].special == TRUE) swprintf_s(buf4, BUFSIZE-1, L"@%s", buf6); else wcscpy_s(buf4, BUFSIZE-1, buf6);		// added 2011/06/03
		if (nw[sl[i].from].branch == TRUE) swprintf_s(buf1, BUFSIZE-1, L"|%s", buf3); else wcscpy_s(buf1, BUFSIZE-1, buf3);		// added 2011/06/04
		if (nw[sl[i].to].branch == TRUE) swprintf_s(buf2, BUFSIZE-1, L"|%s", buf4); else wcscpy_s(buf2, BUFSIZE-1, buf4);		// added 2011/06/04
		fwprintf(ostream, L"%s %s %.2f\n", buf1, buf2, sl[i].spx * 10 / scale);
	}

	fclose(ostream);

	return 0;
}

//
// consolidate the relatonship list file (because there are many duplicated relationships)
//
int consolidate_relationship_list(wchar_t *iname)
{	
	int i, k;
	int nlines;
	FILE *tstream, *ostream;
	wchar_t tname[FNAME_SIZE], oname[FNAME_SIZE];
	wchar_t line[SBUF_SIZE+1], *tline;
	wchar_t prev[MAX_RELATIONS]; 
	struct RELATIONSHIPS *rlist;

	wcscpy_s(oname, FNAME_SIZE-1, iname);
	// change temporary file name everytime, because file system may not be quick enough to delete it before the next call comes in
	swprintf_s(tname, FNAME_SIZE-1, L"MP_Relationship_TMP%03d.txt", tmpname_counter++);	
	_wrename(iname, tname);	// give the input file a new temporary name
	if (_wfopen_s(&tstream, tname, L"rt, ccs=UTF-8") != 0)	// modified 2016/01/19
	{
		fwprintf(logstream, L"ERROR: Cannot open file %s.\n", tname);
		return MSG_FILE_CANNOTOPEN;
	}
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/19
	{
		fwprintf(logstream, L"ERROR: Cannot open file %s.\n", oname);
		return MSG_FILE_CANNOTOPEN;
	}


	// 1st pass, count the toal number of input lines
	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, tstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nlines++;	
	}

	// allocate memory for the list array (with duplications)
	rlist = (struct RELATIONSHIPS *)Jmalloc(nlines * sizeof(struct RELATIONSHIPS), L"consolidate_relationship_list: rlist");
	if (rlist == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nlines; i++) rlist[i].r[0] = '\0';	// initialize the strcuture

	// 2nd pass, read-in the relationship list
	rewind(tstream);	// point back to the begining of the file
	i = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, tstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;	
		tline = line;
		if (i == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				tline = &line[1];	// skip the BOM
		}
		wcscpy_s(rlist[i].r, MAX_RELATIONS-1, tline);
		i++;
	}

	qsort((void *)rlist, (size_t)nlines, sizeof(struct RELATIONSHIPS), compare_rls);

	// 3rd pass, consolidate the duplicated relationship description
	prev[0] = '\0';
	k = 0;
	for (i = 0; i < nlines; i++)
	{
		if (wcscmp(rlist[i].r, prev) != 0)	// hit a new relationship description
		{
			fwprintf(ostream, L"%s", rlist[i].r);
			wcscpy_s(prev, MAX_RELATIONS-1, rlist[i].r);
			k++;
		}
	}

	//fwprintf(logstream, L"Before malloc()\n");  fflush(logstream);
	//void *addr;
	//addr = (void *)malloc(1000);
	//free(addr);
	//fwprintf(logstream, L"After free()\n");  fflush(logstream);

	fclose(tstream);
	fclose(ostream);
	if (_wunlink(tname) == -1)	// delete the temporary relationship-list file (the original input file)
	{ fwprintf(logstream, L"WARNING: Cannot delete file %s.\n", tname); fflush(logstream); }
	Jfree(rlist, L"consolidate_relationship_list: rlist");

	return 0;
}

//
// create the mainpath nodes' alias table
//
int to_path_nodes_alias_table(wchar_t *fname)
{
	int i, k, m;
	FILE *ostream;

	_wfopen_s(&ostream, fname, L"wt, ccs=UTF-8");	// modified 2016/01/19

	for (i = 0; i < qsize; i++)
	{ 
		if (q[i].tcnt == 0 || q[i].hit_a_sink == FALSE)
			continue;
		for (k = 0; k < q[i].len; k++)
		{
			m = q[i].seq[k];
			if (nw[m].mainp == TRUE) 
				fwprintf(ostream, L"%s %s\n", wos[nw[m].ndx2wos].docid, nw[m].name);
		}
	}

	fclose(ostream);

	return 0;
}


//
// use binary search to find the proper position of a node name in the nw[] array
//
int nname_search(struct PN a[], int num, wchar_t *str)
{
	int low, high, cur;

	if (a == (struct PN *)0) return -1;
	if (wcscmp(str, a[0].name) < 0) return -1;

	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, a[cur].name) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, a[high].name) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, a[cur].name) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// this fucntion is to be called by qsort() only
// 
int compare_dndx(const void *n1, const void *n2)
{
	int *t1, *t2;
	
	t1 = (int *)n1;
	t2 = (int *)n2;
	if (*t2 < *t1)
		return 1;
	else if (*t2 == *t1)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_kndx(const void *n1, const void *n2)
{
	struct KWT *t1, *t2;
	
	t1 = (struct KWT *)n1;
	t2 = (struct KWT *)n2;
	if (t2->ndx < t1->ndx)
		return 1;
	else if (t2->ndx == t1->ndx)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_kcnt(const void *n1, const void *n2)
{
	struct KWT *t1, *t2;
	
	t1 = (struct KWT *)n1;
	t2 = (struct KWT *)n2;
	if (t2->cnt > t1->cnt)
		return 1;
	else if (t2->cnt == t1->cnt)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_rls(const void *n1, const void *n2)
{
	struct RELATIONSHIPS *t1, *t2;
	
	t1 = (struct RELATIONSHIPS *)n1;
	t2 = (struct RELATIONSHIPS *)n2;
	if (wcscmp(t2->r, t1->r) < 0)
		return 1;
	else if (wcscmp(t2->r, t1->r) == 0)
		return 0;
	else return -1;
}

//
// given the number of leading codes
// find the number of IPC codes (after consolidating the same codes)
//
struct IPC_CODE 
{
	wchar_t code[MAX_IPC_CODE];
	int ipc_cnt;	// added 2016/05/26, this is used in the function replace_IPC_codes()
};
int count_IPC_codes(int ndx, int n_leading_chars)
{
	int i, j, k;
	int nipc;
	wchar_t *sp, *tp;
	struct IPC_CODE ipc[MAX_IPC];	// changed from 50 to MAX_IPC, 2014/07/14

	nipc = wos[ndx].nipc;
	for (k = 0; k < wos[ndx].nipc; k++)	// write IPC inofrmation
	{
		sp = &wos[ndx].ipc[MAX_IPC_CODE*k];
		tp = ipc[k].code;
		j = 0;
		while (j < n_leading_chars) { j++; *tp++ = *sp++; }
		*tp = '\0';
	}

	qsort((void *)ipc, (size_t)nipc, sizeof(struct IPC_CODE), compare_ipccode);
	// consolidate duplicate IPC code
	wchar_t prev_name[MAX_IPC_CODE];
	prev_name[0] = '\0';
	k = 0;
	for (i = 0; i < nipc; i++)
	{
		if (wcscmp(ipc[i].code, prev_name) != 0)	// hit new name
		{
			wcscpy_s(ipc[k++].code, MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(prev_name, MAX_IPC_CODE, ipc[i].code); 
		}
	}
	nipc = k;

	return nipc;
}

//
// given the number of leading codes
// replace the existing nipc and ipc[] after observing the IPCs as the given number of leading codes
//
int replace_IPC_codes(int ndx, int n_leading_chars, struct WOS *wosx, struct USPTO *usptox)
{
	int i, j, k;
	int nipc, cnt;
	wchar_t *sp, *tp;
	struct IPC_CODE ipc[MAX_IPC];	// changed from 50 to MAX_IPC, 2014/07/14

#ifdef DEBUG
	fwprintf(logstream, L"%s:\t", wosx[ndx].alias);
	for (i = 0; i < wosx[ndx].nipc; i++)
		fwprintf(logstream, L"\t%s", &wosx[ndx].ipc[MAX_IPC_CODE*i]);
	fwprintf(logstream, L"=>\t");
#endif DEBUG

	nipc = wosx[ndx].nipc;
	for (k = 0; k < wosx[ndx].nipc; k++)	// write IPC inofrmation
	{
		sp = &wosx[ndx].ipc[MAX_IPC_CODE*k];
		tp = ipc[k].code;
		j = 0;
		while (j < n_leading_chars) { j++; *tp++ = *sp++; }
		*tp = '\0';
	}

	qsort((void *)ipc, (size_t)nipc, sizeof(struct IPC_CODE), compare_ipccode);
	// consolidate duplicate IPC code
	wchar_t prev_name[MAX_IPC_CODE];
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nipc; i++)
	{
		if (wcscmp(ipc[i].code, prev_name) != 0)	// hit new name
		{		
			if (k > 0) 
			{
				ipc[k-1].ipc_cnt = cnt;
				wosx[ndx].ipc_cnt[k-1] = cnt;
				usptox[ndx].ipc_cnt[k-1] = cnt;
			}
			wcscpy_s(&wosx[ndx].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(&usptox[ndx].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(ipc[k++].code, MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(prev_name, MAX_IPC_CODE, ipc[i].code); 
			cnt = 1;
		}
		else
			cnt++;
	}
	ipc[k-1].ipc_cnt = cnt;
	wosx[ndx].ipc_cnt[k-1] = cnt;
	usptox[ndx].ipc_cnt[k-1] = cnt;
	wosx[ndx].nipc = k;
	usptox[ndx].nipc = k;

#ifdef DEBUG
	for (i = 0; i < wosx[ndx].nipc; i++)
		fwprintf(logstream, L"\t%s", &wosx[ndx].ipc[MAX_IPC_CODE*i]);
	fwprintf(logstream, L"\n");
#endif DEBUG

	return k;
}

//
// replace the existing nipc and ipc[] after observing the CPC4
// CPC4 is CPC code up to the '/'
// this function is used only when CPC (rather than IPC) information is used 
//
int replace_CPC4_codes(int ndx, struct WOS *wosx, struct USPTO *usptox)
{
	int i, k;
	int nipc, cnt;
	wchar_t *sp, *tp;
	struct IPC_CODE ipc[MAX_IPC];	

#ifdef DEBUG
	fwprintf(logstream, L"%s:\t", wosx[ndx].alias);
	for (i = 0; i < wosx[ndx].nipc; i++)
		fwprintf(logstream, L"\t%s", &wosx[ndx].ipc[MAX_IPC_CODE*i]);
	fwprintf(logstream, L"=>\t");
#endif DEBUG

	// here, we assume that wos[].ipc_cnt[] are all equal to 1
	nipc = wosx[ndx].nipc;
	for (k = 0; k < wosx[ndx].nipc; k++)	// get IPC (CPC) inofrmation
	{
		sp = &wosx[ndx].ipc[MAX_IPC_CODE*k];
		tp = ipc[k].code;
		while (*sp != '/') { *tp++ = *sp++; }
		*tp = '\0';
	}

	qsort((void *)ipc, (size_t)nipc, sizeof(struct IPC_CODE), compare_ipccode);
	// consolidate duplicate IPC code
	wchar_t prev_name[MAX_IPC_CODE];
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nipc; i++)
	{
		if (wcscmp(ipc[i].code, prev_name) != 0)	// hit new name
		{
			if (k > 0) 
			{
				ipc[k-1].ipc_cnt = cnt;
				wosx[ndx].ipc_cnt[k-1] = cnt;
				usptox[ndx].ipc_cnt[k-1] = cnt;
			}
			wcscpy_s(&wosx[ndx].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(&usptox[ndx].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(ipc[k++].code, MAX_IPC_CODE, ipc[i].code); 
			wcscpy_s(prev_name, MAX_IPC_CODE, ipc[i].code); 
			cnt = 1;
		}
		else
			cnt++;
	}
	ipc[k-1].ipc_cnt = cnt;
	wosx[ndx].ipc_cnt[k-1] = cnt;
	usptox[ndx].ipc_cnt[k-1] = cnt;
	wosx[ndx].nipc = k;
	usptox[ndx].nipc = k;

#ifdef DEBUG
	for (i = 0; i < wosx[ndx].nipc; i++)
		fwprintf(logstream, L"\t%s", &wosx[ndx].ipc[MAX_IPC_CODE*i]);
	fwprintf(logstream, L"\n");
#endif DEBUG

	return k;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_ipccode(const void *n1, const void *n2)
{
	struct IPC_CODE *t1, *t2;
	
	t1 = (struct IPC_CODE *)n1;
	t2 = (struct IPC_CODE *)n2;
	if (wcscmp(t2->code, t1->code) < 0)
		return 1;
	else if (wcscmp(t2->code, t1->code) == 0)
		return 0;
	else return -1;
}

//
// find patents earlier than the given patent
// input: patent citation file and a given patent
//
struct PATCITATION
{
	wchar_t pid[MAX_PATENT_ID];
	int tc;
};
int find_earlier_patents(wchar_t *cname, int separator)
{
	wchar_t line[LBUF_SIZE+1], *tline;
	struct PATCITATION *ptct;
	FILE *cstream;		// for citation file
	int i;
	int nlines, cnt;
	wchar_t earliest[MAX_PATENT_ID];

	wcscpy(earliest, L"zzzzz");
	// determine the earliest patents among all the leading patents
	for (i = 0; i < qsize; i++)
	{
		if (wcscmp(nw[q[i].seq[0]].name, earliest) < 0)
			wcscpy(earliest, nw[q[i].seq[0]].name);
	}
	fwprintf(logstream, L"\nThe earliest patent on the main path: %s\n", earliest);
	fwprintf(logstream, L"\Patents earlier than this patent (top 20 cited):\n", earliest);

// 
// Open the given citatioin file (will fail if the file does not exist)
//	
	if (_wfopen_s(&cstream, cname, L"rt, ccs=UTF-8") != 0)	// modified 2016/01/19
		return MSG_IFILE_NOTFOUND;

	// 1st pass, count the toal number of input lines
	nlines = 0;
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, cstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nlines++;	
	}

	// allocate memory for the citation array (with duplications)
	ptct = (struct PATCITATION *)Jmalloc(nlines * sizeof(struct PATCITATION), L"find_earlier_patents: ptct");
	if (ptct == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nlines; i++) 
		ptct[i].tc = 1;

	// 2nd pass, find the total number of unique nodes
	double weight;
	wchar_t name1[MAX_PATENT_ID+1], name2[MAX_PATENT_ID+1];
	cnt = 0; 
	rewind(cstream);	// point back to the begining of the file
	while (TRUE)
	{		
		if(fgetws(line, LBUF_SIZE, cstream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;	
		tline = line;
		if (cnt == 0)	// check if there is byte-order-mark (BOM) for UTF-16, added 2016/02/01
		{
			if (line[0] == 0xfeff || line[0] == 0xfffe)	// UTF-16 (big-endian): "FE FF", (little-endian) : "FF FE"
				tline = &line[1];	// skip the BOM
		}
		weight = 0.0;
		parse_line(tline, separator, name1, name2, &weight);
		wcscpy(ptct[cnt].pid, name1);
		cnt++;	
	}

	qsort((void *)ptct, (size_t)cnt, sizeof(struct PATCITATION), compare_patid);

	// consolidate duplicate names
	wchar_t prev_name[MAX_PATENT_ID+1];
	wcscpy_s(prev_name, MAX_PATENT_ID, ptct[0].pid);
	int nc, np;
	nc = 1; np = 1;
	for (i = 1; i < cnt; i++)
	{
		if (wcscmp(ptct[i].pid, prev_name) != 0)
		{			
			ptct[np-1].tc = nc;
			wcscpy_s(ptct[np].pid, MAX_PATENT_ID, ptct[i].pid);
			nc = 1; np++;
			wcscpy_s(prev_name, MAX_PATENT_ID, ptct[i].pid);
		}
		else	
		{
			nc++;
		}
	}
	// found total np unique patents

	fclose(cstream);

	// sort again by the number of citations
	qsort((void *)ptct, (size_t)cnt, sizeof(struct PATCITATION), compare_pattc);

	// display the top 20 cited patents
	cnt = 0;
	for (i = 0; i < np; i++)
	{		
		if (wcscmp(ptct[i].pid, earliest) < 0)
		{
			fwprintf(logstream, L"%s %d\n", ptct[i].pid, ptct[i].tc);
			cnt++;
			if (cnt >= 20)
				break;
		}
	}

	Jfree(ptct, L"find_earlier_patents: ptct");

	return 0;
}

//
// if there is a tie at the end of the top edges, take more
//
int find_actual_top_edges(int n_top_edges, int nslinks, struct SLINK *slinks)
{
	int i;	
	double spx;

	spx = slinks[n_top_edges-1].spx;
	for (i = n_top_edges; i < nslinks; i++)
	{
		if (slinks[i].spx < spx)
			break;
	}

	return i;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_patid(const void *n1, const void *n2)
{
	struct PATCITATION *t1, *t2;
	
	t1 = (struct PATCITATION *)n1;
	t2 = (struct PATCITATION *)n2;
	if (wcscmp(t2->pid, t1->pid) < 0)
		return 1;
	else if (wcscmp(t2->pid, t1->pid) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_pattc(const void *n1, const void *n2)
{
	struct PATCITATION *t1, *t2;
	
	t1 = (struct PATCITATION *)n1;
	t2 = (struct PATCITATION *)n2;
	if (t2->tc > t1->tc)
		return 1;
	else if (t2->tc == t1->tc)
		return 0;
	else return -1;
}

//
// write a Pajek file from nw[]
//
int nw2Pajek(int wtype, wchar_t *spxname)
{
	int i, k;
	wchar_t wline[LBUF_SIZE+1];
	FILE *ostream;

	if (_wfopen_s(&ostream, spxname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/19
			return -1;
	fwprintf(ostream, L"*Network prepared by Mainpath program\n");
	fwprintf(ostream, L"*Vertices %d\n", nnodes);
	for (i = 0; i < nnodes; i++)
		fwprintf(ostream, L"%d \"%s\"\n", i+1, nw[i].alias); 

	fwprintf(ostream, L"*Arcs\n");

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			if (wtype == 1)
				fwprintf(ostream, L"%d %d %.6f\n", i+1, nw[i].out_nbrs[k]+1, nw[i].out_spx[k]);
			else
				fwprintf(ostream, L"%d %d %.6f\n", i+1, nw[i].out_nbrs[k]+1, nw[i].out_relevancy[k]);
		}
	}

	fclose(ostream);

	return 0;
}

//
// combine the paths from the forward search and the backward search
// backward search queue information is in "q"
// put the results in "q"
//
int combine_local_keyroute_path_queues(int qsize_f, SPATH *q_f, int *qsize, SPATH *q)
{
	int i, j, k, m, s1, s2;
	int btail, bhead, ftail, fhead;
	int qsize_b;
	SPATH *q_b;

	q_b = (struct SPATH *)Jmalloc(*qsize * sizeof(struct SPATH), L"combine_local_keyroute_path_queues: q_tmp");
	if (q_b == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < *qsize; i++) q_b[i] = q[i];	// this contains the paths from the backward search
	qsize_b = *qsize;
	// reverse the sequence of paths in the backward search queue
	for (i = 0; i < qsize_b; i++)
	{
		for (k = 0; k < q_b[i].len/2; k++)
		{
			s1 = q_b[i].seq[k]; 
			s2 = q_b[i].seq[q_b[i].len-1-k];
			q_b[i].seq[k] = s2;
			q_b[i].seq[q_b[i].len-1-k] = s1;
		}
	}
	// now, combine backward and forward search results
	// need to get all possible combinations
	m = 0;
	for (i = 0; i < qsize_b; i++)
	{
		btail = q_b[i].seq[q_b[i].len-2];
		bhead = q_b[i].seq[q_b[i].len-1];
		for (j = 0; j < qsize_f; j++)
		{
			ftail = q_f[j].seq[0];
			fhead = q_f[j].seq[1];
			if (btail == ftail && bhead == fhead)	// only when head and tail matches
			{
				for (k = 0; k < q_b[i].len-1; k++) q[m].seq[k] = q_b[i].seq[k];	// ignore the last node in backward sequence
				for (k = 0; k < q_f[j].len-1; k++) q[m].seq[k+q_b[i].len-1] = q_f[j].seq[k+1];	// ignore the first node in the forward sequence
				q[m].len = q_b[i].len + q_f[j].len - 2;	// less the duplicate nodes
				q[m].acnt = q[m].tcnt = 0;	// set the count to zero for now, they can be recalculted if their value is needed!
				q[m].kroute_id = -1;	// the value is -1, for now!
				m++;
			}
		}
	}
	*qsize = m;

	Jfree(q_b, L"combine_local_keyroute_path_queues: q_b");

	return 0;
}

//
// combine the paths from the forward search and the backward search 
// put the results in "q"
//
int combine_global_keyroute_path_queues(int qsize_f, SPATH *q_f, int qsize_b, SPATH *q_b, int *qsize, SPATH *q)
{
	int i, j, k, m, s1, s2;
	int krid_f, krid_b;

	// now, combine backward and forward search results
	// need to get all possible combinations
	m = 0;
	for (i = 0; i < qsize_b; i++)
	{
		krid_b = q_b[i].kroute_id;
		for (j = 0; j < qsize_f; j++)
		{
			krid_f = q_f[j].kroute_id;
			if (krid_f == krid_b)	// only when they are extended from the same key-route
			{
				for (k = 0; k < q_b[i].len; k++) q[m].seq[k] = q_b[i].seq[k];	
				for (k = 0; k < q_f[j].len; k++) q[m].seq[k+q_b[i].len] = q_f[j].seq[k];
				q[m].len = q_b[i].len + q_f[j].len;	
				q[m].acnt = q[m].tcnt = 0;	// set the count to zero for now, they can be recalculted if their value is needed!
				q[m].kroute_id = krid_f;
				m++;
			}
		}
	}
	*qsize = m;

	return 0;
}