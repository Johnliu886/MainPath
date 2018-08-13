//
// config.h
//

//#define RESEARCH_MODALITY_PROJECT	// 科技部" 臺灣人文及社會科學研究樣態" 計畫
//#define UNIVERSITY_INDUSRY_PROJECT  // 何秀青 

//#define IGNORE_SELF_CITATION	// remove the self-citation link, not recommended

#define AUTHOR_TOTAL_SPX	// this definition is for the calculation of author m-index (main stream index)
#define ASSIGNEE_TOTAL_SPX	// this definition is for the calculation of assignee m-index (main stream index)
//#define PATENT_SPX_COUNTRY_IPC_SUMMARY // this definition creates data for regional innovation study, it will not go further to generate the main paths

// select one of the following to determine the product name
//#define DELTA_RELEASE		// for DELTA release
#define GENERAL_RELEASE		// general release

// select the type of the version (student or professional)
//#define PARTNER_VERSION			// no limitation at all
//#define PROFESSIONAL_VERSION	// need key to start
#define STUDENT_VERSION		// no key is needed, but has limited date
//#define LIGHT_VERSION			// no key is needed, but has limited date and cannot create citation file
//#define STUDENT_PRO_VERSION		// no key is needed, but has limited date limitation, allow clan analysis
//#define LAW_VERSION				// can read only Westlaw data
//#define PROFESSIONAL_TRIAL_VERSION

#ifdef DELTA_RELEASE
#define PROFESSIONAL_VERSION
//#define PROFESSIONAL_TRIAL_VERSION
#undef STUDENT_VERSION
#endif

// select one of the following two methods to calculate SPx
//#define OKAY_BUT_SLOW			// this is obsolete starting from 2011/04/02
#define USING_TOPOLOGICAL_SORT	// this is available starting from 2011/04/02

//#define CUT_OFF_YEAR_DATA	// cut off year data (in the function create_new_relationship_list()) when year window is specified

//#define DEA_APPLICATIONS 1	// enable code designed specifically for the need of the "DEA Application Survey" paper

// whether to include critical transition function
//#define CRITICAL_TRANSITION 1

#ifdef DELTA_RELEASE
#define RELEASE_ID 2
#else
#define RELEASE_ID 1
#endif

#ifdef STUDENT_PRO_VERSION
#define RELEASE_ID 2	// force RELEASE_ID to the same as Delta release
#endif

#ifdef STUDENT_VERSION
#define VERSION_ID 2
#endif

#ifdef STUDENT_PRO_VERSION
#define VERSION_ID 2
#endif

#ifdef PARTNER_VERSION
#define VERSION_ID 3
#endif

#ifdef PROFESSIONAL_VERSION
#define VERSION_ID 1
#endif

#ifdef PROFESSIONAL_TRIAL_VERSION
#define VERSION_ID 1
#endif

#ifdef LAW_VERSION
#define VERSION_ID 4
#endif

#ifdef LIGHT_VERSION
#define VERSION_ID 5
#endif
