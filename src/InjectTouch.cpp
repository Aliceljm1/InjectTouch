
#include "stdafx.h"
#include "InjectTouch.h"
#include <map>

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

#define		CHANGE_TO_ZOOM(str) ((str) == "zoomin" ? ZOOMIN : ((str) == "zoomout" ? ZOOMOUT : NORMAL))

int g_dragX = 100, g_dragY = 30;

void inject_touch(ContackList& contactList)
{
	if (contactList.empty()) return;
	BOOL bRet = TRUE;
	SetLastError(0); // 重置
	bRet = InjectTouchInput(static_cast<UINT32>(contactList.size()), &contactList[0]);
	if (!bRet)
	{
		dprintf("inject touch error=0x%x\n", GetLastError());
	}
}

void init_touch(ContackList& list)
{
	int i = 0;
	for (POINTER_TOUCH_INFO& contact : list)
	{
		memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));
		contact.pointerInfo.pointerType = PT_TOUCH; //we're sending touch input
		contact.pointerInfo.pointerId = i++;          //contact 0
		contact.touchFlags = TOUCH_FLAG_NONE;
		contact.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
		contact.orientation = 90;
		contact.pressure = 32000;
	}
}

void make_touch(POINTER_TOUCH_INFO& contact, const std::string& action, int x, int y)
{
	contact.pointerInfo.ptPixelLocation.x = x;
	contact.pointerInfo.ptPixelLocation.y = y;
	if (action == ACTION_DOWN)
		contact.pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
	else if (action == ACTION_UP)
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UP;
	else if (action == ACTION_HOVER)
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE;
	else
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;

	//设置触控面积
	contact.rcContact.top = x - 2;
	contact.rcContact.bottom = x + 2;
	contact.rcContact.left = y - 2;
	contact.rcContact.right = y + 2;
}

void handle_file(const std::string& file, int _touch_num)
{
	g_stroke.clear(); g_strokeGroup.strokeList.clear();
	std::ifstream infile(file.c_str());
	std::string line;
	Stroke temp;
	std::map<int, bool> ids;

	try
	{
		while (std::getline(infile, line))
		{
			int offset = 0, lastDelay = 0;
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
					int index = data.find_first_of(",");
					int x = atoi(data.substr(0, index).c_str());
					int y = atoi(data.substr(index + 1, data.length() - index - 1).c_str());
					Point p = { cmd, ptId, x, y };
					temp.push_back(p);
				}

			calc_offset:
				offset = end + 1; // 执行偏移
			}

			// 每一行事件统一睡眠，只睡一次，以最后一次为准
			g_strokeGroup.delayList.push_back(lastDelay);
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
	int size = g_strokeGroup.strokeList.size();
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


void MoveMouse(int x, int y) {
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	DWORD dx = x * (65535 / screenWidth);
	DWORD dy = y * (65535 / screenHeight);
	//打印dx,dy坐标
	OutputDebugStringA(std::to_string(dx).c_str());
	OutputDebugStringA(",");
	OutputDebugStringA(std::to_string(dy).c_str());
	OutputDebugStringA("\n");

	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, 0);
}

void MouseDown(int x, int y) {
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	DWORD dx = x * (65535 / screenWidth);
	DWORD dy = y * (65535 / screenHeight);
	mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, 0);
}

void MouseUp(int x, int y) {
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	DWORD dx = x * (65535 / screenWidth);
	DWORD dy = y * (65535 / screenHeight);
	mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, 0);
}


void run(const std::string& filepath, const int& _touch_num, const Zoom& zoom)
{
	InitializeTouchInjection(MAX_TOUCH_NUM, TOUCH_FEEDBACK_INDIRECT);
	ContackList contactList;

	for (int i = 0; i < MAX_TOUCH_NUM; i++)
	{
		POINTER_TOUCH_INFO c;
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
	int size = g_strokeGroup.strokeList.size();
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
		filepath = exePath.append("touchinfo.txt");
		break;
	default:
	case 4:
		zoom = CHANGE_TO_ZOOM(std::string(argv[3]));
	case 3:
		_touch_num = atoi(argv[2]);
	case 2:
		filepath = argv[1];
		break;
	}
}

#define text_drag		"drag.txt"
#define text_drawpoint	"drawpoint.txt"
#define text_touchinfo	"touchinfo.txt"
#define text_zoomout	"zoomout.txt"

std::filesystem::path ini_path = get_exe_dir() / L"ini.txt";
std::map<std::string, std::string> data;

Zoom zoom = NORMAL;
bool USE_SEND_INPUT = false;
int _touch_num = 3; // txt中存储的是3指，超过3只会画三个
int timeout = 3000;

void read_ini()
{
	read_map_file(ini_path, data);
	USE_SEND_INPUT = data["USE_SEND_INPUT"] == "1" || data["USE_SEND_INPUT"] == "true";
	zoom = CHANGE_TO_ZOOM(data["ZOOM"]);
	_touch_num = atoi(data["TOUCH_NUM"].data());
	timeout = atoi(data["TIMEOUT"].data());
}

int main(int argc, char* argv[])
{
	read_ini();

	std::string	filepath = text_drag;
	std::string	exePath = std::string(argv[0]);
	std::string	exeName = get_filename(exePath);

	// 处理命令行信息
	hand_cmd_info(argc, argv, _touch_num, filepath, zoom, exeName, exePath);

	//给切换窗口预留时间
	Sleep(timeout);

	filepath = text_touchinfo;
	if (USE_SEND_INPUT)
		run_send_input(filepath);
	else
		run(filepath, _touch_num, zoom);

	return 0;
}