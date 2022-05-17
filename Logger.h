#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/ranges.h>

uint32_t const k_mb_in_bytes = 1048576;

class Logger {
public:
	static void InitLogger() {
		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/logfile.txt", 10 * k_mb_in_bytes, 10));
		
		sinks.at(0)->set_level(spdlog::level::debug);
		sinks.at(1)->set_level(spdlog::level::trace);
		
		auto pConsoleAndFileLogger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
		spdlog::register_logger(pConsoleAndFileLogger);
		spdlog::set_default_logger(pConsoleAndFileLogger);

		SPDLOG_INFO("Logger Initialized");
	};

};