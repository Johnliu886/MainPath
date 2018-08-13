//
// network data structure 
//

#define MAX_NODE_NAME 200

#define FT_ASCII 1
#define FT_UTF_8 2
#define FT_UTF_16_LE 3
#define FT_UTF_16_BE 4
#define FT_UTF_32_LE 5
#define FT_UTF_32_BE 6

#define T_SOURCE 1
#define T_SINK 2
#define T_INTERMEDIATE 3
#define T_ISOLATE 4

#define WRITE_NODE_ID 1
#define WRITE_NODE_NAME 2

// type of node, NOTE: the number assigned affect a node's color in Pajek
#define NINTERMEDIATE 1	
#define NSOURCE 2
#define NMAINPATH 3
#define NTARGET 4

#define XXLBUF_SIZE 65536*4	// extended from 65536, 2010/08/12
#define LBUF_SIZE 16384	// changed from 16384 to 8192, 2012/09/19
#define SBUF_SIZE 4096	// added 2012/01/25

#define MAX_AUTHORS 100		// increased from 20 to 100, 2012/12/13
#define MAX_TKEYWORDS 300	// added 2013/10/16, changed from 100 to 150, 2015/06/11, to 300 again, 2015/07/10
#define MAX_KEYWORDS 20
#define MAX_DOC_ID 30
#define MAX_AUTHOR_NAME 250	// for EU patents, the inventor names can be very long, 2011/03/07, extended again from 200 to 250, 2013/04/01
#define MAX_KEYWORD_NAME 150
#define MAX_DEPARTMENT_NAME 100	// added 2016/01/26
#define MAX_ALIAS MAX_AUTHOR_NAME+MAX_AUTHORS+6	// added +MAX_AUTHORS, 2013/01/30
#define MAX_JOURNAL_NAME 600	// increased from 300 to 600, 2013/03/23
#define MAX_JOURNAL_ABBR 80		// added 2014/07/07, extended to 80, 2017/02/12
#define MAX_DISCIPLINE_NAME 100
#define MAX_INVENTORS 50 // changed from 20 to 50, 2013/07/17
#define MAX_IPC 120		// the maximum number of IPCs for a patent, extended from 80 to 120, 2017/01/05
#define MAX_IPC_CODE 15	// the length of IPC code
#define MAX_USC 80		// the maximum number of US Class for a patent
#define MAX_USC_CODE 15	// the length of USC code
#define MAX_DOI_LEN 80	// extended from 50 to 60 on 2011/12/04, extended again to 80 on 2012/03/29
#define MAX_ASSIGNEE_NAME 512 // changed from 200 to 512
#define MAX_ASSIGNEES 15	// extended from 5 to 10, 2013/07/26, extended again to 16, 2016/12/31
#define MAX_TITLES 500		// extended from 300 to 400, 2014/07/06, to 500 2014/07/22
#define MAX_ISIC_CODE 25	// the length of ISIC code, International Standard Industrial Classification of All Economic Activities (ISIC - rev. 2) of the United Nations
#define MAX_LAWSNOTE_NAME	60	// added 2017/06/02
#define MAX_LAWSNOTE_ID	400		// added 2017/06/02
#define MAX_COUNTRY_NAME 200	// added 2013/04/10
#define MAX_COUNTRIES 500		// added 2013/04/10
#define MAX_COUNTRIES_RP 100	// added 2013/04/10, extended from 2 to 100
#define MAX_COUNTRIES_ASSIGNEE 100		// added 2017/01/04, reduced from 1500 to 100, 2018/03/16

#define DIRECT_BOTH 1
#define DIRECT_OUTWARD 2
#define DIRECT_INWARD 3

#define N_BASE	50
#define N_SBASE	10				// added 2015/10/09
#define N_INCREMENT 20

#define RW_MAINPATH 1
#define RW_PARTITION 2
#define RW_ALL 3				// added 2016/06/16

#define TAIWANTD_MASTER	0x01
#define TAIWANTD_PHD	0x02
#define TAIWANTD_DEGREE_UNKNOWN	0x03
#define TAIWANTD_CHINESE	0x0100
#define TAIWANTD_ENGLISH	0x0200

// following university industry status is added 2016/12/28
#define UNIVERSITY_UNIVERSITY 1
#define UNIVERSITY_INDUSTRY 2
#define UNIVERSITY_UNIVERSITY_INDUSTRY 3

struct PN
{
	wchar_t name[MAX_NODE_NAME];	// document id defined by ISI
	wchar_t alias[MAX_ALIAS];	// alias 
	int ndx;			// to keep track of data's original order 
	int    ndx2wos;		// index of this document in the WOS data array
	int    comp;		// the id of component this node belongs to, counted from 1
	int    type;		
	int    mainp;		// indicate whether the node is in the main path
	int    special;		// indicate special nodes, they have either USC, IPC or assignee the same as given specification
	double bratio;		// boost ratio for special nodes
	int    branch;		// indicate branch nodes
	int    degree;		// overall degree, number of immediate neighbors 
	double strength;	// overall strength = total weight of their connnections 
	int    *nbrs;		// the id of neighbors, index to this PN list
	double *weight;		// weighting of this particular link
	int    cur_mem;		// current memory allocated
	int    in_deg;		// in-degree
	double in_strength;	// inward strength
	int    *in_nbrs;	// the id of in-neighbors, index to this PN list
	double *in_weight;	// inward weight
	double *in_relevancy;	// relevancy of the citation, added 2016/05/04
	int    in_cur_mem;	// current memory allocated
	int    out_deg;		// out-degree
	double out_strength;// outward strength
	int    *out_nbrs;	// the id of out-neighbors, index to this PN list
	double *out_weight;	// outward weight, weight = inward + outward
	double *out_relevancy;	// relevancy of the citation, added 2016/05/04
	int    out_cur_mem;	// current memory allocated
	double *in_spx;		// SPC, SPLC or SPNP
	double *out_spx;	// SPC, SPLC or SPNP
	int *depth;			// depth for each link, added 2013/04/26
	double total_in_spx;	// SPC, SPLC or SPNP, added 2012/06/26
	double total_out_spx;	// SPC, SPLC or SPNP, added 2012/06/26
	double *in_dissim;	// dissimilarity with the inward nodes
	double *out_dissim;	// dissimilarity with the outward nodes
	int    was_here;	// a flag (used in depth-first search)
	int    level;		// the distance to a designated node (used in subnetwork generation, and SPDC calculation--added 2013/04/25)
	int    indx;		// cross reference index between the original network and the subnetwork (used in subnetwork generation)
	int	   t_order;		// topological order
#ifdef USING_TOPOLOGICAL_SORT
#ifdef SPX_INTEGER
	_int64	paths_forward;	// this is used in SPx calculation as a temporary storage
	_int64	paths_backward;	// this is used in SPx calculation as a temporary storage
	_int64	nodes_forward;	// this is used in SPx calculation as a temporary storage
	_int64	nodes_backward;	// this is used in SPx calculation as a temporary storage
#endif SPX_INTEGER
	double	paths_forward;	// changed to double, 2018/05/03
	double	paths_backward;	// changed to double, 2018/05/03
	double	nodes_forward;	// changed to double, 2018/05/03
	double	nodes_backward;	// changed to double, 2018/05/03
	double  decay_backward; // added 2013/04/03, for the calculation of SPGD
#endif USING_TOPOLOGICAL_SORT
#ifdef OKAY_BUT_SLOW	
	_int64	forward;	// this is used in SPx calculation as a temporary storage
	_int64	backward;	// this is used in SPx calculation as a temporary storage
#endif OKAY_BUT_SLOW
	int flag;			// general flag
	int partition;		// partition ID, added 2014/04/07
	int id_serialdoc;	// indicate that the serialdoc it belongs
};

// structure for alias table
struct ATABLE
{
	wchar_t truename[MAX_NODE_NAME];
	wchar_t alias[MAX_NODE_NAME];
};


// data structure for components
//
#define MAX_COMP 50000	// the number of components for a large set of patent can be very large
struct COMP
{
	int nnodes;
};


struct SLINK
{
	int from;	// index of the source node
	int to;		// index of the target node
	double spx;	// SPx for the link
	double relevancy;	// document relevancy, added 2017/01/17
	int year_s;	// publish year of the source ndoe
	int year_t;	// publish year of the target ndoe
	double year_avg;
};

struct NODE
{
	int indx;
	int type;		// NSOURCE, NTARGET
	int mainp;		// TRUE, FALSE
	int special;	// TRUE, FALSE
	int branch;		// TRUE, FALSE
	wchar_t name[MAX_NODE_NAME];
	wchar_t id[MAX_NODE_NAME];	// original document id 
};


#define MAX_PATH_QUEUE 15000	// extended from 10000 (2011/04/17) for the requirement of Delta Project, which use a large combination of global 30 x key-routes 600
#define EXT_PATH_QUEUE 10000	// the space in the extended queue is reserved for the situation when there are many equal counts
#define MAX_SHORTESTPATH MAX_PATH_QUEUE
#define MAX_PATH_LENGTH 1000
struct SPATH
{
	int len;
	double tcnt;	// total count, sum of SPx of all edges in the path 
	double acnt;	// average count per edge
	int seq[MAX_PATH_LENGTH];
	double spx[MAX_PATH_LENGTH];
	int hit_a_sink;		// used in search forward
	int hit_a_source;	// used in search backward
	int kroute_id;		// indicate which key-route, used only in the function combine_global_keyroute_path_queues()
};

struct WOS 
{
	int ndx;	// to keep track of data's original order 
	int ndx2nw;	// index to the nw[] array
	int ndx2title; // index to the ttd_title[] array, for TaiwanTD data only
	wchar_t docid[MAX_DOC_ID];
	wchar_t alias[MAX_ALIAS];
	wchar_t doi[MAX_DOI_LEN];
	wchar_t country[3];	// 2 characters + null
	int journal;	// journal name, index to the journal table
	int department;	// department name, index to the department table, // added 2016/01/26
	int year;	// year published (paper), or issued (patent)
	int month;	// will have meaningful data for patent only
	int a_year;		// year application, for patent only
	int a_month;	// month application, for patent only
	int tc;		// total citation
	int volume;	// volume number
	int issue;	// issue number, used only in TCI data
	int bpage;	// beginning page
	int epage;	// endng page, only used in Scopus data
	int nau;
	int author[MAX_AUTHORS];	// index to the author table
	int nde;				// after 2012/06/09, also used as number assignees if the source is patent data
	int DE[MAX_KEYWORDS];	// author keyword, index to the DE keyword table, after 2012/06/09 also used save assignee index
	int nid;
	int ID[MAX_KEYWORDS];	// keywork plus, index to the ID keyword table
	int ndspln;
	int dspln[MAX_DISCIPLINE_NAME];	// discipline, index to the discipline (SC) keyword table
	int nipc;
	wchar_t ipc[MAX_IPC*MAX_IPC_CODE];	// for WEBPAT data, this also store CPC codes
	int ipc_cnt[MAX_IPC];	// count of each IPC (CPC) code, added 2016/05/26
	int ncountries;					// added 2013/04/10
	int countries[MAX_COUNTRIES];	// country, index to the country table (extracted frm C1 field), added 2013/04/10
	int ncountries_rp;				// added 2013/04/10
	int countries_rp[MAX_COUNTRIES_RP];// country, index to the country table (extracted frm RP field), added 2013/04/10
	int ntkws;					// added 2013/10/26
	int tkws[MAX_TKEYWORDS];		// added 2013/10/26
	int tkws_cnt[MAX_TKEYWORDS];	// added 2015/07/10, counts of duplicated keywords
	int info;	// for Taiwan T&D data, added 2016/01/15
	int partition;	// this save the results after clustering the document projection of a two-mode coword network
#ifdef DEA_APPLICATIONS
	int app_type;	// 0=theoretical, 1=non-theoretical
#endif DEA_APPLICATIONS
};


struct P_IPC
{
	int nipc;
	wchar_t ipc[MAX_IPC*MAX_IPC_CODE];
};

struct KWORDS 
{
	wchar_t name[MAX_KEYWORD_NAME];
	int cnt;
};

struct DEPARTMENTS	// added 2016/01/26
{
	wchar_t name[MAX_DEPARTMENT_NAME];
	int cnt;
};

struct AUTHORS 
{
	wchar_t name[MAX_AUTHOR_NAME];
	int ndx;	// sequence id according to author name order
	int ndx1;	// index to the 1st_authors[] array
	int ndx2;	// index to the areport[] array
	int xndx;	// cross index, partition to/from full array
	int sndx;	// index to the scholars[] array
	int h;		// H-index for this author
	int g;		// G-index for this author
	double hm;		// Hm-index, which considers multi-authors, for this author, added 2015/11/19
	int h_cn;		// H-index for this author, within the citation network, added 2016/11/19
	int g_cn;		// G-index for this author, within the citation network, added 2016/11/19
	double hm_cn;	// Hm-index, which considers multi-authors, for this author, added 2016/11/19
	int np;		// number of papers (number of times as the author, 1st, 2nd, 3rd, etc.)
	int *paper;	// index to the WOS table
	int cnt1;	// number of times as the 1st author
	int nsingle; // number papers as single authors	// added 2015/07/17
	int inter; // flag for international collaboration	// added 2015/07/17
	int byear;	// beginning year of this author's publication
	int eyear;	// ending year of this author's publication
	int groupid;	// group id of the author	// added 2015/10/07
	int total_descendants;
	double total_in_spx;		// SPC, SPLC or SPNP, added 2010/12/30
	double total_out_spx;		// SPC, SPLC or SPNP, added 2010/12/30
	double total_paths;			// this is intended to replace total_in_spx and total_out_spx
	double multiple_work_paths;	// the number of duplicate paths caused by having at least two works of an author in a path
	int total_cites;			// total citation (a sum of TC in WOS) for publications of the author
	int total_outdeg;			// total out-degree
	int cflag;	
	// following entries about countries and locations are added 2013/04/10
	int ncountries;
	int countries[MAX_COUNTRIES];	// country, index to the country table (extracted frm C1 field)
	int country_cnt[MAX_COUNTRIES];	// count of each country
	int ncountries_rp;			
	int countries_rp[MAX_COUNTRIES_RP];	// country of the 'reprint author', index to the country table (extracted frm RP field)
	int country_cnt_rp[MAX_COUNTRIES_RP];	// count of each country
	int location;					// index of the most significant country for this author
	int location_status;			// the number locations of equal significance
	// linkage variables for coauthor network, added 2015/10/09
	int    degree;		// overall degree, number of immediate neighbors 
	int    *nbrs;		// the id of neighbors, index to this AUTHORS list
	double *weight;		// weight of this particular link
	int    cur_mem;		// current memory allocated for "nbrs", "weight", etc.
	// linkage variables for cocitation network, added 2015/10/21
	int    ctn_indegree;		// in-degree, number of immediate neighbors 
	int    *ctn_innbrs;			// the id of inward neighbors, index to this AUTHORS list
	double *ctn_inweight;		// inward weight of this particular link
	int    ctn_incur_mem;		// current memory allocated for "innbrs", "inweight", etc.
	int    ctn_outdegree;		// outward degree, number of immediate neighbors 
	int    *ctn_outnbrs;		// the id of outward neighbors, index to this AUTHORS list
	double *ctn_outweight;	// outward weight of this particular link
	int    ctn_outcur_mem;		// current memory allocated for "outnbrs", "outweight", et
};

struct JOURNALS 
{
	wchar_t name[MAX_JOURNAL_NAME];
	wchar_t abbr[MAX_JOURNAL_ABBR];
	int h;		// H-index for this journal
	int g;		// G-index for this journal
	int h_cn;		// H-index for this journal, within the citation network, added 2016/11/17
	int g_cn;		// G-index for this journal, within the citation network, added 2016/11/17
	int np;		// number of articles, all years
	int *paper;	// index to the WOS table
	int cnt1;	// number of articles, since 2000
	int byear;	// beginning year of this journal's publication
	int eyear;	// ending year of this journal's publication
	int sum_tc;		// total number of citations as indicated in the WOS TC filed (or Scopus "Cited by" field)
};

struct TKEYWORDS	// title keywords
{
	wchar_t name[MAX_TKEYWORDS];
	int cnt;	// count in titles
	int acnt;	// count in titles and abstracts, duplicated keywords count as 1
	int acnt2;	// count in titles and abstracts, allow duplicated keywords
	int ranking;
	int ndx;	// index to the original keyword array
	int cross_ndx;	// this is used for title keywords and report keywords
	int ndx1;	// index to the document 1 keyword array, used in pool_keywords()
	int ndx2;	// index to the document 2 keyword array, used in pool_keywords()
};

struct AUTHOR_REPORT	// the content of author report file
{
	wchar_t name[MAX_AUTHOR_NAME];
	int ndx;	// index to the author array
};

struct ASSIGNEES 
{
	wchar_t name[MAX_ASSIGNEE_NAME];
	int ndx;	// sequence id according to author name order
	int ndx1;	// index to the 1st_assignees[] array
	int xndx;	// cross index, partition to/from full array
	int cnt1;	// number of times as the 1st assignee
	int np;		// number of patents, all years
	int *paper;	// index to the WOS table
	int byear;	// beginning year of this assignee's patents
	int eyear;	// ending year of this assignee's patents
	int groupid;	// group id of the author	// added 2015/10/07
	int flag;	// added 2013/11/01
	// following entries about countries and locations are added 2016/12/21
	int ncountries;
	int countries[MAX_COUNTRIES_ASSIGNEE];	// country, index to the country table (extracted frm C1 field)
	int country_cnt[MAX_COUNTRIES_ASSIGNEE];	// count of each country
	int location;					// index of the most significant country for this author
	int location_status;			// the number locations of equal significance
	int nareas;						// areas (under the country) specified in patents
	int areas[MAX_COUNTRIES_ASSIGNEE];		// area, index to the area tablel (US state table, for now)
	int area_cnt[MAX_COUNTRIES_ASSIGNEE];	// count of each area
	int location_area;				// index of the most significant area for this author
	int location_area_status;		// the number areas of equal significance
	// linkage variables for coassignee network, added 2016/12/24
	int    degree;		// overall degree, number of immediate neighbors 
	int    *nbrs;		// the id of neighbors, index to this AUTHORS list
	double *weight;		// weight of this particular link
	int    cur_mem;		// current memory allocated for "nbrs", "weight", etc.
	int total_descendants;		// added 2017/11/02
	double total_paths;			// added 2017/11/02
	double multiple_work_paths;	// added 2017/11/02, the number of duplicate paths caused by having at least two works of an author in a path
	double total_in_spx;		// SPC, SPLC or SPNP, added 2017/11/02
	double total_out_spx;		// SPC, SPLC or SPNP, added 2017/11/02
	int total_outdeg;			// total out-degree, added 2017/11/02
};

struct DISCIPLINES 
{
	wchar_t name[MAX_DISCIPLINE_NAME];
};

struct COUNTRIES 
{
	wchar_t name[MAX_COUNTRY_NAME];
};

struct FWDIST
{
	float dist;		// 4 bytes
};

struct FWDISTANCE
{
	struct FWDIST d;// 4 bytes
	short i;		// 2 bytes
	short j;		// 2 bytes
};

//
// following are definitions related to patent data
//
#define MAX_PATENT_ID 15
struct USPTO 
{
	int ndx2nw;	// index to the nw[] array
	wchar_t pid[MAX_PATENT_ID];
	int year;	// year, issued
	int month;	// month, issued
	int a_year;		// year, applied
	int a_month;	// month, applied
	int tc;		// total citation
	wchar_t country[30];	// assume 3 countries, changed from 3 to 9, 2017/03/07, extended to 30 for free style country names, 2018/01/03
	int ncountries;					// added 2016/12/21
	int countries[MAX_COUNTRIES];	// country, index to the country table, added 2016/12/21
	int country_cnt[MAX_COUNTRIES];	// count of each country
	int location;					// index of the most significant country for this patent
	int location_status;			// the number locations of equal significance
	int nareas;						// areas (under the country) specified in patents
	int areas[MAX_COUNTRIES];		// area, index to the area tablel (US state table, for now)
	int area_cnt[MAX_COUNTRIES];	// count of each area
	int location_area;				// index of the most significant area for this author
	int location_area_status;		// the number areas of equal significance
	int ninventor;
	int inventor[MAX_INVENTORS];	// inventor names, index to the inventor table
	int nassignee;
	int assignee[MAX_ASSIGNEES];	// inventor names, index to the inventor table
	int nipc;
	wchar_t ipc[MAX_IPC*MAX_IPC_CODE];
	int ipc_cnt[MAX_IPC];	// count of each IPC (CPC) code, added 2016/05/26
	int nusc;
	wchar_t usc[MAX_USC*MAX_USC_CODE];
	int nisic;
	wchar_t isic[MAX_IPC*MAX_ISIC_CODE];	// ISIC codes
	double isic_percent[MAX_IPC];				// the percentage of the corresponding ISIC
	int ntkws;					// added 2014/07/23
	int tkws[MAX_TKEYWORDS];		// added 2014/07/23
	int UI;		// added 2016/12/06, indicate uiversity industry collaboration status
};

// following are definitions related to law case data
//
#define MAX_LAWCASE_ID 150	// extended from 30 to 150 on 2013/02/05
struct LAWCASE 
{
	int ndx2nw;	// index to the nw[] array
	wchar_t cid[4*MAX_LAWCASE_ID];	// case ID, 4 maximum, modified 2015/06/03
	int year;	// year decided
	int ayear;	// year argued/heard
	wchar_t flag[4];	// Westlaw KeyCite flag
	int tc;		// total citation
	int nplaintiff;
	int plaintiff[MAX_INVENTORS];	// inventor names, index to the plantiff table
	//int ndefendant;
	//wchar_t defendant[MAX_IPC*MAX_IPC_CODE];
};

//
// for serial documents (歷審資料)
//
struct SERIALDOCS // added 2017/05/30
{
	wchar_t sdocid[MAX_LAWSNOTE_ID];	// for Lawsnote: 歷審資料 ID, or for WOS: an identification number
	int nd;		// number of docuemnts in the series
	int ndx[200];	// index in the nw[] array for each serial document for this set of serial document
	wchar_t docname[200][MAX_LAWSNOTE_NAME];	// assume at most 200 docs in this family, for WOS this store WOS UT codes
};

//
// Assignee name alias table
//
struct ANAME_ALIAS 
{
	wchar_t name[MAX_ASSIGNEE_NAME];
	wchar_t alias[MAX_ASSIGNEE_NAME];
};


//
// Author name alias table
//
struct AUTHOR_ALIAS
{
	wchar_t name[MAX_AUTHOR_NAME];	// author true name
	wchar_t alias[MAX_AUTHOR_NAME];	// author's other name
};

//
// Keyword alias table
//
struct KEYWORD_ALIAS
{
	wchar_t name[MAX_KEYWORD_NAME];	// generic keyword name
	wchar_t alias[MAX_KEYWORD_NAME];	// keyword's other name
};

//
// Department name alias table
//
struct DEPARTMENT_ALIAS
{
	wchar_t name[MAX_AUTHOR_NAME];	// generic department name
	wchar_t alias[MAX_AUTHOR_NAME];	// variations
};

//
// Journal name alias table
//
struct JOURNAL_ALIAS
{
	wchar_t name[MAX_JOURNAL_NAME];	// author true name
	wchar_t alias[MAX_JOURNAL_NAME];	// author's other name
};

//
// Options for processing Westlaw KEYCITE data
//
struct CITE_OPTIONS 
{
	int include_4star;	// **** in negative cases
	int include_3star;	// *** in negative cases
	int include_2star;	// ** in negative cases
	int include_1star;	// * in negative cases
	int include_examined;	// the following four options are for positive cases
	int include_discussed;
	int include_cited;
	int include_mentioned;
};

struct CLAN_PARAMS
{
	wchar_t ccode[MAX_ASSIGNEE_NAME];	// clan code
	double bratio;	// boost ratio
	int ctype;		// clan type: Assignee, UPC, IPC
};

struct LOCATION 
{
	wchar_t name[25];
	int c1;
	int c2;
	int island;
};


struct IPC_ISIC	// added 2013/07/20
{
	wchar_t ipc[MAX_IPC_CODE];
	wchar_t isic[MAX_ISIC_CODE];
	double percent;
};


struct CMEMBER	// for clustering results, added 2014/05/14
{ 
	int id; 
	int mcnt; 
};	

#define SIZE_LSTRING 19
struct PFID		// for WebPat data
{
	wchar_t lstr[SIZE_LSTRING];
	int code;
};

struct TDID		// for TaiwanT&D data
{
	wchar_t lstr[SIZE_LSTRING];
	size_t len;
	int code;
};

struct PTITLES		// paper titles 
{
	wchar_t name[MAX_TITLES];
	int year;
};

struct TTD_TITLE
{
	wchar_t name[MAX_TITLES];
	int ndx;	// index to the wos[] array
	int year;
};

#ifdef XXX
struct INVENTORS 
{
	wchar_t name[MAX_AUTHOR_NAME];
	int np;		// number of patens (number of times as the inventors, 1st, 2nd, 3rd, etc.)
	int *patent;// index to the USPTO table
	int cnt1;	// number of times as the 1st inventor
	int byear;	// beginning year of this author's patents
	int eyear;	// ending year of this author's patent
};
#endif XXX

struct ARRAY_STATS
{
	double min;
	double max;
	double mean;
	double stdev;
};


#define APSIZE 14
struct APAIR 
{
	wchar_t names[APSIZE];	// author pair, or assignee pair
};


struct USSTATES 
{ 
	int len; 
	wchar_t *name; 
	wchar_t *abbr; 
};