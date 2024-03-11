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

std::string get_filename(std::string path)
{
	std::size_t position = path.find_last_of("/\\");
	std::string filename = path.substr(position + 1);
	return filename;
}