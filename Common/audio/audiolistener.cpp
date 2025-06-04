#include <AL/al.h>
#include "audiolistener.hpp"

using namespace std;

unsigned char ComponentFactory<AudioListener>::numListeners = 0;

AudioListener::AudioListener(unordered_map<type_index, vector<unique_ptr<Component>>>& components, Transform& transform, Object* object)
	: Component(components, transform, object)
{

}

void AudioListener::FixedUpdate(Clock& clock) {
	auto pos = transform.position();
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}