#include "stdafx.h"
#include <string>
#include <filesystem>
#include <map>

#define CHECK_TEMP_STRING	std::is_same<StringT, std::string>::value
#define CHECK_TEMP_WSTRING	std::is_same<StringT, std::wstring>::value

void dprintf(const char* format, ...);

std::string get_filename(std::string path);

std::filesystem::path get_exe_path();

std::filesystem::path get_exe_dir();

template <typename StringT>
static StringT replace_all(StringT str, const StringT& target, const StringT& dest)
{
	static_assert(CHECK_TEMP_STRING || CHECK_TEMP_WSTRING,
		"Invalid type for replace_all, only std::string and std::wstring are supported");

	size_t start_pos = 0;
	while ((start_pos = str.find(target, start_pos)) != StringT::npos) {
		str.replace(start_pos, target.length(), dest);
		start_pos += dest.length();
	}
	return str;
}