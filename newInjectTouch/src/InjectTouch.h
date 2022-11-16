#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "utils.h"

using namespace std;

int g_dragX = 100;

struct Point
{
	string cmd;
	int pointerId;
	int x;
	int y;
	int sleep;
};

typedef vector<Point> Stroke;

struct StrokeGroupInfo
{
	vector<int> delayList;
	vector<Stroke> strokeList;
};

Stroke              g_stroke;
StrokeGroupInfo g_strokeGroup;

typedef vector<POINTER_TOUCH_INFO> ContackList;


void InjectTouch(ContackList& contactList);
void InitTouch(ContackList& list);
void MakeTouch(POINTER_TOUCH_INFO& contact, const string& action, int x, int y);
void HandleFile(const string& file);
void HandleCommandInfo(int argc, char* argv[], int& TOUCH_NUM, string& filepath, string& exename, string& exePath, string& type);
void doTouch(ContackList& list, Stroke& Stroke, string action, const std::string& type, int idx);
void InjectTouchEvent(ContackList& list, const string& type);
void run(int& TOUCH_NUM, string& filepath, string& exename, string& exePath, string& type);