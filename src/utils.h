#include "stdafx.h"
#include <string>
#include <filesystem>
#include <map>

void dprintf(const char* format, ...);

std::string get_filename(std::string path);

std::filesystem::path get_exe_path();

std::filesystem::path get_exe_dir();

bool read_map_file(const std::wstring& path, std::map<std::string, std::string>& map);