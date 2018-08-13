//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by app.rc

#include "config.h"

//#define MDEBUG

#define FNAME_SIZE 260
#define BLOCK_SIZE 256

// type of full record data
#define UNKNOWN_DATA 0x100
#define WOS_DATA 0x101
#define USPTO_DATA 0x102
#define THOMSON_INNOVATION_DATA 0x103
#define PGUIDER_DATA 0x104
#define WESTLAW_DATA 0x105
#define WUHAN_DATA 0x106
#define SCOPUS_DATA 0x107
#define TAIWAN_TD_DATA 0x108
#define TCI_DATA 0x109
#define WEBPAT2_DATA 0x10a
#define WEBPAT3_DATA 0x10b
#define WESTLAW_DATA2 0x10c
#define LAWSNOTE_DATA 0x10d
#define WOS_MULTIPLE_DATA 0x10e
#define TCI_MULTIPLE_DATA 0x10f
#define WEBPAT3_MULTIPLE_DATA 0x110

// network mode difinitions
#define MUNKNOWN 0
#define MFORWARD 1
#define MBACKWARD 2
#define MDIRECTED 1
#define MNONDIRECTED 2
#define M1_MODE 1
#define M2_MODE 2

// delimiter types
#define SP_TAB 0
#define SP_SPACE 1
#define SP_TAB_SPACE 2
#define SP_SEMIC 3
#define SP_COLON 4
#define SP_FSLASH 5
#define SP_BSLASH 6
#define SP_DOUBLE_QUOTE 7

#ifdef OBSOLETE
// GroupFinder options, added 2014/04/11
#define GF_DISABLE 0
#define GF_UNDIRECTED_UNWEIGHTED 1
#define GF_DIRECTED_UNWEIGHTED 2
#endif OBSOLETE

#define GF_UNDIRECTED_WEIGHTED 999
#define GF_DIRECTED_WEIGHTED 999

// GroupFinder options, added 2014/04/11
#define CCoauthor_DISABLE 0
//#define CCoauthor_UNWEIGHTED 1

// Clustering options, added 2016/07/05
#define GF_DISABLE 0
#define GF_UNDIRECTED_UNWEIGHTED 1
#define CCoauthor_UNWEIGHTED 2
#define CCoword_UNWEIGHTED 3
#define CCoassignee_UNWEIGHTED 4
#define GF_DIRECTED_UNWEIGHTED 5

// search path count method
#define S_SPC 0
#define S_SPLC 1
#define S_SPNP 2
#define S_SPAD 3
#define S_SPGD 4	// added 2013/05/22
#define S_SPHD 5	// added 2014/01/07

// if souce column only
#define SOURCE_COLUMN_NODES_ONLY 0
#define NODES_IN_WOS_DATA 1
#define ALL_NODES 2

// path type
#define P_LOCAL_F 0
#define P_LOCAL_B 1
#define P_LOCAL_KEY_ROUTE 2
#define P_GLOBAL 3
#define P_GLOBAL_KEY_ROUTE 4
#define P_LOCAL_FB 5

// clan type for patents
#define CTYPE_IPC 1
#define CTYPE_UPC 2
#define CTYPE_ASSIGNEE 3
#define CTYPE_COUNTRY 4

// clan type for papers
#define PTYPE_WOS 1
#define PTYPE_AUTHOR 2

// strategy for finding relevancy (patent), added 2016/05/07
#define RELEVANCY_FLAT 0
#define RELEVANCY_CPC_JACCARD 1
#define RELEVANCY_CPC3_JACCARD 2
#define RELEVANCY_CPC4_JACCARD 3
#define RELEVANCY_CITATION_JACCARD 4
#define RELEVANCY_CPC3_CITATION_JACCARD 5
#define RELEVANCY_CPC4_CITATION_JACCARD 6

// strategy for finding relevancy (documents: WOS/Scopus), added 2017/01/17
#define RELEVANCY_D_KEYWORD_JACCARD 1
#define RELEVANCY_D_CITATION_JACCARD 2
#define RELEVANCY_D_KEYWORD_CITATION_JACCARD 3

// method of handling citation level (for Westlaw data only)
#define FLAT_IGNORE_SIGN 1
#define LINEAR_IGNORE_SIGN 2
#define EXPONENTIAL_IGNORE_SIGN 3
#define FLAT_TAKE_SIGN 4
#define LINEAR_TAKE_SIGN 5
#define EXPONENTIAL_TAKE_SIGN 6

// message types
#define MSG_FILE_CREATED 1
#define MSG_IFILE_NOTFOUND 2
#define MSG_OFILE_CANNOTOPEN 3
#define MSG_OFILE_CANNOTWRITE 4
#define MSG_ACYCLIC 5
#define MSG_WOSFILE_NOTFOUND 6
#define MSG_WOSFILE_FORMAT_ERROR 7
#define MSG_MEMORY 8
#define MSG_CFILE_CANNOTOPEN 9
#define MSG_FILE_CITATION_FILE_NOT_PROVIDED 10
#define MSG_FILE_NAME_TOO_LONG 11
#define MSG_CLANFILE_CANNOTOPEN 12
#define MSG_CLANFILE_FORMAT_ERROR 13
#define MSG_NOT_ENOUGH_MEMORY 14
#define MSG_FILE_FORMAT_ERROR 15
#define MSG_SOFTWARE_EXPIRED 16
#define MSG_EXCEEDING_LIMIT 17
#define MSG_FILE_CANNOTOPEN 18
#define MSG_WESTLAW_FILE_NOTFOUND 19
#define MSG_SPX_OVERFLOW 20
#define MSG_PARALLEL_DATA_ERROR 21

#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <string.h>
#include "checklicense.h"

int read_WOS(wchar_t *);
int main_function(wchar_t*, wchar_t*, wchar_t*, wchar_t*, int, int, int, int, int, int, int, int, int, int, double, double, int, double, double, int, int, int, int, double, wchar_t*, int, int, int, int, int, int, int, int, int, wchar_t*, int);
int error(int i);
int ReadSerial(wchar_t *, wchar_t *, char *);
int parse_clan_spec(wchar_t *);
int parse_clan_line(wchar_t *, wchar_t *, wchar_t *, wchar_t *);
int WOS_or_Others(wchar_t *);
int Jfree(void *, wchar_t *);
void *Jmalloc(size_t, wchar_t *);
int Jheap_check();