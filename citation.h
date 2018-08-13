//
// citation.h
//

#define P_JOURNAL		1
#define P_PROCEEDING_BOOK	2
#define P_REPORT		3
#define P_PATENT		4
#define P_XAUTHOR		5
#define P_OTHER			6

#define P_CHINESE	0x80	// reference in Chinese
#define P_ENGLISH	0x81	// reference in English

struct CITING_DOC
{
	int type;
	int nf;							// number of inoformation
	wchar_t aname[MAX_AUTHOR_NAME];	// author name
	int year;						// publish year
	wchar_t jname[MAX_JOURNAL_NAME];	// journal name
	int volume;						// volume number
	int page;					// page number	
	int page2;					// ending page
	wchar_t doi[MAX_DOI_LEN];	// DOI string 
};
