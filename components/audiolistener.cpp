#include <AL/al.h>
#include "audiolistener.hpp"

using namespace std;

unsigned char ComponentFactory<AudioListener>::numListeners = 0;

AudioListener::AudioListener(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object) 
	: Component(components, transform, object)
{

}

void AudioListener::FixedUpdate() {
	auto pos = transform.position();
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}