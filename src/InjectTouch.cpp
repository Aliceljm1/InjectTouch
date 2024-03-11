
#include "stdafx.h"
#include "InjectTouch.h"
#include <map>

#define		MAX_TOUCH_NUM	10			// max touch num  default 10
#define		CMD_LEFT_DOWN	"LeftDown"
#define		CMD_MOVE_TO		"MoveTo"
#define		CMD_LEFT_UP		"LeftUp"
#define		CMD_DELAY		"Delay"

#define		ACTION_DOWN		"down"
#define		ACTION_MOVE		"move"
#define		ACTION_UP		"up"
#define		ACTION_HOVER	"hover"

int g_dragX = 100;

void inject_touch(ContackList& contactList)
{
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
				int start = line.find_first_of("{", offset),
					end = line.find_first_of("}", offset);
				if (start == -1 || end == -1) break;

				std::string item = line.substr(start + 1, end - 1);
				std::istringstream iss(item);

				std::string cmd, data;
				int ptId;
				if (!(iss >> cmd >> data >> ptId))
				{
					dprintf("read data error");
					break;
				}

				// 如果ids数量等于_touch_num, 且当前ptId不在ids中(新增当前id则会超过_touch_num)
				if (ids.size() >= _touch_num && !ids[ptId]) 
					goto calc_offset;
				ids[ptId] = true;

				if (cmd == CMD_DELAY)
				{
					lastDelay = atoi(data.c_str());
				}
				else if (cmd == CMD_LEFT_DOWN || cmd == CMD_MOVE_TO || cmd == CMD_LEFT_UP)
				{
					int index = data.find_first_of(",");
					int x = atoi(data.substr(0, index).c_str());
					int y = atoi(data.substr(index + 1, data.length() - index - 1).c_str());
					Point p = { cmd, ptId, x, y, lastDelay };
					temp.push_back(p);
				}

			calc_offset:
				offset = end + 1; // 执行偏移
			}

			g_strokeGroup.delayList.push_back(lastDelay);
			g_strokeGroup.strokeList.push_back(temp);
		}
	}
	catch (const std::exception& e)
	{
		dprintf("parse file error=%s", e.what());
	}
}

void do_touch(ContackList& list, Stroke& Stroke, std::string action, int idx)
{
	//if (type == "drag")
	//{
	//	for (auto &p : Stroke) 
	//	{
	//		MakeTouch(list.at(p.pointerId), action, p.x, p.y);
	//	}
	//}
	//else if (type == "zoomout")
	//{
	//	for (auto &p : Stroke) 
	//	{
	//		short flag = (p.pointerId & 1) ? 1 : -1;
	//		MakeTouch(list.at(p.pointerId), action, p.x + g_dragX * idx * flag, p.y + g_dragX * (idx));
	//	}
	//}

	for (auto& p : Stroke)
	{
		make_touch(list.at(p.pointerId), action, p.x, p.y);
	}

	inject_touch(list);
}

/**
 * 模拟触控消息，存在如下原则，down和up附近必须要一同坐标move
 * add by ljm
 */
void inject_touch_event(ContackList& list)
{
	int size = g_strokeGroup.strokeList.size();
	for (int i = 0; i < size; ++i)
	{
		Stroke strokeDown;
		Stroke strokeMove;
		Stroke strokeUp;

		for (Point p : g_strokeGroup.strokeList[i])
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

		do_touch(list, strokeDown, ACTION_DOWN, i);
		Sleep(g_strokeGroup.delayList[i] + 10);
		do_touch(list, strokeMove, ACTION_MOVE, i);
		do_touch(list, strokeUp, ACTION_UP, i);
	}
}

void run(std::string& filepath, int _touch_num)
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
	inject_touch_event(contactList);
}

void hand_cmd_info(int argc, char* argv[], int& _touch_num, std::string& filepath, std::string& exename, std::string& exePath)
{
	if (argc == 1)
	{
		exePath.resize(exePath.length() - exename.length());
		filepath = exePath.append("touchinfo.txt");
	}
	else if (argc == 2)
	{
		filepath = argv[1];
	}
	else if (argc == 3)
	{
		filepath = argv[1];
		_touch_num = atoi(argv[2]);
	}
}

#define text_drag		"drag.txt"
#define text_drawpoint	"drawpoint.txt"
#define text_touchinfo	"touchinfo.txt"
#define text_zoomout	"zoomout.txt"

int main(int argc, char* argv[])
{
	int _touch_num = 1; // txt中存储的是3指，超过3只会画三个
	std::string	filepath = text_drag;
	std::string	exePath = std::string(argv[0]);
	std::string	exeName = get_filename(exePath);

	// 处理命令行信息
	hand_cmd_info(argc, argv, _touch_num, filepath, exeName, exePath);

	//给切换窗口预留时间
	Sleep(3000);

	run(filepath, _touch_num);

	return 0;
}