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
	wchar_t cname[MAX_AUTHOR_NAME];	// ����m�W
	wchar_t ename[MAX_AUTHOR_NAME];	// �^��m�W, last name + abbreviated 1st name
	wchar_t e1stname[MAX_AUTHOR_NAME];// �^��m�W, complet 1st name
	wchar_t orgname[MAX_DEPARTMENT_NAME];	// 	�A�Ⱦ���
	wchar_t position[30];	// ¾��
	int pos;	// ¾��, �Ʀr��, 0-�դh��/���v, 1-�U�z�б�, 2-�Ʊб�, 3-�б�, 4-�S�u�б�, 5-���y�б�
	wchar_t afield[MAX_AFIELD_NAME];	// �Ǫ����
	wchar_t afield2[MAX_AFIELD_NAME];	// ���Ǫ����
	int phdyear;	// year obtained phd degree, ���o�̰��Ǧ�~��
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
	wchar_t cname[MAX_AUTHOR_NAME];	// ����m�W
	int nsame;	// number of scholars with this Chinese name
	int peers[MAX_SANENAME];	// index (to the scholars[]) of all the scholars with this Chinese name
};


struct SAMENAME_ENGLISH
{
	wchar_t ename[MAX_AUTHOR_NAME];	// �^��m�W, last name + abbreviated 1st name
	int nsame;	// number of scholars with this English name
	int peers[MAX_SANENAME];	// index (to the scholars[]) of all the scholars with theis English name
};


