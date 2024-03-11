#include "utils.h"
#include <stdio.h>

void dprintf(const char* format, ...)
{
	va_list vlArgs = NULL;

	va_start(vlArgs, format);
	size_t nLen = _vscprintf(format, vlArgs) + 1;
	char* strBuffer = new char[nLen];
	_vsnprintf_s(strBuffer, nLen, nLen, format, vlArgs);
	va_end(vlArgs);

	OutputDebugStringA(strBuffer);

	delete[] strBuffer;
}
