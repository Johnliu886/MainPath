//---------------------------------------------------------------------------

#ifndef checklicenseH
#define checklicenseH
#include <iostream>
using std::string;

bool CheckLicense(string strSerial);
string GetMachineCode();
string GetSerial(string strMachineCode);
//---------------------------------------------------------------------------
#endif
