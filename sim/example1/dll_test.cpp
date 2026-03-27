// Automatically generated C++ file on Fri Aug  1 15:26:47 2025
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD dll_test.cpp kernel32.lib


union uData
{
   bool b;
   char c;
   unsigned char uc;
   short s;
   unsigned short us;
   int i;
   unsigned int ui;
   float f;
   double d;
   long long int i64;
   unsigned long long int ui64;
   char *str;
   unsigned char *bytes;
};

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) { return 1; }

// #undef pin names lest they collide with names in any header file(s) you might include.
#undef out

extern "C" __declspec(dllexport) void dll_test(void **opaque, double t, union uData *data)
{
   double  Ro      = data[0].d; // input parameter
   double  Po      = data[1].d; // input parameter
   int     expType = data[2].i; // input parameter
   double &out     = data[3].d; // output

// Implement module evaluation code here:
   out = (float)(expType)+1.0;

}
