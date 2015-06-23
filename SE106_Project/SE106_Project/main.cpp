#include "stdafx.h"
#include "application.h"

int _tmain(int argc, _TCHAR* argv[])
{
	Application *app = Application::getInstance();
	int ret = app->exec();
	//delete app;
	//退出时操作系统会回收堆内存，无需delete也不会泄漏内存
	return ret;
}