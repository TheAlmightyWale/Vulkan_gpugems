#include "App.h"
#include "Logger.h"

int main() {
	App application("GpuGems");
	application.Start();

	while (!application.ShouldQuit())
	{
		application.Process();
	}

	SPDLOG_INFO("Exiting App");
}