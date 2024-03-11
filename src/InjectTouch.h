#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "utils.h"

struct Point
{
	std::string cmd;
	int pointerId;
	int x;
	int y;
	int sleep;
};

typedef std::vector<Point> Stroke;

struct StrokeGroupInfo
{
	std::vector<int> delayList;
	std::vector<Stroke> strokeList;
};

Stroke              g_stroke;
StrokeGroupInfo		g_strokeGroup;

typedef std::vector<POINTER_TOUCH_INFO> ContackList;


void inject_touch(ContackList& contactList);
void init_touch(ContackList& list);
void make_touch(POINTER_TOUCH_INFO& contact, const std::string& action, int x, int y);
void handle_file(const std::string& file, int _touch_num);
void hand_cmd_info(int argc, char* argv[], int& _touch_num, std::string& filepath, std::string& exename, std::string& exePath);
void do_touch(ContackList& list, Stroke& Stroke, std::string action, int idx);
void inject_touch_event(ContackList& list);
void run(std::string& filepath, int _touch_num);