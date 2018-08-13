//
// ReadSerial.cpp
//

#include "stdafx.h"
#include "resource.h"
#include "checklicense.h"

#define SOFTWARE_KEY_IN_REGISTRY 
//#define SOFTWARE_KEY_IN_FILE

extern int read_protection_key_from_registry(wchar_t *, wchar_t *, char *);

#ifdef SOFTWARE_KEY_IN_REGISTRY
int ReadSerial(wchar_t *str1, wchar_t *str2, char *str3)
{
	int i;
	int ret;
	FILE *stream;
	wchar_t bfr1[BLOCK_SIZE];	// for name
	wchar_t bfr2[BLOCK_SIZE];	// for organization
	char bfr3[BLOCK_SIZE];	// for serial string

	ret = read_protection_key_from_registry(bfr1, bfr2, bfr3);
	if (ret != 0)
		return -1;

	// decrypt all characters in software key with XOR
	//for (i = 0; i < BLOCK_SIZE; i++)
	//	bfr3[i] = bfr3[i] ^ 0xff;

	for (i = 0; i < BLOCK_SIZE/sizeof(wchar_t); i++)
	{
		str1[i] = (wchar_t)bfr1[i];
		str2[i] = (wchar_t)bfr2[i];
	}

	for (i = 0; i < BLOCK_SIZE/sizeof(wchar_t); i++)
		str3[i] = (unsigned char)bfr3[i];

	return 0;
}
#endif SOFTWARE_KEY_IN_REGISTRY


#ifdef SOFTWARE_KEY_IN_FILE
int ReadSerial(wchar_t *str1, wchar_t *str2, char *str3)
{
	int i;
	int ret;
	FILE *stream;
	unsigned char bfr1[BLOCK_SIZE];	// for name
	unsigned char bfr2[BLOCK_SIZE];	// for organization
	unsigned char bfr3[BLOCK_SIZE];	// for serial string

	ret = _wfopen_s(&stream, L"SoftwareKey", L"rb");
	if (ret != 0)
		return -1;
	ret = fread(bfr1, sizeof(unsigned char), BLOCK_SIZE, stream);
	ret = fread(bfr2, sizeof(unsigned char), BLOCK_SIZE, stream);
	ret = fread(bfr3, sizeof(unsigned char), BLOCK_SIZE, stream);
	fclose(stream);

	// decrypt all characters with XOR
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		bfr1[i] = bfr1[i] ^ 0xff;
		bfr2[i] = bfr2[i] ^ 0xff;
		bfr3[i] = bfr3[i] ^ 0xff;
	}

	for (i = 0; i < BLOCK_SIZE/sizeof(wchar_t); i++)
	{
		str1[i] = (wchar_t)bfr1[i];
		str2[i] = (wchar_t)bfr2[i];
	}

	for (i = 0; i < BLOCK_SIZE/sizeof(wchar_t); i++)
		str3[i] = (char)bfr3[i];

	return 0;
}
#endif SOFTWARE_KEY_IN_FILE
