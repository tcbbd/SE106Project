#include "stdafx.h"
#include "application.h"

int _tmain(int argc, _TCHAR* argv[])
{
	Application *app = Application::getInstance();
	int ret = app->exec();
	//delete app;
	//�˳�ʱ����ϵͳ����ն��ڴ棬����deleteҲ����й©�ڴ�
	return ret;
}