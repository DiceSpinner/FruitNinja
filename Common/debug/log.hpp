#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H
#include <iostream>

namespace Debug {
	template<typename... Args>
	void Log(Args&&... args) {
#ifdef _DEBUG
		(std::cout << ... << args) << std::endl;
#endif
	}

	template<typename... Args>
	void LogError(Args&&... args) {
#ifdef _DEBUG
		(std::cerr << ... << args) << std::endl;
#endif
	}
}

#endif