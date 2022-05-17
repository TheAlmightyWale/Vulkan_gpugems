#include "App.h"

int main() {
	App application("GpuGems");
	application.Start();

	while (!application.ShouldQuit())
	{
		application.Process();
	}
}