#ifndef ACTIVE_OBJECTS_H
#define ACTIVE_OBJECTS_H
#include <memory>
#include <queue>
#include "../core/object.hpp"
namespace GameState {
	extern std::queue<std::shared_ptr<Object>> newObjects;
}

#endif