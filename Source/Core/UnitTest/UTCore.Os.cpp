#include "Common.h"
#include <iostream>
#include <sstream>

#if K3DPLATFORM_OS_WIN
#pragma comment(linker,"/subsystem:console")
#endif

using namespace std;
using namespace k3d;

void TestOs()
{
	K3D_ASSERT(Os::MakeDir(KT("./TestOs")));
	K3D_ASSERT(Os::Remove(KT("./TestOs")));
	while (1)
	{
		Os::Sleep(1000);
		int count = Os::GetCpuCoreNum();
		float * usage = Os::GetCpuUsage();
		stringstream str;
		str << "usage: ";
		for (int i = 0; i < count; i++)
		{
			str << usage[i] << ",";
		}
		cout << str.str() << endl;
	}
}


int main(int argc, char**argv)
{
	TestOs();
	return 0;
}