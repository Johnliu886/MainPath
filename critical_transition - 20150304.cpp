//
// critical_transition.cpp
//

//
// Revision History:
// 2014/12/26 Basic function works
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

#define N_TKEYWORD_ATTR 30
#define N_INDEGREE_ATTR 10
#define N_OUTDEGREE_ATTR 10
#define N_ATTRIBUTES (N_TKEYWORD_ATTR+N_INDEGREE_ATTR+N_OUTDEGREE_ATTR)	// the number of attributes (title keywords, abstract keywords, cited doc., etc.) to include in the distribution

extern FILE *logstream;
extern struct TKEYWORDS *tword;	// added 2015/03/04
extern int nkreport;	// added 2015/03/04

extern int compare_tkeyword(const void *, const void *);
extern int compare_wordcount(const void *, const void *);

struct PDIST
{
	int cnt;		// count
	double prob;	// probability
};

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
// pool keywords of three papers together and remove duplicates
// NOTE: this function also filters out keywords that are not listed in the "Keyword report.txt" file, 2015/03/04
//
int pool_keywords(int p1, int p2, int p3, struct WOS *wos, struct TKEYWORDS *tkeyword, int *ntk, struct TKEYWORDS *tk)
{
	int i, k;
	int nttk;
	int cnt;
	int cnt2;
	struct TKEYWORDS *ttk;
	wchar_t prev_name[MAX_TKEYWORDS];

	ttk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"pool_keywords: ttk");
	if (ttk == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// NOTE: wos[].tkws[] include keywords in both the title and abstract
	nttk = 0;
	for (k = 0; k < wos[p1].ntkws; k++)	// paper 1
	{
		if (is_in_keyword_report(tkeyword[wos[p1].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx = wos[p1].tkws[k];	// set the link from tkeyword[] to ttk[]
			wcscpy(ttk[nttk++].name, tkeyword[wos[p1].tkws[k]].name);
		}
	}
	for (k = 0; k < wos[p2].ntkws; k++)	// paper 2
	{
		if (is_in_keyword_report(tkeyword[wos[p2].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx = wos[p2].tkws[k];	// set the link from tkeyword[] to ttk[]
			wcscpy(ttk[nttk++].name, tkeyword[wos[p2].tkws[k]].name);
		}
	}
	for (k = 0; k < wos[p3].ntkws; k++)	// paper 3
	{
		if (is_in_keyword_report(tkeyword[wos[p3].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx = wos[p3].tkws[k];	// set the link from tkeyword[] to ttk[]
			wcscpy(ttk[nttk++].name, tkeyword[wos[p3].tkws[k]].name);
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
		if (ttk[i].cnt >= 2)	// assign *ntk the number of keywords that have count larger than 1
			cnt2++;
	}
	*ntk = cnt2;
	free(ttk);

#ifdef DEBUG
	fwprintf(logstream, L"***%d\n", *ntk);
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
int additive_smoothing(int v, double delta, struct PDIST *p)
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
int smoothing(int v, double delta, struct PDIST *p)
{
	additive_smoothing(v, delta, p);

	return 0;
}

// calculate Kullback-Leibler divergence I(q:p) in bits, given two distribution p and q. p represents the "a priori" distribution and q the posteriori distribution.
// I(q:p) = sum[q[i]*log2(q[i]/p[i])]
// "I is th expected information value of the message that the a priori distribution is transformed into the a posteriori one." see page 1952, 
//  Lucio-Arias, D., & Leydesdorff, L. (2008). Main-path analysis and path-dependent transitions in HistCite-based historiograms. 
//  Journal of the American Society for Information Science and Technology, 59(12), 1948-1962.
//
double Kullback_Leibler(int n, PDIST *q, PDIST *p)
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
double Jensen_Shannon(int n, PDIST *p, PDIST *q)
{
	int i;
	double JS;
	PDIST *m;

	m = (PDIST *)Jmalloc(n * sizeof(PDIST), L"Jensen_Shannon: m");
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

//
// Given a set of paths, find whether there are critical transitions on the path
// NOTE: the sequence is a->b->c
//
int critical_transition(int npaths, struct SPATH *paths, int mptype, int nnodes, struct PN *nw, int nwos, struct WOS *wos, int ntkeywords, struct TKEYWORDS *tkeyword)
{
	int i, j, k, r;
	int ia, ib, ic;	// index in the nw[] array
	int iia, iib, iic;	// index in the wos[] array
	int ntk;
	int nattr;
	int ranking;
	PDIST *a, *b, *c;	// probability distribution array
	double KLac, KLab, KLbc;
	struct TKEYWORDS *tk;
	int ndx2tk;
	int indx[N_INDEGREE_ATTR];	// index (for nw[]) of the top in-degree nodes
	int ondx[N_OUTDEGREE_ATTR];	// index (for nw[]) of the top out-degree nodes

	tk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"critical_transition: tk");
	if (tk == NULL) return MSG_NOT_ENOUGH_MEMORY;

	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_indegree);	// sort according to a node's degree
	for (i = 0; i < N_INDEGREE_ATTR; i++)
		indx[i] = nw[i].ndx;	// indx[0] contains the index that points to the node that has the largest in-degree, etc.
	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_outdegree);	// sort according to a node's degree
	for (i = 0; i < N_OUTDEGREE_ATTR; i++)
		ondx[i] = nw[i].ndx;	// ondx[0] contains the index that points to the node that has the largest out-degree, etc.
	//for (i = 0; i < nnodes; i++)
	//	fwprintf(logstream, L"%d\t%s\t%s\t%d\t%d\n", nw[i].degree, nw[i].name, nw[i].alias, nw[i].ndx2wos, nw[i].ndx);
	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_nwname);	// sort back to the original order
	//for (i = 0; i < N_CITATION_ATTR; i++)
	//	fwprintf(logstream, L"***%d %d\n", indx[i], nw[dndx[i]].degree);

	a =  (PDIST *)Jmalloc(N_ATTRIBUTES * sizeof(PDIST), L"critical_transition: a");
	b = (PDIST *)Jmalloc(N_ATTRIBUTES * sizeof(PDIST), L"critical_transition: b");
	c =  (PDIST *)Jmalloc(N_ATTRIBUTES * sizeof(PDIST), L"critical_transition: c");

	fwprintf(logstream, L"\nCritical transitions:\n");

	for (i = 0; i < npaths; i++)
	{ 
		if (mptype == P_LOCAL_F && (paths[i].tcnt == 0 || paths[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (paths[i].tcnt == 0 || paths[i].hit_a_source == FALSE)) continue;
		fwprintf(logstream, L"paths=%d\n", i);
		if (paths[i].len < 3) continue;	// only when there are more than 3 nodes on the path
		for (k = 0; k < (paths[i].len - 2); k++)
		{
			ia = paths[i].seq[k]; ib = paths[i].seq[k+1]; ic = paths[i].seq[k+2];
			// establish the probability distribution array for a, b, and c
			for (j = 0; j < N_ATTRIBUTES; j++) {a[j].cnt = b[j].cnt = c[j].cnt = 0; }
			iia = nw[ia].ndx2wos; iib = nw[ib].ndx2wos; iic = nw[ic].ndx2wos;
			for (j = 0; j < ntkeywords; j++) tkeyword[j].ndx = -1;  // added 2015/03/04
			pool_keywords(iia, iib, iic, wos, tkeyword, &ntk, tk);	// pooled keywords are stored in tk[]
			// NOTE: after pooing keywords, tk[].acnt=0, tk[].cnt=total count of the keyword in both the title and abstract
			//fwprintf(logstream, L"##### ");
			//for (r = 0; r <  ntk; r++)
			//	fwprintf(logstream, L"%03d %s: %d %d %d %d\n", r, tk[r].name, tk[r].acnt, tk[r].cnt, tk[r].ranking, tk[r].ndx);
			//fflush(logstream);

			// Note: the sequence is a->b->c
			// distribution for a
			for (r = 0; r < wos[iia].ntkws; r++)	// titlle keyword distribution for a
			{
				ndx2tk = tkeyword[wos[iia].tkws[r]].ndx;
				if (ndx2tk == -1)	// not in the current keyword pool
					continue;
				ranking = tk[ndx2tk].ranking;
				//fwprintf(logstream, L"#%d[%s] ", ranking, tkeyword[wos[iip].tkws[r]].name);
				if (ntk > N_TKEYWORD_ATTR) nattr = N_ATTRIBUTES; else nattr = ntk+N_INDEGREE_ATTR+N_OUTDEGREE_ATTR;
				if (ranking < nattr) a[ranking].cnt++;
			}
			for (r = 0; r < nw[ia].in_deg; r++)	// indegree citation distribution for a
			{
				for (j = 0; j < N_INDEGREE_ATTR; j++)
				{
					if (nw[ia].in_nbrs[r] == ondx[j]) a[N_TKEYWORD_ATTR+j].cnt++;
				}
			}
			for (r = 0; r < nw[ia].out_deg; r++)	// outdegree citation distribution for a
			{
				for (j = 0; j < N_OUTDEGREE_ATTR; j++)
				{
					if (nw[ia].out_nbrs[r] == indx[j]) a[N_TKEYWORD_ATTR+N_INDEGREE_ATTR+j].cnt++;
				}
			}
			// distribution for b
			for (r = 0; r < wos[iib].ntkws; r++)	// titlle keyword distribution for b
			{
				ndx2tk = tkeyword[wos[iib].tkws[r]].ndx;
				if (ndx2tk == -1)	// not in the current keyword pool
					continue;
				ranking = tk[ndx2tk].ranking;
				if (ntk > N_TKEYWORD_ATTR) nattr = N_ATTRIBUTES; else nattr = ntk+N_INDEGREE_ATTR+N_OUTDEGREE_ATTR;
				if (ranking < nattr) b[ranking].cnt++;
			}
			for (r = 0; r < nw[ib].in_deg; r++)	// indegree citation distribution for b
			{
				for (j = 0; j < N_INDEGREE_ATTR; j++)
				{
					if (nw[ib].in_nbrs[r] == ondx[j]) b[N_TKEYWORD_ATTR+j].cnt++;
				}
			}
			for (r = 0; r < nw[ib].out_deg; r++)	// outdegree citation distribution for b
			{
				for (j = 0; j < N_OUTDEGREE_ATTR; j++)
				{
					if (nw[ib].out_nbrs[r] == indx[j]) b[N_TKEYWORD_ATTR+N_INDEGREE_ATTR+j].cnt++;
				}
			}
			// distribution for c
			for (r = 0; r < wos[iic].ntkws; r++)	// titlle keyword distribution for c
			{
				ndx2tk = tkeyword[wos[iic].tkws[r]].ndx;
				if (ndx2tk == -1)	// not in the current keyword pool
					continue;
				ranking = tk[ndx2tk].ranking;
				if (ntk > N_TKEYWORD_ATTR) nattr = N_ATTRIBUTES; else nattr = ntk+N_INDEGREE_ATTR+N_OUTDEGREE_ATTR;
				if (ranking < nattr) c[ranking].cnt++;
			}
			for (r = 0; r < nw[ic].in_deg; r++)		// indegree citation distribution for c
			{
				for (j = 0; j < N_INDEGREE_ATTR; j++)
				{
					if (nw[ic].in_nbrs[r] == ondx[j]) c[N_TKEYWORD_ATTR+j].cnt++;
				}
			}
			for (r = 0; r < nw[ic].out_deg; r++)	// outdegree citation distribution for c
			{
				for (j = 0; j < N_OUTDEGREE_ATTR; j++)
				{
					if (nw[ic].out_nbrs[r] == indx[j]) c[N_TKEYWORD_ATTR+N_INDEGREE_ATTR+j].cnt++;
				}
			}
//#ifdef DEBUG
			fwprintf(logstream, L"\na: [");
			for (j = 0; j < nattr; j++) fwprintf(logstream, L"%d ", a[j].cnt);
			fwprintf(logstream, L"]\nb: [");
			for (j = 0; j < nattr; j++) fwprintf(logstream, L"%d ", b[j].cnt);
			fwprintf(logstream, L"]\nc: [");
			for (j = 0; j < nattr; j++) fwprintf(logstream, L"%d ", c[j].cnt);
			fwprintf(logstream, L"]\n"); fflush(logstream);
//#endif DEBUG
			smoothing(nattr, 0.5, a);
			smoothing(nattr, 0.5, b);
			smoothing(nattr, 0.5, c);
#ifdef DEBUG
			for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", a[j].prob);
			fwprintf(logstream, L"\n"); fflush(logstream);
#endif DEBUG
			KLac  = Kullback_Leibler(nattr, c, a);
			KLab = Kullback_Leibler(nattr, b, a);
			KLbc = Kullback_Leibler(nattr, c, b);
			if (KLac-(KLab+KLbc) > 0)
			{
				fwprintf(logstream, L"[%s->%s->%s]  ", nw[ia].alias, nw[ib].alias, nw[ic].alias);
				fwprintf(logstream, L"    [%f, %f, %f] ==> %f\n", KLac, KLab, KLbc, KLac-(KLab+KLbc)); fflush(logstream);
			}
		}
	}

	Jfree(a, L"critical_transition: a");
	Jfree(b, L"critical_transition: b");
	Jfree(c, L"critical_transition: c");

	return 0;
}

//
// function to test expected_information()
//
int test_eI()
{
	int i, n;
	PDIST q[N_ATTRIBUTES];
	PDIST p[N_ATTRIBUTES];
	double KL, JS1, JS2;

	n = N_ATTRIBUTES;
	for (i = 0; i < n; i++)
	{
		q[i].prob = 1.0 / n;
		p[i].prob = 1.0 / n;
	}
	q[5].prob = 0.02; q[15].prob = 0.08; 
	q[2].prob = 0.0; q[8].prob = 0.10;

	KL = Kullback_Leibler(n, q, p);
	JS1 = Jensen_Shannon(n, q, p);
	JS2 = Jensen_Shannon(n, p, q);
	fwprintf(logstream, L"KL = %f, JS1 = %f, JS2 = %f\n", KL, JS1, JS2);

	return 0;
}
