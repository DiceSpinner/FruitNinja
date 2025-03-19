#include "audiosource_pool.hpp"
#include "infrastructure/object.hpp"
#include "infrastructure/object_pool.hpp"

using namespace std;
static ObjectPool<Object>* pool;

static Object* createAudioSource() {
	Object* obj = new Object();
	AudioSource* source = obj->AddComponent<AudioSource>();
	source->disableWhileNotPlaying = true;
	return obj;
}

void initializeAudioSourcePool(size_t size) {
	pool = new ObjectPool<Object>(size, createAudioSource);
}

shared_ptr<Object> acquireAudioSource() {
	auto ptr = pool->Acquire();
	if (!ptr) {
		return {};
	}
	shared_ptr<Object> result = move(ptr);
	result->SetEnable(true);
	return result;
}