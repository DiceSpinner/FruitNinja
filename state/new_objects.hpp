#ifndef ACTIVE_OBJECTS_H
#define ACTIVE_OBJECTS_H
#include <memory>
#include <queue>
#include "../core/object.hpp"
namespace Game {
	extern std::queue<std::shared_ptr<Object>> newObjects;
}

#endif