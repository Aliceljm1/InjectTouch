
#include "stdafx.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "utils.h"
#include "InjectTouch.h"

using namespace std;

struct Point
{
	int pointerId;
	int x;
	int y;
	int sleep;
};

typedef vector<Point> Stroke;
Stroke              g_stroke;
vector<Stroke>      g_strokeList;
typedef vector<POINTER_TOUCH_INFO> ContackList;
int g_dragX = 100;
static int TOUCH_NUM;

void InjectTouch(ContackList& contactList)
{
	BOOL bRet = TRUE;
	bRet = InjectTouchInput(static_cast<UINT32>(contactList.size()), &contactList[0]);
	if (!bRet)
	{
		dprintf("error=%d\n", GetLastError());
	}
}

void InitTouch(ContackList& list)
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


void MakeTouch(POINTER_TOUCH_INFO& contact, const string& action, int x, int y)
{
	contact.pointerInfo.ptPixelLocation.x = x;
	contact.pointerInfo.ptPixelLocation.y = y;
	if (action == "down")
		contact.pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
	else if (action == "up")
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UP;
	else if (action == "hover")
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE;
	else
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;

	//设置触控面积
	contact.rcContact.top = x - 2;
	contact.rcContact.bottom = x + 2;
	contact.rcContact.left = y - 2;
	contact.rcContact.right = y + 2;
}


void HandleFile(const string& file)
{
	g_stroke.clear(); g_strokeList.clear();
	ifstream infile(file.c_str());
	string line;
	Stroke temp; int lastDelay;
	try
	{
		while (std::getline(infile, line))
		{
			std::istringstream iss(line);
			string cmd, data;
			int pointerId;
			if (!(iss >> cmd >> data))
			{
				dprintf("read data error");
				break;
			} // error

			if (cmd == "LeftDown")
			{
				temp.clear();
			}
			else if (cmd == "LeftUp")
				g_strokeList.push_back(temp);
			else if (cmd == "MoveTo")
			{
				int index = data.find_first_of(",");
				int x = atoi(data.substr(0, index).c_str());
				int y = atoi(data.substr(index + 1, data.length() - index - 1).c_str());
				Point p = { pointerId, x, y, lastDelay };
				temp.push_back(p);
			}
			else if (cmd == "Delay")
			{
				lastDelay = atoi(data.c_str());
			}

		}

	}
	catch (const std::exception& e)
	{
		dprintf("error=%s", e.what());
	}
}

void doTouch(ContackList& list, Point& p, const string& action, const std::string& type, int idx)
{
	MakeTouch(list.at(0), action, p.x, p.y);
	if (type == "drag")
	{
		for (int i = 1; i < TOUCH_NUM; i++)
		{
			MakeTouch(list.at(i), action, p.x + g_dragX * i, p.y);
		}
	}
	else if (type == "zoomout")
	{
		for (int i = 1; i < TOUCH_NUM; i++)
		{
			MakeTouch(list.at(i), action, p.x + g_dragX * (idx + i), p.y);
		}
	}
	InjectTouch(list);
}

/**
*模拟触控消息，存在如下原则，down和up附近必须要一同坐标move
* add by ljm
*/
void InjectTouchEvent(ContackList& list, const string& type)
{
	for (auto& stroke : g_strokeList)
	{
		for (int i = 0; i < stroke.size(); i++)
		{
			Point p = stroke[i];
			if (i == 0)
			{
				doTouch(list, p, "down", type, i);
			}

			Sleep(p.sleep + 5);
			doTouch(list, p, "move", type, i);

			if (i == stroke.size() - 1)
			{
				doTouch(list, p, "up", type, i);
			}

		}
	}
}


int main(int argc, char* argv[])
{
	string filepath = "drag.txt";
	string exename = "InjectTouch.exe";
	string exePath(argv[0]);
	string type = "drag";// 双指手势，drag拖拉，zoomout放大,zoomin
	TOUCH_NUM = 2;

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
		TOUCH_NUM = atoi(argv[2]);
	}
	else if(argc == 4)
	{
		filepath = argv[1];
		TOUCH_NUM = atoi(argv[2]);
		type = argv[3];
	}

	Sleep(3000);//给切换窗口预留时间 
	InitializeTouchInjection(TOUCH_NUM, TOUCH_FEEDBACK_INDIRECT);
	ContackList contactList;
	if (type != "")
	{
		for (int i = 0; i < TOUCH_NUM; i++)
		{
			POINTER_TOUCH_INFO c;
			//POINTER_TOUCH_INFO *c = new POINTER_TOUCH_INFO;
			contactList.push_back(c);
		}
	}

	InitTouch(contactList);
	if (type != "")
	{
		for (int i = 0; i < TOUCH_NUM; i++)
			MakeTouch(contactList.at(i), "hover", 200 + g_dragX * i, 200);//经验代码，不加会失效
	}
	InjectTouch(contactList);

	HandleFile(filepath);
	InjectTouchEvent(contactList, type);
	return 0;
}