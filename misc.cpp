//
// misc.cpp
//

//
// Revision History:
// 2013/11/09 Added to check ':', ';', '-', '\'', '?' for the condifiton of a separated word
//            Added a new argument "begin_at" to the function index_of()
// 2016/06/25 Added a new function index_of2(), this function does not do the separate word check
//

#include "stdafx.h"
#include <string.h>
#include <wchar.h>
#using <mscorlib.dll>

using namespace System;

int index_of(int begin_at, wchar_t *str, wchar_t *substr)
{
	int i;
	int at, len, sublen;
	wchar_t ch, chb, che;
	int spos;

	len = (int)wcslen(str);
	sublen = (int)wcslen(substr);
	// change string type
	String^ s = "";			 
	for (i = 0; i < len; i++) 
	{
		ch = towlower(str[i]);
		s = String::Concat(s, Convert::ToString(ch));	
	}

	String^ subs = "";			 
	for (i = 0; i < sublen; i++) 
	{
		ch = towlower(substr[i]);
		subs = String::Concat(subs, Convert::ToString(ch));	
	}

	spos = begin_at;
	while (true)
	{
		at = s->IndexOf(subs, spos);	// search the substring "subs" from the start position "spos"
		if (at == -1)	// substring not found
			return at;
		// make sure that what we found is a separated word
		if (at == 0) chb = ' '; else chb = str[at-1];
		che = str[at+sublen];
		if ((chb == ' ' || chb == ',' || chb == '.' || chb == '\t') && (che == ' ' || che == ',' || che == '.' || che == '\t' || che == ':' || che == ';' || che == '-' || che == '\'' || che == '?' || che == '\0'))
			return at;
		spos = at + 1;	// continue the search
		if (spos == len)
			return -1;
	}
}

//
// this function does not do the separate word check as in the function index_of()
//
int index_of2(int begin_at, wchar_t *str, wchar_t *substr)
{
	int i;
	int at, len, sublen;
	wchar_t ch;

	len = (int)wcslen(str);
	sublen = (int)wcslen(substr);
	// change string type
	String^ s = "";			 
	for (i = 0; i < len; i++) 
	{
		ch = towlower(str[i]);
		s = String::Concat(s, Convert::ToString(ch));	
	}

	String^ subs = "";			 
	for (i = 0; i < sublen; i++) 
	{
		ch = towlower(substr[i]);
		subs = String::Concat(subs, Convert::ToString(ch));	
	}

	at = s->IndexOf(subs, begin_at);	// search the substring "subs" from the start position "spos"

	return at;
}
