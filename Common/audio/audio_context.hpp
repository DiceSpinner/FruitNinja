#ifndef AUDIO_CONTEXT_H
#define AUDIO_CONTEXT_H
#include <al/al.h>
#include "infrastructure/object.hpp"

namespace Audio {
	void initContext(ObjectManager& manager);
	void destroyContext();
}
#endif