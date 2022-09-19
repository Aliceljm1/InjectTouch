
#include "stdafx.h"
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

struct Point
{
	int x;
	int y;
	int sleep;
};

typedef vector<Point> Stroke;
Stroke              g_stroke;
vector<Stroke>      g_strokeList;

static inline void dprintf(const char* format, ...)
{
	va_list vlArgs = NULL;

	va_start(vlArgs, format);
	size_t nLen = _vscprintf(format, vlArgs) + 1;
	char* strBuffer = new char[nLen];
	_vsnprintf_s(strBuffer, nLen, nLen, format, vlArgs);
	va_end(vlArgs);

	OutputDebugStringA(strBuffer);

	delete[] strBuffer;
}

void InitTouch(POINTER_TOUCH_INFO& contact)
{
	memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));
	contact.pointerInfo.pointerType = PT_TOUCH; //we're sending touch input
	contact.pointerInfo.pointerId = 0;          //contact 0
	contact.touchFlags = TOUCH_FLAG_NONE;
	contact.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
	contact.orientation = 90;
	contact.pressure = 32000;
}


void MakeTouch(POINTER_TOUCH_INFO& contact, const string& action, int x, int y)
{

	BOOL bRet = TRUE;
	contact.pointerInfo.ptPixelLocation.x = x;
	contact.pointerInfo.ptPixelLocation.y = y;
	if (action == "down")
		contact.pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
	else if (action == "up")
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UP;
	else
		contact.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
	
	//设置触控面积
	contact.rcContact.top = x - 2;
	contact.rcContact.bottom = x + 2;
	contact.rcContact.left = y - 2;
	contact.rcContact.right = y + 2;

	bRet = InjectTouchInput(1, &contact);
	if (!bRet) {
		printf("error=%d", GetLastError());
	}
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
				int y = atoi(data.substr(index+1, data.length() - index-1).c_str());
				Point p={ x,y, lastDelay };
				temp.push_back(p);
			}
			else if (cmd == "Delay") {
				lastDelay = atoi(data.c_str());
			}

		}

	}
	catch (const std::exception& e)
	{
		dprintf("error=%s", e.what());
	}
}

/**
*模拟触控消息，存在如下原则，down和up附近必须要一同坐标move
* add by ljm
*/
void InjectTouchEvent(POINTER_TOUCH_INFO& contact)
{
	for (auto& stroke: g_strokeList) {
		for (int i = 0; i < stroke.size(); i++) {
			Point p = stroke[i];
			if (i == 0)
				MakeTouch(contact, "down", p.x, p.y);
			
			Sleep(p.sleep);
			MakeTouch(contact, "move", p.x, p.y);

			if(i==stroke.size()-1)
				MakeTouch(contact, "up", p.x, p.y);
		}
	}
}

int main(int argc, char* argv[])
{

	Sleep(3000);

	InitializeTouchInjection(10, TOUCH_FEEDBACK_INDIRECT);
	POINTER_TOUCH_INFO contact;
	InitTouch(contact);
	MakeTouch(contact, "down", 200, 200);
	MakeTouch(contact, "up", 200, 200);
	Sleep(100);

	string filepath(argv[1]);
	//filepath=filepath.substr(1, filepath.length() - 3);//去除前后空格
	HandleFile(filepath);
	InjectTouchEvent(contact);
	return 0;
}

