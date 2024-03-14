#include "utils.h"
#include <stdio.h>
#include <random>
#include <Tlhelp32.h>
#include <type_traits>
#include <fstream>

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

bool read_map_file(const std::wstring& path, std::map<std::string, std::string>& map)
{
	std::ifstream file(path);
	
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			std::istringstream is_line(line);
			std::string key;
			if (getline(is_line, key, '=')) {
				std::string value_str;
				if (getline(is_line, value_str)) {
					map[key] = value_str;
				}
			}
		}
		file.close();
	}
	else {
		return false;
	}
	return true;
}