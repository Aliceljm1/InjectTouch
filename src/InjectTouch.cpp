
#include "stdafx.h"
#include "InjectTouch.h"
#include "utils.h"
#include "mouse.hpp"

#include <filesystem>
#include <iostream>
#include <type_traits>
#include <fstream>
#include <sstream>

#define		MAX_TOUCH_NUM	10			// max touch num  default 10
#define		NONE_PT_ID		-1		

#define		CMD_LEFT_DOWN	"LeftDown"
#define		CMD_MOVE_TO		"MoveTo"
#define		CMD_LEFT_UP		"LeftUp"
#define		CMD_DELAY		"Delay"

#define		ACTION_DOWN		"down"
#define		ACTION_MOVE		"move"
#define		ACTION_UP		"up"
#define		ACTION_HOVER	"hover"

#define		FLAG_DOWN		(POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT)
#define		FLAG_UP			(POINTER_FLAG_UP)
#define		FLAG_HOVER		(POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE)
#define		FLAG_MOVE		(POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT)

#define		text_drag		"points_set\\drag.txt"
#define		text_drawpoint	"points_set\\drawpoint.txt"
#define		text_touchinfo	"points_set\\touchinfo.txt"
#define		text_zoomout	"points_set\\zoomout.txt"
#define		text_drawline	"points_set\\drawline.txt"

#define		CHANGE_TO_ZOOM(str) ((str) == "zoomin" ? ZOOMIN : ((str) == "zoomout" ? ZOOMOUT : NORMAL))

std::filesystem::path ini_path = L"ini\\ini.txt"; // 配置文件路径

HSYNTHETICPOINTERDEVICE hDevice = NULL;
Zoom zoom = NORMAL;
bool USE_SEND_INPUT = false;
int _touch_num = 3; // txt中存储的是3指，超过3只会画三个
int timeout = 3000;
int g_offset_x = 0, g_offset_y = 0;
int g_dragX = 100, g_dragY = 30;
bool clone_mode = false;
int g_delay = 0;

void inject_touch(ContackList& contactList)
{
	if (contactList.empty()) return;
	BOOL bRet = TRUE;
	SetLastError(0);
	bRet = InjectSyntheticPointerInput(hDevice, &contactList[0], static_cast<UINT32>(contactList.size()));
	if (!bRet)
	{
		dprintf("inject touch error=0x%x\n", GetLastError());
	}
}

void init_touch(ContackList& list)
{
	int i = 0;
	for (POINTER_TYPE_INFO& contact : list)
	{
		memset(&contact, 0, sizeof(POINTER_TYPE_INFO));
		contact.type = PT_TOUCH;
		contact.touchInfo.pointerInfo.pointerType = PT_TOUCH; //we're sending touch input
		contact.touchInfo.pointerInfo.pointerId = i++;
		contact.touchInfo.pointerInfo.dwTime = 0;
		contact.touchInfo.touchFlags = TOUCH_FLAG_NONE;
		contact.touchInfo.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
		contact.touchInfo.orientation = 90;
		contact.touchInfo.pressure = 32000;
	}
}

void make_touch(POINTER_TYPE_INFO& contact, const std::string& action, int x, int y)
{
	contact.touchInfo.pointerInfo.ptPixelLocation.x = x;
	contact.touchInfo.pointerInfo.ptPixelLocation.y = y;
	if (action == ACTION_DOWN)
		contact.touchInfo.pointerInfo.pointerFlags = FLAG_DOWN;
	else if (action == ACTION_UP)
		contact.touchInfo.pointerInfo.pointerFlags = FLAG_UP;
	else if (action == ACTION_HOVER)
		contact.touchInfo.pointerInfo.pointerFlags = FLAG_HOVER;
	else
		contact.touchInfo.pointerInfo.pointerFlags = FLAG_MOVE;

	//设置触控面积
	contact.touchInfo.rcContact.top = x - 2;
	contact.touchInfo.rcContact.bottom = x + 2;
	contact.touchInfo.rcContact.left = y - 2;
	contact.touchInfo.rcContact.right = y + 2;
}

void handle_file(const std::filesystem::path& file, int _touch_num)
{
	bool exist = std::filesystem::exists(file);
	if (!exist)
	{
		std::cout << "触控点文件不存在，请检查路径" << std::endl;
		return;
	}

	g_stroke.clear(); g_strokeGroup.strokeList.clear();
	std::ifstream infile(file);

	std::string line;
	Stroke temp;
	std::map<int, bool> ids;

	try
	{
		while (std::getline(infile, line))
		{
			std::string::size_type offset = 0;
			INT16 lastDelay = 0;
			temp.clear();
			while (TRUE)
			{
				std::string::size_type start = line.find_first_of("{", offset),
					end = line.find_first_of("}", offset);
				if (start == std::string::npos || end == std::string::npos) break;

				std::string item = line.substr(start + 1, end - 1);
				std::istringstream iss(item);

				std::string cmd, data;
				int ptId = NONE_PT_ID;
				iss >> cmd >> data;
				if (cmd != CMD_DELAY)
				{
					iss >> ptId; // 如果不是睡眠命令，则需要读取ID
					if (ptId == NONE_PT_ID)
					{
						dprintf("parse file error, pointer id is -1");
						return;
					}

					// 如果ids数量等于_touch_num, 且当前ptId不在ids中(新增当前id则会超过_touch_num)
					if (ids.size() >= _touch_num && !ids[ptId])
						goto calc_offset;
					ids[ptId] = true;
				}

				if (cmd == CMD_DELAY)
				{
					lastDelay = atoi(data.c_str());
				}
				else if (cmd == CMD_LEFT_DOWN || cmd == CMD_MOVE_TO || cmd == CMD_LEFT_UP)
				{
					std::string::size_type index = data.find_first_of(",");
					int x = atoi(data.substr(0, index).c_str());
					int y = atoi(data.substr(index + 1, data.length() - index - 1).c_str());
					Point p = { cmd, ptId, x + g_offset_x, y + g_offset_y };
					temp.push_back(p);
				}

			calc_offset:
				offset = end + 1; // 执行偏移
			}

			// 每一行事件统一睡眠，只睡一次，以最后一次为准
			g_strokeGroup.delayList.push_back(max(lastDelay + g_delay, 0));
			g_strokeGroup.strokeList.push_back(temp);
		}
	}
	catch (const std::exception& e)
	{
		dprintf("parse file error=%s", e.what());
	}
}

void do_touch(ContackList& list, Stroke& Stroke, std::string action, int idx, const Zoom& zoom)
{
	if (Stroke.empty()) return;
	switch (zoom)
	{
	default:
	case NORMAL:
		for (auto& p : Stroke)
		{
			make_touch(list.at(p._ptId), action, p._x, p._y);
		}
		break;
	case ZOOMOUT:
		for (auto& p : Stroke)
		{
			char flag = (p._ptId & 1) ? 1 : -1;
			make_touch(list.at(p._ptId), action, p._x + g_dragX * idx * flag, p._y + g_dragY * idx);
		}
		break;
	case ZOOMIN:
		break;
	}
	inject_touch(list);
}

/**
 * 模拟触控消息，存在如下原则，down和up附近必须要一同坐标move
 * add by ljm
 */
void inject_touch_event(ContackList& list, const Zoom& zoom)
{
	int size = static_cast<int>(g_strokeGroup.strokeList.size());
	for (int i = 0; i < size; ++i)
	{
		Stroke strokeDown;
		Stroke strokeMove;
		Stroke strokeUp;

		for (auto& p : g_strokeGroup.strokeList[i])
		{
			if (p.cmd == CMD_LEFT_DOWN)
			{
				strokeDown.push_back(p);
			}

			strokeMove.push_back(p);

			if (p.cmd == CMD_LEFT_UP)
			{
				strokeUp.push_back(p);
			}
		}

		do_touch(list, strokeDown, ACTION_DOWN, i, zoom);
		if (!strokeDown.empty()) Sleep(g_strokeGroup.delayList[i]);
		do_touch(list, strokeMove, ACTION_MOVE, i, zoom);
		if (!strokeMove.empty()) Sleep(g_strokeGroup.delayList[i]);
		do_touch(list, strokeUp, ACTION_UP, i, zoom);
	}
}


void run(const std::filesystem::path& filepath, int& _touch_num, Zoom& zoom)
{
	hDevice = CreateSyntheticPointerDevice(PT_TOUCH, MAX_TOUCH_NUM, POINTER_FEEDBACK_DEFAULT);
	_touch_num = min(_touch_num, MAX_TOUCH_NUM);

	ContackList contactList;
	for (int i = 0; i < MAX_TOUCH_NUM; i++)
	{
		POINTER_TYPE_INFO c;
		contactList.push_back(c);
	}

	init_touch(contactList);

	for (int i = 0; i < MAX_TOUCH_NUM; i++)
		make_touch(contactList.at(i), ACTION_HOVER, 0, 0);//经验代码，不加会失效

	inject_touch(contactList);
	handle_file(filepath, _touch_num);
	inject_touch_event(contactList, zoom);
}

void run_send_input(const std::string& filepath)
{
	handle_file(filepath, 1);
	int size = static_cast<int>(g_strokeGroup.strokeList.size());
	for (int i = 0; i < size; ++i)
	{
		for (auto& p : g_strokeGroup.strokeList[i])
		{
			MoveMouse(p._x, p._y);
			if (p.cmd == CMD_LEFT_DOWN)
				MouseDown(p._x, p._y);
			else if (p.cmd == CMD_LEFT_UP)
				MouseUp(p._x, p._y);
			Sleep(g_strokeGroup.delayList[i]);
		}
	}
}

void hand_cmd_info(int argc, char* argv[], int& _touch_num, std::string& filepath, Zoom& zoom,
	std::string& exename, std::string& exePath)
{
	switch (argc)
	{
	case 1:
		exePath.resize(exePath.length() - exename.length());
		break;
	default:
	case 4:
		zoom = CHANGE_TO_ZOOM(std::string(argv[3]));
		[[fallthrough]];
	case 3:
		_touch_num = atoi(argv[2]);
		[[fallthrough]];
	case 2:
		filepath = argv[1];
		break;
	}
}

int main(int argc, char* argv[])
{
	read_ini();

	std::string	filepath = text_touchinfo;
	std::string	exePath = std::string(argv[0]);
	std::string	exeName = get_filename(exePath);

	// 处理命令行信息
	hand_cmd_info(argc, argv, _touch_num, filepath, zoom, exeName, exePath);

	//给切换窗口预留时间
	Sleep(timeout);

	if (USE_SEND_INPUT)
		run_send_input(filepath);
	else
		run(filepath, _touch_num, zoom);

	return 0;
}

bool read_map_file(const std::filesystem::path& _path, std::map<std::string, std::string>& map)
{
	bool exist = std::filesystem::exists(_path);
	if (!exist)
	{
		std::cout << "配置文件不存在，停止读取配置" << std::endl;
		return true;
	}

	std::ifstream file(_path);
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			if (line.empty()) continue;
			size_t pos = line.find('#');
			if (pos != std::string::npos) line.erase(pos);
			line = replace_all<std::string>(line, " ", "");
			line = replace_all<std::string>(line, "\t", "");
			std::istringstream is_line(line);
			std::string key;
			if (std::getline(is_line, key, '=')) {
				if (key.empty()) continue;
				std::string value;
				if (std::getline(is_line, value)) {
					if (value.empty()) continue;
					map[key] = value;
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

void read_ini()
{
	std::map<std::string, std::string> data;
	read_map_file(ini_path, data);
	USE_SEND_INPUT = data["USE_SEND_INPUT"] == "1" || data["USE_SEND_INPUT"] == "true";
	zoom = CHANGE_TO_ZOOM(data["ZOOM"]);
	_touch_num = atoi(data["TOUCH_NUM"].data());
	timeout = atoi(data["TIMEOUT"].data());
	g_offset_x = atoi(data["OFFSET_X"].data());
	g_offset_y = atoi(data["OFFSET_Y"].data());
	g_delay = atoi(data["DELAY"].data());
}