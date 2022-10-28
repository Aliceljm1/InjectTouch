
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
	int x;
	int y;
	int sleep;
};

typedef vector<Point> Stroke;
Stroke              g_stroke;
vector<Stroke>      g_strokeList;
typedef vector<POINTER_TOUCH_INFO> ContackList;
int g_dragX = 100;


void InjectTouch(ContackList& contactList)
{
	BOOL bRet = TRUE;
	bRet =InjectTouchInput(static_cast<UINT32>(contactList.size()), &contactList[0]);
	if (!bRet) {
		dprintf("error=%d\n", GetLastError());
	}
}

void InitTouch(ContackList& list)
{
	int i = 0;
	for (POINTER_TOUCH_INFO & contact : list)
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
	
	//���ô������
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

void doTouch(ContackList& list, Point& p,const string&action, const std::string& type, int i)
{
	MakeTouch(list.at(0), action, p.x, p.y);
	if (type == "drag") {
		MakeTouch(list.at(1), action, p.x + g_dragX, p.y);
	}
	else if (type == "zoomout") {
		MakeTouch(list.at(1), action, p.x + g_dragX * (i + 1), p.y);
	}
	InjectTouch(list);
}

/**
*ģ�ⴥ����Ϣ����������ԭ��down��up��������Ҫһͬ����move
* add by ljm
*/
void InjectTouchEvent(ContackList& list,const string& type)
{
	for (auto& stroke: g_strokeList) {
		for (int i = 0; i < stroke.size(); i++) {
			Point p = stroke[i];
			if (i == 0) {
				doTouch(list, p, "down", type, i);
			}
			
			Sleep(p.sleep);
			doTouch(list, p, "move", type, i);

			if (i == stroke.size() - 1) {
				doTouch(list, p, "up", type, i);
			}

		}
	}
}


int main(int argc, char* argv[])
{

	Sleep(3000);//���л�����Ԥ��ʱ�� 
	static int TOUCH_NUM = 2;
	string type = "zoomout";//˫ָ���ƣ�drag������zoomout�Ŵ�,zoomin

	InitializeTouchInjection(TOUCH_NUM, TOUCH_FEEDBACK_INDIRECT);
	POINTER_TOUCH_INFO c1, c2;
	ContackList contactList; contactList.push_back(c1); 
	if(type!="") contactList.push_back(c2);
		
	InitTouch(contactList);

	MakeTouch(contactList.at(0), "hover", 200, 200);if(type != "")  
	if (type != "")MakeTouch(contactList.at(1), "hover", 200+ g_dragX, 200);//������룬���ӻ�ʧЧ
	InjectTouch(contactList);


	Sleep(100);
	string filepath = "";
	string exename = "InjectTouch.exe";
	string exePath(argv[0]);
	if (argc == 1) {
		exePath.resize(exePath.length() - exename.length());
		filepath = exePath.append("touchinfo.txt");
	}
	else if (argc == 2)
		filepath = argv[1];

	HandleFile(filepath);
	InjectTouchEvent(contactList,type);
	
	return 0;
}


