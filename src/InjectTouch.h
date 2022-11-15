#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "utils.h"

using namespace std;

int g_dragX = 100;

struct Point
{
	int pointerId;
	int x;
	int y;
	int sleep;

	bool operator==(const Point & p) const 
	{
		return x == p.x && y == p.y && pointerId == p.pointerId;
	}
};

struct hash_point 
{
	size_t operator()(const Point & p) const 
	{
		return hash<int>()(p.x) ^ hash<int>()(p.y) ^ hash<int>()(p.pointerId);
	}
};

typedef vector<Point> Stroke;
typedef vector<Stroke> StrokeGroup;
unordered_map<Point, bool, hash_point> sets;

struct StrokeGroupInfo
{
	int groupCount; // ×éÊý
	vector<StrokeGroup> strokeList;
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