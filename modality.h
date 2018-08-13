//
// data structure related to Research Modality Project
//

#define MAX_AFIELD_NAME 30

#define SEX_MALE 1
#define SEX_FEMALE 2
#define MAX_SANENAME 100

struct SCHOLAR
{
	int ndx;	// original order as read
	int andx;	// index to the authors[] array
	int pndx;	// index to the authors_p[] array
	wchar_t cname[MAX_AUTHOR_NAME];	// 中文姓名
	wchar_t ename[MAX_AUTHOR_NAME];	// 英文姓名, last name + abbreviated 1st name
	wchar_t e1stname[MAX_AUTHOR_NAME];// 英文姓名, complet 1st name
	wchar_t orgname[MAX_DEPARTMENT_NAME];	// 	服務機關
	wchar_t position[30];	// 職稱
	int pos;	// 職稱, 數字化, 0-博士後/講師, 1-助理教授, 2-副教授, 3-教授, 4-特聘教授, 5-講座教授
	wchar_t afield[MAX_AFIELD_NAME];	// 學門領域
	wchar_t afield2[MAX_AFIELD_NAME];	// 次學門領域
	int phdyear;	// year obtained phd degree, 取得最高學位年度
	int sex;	// 1 for male, 2 for female
	double avg_author_cnt;	// average author count of this scholar
	double pfreq;	// publishing frequency
	int byear;
	int eyear;
	int tc;	// total citations in WOS
	double average_journal_rank;	// average - the rank of the journal of all published papers (WOS journal only)
	int outstanding_cnt;	// number of times published in outstanding journal
	int TSSCI_cnt;					// number of times published in TSSCI journal
};

struct SAMENAME_CHINESE
{
	wchar_t cname[MAX_AUTHOR_NAME];	// 中文姓名
	int nsame;	// number of scholars with this Chinese name
	int peers[MAX_SANENAME];	// index (to the scholars[]) of all the scholars with this Chinese name
};


struct SAMENAME_ENGLISH
{
	wchar_t ename[MAX_AUTHOR_NAME];	// 英文姓名, last name + abbreviated 1st name
	int nsame;	// number of scholars with this English name
	int peers[MAX_SANENAME];	// index (to the scholars[]) of all the scholars with theis English name
};


