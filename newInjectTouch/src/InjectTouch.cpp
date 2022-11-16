
#include "stdafx.h"
#include "InjectTouch.h"

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
	g_stroke.clear(); g_strokeGroup.strokeList.clear();
	ifstream infile(file.c_str());
	string line;
	Stroke temp;
	try
	{
		while (std::getline(infile, line))
		{
			int offset = 0, lastDelay = 0;;
			temp.clear();
			while(TRUE) 
			{
				int start = line.find_first_of("{", offset), end = line.find_first_of("}", offset);
				if (start == -1 || end == -1) break;

				string item = line.substr(start + 1, end - 1);
				std::istringstream iss(item);

				string cmd, data;
				int pointerId;
				if (!(iss >> cmd >> data >> pointerId))
				{
					dprintf("read data error");
					break;
				} // error
				if (cmd == "Delay")
				{
					lastDelay = atoi(data.c_str());
				}
				else if(cmd == "LeftDown" || cmd == "MoveTo" || cmd == "LeftUp")
				{
					int index = data.find_first_of(",");
					int x = atoi(data.substr(0, index).c_str());
					int y = atoi(data.substr(index + 1, data.length() - index - 1).c_str());
					Point p = { cmd, pointerId, x, y, lastDelay };
					temp.push_back(p);
				}
				offset = end + 1;
			}
			g_strokeGroup.delayList.push_back(lastDelay);
			g_strokeGroup.strokeList.push_back(temp);
		}
	}
	catch (const std::exception& e)
	{
		dprintf("error=%s", e.what());
	}
}

void doTouch(ContackList& list, Stroke& Stroke, string action, const std::string& type, int idx)
{
	if (type == "drag")
	{
		for (auto &p : Stroke) 
		{
			MakeTouch(list.at(p.pointerId), action, p.x, p.y);
		}
	}
	else if (type == "zoomout")
	{
		for (auto &p : Stroke) 
		{
			short flag = (p.pointerId & 1) ? 1 : -1;
			MakeTouch(list.at(p.pointerId), action, p.x + g_dragX * idx * flag, p.y + g_dragX * (idx));
		}
	}
	InjectTouch(list);
}

/**
 * 模拟触控消息，存在如下原则，down和up附近必须要一同坐标move
 * add by ljm
 */
void InjectTouchEvent(ContackList& list, const string& type)
{
	int size = g_strokeGroup.strokeList.size();
	for (int i = 0; i < size; ++i)
	{
		Stroke strokeDown;
		Stroke strokeMove;
		Stroke strokeUp;

		for (Point p : g_strokeGroup.strokeList[i])
		{
			if(p.cmd == "LeftDown")
			{
				strokeDown.push_back(p);
			}

			strokeMove.push_back(p);

			if(p.cmd == "LeftUp")
			{
				strokeUp.push_back(p);
			}
		}

		doTouch(list, strokeDown, "down", type, i);
		Sleep(g_strokeGroup.delayList[i] + 10);
		doTouch(list, strokeMove, "move", type, i);
		doTouch(list, strokeUp, "up", type, i);
	}
}

void HandleCommandInfo(int argc, char* argv[], int& TOUCH_NUM, string& filepath, string& exename, string& exePath, string& type)
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
		TOUCH_NUM = atoi(argv[2]);
	}
	else if (argc == 4)
	{
		filepath = argv[1];
		TOUCH_NUM = atoi(argv[2]);
		type = argv[3];
	}
}

void run(int& TOUCH_NUM, string& filepath, string& exename, string& exePath, string& type)
{
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
			MakeTouch(contactList.at(i), "hover", 0, 0);//经验代码，不加会失效
	}
	InjectTouch(contactList);

	HandleFile(filepath);
	InjectTouchEvent(contactList, type);
}

int main(int argc, char* argv[])
{
	// max touch num  default 10
	int TOUCH_NUM = 10; // 文本中存储的数量为5，TOUCH_NUM >= 5
	string filepath = "drag.txt";
	string exename = "InjectTouch.exe";
	string exePath(argv[0]);
	string type = "drag";// 双指手势，drag拖拉，zoomout放大,zoomin

	// 处理命令行信息
	HandleCommandInfo(argc, argv, TOUCH_NUM, filepath, exename, exePath, type);

	run(TOUCH_NUM, filepath, exename, exePath, type);
	
	return 0;
}