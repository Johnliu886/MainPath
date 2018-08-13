//
// critical_transition.cpp
//

//
// Revision History:
// 2014/12/26 Basic function works
// 2014/05/28 Tuned to a state proper for MOST project midterm report
// 2016/06/03 Added codes to set distance to 1 when n=0 in the Jaccard distance calculation
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
#include "document relevancy.h"

//#define N_TKEYWORD_ATTR 30
//#define N_INDEGREE_ATTR 10
//#define N_OUTDEGREE_ATTR 10
//#define N_ATTRIBUTES (N_TKEYWORD_ATTR+N_INDEGREE_ATTR+N_OUTDEGREE_ATTR)	// the number of attributes (title keywords, abstract keywords, cited doc., etc.) to include in the distribution

#define N_ATTRIBUTES 50000	// assume that together 3 articlea can be cited as much as 50,000 times

int test_eI();

extern FILE *logstream;
extern struct TKEYWORDS *tword;	// added 2015/03/04
extern int nkreport;	// added 2015/03/04
extern double spx_cut;			// spx_cut is defined in "network_stats.cpp", added 2015/05/28

extern int array_statistics(int, double *, struct ARRAY_STATS *);
extern int compare_tkeyword(const void *, const void *);
extern int compare_wordcount(const void *, const void *);
int compare_tmpid(const void *, const void *);

//
// check if a keyword (in index) is listed in the "Keyword report.txt" file
// this function is added 2015/03/04
//
bool is_in_keyword_report(wchar_t *kname)
{
	int i;

	if (tword == NULL)	// "Keyword report.txt" file is not given
		return 0;	// false
	for (i = 0; i < nkreport; i++)	// stupid search for now!
	{
		if (wcscmp(kname, tword[i].name) == 0)	// hit
			return 1;	// true
	}

	return 0;	// false
}

//
// pool incoming neighbors of three papers together and remove duplicates
//
int pool_out_nbrs(int p1, int p2, int p3, int nnodes, struct PN *nw, int *n_outnbrs, struct NEIGHBOR *out_nbrs)
{
	int i, k;
	int ntmp;
	int cnt;
	struct NEIGHBOR *tmp;
	int prev;

	tmp = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"pool_out_nbrs: tmp");
	if (tmp == NULL) return MSG_NOT_ENOUGH_MEMORY;

	ntmp = 0;
	for (k = 0; k < nw[p1].out_deg; k++)	// paper 1
		tmp[ntmp++].id = nw[p1].out_nbrs[k];
	for (k = 0; k < nw[p2].out_deg; k++)	// paper 2
		tmp[ntmp++].id = nw[p2].out_nbrs[k];
	for (k = 0; k < nw[p3].out_deg; k++)	// paper 3
		tmp[ntmp++].id = nw[p3].out_nbrs[k];

	qsort((void *)tmp, (size_t)ntmp, sizeof(struct NEIGHBOR), compare_tmpid);
	// consolidate duplicate ids
	prev = -1;
	k = 0; cnt = 1;
	for (i = 0; i < ntmp; i++)
	{
		if (tmp[i].id != prev)	// hit a new id
		{
			if (k > 0) tmp[k-1].cnt = cnt;
			tmp[k++].id = tmp[i].id;
			prev = tmp[i].id;
			cnt = 1;
		}
		else
			cnt++;
	}
	tmp[k-1].cnt = cnt;

	for (i = 0; i < k; i++)	// move to out_nbrs[] 
		out_nbrs[i] = tmp[i];

	*n_outnbrs = k;
	free(tmp);

#ifdef DEBUG
	fwprintf(logstream, L"***n_outnbrs=%d for %s %s %s\n", *n_outnbrs, nw[p1].alias, nw[p2].alias, nw[p3].alias);
	for (i = 0; i < *n_outnbrs; i++)
		fwprintf(logstream, L"[%03d %02d %02d] ", i, out_nbrs[i].id, out_nbrs[i].cnt);
	fflush(logstream);
#endif DEBUG

	return 0;
}

//
// pool incoming neighbors of three papers together and remove duplicates
//
int pool_in_nbrs(int p1, int p2, int p3, int nnodes, struct PN *nw, int *n_innbrs, struct NEIGHBOR *in_nbrs)
{
	int i, k;
	int ntmp;
	int cnt;
	struct NEIGHBOR *tmp;
	int prev;

	tmp = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"pool_in_nbrs: tmp");
	if (tmp == NULL) return MSG_NOT_ENOUGH_MEMORY;

	ntmp = 0;
	for (k = 0; k < nw[p1].in_deg; k++)	// paper 1
		tmp[ntmp++].id = nw[p1].in_nbrs[k];
	for (k = 0; k < nw[p2].in_deg; k++)	// paper 2
		tmp[ntmp++].id = nw[p2].in_nbrs[k];
	for (k = 0; k < nw[p3].in_deg; k++)	// paper 3
		tmp[ntmp++].id = nw[p3].in_nbrs[k];

	//fwprintf(logstream, L"!!!!! %d [%s %s %s]\n", ntmp, nw[p1].alias, nw[p2].alias, nw[p3].alias); fflush(logstream);

	qsort((void *)tmp, (size_t)ntmp, sizeof(struct NEIGHBOR), compare_tmpid);
	// consolidate duplicate ids
	prev = -1;
	k = 0; cnt = 1;
	for (i = 0; i < ntmp; i++)
	{
		if (tmp[i].id != prev)	// hit a new id
		{
			if (k > 0) tmp[k-1].cnt = cnt;
			tmp[k++].id = tmp[i].id;
			prev = tmp[i].id;
			cnt = 1;
		}
		else
			cnt++;
	}
	tmp[k-1].cnt = cnt;

	for (i = 0; i < k; i++)	// move to in_nbrs[] 
		in_nbrs[i] = tmp[i];

	*n_innbrs = k;
	free(tmp);

#ifdef DEBUG
	fwprintf(logstream, L"***n_outnbrs=%d for %s %s %s\n", *n_innbrs, nw[p1].alias, nw[p2].alias, nw[p3].alias);
	for (i = 0; i < *n_innbrs; i++)
		fwprintf(logstream, L"[%03d %02d %02d] ", i, in_nbrs[i].id, in_nbrs[i].cnt);
	fflush(logstream);
#endif DEBUG

	return 0;
}

//
// pool keywords of three papers together and remove duplicates
// NOTE: this function also filters out keywords that are not listed in the "Keyword report.txt" file, 2015/03/04
//
int pool_keywords_3(int p1, int p2, int p3, struct WOS *wos, struct TKEYWORDS *tkeyword, int *ntk, struct TKEYWORDS *tk)
{
	int i, k;
	int nttk;
	int cnt;
	int cnt2;
	struct TKEYWORDS *ttk;
	wchar_t prev_name[MAX_TKEYWORDS];

	ttk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"pool_keywords_3: ttk");
	if (ttk == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// NOTE: wos[].tkws[] include keywords in both the title and abstract
	nttk = 0;
	for (k = 0; k < wos[p1].ntkws; k++)	// paper 1
	{
		if (is_in_keyword_report(tkeyword[wos[p1].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx = wos[p1].tkws[k];	// set the link from tkeyword[] to ttk[]
			wcscpy(ttk[nttk++].name, tkeyword[wos[p1].tkws[k]].name);
			//fwprintf(logstream, L"P1 %s ", tkeyword[wos[p1].tkws[k]].name);
		}
	}
	for (k = 0; k < wos[p2].ntkws; k++)	// paper 2
	{
		if (is_in_keyword_report(tkeyword[wos[p2].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx = wos[p2].tkws[k];	// set the link from tkeyword[] to ttk[]
			wcscpy(ttk[nttk++].name, tkeyword[wos[p2].tkws[k]].name);
			//fwprintf(logstream, L"P2 %s ", tkeyword[wos[p2].tkws[k]].name);
		}
	}
	for (k = 0; k < wos[p3].ntkws; k++)	// paper 3
	{
		if (is_in_keyword_report(tkeyword[wos[p3].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx = wos[p3].tkws[k];	// set the link from tkeyword[] to ttk[]
			wcscpy(ttk[nttk++].name, tkeyword[wos[p3].tkws[k]].name);
			//fwprintf(logstream, L"P3 %s ", tkeyword[wos[p3].tkws[k]].name);
		}
	}

	qsort((void *)ttk, (size_t)nttk, sizeof(struct TKEYWORDS), compare_tkeyword);
	// consolidate duplicate title keywords names
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nttk; i++)
	{
		if (wcscmp(ttk[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) ttk[k-1].cnt = cnt;
			ttk[k].ndx = ttk[i].ndx;
			wcscpy_s(ttk[k++].name, MAX_TKEYWORDS, ttk[i].name); 
			wcscpy_s(prev_name, MAX_TKEYWORDS, ttk[i].name); 
			cnt = 1;
		}
		else
			cnt++;
	}
	ttk[k-1].cnt = cnt;

	// sort to the order of counts
	qsort((void *)ttk, (size_t)k, sizeof(struct TKEYWORDS), compare_wordcount);
	for (i = 0; i < k; i++) 
	{
		ttk[i].ranking = i;	// rank begins from 0
		tkeyword[ttk[i].ndx].ndx = i;	// set the link of ttk[] back to tkeyword[]
	}

	cnt2 = 0;
	for (i = 0; i < k; i++)	// move to tk[] 
	{
		tk[i] = ttk[i];
		if (ttk[i].cnt >= 1)	// assign *ntk the number of keywords that have count larger than 0
			cnt2++;
	}
	*ntk = cnt2;

	Jfree(ttk, L"pool_keywords_3: ttk");

#ifdef DEBUG
	fwprintf(logstream, L"*****ntk=%d\n", *ntk);
	for (i = 0; i < *ntk; i++)
		fwprintf(logstream, L"%02d %02d [%s]\n", tk[i].ranking, tk[i].cnt, tk[i].name);
	fflush(logstream);
#endif DEBUG

	return 0;
}

// Formula for additive smoothing is from page 363 of the following paper:
// Chen, S. F., & Goodman, J. (1999). 
// An empirical study of smoothing techniques for language modeling. 
// Computer Speech & Language, 13(4), 359-393.
//
// v is the vocabulary, or set of all words considered (assuming we are working on a language problem)
// delta is the factor to add to the count of all words, 0 < delta <= 1, usually is set to 1
//
int additive_smoothing(int v, double delta, struct PSTRING *p)
{
	int i;
	int tcnt;

	tcnt = 0;
	for (i = 0; i < v; i++)
		tcnt += p[i].cnt;
	for (i = 0; i < v; i++)
		p[i].prob = (delta + p[i].cnt) / ((delta * v) + tcnt);
	
	return 0;
}

//
// "smoothing" means give a zeros in the probability an estimated non-zero value
//
int smoothing(int v, double delta, struct PSTRING *p)
{
	additive_smoothing(v, delta, p);

	return 0;
}

//
// calculate Hamming distance
// reference: Wikipedia article ==> Hamming distance
//
double hamming_distance(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double dist;
 
	dist = 0;
	for (i = 0; i < n; i++)
	{
		if (q[i].cnt != p[i].cnt)
			dist += 1;
	}

    return dist; // Return the number of different positions in sequences
}

//
// calculate distance based on Jaccard similarity (NOT taking weight into consideration)
// distance = 1 - Jaccard similarity
// the distance ranges from 0 to 1
// 0: the two vectors overlap completely
// 1: the two vectors do not overlap at all
//
double Jaccard_distance(int n, PSTRING *q, PSTRING *p)
{
	int i;
	int Junion, Jintersection;
	double Jsim, dist;

	if (n == 0) return 1.0;	// this happens when CPC of both patents are not provided

	Junion = Jintersection = 0;
	for (i = 0; i < n; i++)
	{
		if (p[i].prob > 0.0 && q[i].prob > 0.0)	
		{
			Jintersection += 1;
			Junion += 1;
		}
		else if ((p[i].prob > 0.0 && q[i].prob == 0.0) || (p[i].prob == 0.0 && q[i].prob > 0.0))
		{
			Jintersection += 0;
			Junion += 1;
		}
		else if (p[i].prob == 0.0 && q[i].prob == 0.0)	// this one is not likely to happen
		{
			Jintersection += 0;
			Junion += 0;
		}
	}
	Jsim = (double)Jintersection / (double)Junion;
	//fwprintf(logstream, L"!!!!! %.4f %.4f %.6f\n", Jintersection, Junion, Jsim);

	dist = 1.0 - Jsim;	

    return dist; 
}

//
// calculate distance based on Jaccard similarity (generalized, take weight into consideration)
// generalized Jaccard similarity = SUMi[min(xi, yi)] / SUMi[max(xi,yi)]
// that is, distance = 1 - (generalzed Jaccard similarity)
// the distance ranges from 0 to 1
// 0: the two vectors overlap completely
// 1: the two vectors do not overlap at all
//
double Jaccard_distance_generalized(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double Junion, Jintersection;
	double Jsim, dist;

	if (n == 0) return 1.0;	// this happens when CPC of both patents are not provided

	Junion = Jintersection = 0;
	for (i = 0; i < n; i++)
	{
		//fwprintf(logstream, L"$$$$$ %d %f %f\n", i, p[i].prob, q[i].prob);
		if (p[i].prob <= q[i].prob)	
		{
			Jintersection += p[i].prob;	// take the smaller one, 0 for (0, 1), 1 for (1,1), 3 for (3, 5)
			Junion += q[i].prob;
		}
		else
		{
			Jintersection += q[i].prob;
			Junion += p[i].prob;
		}
	}
	Jsim = (double)Jintersection / (double)Junion;
	//fwprintf(logstream, L"!!!!! %.4f %.4f %.6f\n", Jintersection, Junion, Jsim);

	dist = 1.0 - Jsim;	

    return dist; 
}


//
// calculate overlapping distance (based on overlapping count)
// maximum distance = 1 
//
double overlapping_distance(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double fsim, dist;

	fsim = 0;
	for (i = 0; i < n; i++)
	{
		if (p[i].cnt != 0 && q[i].cnt != 0) 
		{
			if (p[i].cnt <= q[i].cnt)	// take the smaller one, 0 for (0, 1), 1 for (1,1), 3 for (3, 5)
				fsim += p[i].cnt;
			else
				fsim += q[i].cnt;
		}
	}
	// distance = the reverse of similarity, plus one so that the denominator will not be zero when fsim=0
	// max fsim=n++ ==> minimum distance is less than 1/n  
	// min fsim = 0 ==> maximum distance = 1
	dist = 1.0 / (fsim + 1);	

    return dist; 
}

#ifdef OD_NOT_NORMALIZED
//
// calculate overlapping distance (based on overlapping count)
// maximum distance = n (i.e., distance is longer when n is bigger, may not be fair)
//
double overlapping_distance(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double fsim, dist;

	fsim = 0;
	for (i = 0; i < n; i++)
	{
		if (p[i].cnt != 0 && q[i].cnt != 0) 
		{
			if (p[i].cnt <= q[i].cnt)	// take the smaller one, 0 for (0, 1), 1 for (1,1), 3 for (3, 5)
				fsim += p[i].cnt;
			else
				fsim += q[i].cnt;
		}
	}
	// distance = the reverse of similarity, plus one so that the denominator will not be zero when fsim=0
	// max fsim=n++ ==> minimum distance a little less than 1  
	// min fsim = 0 ==> maximum distance = n
	dist = n / (fsim + 1);	

    return dist; 
}
#endif OD_NOT_NORMALIZED

//
// calculate Cosine distance
//
double cosine_distance(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double siproduct, pssqr, qssqr;
	double dist;

	siproduct = pssqr = qssqr = 0.0;
	for (i = 0; i < n; i++)
	{
		pssqr += (double)(p[i].cnt * p[i].cnt);
		qssqr += (double)(q[i].cnt * q[i].cnt);
		siproduct += (double)(p[i].cnt * q[i].cnt);
	}
	if (pssqr == 0.0 || qssqr == 0.0)	// one of the vector is completely zero
		dist = 1.0;
	else
		dist = 1.0 - siproduct / (sqrt(pssqr) * sqrt(qssqr));

#ifdef DEBUG
	for (i = 0; i < n; i++)
		fwprintf(logstream, L"%d ", p[i].cnt);
	fwprintf(logstream, L"\n");
	for (i = 0; i < n; i++)
		fwprintf(logstream, L"%d ", q[i].cnt);
	fwprintf(logstream, L"\n\n");
#endif DEBUG

    return dist; 
}

//
// calculate Euclidean distance
//
double euclidean_distance(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double dist;

	dist = 0;
	for (i = 0; i < n; i++)
	{
		dist += (q[i].cnt - p[i].cnt) * (q[i].cnt - p[i].cnt);
	}
	dist = sqrt(dist);

    return dist; 
}

//
// calculate Kullback-Leibler divergence I(q:p) in bits, given two distribution p and q. p represents the "a priori" distribution and q the posteriori distribution.
// I(q:p) = sum[q[i]*log2(q[i]/p[i])]
// "I is th expected information value of the message that the a priori distribution is transformed into the a posteriori one." see page 1952, 
//  Lucio-Arias, D., & Leydesdorff, L. (2008). Main-path analysis and path-dependent transitions in HistCite-based historiograms. 
//  Journal of the American Society for Information Science and Technology, 59(12), 1948-1962.
//
double Kullback_Leibler(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double KL;

	KL = 0.0;
	for (i = 0; i < n; i++)
	{
		if (q[i].prob == 0.0 && p[i].prob == 0.0)
			KL += 0.0;
		else
			KL += q[i].prob * log(q[i].prob/p[i].prob) / log(2.0);	// NOTE: log(x) at base a = ln(x) / ln(a)
	}

	return KL;
}

//
// Jensen-Shannon divergence, see
// Lin, J. (1991). Divergence measures based on the Shannon entropy. Information Theory, IEEE Transactions on, 37(1), 145-151.
// and
// Sims, G. E., Jun, S. R., Wu, G. A., & Kim, S. H. (2009). 
// Alignment-free genome comparison with feature frequency profiles (FFP) and optimal resolutions. 
// Proceedings of the National Academy of Sciences, 106(8), 2677-2682.
//
double Jensen_Shannon(int n, PSTRING *q, PSTRING *p)
{
	int i;
	double JS;
	PSTRING *m;

	m = (PSTRING *)Jmalloc(n * sizeof(PSTRING), L"Jensen_Shannon: m");
	for (i = 0; i < n; i++) 
		m[i].prob = (p[i].prob + q[i].prob) / 2;
	JS = Kullback_Leibler(n, p, m) / 2 +  Kullback_Leibler(n, q, m) / 2;
	Jfree(m, L"Jensen_Shannon: m");

	return JS;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_indegree(const void *n1, const void *n2)
{
	struct PN *t1, *t2;
	
	t1 = (struct PN *)n1;
	t2 = (struct PN *)n2;
	if (t2->in_deg > t1->in_deg)
		return 1;
	else if (t2->in_deg == t1->in_deg)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_outdegree(const void *n1, const void *n2)
{
	struct PN *t1, *t2;
	
	t1 = (struct PN *)n1;
	t2 = (struct PN *)n2;
	if (t2->out_deg > t1->out_deg)
		return 1;
	else if (t2->out_deg == t1->out_deg)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_tmpid(const void *n1, const void *n2)
{
	struct NEIGHBOR *t1, *t2;
	
	t1 = (struct NEIGHBOR *)n1;
	t2 = (struct NEIGHBOR *)n2;
	if (t2->id > t1->id)
		return 1;
	else if (t2->id == t1->id)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_nwname(const void *n1, const void *n2)
{
	struct PN *t1, *t2;
	
	t1 = (struct PN *)n1;
	t2 = (struct PN *)n2;
	if (wcscmp(t2->name, t1->name) < 0)
		return 1;
	else if (wcscmp(t2->name, t1->name) == 0)
		return 0;
	else return -1;
}

#define SMOOTHING_DELTA (double)1.0	// use 0.1 rather than 1.0 is because the counts for each attributes are small
//#define COSINE_DISTANCE
//#define JACCARD_DISTANCE	// Jaccard distance
#define OVERLAPPING_DISTANCE	// the emphasis is on "similarity"
//#define EUCLIDEAN_DISTANCE	// geometric distance
//#define HAMMING_DISTANCE	// Hamming distance is symmetrical
//#define KULLBACK_LEIBLER	// Kullback_Leibler divergence is not symmetrical, i.e., Dab is not equal to KLba
//#define JENSEN_SHANNON	// Jensen Shannon divergence is symmetrical, which should not be appropriate for tracing changes in directed networks
//
// calculate distance between two given strings (from a to b)
//
double distance(int nattr, PSTRING *b, PSTRING *a)
{
	double dist;

#ifdef JACCARD_DISTANCE
	dist = Jaccard_distance_generalized(nattr, b, a);
#endif JACCARD_DISTANCE

#ifdef COSINE_DISTANCE
	dist = cosine_distance(nattr, b, a);
#endif COSINE_DISTANCE

#ifdef EUCLIDEAN_DISTANCE
	dist = euclidean_distance(nattr, b, a);
#endif EUCLIDEAN_DISTANCE

#ifdef OVERLAPPING_DISTANCE
	dist = overlapping_distance(nattr, b, a);
#endif OVERLAPPING_DISTANCE

#ifdef HAMMING_DISTANCE
	dist = hamming_distance(nattr, b, a);
#endif HAMMING_DISTANCE

#ifdef KULLBACK_LEIBLER		
	dist = Kullback_Leibler(nattr, b, a);	// from a to b
#endif KULLBACK_LEIBLER

#ifdef JENSEN_SHANNON		
	dist = Jensen_Shannon(nattr, b, a);	// a and b
#endif JENSEN_SHANNON

	return dist;
}

//
// Given a set of paths, find whether there are critical transitions on the path
// NOTE: the sequence is a->b->c
// this function finds distances in keywords, in_nbrs, and out_nbrs separately and then combine them 
//
int critical_transition(int npaths, struct SPATH *paths, int mptype, int nnodes, struct PN *nw, int nwos, struct WOS *wos, int ntkeywords, struct TKEYWORDS *tkeyword)
{
	int i, j, k, r, m;
	int ia, ib, ic;	// index in the nw[] array
	int iia, iib, iic;	// index in the wos[] array
	int ka, kb, kc, kin, kout;
	int in_cnt, out_cnt, tcnt;
	int ntk, n_innbrs, n_outnbrs;
	int nattr;
	int ct_cnt;
	int ranking;
	PSTRING *a, *b, *c;	// probability distribution array
	double DKac, DKab, DKbc;
	double DIac, DIab, DIbc;
	double DOac, DOab, DObc;
	double Dac, Dab, Dbc;
	struct TKEYWORDS *tk;
	struct NEIGHBOR *in_nbrs;
	struct NEIGHBOR *out_nbrs;
	int ndx2tk;
	struct ARRAY_STATS dist_stats;
	double *dist;

	dist = (double *)Jmalloc(100000 * sizeof(double), L"critical_transition: dist");	// assume no more than 100,000 (a,b,c) triads to check for each b
	if (dist == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"critical_transition: tk");
	if (tk == NULL) return MSG_NOT_ENOUGH_MEMORY;
	in_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"critical_transition: in_nbrs");
	if (in_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	out_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"critical_transition: out_nbrs");
	if (out_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;

	a = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: a");
	b = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: b");
	c = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: c");

	fwprintf(logstream, L"\nCritical transitions:\n");
#ifdef COSINE_DISTANCE
	fwprintf(logstream, L"Apply Cosine Distance\n");
#endif COSINE_DISTANCE
#ifdef JACCARD_DISTANCE
	fwprintf(logstream, L"Apply Jaccard Distance\n");
#endif JACCARD_DISTANCE
#ifdef OVERLAPPING_DISTANCE
	fwprintf(logstream, L"Apply Overlapping Distance\n");
#endif OVERLAPPING_DISTANCE
#ifdef KULLBACK_LEIBLER	
	fwprintf(logstream, L"Apply Kullback-Leibler Divergence\n");	
	fwprintf(logstream, L"Smoothing delta is set to: %f\n", SMOOTHING_DELTA);
#endif KULLBACK_LEIBLER
	fwprintf(logstream, L"Links with SPX less than %f are ignored in the dogleg identifiction process.\n", spx_cut);

	for (i = 0; i < nnodes; i++) nw[i].flag = 0;	// added 2016/09/22, initialize 'flag', which is used to denote whether the node is checked on critical transition

	for (i = 0; i < npaths; i++)
	{ 
		if (mptype == P_LOCAL_F && (paths[i].tcnt == 0 || paths[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (paths[i].tcnt == 0 || paths[i].hit_a_source == FALSE)) continue;
		fwprintf(logstream, L"paths=%d\n", i);
		if (paths[i].len < 3) continue;	// only when there are more than 3 nodes on the path
		fwprintf(logstream, L"Routes\tNDoglegs\tN Triads\tPercentage\tMean\tMin\tMax\tStDev\n");
		for (k = 0; k < (paths[i].len - 2); k++)
		{
			ka = paths[i].seq[k]; kb = paths[i].seq[k+1]; kc = paths[i].seq[k+2];
			if (nw[kb].flag == 1)	// skip if the center node is processed, added 2016/09/22
				continue;
			nw[kb].flag = 1;
			fwprintf(logstream, L"[%s->%s->%s]\t", nw[ka].alias, nw[kb].alias, nw[kc].alias);
			ct_cnt = in_cnt = 0; tcnt = 0;
			for (kin = 0; kin < nw[kb].in_deg; kin++)	// check on all the doglegs centered on 'ib'
			{
				if (nw[kb].in_spx[kin] < spx_cut)	// ignore those with SPX less than a cut off value, added 2015/05/28
					continue;
				in_cnt++; out_cnt = 0; 
				for (kout = 0; kout < nw[kb].out_deg; kout++)
				{
					if (nw[kb].out_spx[kout] < spx_cut)	// ignore those with SPX less than a cut off value, added 2015/05/28
						continue;
					out_cnt++;
					//fwprintf(logstream, L"\t\t%d %d\n", kin, kout);
					ia = nw[kb].in_nbrs[kin]; ib = kb; ic = nw[kb].out_nbrs[kout];
					// establish the probability distribution array for a, b, and c
					for (j = 0; j < N_ATTRIBUTES; j++) {a[j].cnt = b[j].cnt = c[j].cnt = 0; }
					iia = nw[ia].ndx2wos; iib = nw[ib].ndx2wos; iic = nw[ic].ndx2wos;
					for (j = 0; j < ntkeywords; j++) tkeyword[j].ndx = -1;  // added 2015/03/04
					pool_keywords_3(iia, iib, iic, wos, tkeyword, &ntk, tk);	// pooled keywords are stored in tk[]
					// NOTE: after pooing keywords, tk[].acnt=0, tk[].cnt=total count of the keyword in both the title and abstract
					//fwprintf(logstream, L"##### ");
					//for (r = 0; r <  ntk; r++) fwprintf(logstream, L"%03d %s: %d %d %d %d\n", r, tk[r].name, tk[r].acnt, tk[r].cnt, tk[r].ranking, tk[r].ndx);
					//fflush(logstream);
					// Note: the sequence is a->b->c
					// keyword distribution for a
					for (r = 0; r < wos[iia].ntkws; r++)	// titlle keyword distribution for a
					{
						ndx2tk = tkeyword[wos[iia].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) a[ranking].cnt++;
					}
					// keyword distribution for b
					for (r = 0; r < wos[iib].ntkws; r++)	// titlle keyword distribution for b
					{
						ndx2tk = tkeyword[wos[iib].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) b[ranking].cnt++;
					}
					// keyword distribution for c
					for (r = 0; r < wos[iic].ntkws; r++)	// titlle keyword distribution for c
					{
						ndx2tk = tkeyword[wos[iic].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) c[ranking].cnt++;
					}	
					// convert count into probability
					for (r = 0; r < ntk; r++) a[r].prob = (double)a[r].cnt / ntk;
					for (r = 0; r < ntk; r++) b[r].prob = (double)b[r].cnt / ntk;
					for (r = 0; r < ntk; r++) c[r].prob = (double)c[r].cnt / ntk;
#ifdef DEBUG
					for (r = 0; r < ntk; r++) fwprintf(logstream, L"a%d-%.4f\t", r+1, a[r].prob); 
					fwprintf(logstream, L"\n");
					for (r = 0; r < ntk; r++) fwprintf(logstream, L"b%d-%.4f\t", r+1, b[r].prob);
					fwprintf(logstream, L"\n");
					for (r = 0; r < ntk; r++) fwprintf(logstream, L"c%d-%.4f\t", r+1, c[r].prob);
					fwprintf(logstream, L"\n");
#endif DEBUG

#ifdef KULLBACK_LEIBLER		
					smoothing(ntk, SMOOTHING_DELTA, a);		// NOTE: smoothing overwrites the existing [].prob
					smoothing(ntk, SMOOTHING_DELTA, b);	
					smoothing(ntk, SMOOTHING_DELTA, c);	
#endif KULLBACK_LEIBLER
					DKac = distance(ntk, c, a);	// from a to c
					DKab = distance(ntk, b, a);	// from a to b
					DKbc = distance(ntk, c, b);	// from b to c
					//fwprintf(logstream, L"+++++DKac=%.6f, DKab=%.6f, DKbc=%.6f, DK=%.6f\n", DKac, DKab, DKbc, DKac-(DKab+DKbc));

					for (j = 0; j < N_ATTRIBUTES; j++) {a[j].cnt = b[j].cnt = c[j].cnt = 0; }
					pool_in_nbrs(ia, ib, ic, nnodes, nw, &n_innbrs, in_nbrs);		// pooled citations are stored in in_nbrs[]				
					for (r = 0; r < nw[ia].in_deg; r++)	// indegree citation distribution for a
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ia].in_nbrs[r]) a[j].cnt++;
						}
					}
					for (r = 0; r < nw[ib].in_deg; r++)	// indegree citation distribution for b
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ib].in_nbrs[r]) b[j].cnt++;
						}
					}
					for (r = 0; r < nw[ic].in_deg; r++)	// indegree citation distribution for c
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ic].in_nbrs[r]) c[j].cnt++;
						}
					}
					// convert count into probability
					for (r = 0; r < n_innbrs; r++) a[r].prob = (double)a[r].cnt / n_innbrs; 
					for (r = 0; r < n_innbrs; r++) b[r].prob = (double)b[r].cnt / n_innbrs; 
					for (r = 0; r < n_innbrs; r++) c[r].prob = (double)c[r].cnt / n_innbrs; 
#ifdef DEBUG
					for (r = 0; r < n_innbrs; r++) fwprintf(logstream, L"a%d-%.4f\t", r+1, a[r].prob);
					fwprintf(logstream, L"\n");
					for (r = 0; r < n_innbrs; r++) fwprintf(logstream, L"b%d-%.4f\t", r+1, b[r].prob);
					fwprintf(logstream, L"\n");
					for (r = 0; r < n_innbrs; r++) fwprintf(logstream, L"c%d-%.4f\t", r+1, c[r].prob);
					fwprintf(logstream, L"\n");
#endif DEBUG

#ifdef KULLBACK_LEIBLER		
					smoothing(n_innbrs, SMOOTHING_DELTA, a);	
					smoothing(n_innbrs, SMOOTHING_DELTA, b);	
					smoothing(n_innbrs, SMOOTHING_DELTA, c);	
#endif KULLBACK_LEIBLER
					DIac = distance(n_innbrs, c, a);	// from a to c
					DIab = distance(n_innbrs, b, a);	// from a to b
					DIbc = distance(n_innbrs, c, b);	// from b to c
					//fwprintf(logstream, L"#####DIac=%.6f, DIab=%.6f, DIbc=%.6f, DI=%.6f\n", DIac, DIab, DIbc, DIac-(DIab+DIbc));

					for (j = 0; j < N_ATTRIBUTES; j++) {a[j].cnt = b[j].cnt = c[j].cnt = 0; }
					pool_out_nbrs(ia, ib, ic, nnodes, nw, &n_outnbrs, out_nbrs);		// pooled citations are stored in out_nbrs[]				
					for (r = 0; r < nw[ia].out_deg; r++)	// outdegree citation distribution for a
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ia].out_nbrs[r]) a[j].cnt++;
						}
					}
					for (r = 0; r < nw[ib].out_deg; r++)	// outdegree citation distribution for b
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ib].out_nbrs[r]) b[j].cnt++;
						}
					}
					for (r = 0; r < nw[ic].out_deg; r++)	// outdegree citation distribution for c
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ic].out_nbrs[r]) c[j].cnt++;
						}
					}
					// convert count into probability
					for (r = 0; r < n_outnbrs; r++) a[r].prob = (double)a[r].cnt / n_outnbrs; 
					for (r = 0; r < n_outnbrs; r++) b[r].prob = (double)b[r].cnt / n_outnbrs; 
					for (r = 0; r < n_outnbrs; r++) c[r].prob = (double)c[r].cnt / n_outnbrs; 
#ifdef DEBUG
					for (r = 0; r < n_outnbrs; r++) fwprintf(logstream, L"a%d-%.4f\t", r+1, a[r].prob);
					fwprintf(logstream, L"\n");
					for (r = 0; r < n_outnbrs; r++) fwprintf(logstream, L"b%d-%.4f\t", r+1, b[r].prob);
					fwprintf(logstream, L"\n");
					for (r = 0; r < n_outnbrs; r++) fwprintf(logstream, L"c%d-%.4f\t", r+1, c[r].prob);
					fwprintf(logstream, L"\n");
#endif DEBUG

#ifdef KULLBACK_LEIBLER		
					smoothing(n_outnbrs, SMOOTHING_DELTA, a);	
					smoothing(n_outnbrs, SMOOTHING_DELTA, b);	
					smoothing(n_outnbrs, SMOOTHING_DELTA, c);	
#endif KULLBACK_LEIBLER
					DOac = distance(n_outnbrs, c, a);	// from a to c
					DOab = distance(n_outnbrs, b, a);	// from a to b
					DObc = distance(n_outnbrs, c, b);	// from b to c
					//fwprintf(logstream, L"@@@@@DOac=%.6f, DOab=%.6f, DObc=%.6f, DO=%.6f\n", DOac, DOab, DObc, DOac-(DOab+DObc));

					//Dac = DKac; Dab = DKab; Dbc = DKbc;
					//Dac = DIac; Dab = DIab; Dbc = DIbc;
					//Dac = DOac; Dab = DOab; Dbc = DObc;

					//Dac = (DKac + DIac + DOac) / 3;
					//Dab = (DKab + DIab + DOab) / 3;
					//Dbc = (DKbc + DIbc + DObc) / 3;

					Dac = DKac / 2 + (DIac + DOac) / 4;
					Dab = DKab / 2 + (DIab + DOab) / 4;
					Dbc = DKbc / 2 + (DIbc + DObc) / 4;
					//fwprintf(logstream, L"*****Dac=%.6f, Dab=%.6f, Dbc=%.6f, D=%.6f\n", Dac, Dab, Dbc, Dac-(Dab+Dbc));

#ifdef XXX
					pool_in_nbrs(ia, ib, ic, nnodes, nw, &n_innbrs, in_nbrs);		// pooled keywords are stored in in_nbrs[]
					pool_out_nbrs(ia, ib, ic, nnodes, nw, &n_outnbrs, out_nbrs);	// pooled keywords are stored in out_nbrs[]

					//n_innbrs = n_outnbrs = 0;	// for debugging purpose
					//ntk = 0; n_innbrs = 0;			// for debugging purpose

					nattr = ntk + n_innbrs + n_outnbrs;

					// Note: the sequence is a->b->c
					// distribution for a
					for (r = 0; r < wos[iia].ntkws; r++)	// titlle keyword distribution for a
					{
						ndx2tk = tkeyword[wos[iia].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) a[ranking].cnt++;
					}
					for (r = 0; r < nw[ia].in_deg; r++)	// indegree citation distribution for a
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ia].in_nbrs[r]) a[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ia].out_deg; r++)	// outdegree citation distribution for a
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ia].out_nbrs[r]) a[ntk+n_innbrs+j].cnt++;
						}
					}
					// distribution for b
					for (r = 0; r < wos[iib].ntkws; r++)	// titlle keyword distribution for b
					{
						ndx2tk = tkeyword[wos[iib].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) b[ranking].cnt++;
					}
					for (r = 0; r < nw[ib].in_deg; r++)	// indegree citation distribution for b
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ib].in_nbrs[r]) b[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ib].out_deg; r++)	// outdegree citation distribution for b
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ib].out_nbrs[r]) b[ntk+n_innbrs+j].cnt++;
						}
					}
					// distribution for c
					for (r = 0; r < wos[iic].ntkws; r++)	// titlle keyword distribution for c
					{
						ndx2tk = tkeyword[wos[iic].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) c[ranking].cnt++;
					}			
					for (r = 0; r < nw[ic].in_deg; r++)	// indegree citation distribution for c
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ic].in_nbrs[r]) c[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ic].out_deg; r++)	// outdegree citation distribution for c
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ic].out_nbrs[r]) c[ntk+n_innbrs+j].cnt++;
						}
					}

		//#ifdef DEBUG
					fwprintf(logstream, L"[%f, %f, %f] ==> %f\t", Dac, Dab, Dbc, Dac-(Dab+Dbc)); fflush(logstream);
					fwprintf(logstream, L"[%s->%s->%s]\n", nw[ia].alias, nw[ib].alias, nw[ic].alias);
		//#endif DEBUG
#endif
					dist[tcnt++] = Dac - (Dab+Dbc);
					if (Dac-(Dab+Dbc) > 0.0)
					{
						ct_cnt++;
						//fwprintf(logstream, L"\t\t[%s->%s->%s]  ", nw[ia].alias, nw[ib].alias, nw[ic].alias);
						//fwprintf(logstream, L"\t\t\t[%f, %f, %f] ==> %f\n", Dac, Dab, Dbc, Dac-(Dab+Dbc)); fflush(logstream);
					}
				}
			}
			array_statistics(tcnt, dist, &dist_stats);
			fwprintf(logstream, L"@@@@@ %d\t%d\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\n", 
				ct_cnt, in_cnt*out_cnt, (double)ct_cnt/(in_cnt*out_cnt), dist_stats.mean, dist_stats.min, dist_stats.max, dist_stats.stdev);
		}
	}

	Jfree(a, L"critical_transition: a");
	Jfree(b, L"critical_transition: b");
	Jfree(c, L"critical_transition: c");

exit(0);
	return 0;
}

#ifdef ALL_PARAMETERS_AS_ONE_VECTOR
//
// Given a set of paths, find whether there are critical transitions on the path
// NOTE: the sequence is a->b->c
//
int critical_transition(int npaths, struct SPATH *paths, int mptype, int nnodes, struct PN *nw, int nwos, struct WOS *wos, int ntkeywords, struct TKEYWORDS *tkeyword)
{
	int i, j, k, r, m;
	int ia, ib, ic;	// index in the nw[] array
	int iia, iib, iic;	// index in the wos[] array
	int ka, kb, kc, kin, kout;
	int in_cnt, out_cnt;
	int ntk, n_innbrs, n_outnbrs;
	int nattr;
	int ct_cnt;
	int ranking;
	PSTRING *a, *b, *c;	// probability distribution array
	double Dac, Dab, Dbc;
	struct TKEYWORDS *tk;
	struct NEIGHBOR *in_nbrs;
	struct NEIGHBOR *out_nbrs;
	int ndx2tk;
	//int indx[N_INDEGREE_ATTR];	// index (for nw[]) of the top in-degree nodes
	//int ondx[N_OUTDEGREE_ATTR];	// index (for nw[]) of the top out-degree nodes

	tk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"critical_transition: tk");
	if (tk == NULL) return MSG_NOT_ENOUGH_MEMORY;
	in_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"critical_transition: in_nbrs");
	if (in_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	out_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"critical_transition: out_nbrs");
	if (out_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;

	//qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_indegree);	// sort according to a node's degree
	//for (i = 0; i < N_INDEGREE_ATTR; i++)
	//	indx[i] = nw[i].ndx;	// indx[0] contains the index that points to the node that has the largest in-degree, etc.
	//qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_outdegree);	// sort according to a node's degree
	//for (i = 0; i < N_OUTDEGREE_ATTR; i++)
	//	ondx[i] = nw[i].ndx;	// ondx[0] contains the index that points to the node that has the largest out-degree, etc.
	//for (i = 0; i < nnodes; i++)
	//	fwprintf(logstream, L"%d\t%s\t%s\t%d\t%d\n", nw[i].degree, nw[i].name, nw[i].alias, nw[i].ndx2wos, nw[i].ndx);
	//qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_nwname);	// sort back to the original order
	//for (i = 0; i < N_CITATION_ATTR; i++)
	//	fwprintf(logstream, L"***%d %d\n", indx[i], nw[dndx[i]].degree);

	a = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: a");
	b = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: b");
	c = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: c");

	fwprintf(logstream, L"\nCritical transitions:\n");
#ifdef OVERLAPPING_DISTANCE
	fwprintf(logstream, L"Apply Overlapping Distance\n");
#endif OVERLAPPING_DISTANCE
#ifdef KULLBACK_LEIBLER	
	fwprintf(logstream, L"Apply Kullback-Leibler Divergence\n");	
	fwprintf(logstream, L"Smoothing delta is set to: %f\n", SMOOTHING_DELTA);
#endif KULLBACK_LEIBLER
	fwprintf(logstream, L"Links with SPX less than %f are ignored in the dogleg identifiction process.\n", spx_cut);

	for (i = 0; i < npaths; i++)
	{ 
		if (mptype == P_LOCAL_F && (paths[i].tcnt == 0 || paths[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (paths[i].tcnt == 0 || paths[i].hit_a_source == FALSE)) continue;
		fwprintf(logstream, L"paths=%d\n", i);
		if (paths[i].len < 3) continue;	// only when there are more than 3 nodes on the path
		for (k = 0; k < (paths[i].len - 2); k++)
		{
			ka = paths[i].seq[k]; kb = paths[i].seq[k+1]; kc = paths[i].seq[k+2];
			fwprintf(logstream, L"\t\t[%s->%s->%s]\t", nw[ka].alias, nw[kb].alias, nw[kc].alias);
			ct_cnt = in_cnt = 0;
			for (kin = 0; kin < nw[kb].in_deg; kin++)	// check on all the doglegs centered on 'ib'
			{
				if (nw[kb].in_spx[kin] < spx_cut)	// ignore those with SPX less than a cut off value, added 2015/05/28
					continue;
				in_cnt++; out_cnt = 0;
				for (kout = 0; kout < nw[kb].out_deg; kout++)
				{
					if (nw[kb].out_spx[kout] < spx_cut)	// ignore those with SPX less than a cut off value, added 2015/05/28
						continue;
					out_cnt++;
					//fwprintf(logstream, L"\t\t%d %d\n", kin, kout);
					ia = nw[kb].in_nbrs[kin]; ib = kb; ic = nw[kb].out_nbrs[kout];
					// establish the probability distribution array for a, b, and c
					for (j = 0; j < N_ATTRIBUTES; j++) {a[j].cnt = b[j].cnt = c[j].cnt = 0; }
					iia = nw[ia].ndx2wos; iib = nw[ib].ndx2wos; iic = nw[ic].ndx2wos;
					for (j = 0; j < ntkeywords; j++) tkeyword[j].ndx = -1;  // added 2015/03/04
					pool_keywords_3(iia, iib, iic, wos, tkeyword, &ntk, tk);	// pooled keywords are stored in tk[]
					// NOTE: after pooing keywords, tk[].acnt=0, tk[].cnt=total count of the keyword in both the title and abstract
					//fwprintf(logstream, L"##### ");
					//for (r = 0; r <  ntk; r++) fwprintf(logstream, L"%03d %s: %d %d %d %d\n", r, tk[r].name, tk[r].acnt, tk[r].cnt, tk[r].ranking, tk[r].ndx);
					//fflush(logstream);

					pool_in_nbrs(ia, ib, ic, nnodes, nw, &n_innbrs, in_nbrs);		// pooled keywords are stored in in_nbrs[]
					pool_out_nbrs(ia, ib, ic, nnodes, nw, &n_outnbrs, out_nbrs);	// pooled keywords are stored in out_nbrs[]

					//n_innbrs = n_outnbrs = 0;	// for debugging purpose
					//ntk = 0; n_innbrs = 0;			// for debugging purpose

					nattr = ntk + n_innbrs + n_outnbrs;

					// Note: the sequence is a->b->c
					// distribution for a
					for (r = 0; r < wos[iia].ntkws; r++)	// titlle keyword distribution for a
					{
						ndx2tk = tkeyword[wos[iia].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) a[ranking].cnt++;
					}
					for (r = 0; r < nw[ia].in_deg; r++)	// indegree citation distribution for a
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ia].in_nbrs[r]) a[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ia].out_deg; r++)	// outdegree citation distribution for a
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ia].out_nbrs[r]) a[ntk+n_innbrs+j].cnt++;
						}
					}
					// distribution for b
					for (r = 0; r < wos[iib].ntkws; r++)	// titlle keyword distribution for b
					{
						ndx2tk = tkeyword[wos[iib].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) b[ranking].cnt++;
					}
					for (r = 0; r < nw[ib].in_deg; r++)	// indegree citation distribution for b
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ib].in_nbrs[r]) b[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ib].out_deg; r++)	// outdegree citation distribution for b
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ib].out_nbrs[r]) b[ntk+n_innbrs+j].cnt++;
						}
					}
					// distribution for c
					for (r = 0; r < wos[iic].ntkws; r++)	// titlle keyword distribution for c
					{
						ndx2tk = tkeyword[wos[iic].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) c[ranking].cnt++;
					}			
					for (r = 0; r < nw[ic].in_deg; r++)	// indegree citation distribution for c
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ic].in_nbrs[r]) c[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ic].out_deg; r++)	// outdegree citation distribution for c
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ic].out_nbrs[r]) c[ntk+n_innbrs+j].cnt++;
						}
					}
		#ifdef HAMMING_DISTANCE
					// changes a keyword count to a flag (1) 
					for (j = 0; j < ntk; j++) { if (a[j].cnt != 0) a[j].cnt = 1; }
					for (j = 0; j < ntk; j++) { if (b[j].cnt != 0) b[j].cnt = 1; }
					for (j = 0; j < ntk; j++) { if (c[j].cnt != 0) c[j].cnt = 1; }
		#endif HAMMING_DISTANCE
		#ifdef KULLBACK_LEIBLER		
					smoothing(nattr, SMOOTHING_DELTA, a);	
					smoothing(nattr, SMOOTHING_DELTA, b);	
					smoothing(nattr, SMOOTHING_DELTA, c);	
		#endif KULLBACK_LEIBLER
		#ifdef JENSEN_SHANNON		
					fwprintf(logstream, L"Jensen Shannon (symmetrical) ");
					smoothing(nattr, SMOOTHING_DELTA, a);	// use 0.1 rather than 1.0 is because the counts for each attributes are small
					smoothing(nattr, SMOOTHING_DELTA, b);	
					smoothing(nattr, SMOOTHING_DELTA, c);	
		#endif JENSEN_SHANNON

		#ifdef DEBUG	// display probability distribution
					fwprintf(logstream, L"\n%s->%s->%s (ntk=%d, n_innbrs=%d, n_outnbrs=%d)\n", nw[ia].alias, nw[ib].alias, nw[ic].alias, ntk, n_innbrs, n_outnbrs);
					fwprintf(logstream, L"a: [");
					for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", a[j].prob);
					fwprintf(logstream, L"]\nb: [");
					for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", b[j].prob);
					fwprintf(logstream, L"]\nc: ["); fflush(logstream);
					for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", c[j].prob);
					fwprintf(logstream, L"]\n"); fflush(logstream);
		#endif DEBUG
		#ifdef DEBUG	// display the contents that determine probability distribution
					fwprintf(logstream, L"\n%s->%s->%s (ntk=%d, n_innbrs=%d, n_outnbrs=%d)\n", nw[ia].alias, nw[ib].alias, nw[ic].alias, ntk, n_innbrs, n_outnbrs);
					fwprintf(logstream, L"a: ");
					for (j = 0; j < ntk; j++) fwprintf(logstream, L"%s[%d] ", tk[j].name, a[j].cnt);
					for (j = 0; j < n_innbrs; j++) fwprintf(logstream, L"%d(%d) ", in_nbrs[j].id, a[ntk+j].cnt);
					for (j = 0; j < n_outnbrs; j++) fwprintf(logstream, L"%d{%d} ", out_nbrs[j].id, a[ntk+n_innbrs+j].cnt);
					fwprintf(logstream, L"\nb: ");
					for (j = 0; j < ntk; j++) fwprintf(logstream, L"%s[%d] ", tk[j].name, b[j].cnt);
					for (j = 0; j < n_innbrs; j++) fwprintf(logstream, L"%d(%d) ", in_nbrs[j].id, b[ntk+j].cnt);
					for (j = 0; j < n_outnbrs; j++) fwprintf(logstream, L"%d{%d} ", out_nbrs[j].id, b[ntk+n_innbrs+j].cnt);
					fwprintf(logstream, L"\nc: "); fflush(logstream);
					for (j = 0; j < ntk; j++) fwprintf(logstream, L"%s[%d] ", tk[j].name, c[j].cnt);
					for (j = 0; j < n_innbrs; j++) fwprintf(logstream, L"%d(%d) ", in_nbrs[j].id, c[ntk+j].cnt);
					for (j = 0; j < n_outnbrs; j++) fwprintf(logstream, L"%d{%d} ", out_nbrs[j].id, c[ntk+n_innbrs+j].cnt);
					fwprintf(logstream, L"\n"); fflush(logstream);
		#endif DEBUG

					Dac = distance(nattr, c, a);	// from a to c
					Dab = distance(nattr, b, a);	// from a to b
					Dbc = distance(nattr, c, b);	// from b to c

		//#ifdef DEBUG
					fwprintf(logstream, L"[%f, %f, %f] ==> %f\t", Dac, Dab, Dbc, Dac-(Dab+Dbc)); fflush(logstream);
					fwprintf(logstream, L"[%s->%s->%s]\n", nw[ia].alias, nw[ib].alias, nw[ic].alias);
		//#endif DEBUG
					if (Dac-(Dab+Dbc) > 0)
					{
						ct_cnt++;
						//fwprintf(logstream, L"\t\t[%s->%s->%s]  ", nw[ia].alias, nw[ib].alias, nw[ic].alias);
						//fwprintf(logstream, L"\t\t\t[%f, %f, %f] ==> %f\n", Dac, Dab, Dbc, Dac-(Dab+Dbc)); fflush(logstream);
					}
				}
			}
			fwprintf(logstream, L"%d/%d=\t%f\n", ct_cnt, in_cnt*out_cnt, (double)ct_cnt/(in_cnt*out_cnt));
		}
	}

	Jfree(a, L"critical_transition: a");
	Jfree(b, L"critical_transition: b");
	Jfree(c, L"critical_transition: c");

	return 0;
}
#endif ALL_PARAMETERS_AS_ONE_VECTOR

//
// function to test expected_information()
//
int test_eI()
{
	int i, n;
	PSTRING q[N_ATTRIBUTES];
	PSTRING p[N_ATTRIBUTES];
	double KL, JS1, JS2;

#ifdef XXX
	n = N_ATTRIBUTES;
	for (i = 0; i < n; i++)
	{
		q[i].prob = 1.0 / n;
		p[i].prob = 1.0 / n;
	}
	q[5].prob = 0.02; q[15].prob = 0.08; 
	q[2].prob = 0.0; q[8].prob = 0.10;
#endif
	n = 1;
	q[0].prob = 1.0;
	p[0].prob = 1.0;

	KL = Kullback_Leibler(n, q, p);
	JS1 = Jensen_Shannon(n, q, p);
	JS2 = Jensen_Shannon(n, p, q);
	fwprintf(logstream, L"KL = %f, JS1 = %f, JS2 = %f\n", KL, JS1, JS2);

	return 0;
}
