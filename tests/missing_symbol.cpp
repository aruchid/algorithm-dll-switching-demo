#ifdef _WIN32
__declspec(dllexport)
#endif
int unrelated_symbol() { return 0; }
