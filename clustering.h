//
// clusering.h
//

struct RESEARCH_GROUP
{
	int nscholars;	// number of scholars
	int *scholars;	// point to an arrary of authors
	int np;			// total number of papers
	int byear;		// beginning year of this group's publication
	int eyear;		// ending year of this grouop's publication
	double average_academic_age;	// average of the academic age of all the scholars in this group
	double total_paths;		// total number of paths for the group (normalize it become m-index)
	double mindex;			// mindex = (total paths for this group) / (overall total paths)
	double multiple_work_paths;	// the number of duplicate paths caused by having at least two works of a research group in a path
	int degcentrality;	// this is different from 'degree' in that it does not considers self-link
	int wdegcentrality;	// weighted degree centrality, this is also called 'strength'
	int weightedintralinks;	// weighted intra links
	double density;			// network density
	double centralization;	// network degree centralization, non-weighted and normalize with a star network
	double centralization2;	// network degree centralization, weighted and normalize with random networks with the same number of links
	int ncountries;
	int countries[MAX_COUNTRIES];	// country, index to the country table (extracted frm C1 field)
	int country_cnt[MAX_COUNTRIES];	// count of each country
	struct AUTHORS *athrs;
	struct ASSIGNEES *asgns;	// added 2017/03/25
	// following are the author linkage information of the research group network (shrinkied co-author network)
	int    degree;		// overall degree, number of immediate neighbors 
	int    *nbrs;		// the id of neighbors, index to this RESEARCH_GROUP list
	double *weight;		// weight of this particular link
	// following are the citation linkage information of the research group network (shrinkied author citation network)
	int    ctn_indegree;		// overall degree, number of immediate neighbors 
	int    *ctn_innbrs;		// the id of neighbors, index to this RESEARCH_GROUP list
	double *ctn_inweight;		// weight of this particular link
	int    ctn_outdegree;		// overall degree, number of immediate neighbors 
	int    *ctn_outnbrs;		// the id of neighbors, index to this RESEARCH_GROUP list
	double *ctn_outweight;		// weight of this particular link
};
