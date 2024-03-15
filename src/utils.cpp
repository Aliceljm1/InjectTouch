#include "utils.h"
#include <stdio.h>
#include <random>

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

std::filesystem::path get_exe_path()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	std::filesystem::path path_ = std::wstring(buffer);
	return path_;
}

std::filesystem::path get_exe_dir()
{
	return get_exe_path().parent_path();
}