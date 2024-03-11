#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include "utils.h"

enum Zoom
{
	NORMAL,
	ZOOMIN,
	ZOOMOUT
};

struct Point
{
	std::string cmd;
	int _ptId;
	int _x;
	int _y;
	//int sleep;
};

typedef std::vector<Point>				Stroke;
typedef std::vector<POINTER_TOUCH_INFO> ContackList;

struct StrokeGroupInfo
{
	std::vector<int> delayList;
	std::vector<Stroke> strokeList;
};

Stroke									g_stroke;
StrokeGroupInfo							g_strokeGroup;

void inject_touch(ContackList& contactList);
void init_touch(ContackList& list);
void make_touch(POINTER_TOUCH_INFO& contact, const std::string& action, int x, int y);
void handle_file(const std::string& file, int _touch_num);
void hand_cmd_info(int argc, char* argv[], int& _touch_num, std::string& filepath, Zoom &zoom, 
	std::string& exename, std::string& exePath);
void do_touch(ContackList& list, Stroke& Stroke, std::string action, int idx, const Zoom& zoom);
void inject_touch_event(ContackList& list, const Zoom& zoom);
void run(const std::string& filepath, const int &_touch_num, const Zoom& zoom);