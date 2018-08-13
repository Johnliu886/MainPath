//
// parse_clan_spec.cpp
//

//
// Revision History:
// 2012/10/18 Modification  : added to handle a new type of clan code: country
// 2013/01/13 Added function: added codes to handle "WOS" and "Author" clan code (for WOS data)
// 2017/03/02 Modification  : added codes to support unicode characters (in fopen())
// 2017/03/02 Modification  : added codes to recognize "專利權人"
//

#include "stdafx.h"
#include <stdio.h>
#include <wchar.h>
#include "resource.h"
#include "network.h"

int parse_clan_line(wchar_t *, wchar_t *, wchar_t *, wchar_t *);

//
// parse the content of the given clan specification file
//
int parse_clan_spec(wchar_t *fname, struct CLAN_PARAMS *pm, int *nclan)
{
	FILE *cstream;
	int i;
	wchar_t line[LBUF_SIZE], buf1[LBUF_SIZE], buf2[LBUF_SIZE], buf3[LBUF_SIZE];

	if (_wfopen_s(&cstream, fname, L"rt, ccs=UTF-8") != 0)	// open clan specification file, changed to support unicode, 2017/03/02
		return MSG_CLANFILE_CANNOTOPEN;	// error

	if(fgetws(line, LBUF_SIZE, cstream) != NULL) // read the 1st line
	{
		 if (wcscmp(line, L"*** CLAN SPECIFICATION ***\n") != 0)
			return MSG_CLANFILE_FORMAT_ERROR;
	}

	i = 0;
	while (true)
	{		
		if(fgetws(line, LBUF_SIZE, cstream) == NULL) break;
		if (line[0] == '\n' || line[0] == '\r') continue;
		//swscanf_s(line, L"%s\t%s\t%s", buf1, LBUF_SIZE, buf2, LBUF_SIZE, buf3, LBUF_SIZE);
		parse_clan_line(line, buf1, buf2, buf3);
		if (wcscmp(buf1, L"IPC") == 0) pm[i].ctype = CTYPE_IPC; 
		else if (wcscmp(buf1, L"UPC") == 0) pm[i].ctype = CTYPE_UPC; 
		else if (wcscmp(buf1, L"Assignee") == 0) pm[i].ctype = CTYPE_ASSIGNEE; 
		else if (wcscmp(buf1, L"專利權人") == 0) pm[i].ctype = CTYPE_ASSIGNEE;	// added 2017/03/02
		else if (wcscmp(buf1, L"Country") == 0) pm[i].ctype = CTYPE_COUNTRY;	// added 2012/10/17
		else if (wcscmp(buf1, L"WOS") == 0) pm[i].ctype = PTYPE_WOS;			// added 2013/01/13
		else if (wcscmp(buf1, L"Author") == 0) pm[i].ctype = PTYPE_AUTHOR;		// added 2013/01/13
		else pm[i].ctype = 0;
		if (pm[i].ctype != 0)
		{
			pm[i].bratio = _wtof(buf2);
			wcscpy(pm[i].ccode, buf3);
			i++;	
		}
	}		

	fclose(cstream);	
	*nclan = i;

	return 0;
}

//
// parse a line in the clan specification file
//
int parse_clan_line(wchar_t *line, wchar_t *ctype, wchar_t *bratio, wchar_t *ccode)
{
	wchar_t ch, *sp, *tp;
	wchar_t tmps[LBUF_SIZE];
	int state;

	sp = line;
	// remove the leading spaces
	while (*sp == ' ') sp++;

	// start parsing
	tp = tmps;
	state = 0;
	while (*sp != '\0')
	{
		ch = *sp; 
		switch (state)
		{
		case 0:	// getting clan type
			if (ch == '\t' || ch == ' ')
			{
				*tp++ = '\0'; 
				wcscpy(ctype, tmps);
				state = 1; 
				sp++; tp = tmps;
			}			
			else 
				*tp++ = *sp++;
			break;
		case 1:	// getting boost ratio
			if (ch == '\t' || ch == ' ')
			{
				*tp++ = '\0'; 
				wcscpy(bratio, tmps);
				state = 2; 
				sp++; tp = tmps;
			}			
			else 
				*tp++ = *sp++;
			break;
		case 2:	// getting clan codes
			if (ch == '\n' || ch == '\r')
			{
				*tp = '\0'; 
				wcscpy(ccode, tmps);
				return 0;
			}
			else
				*tp++ = *sp++;
			break;
		default:
			break;
		}
	}
	*tp = '\0'; 
	wcscpy(ccode, tmps);

	return 0;
}
