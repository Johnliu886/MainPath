//
// document relevancy.h
//


#define LENGTH_IPC_VECTOR 300
struct IPC_VECTOR
{
	wchar_t ipc[MAX_IPC_CODE];
	int cnt;	// total number of this IPC code
	int ndx1;	// index to the patent 1 IPC array
	int ndx2;	// index to the patent 2 IPC array
};

#define N_ATTRIBUTES 50000	// assume that an article can be cited as much as 50,000 times

struct PSTRING
{
	int cnt;		// count
	double prob;	// probability
};

struct NEIGHBOR
{
	int id;
	int cnt;
	int ndx1;	// index to the document 1 nbrs array
	int ndx2;	// index to the document 2 nbrs array
};
