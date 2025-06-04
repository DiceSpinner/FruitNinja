#include "audiosource_pool.hpp"
#include "infrastructure/object.hpp"
#include "infrastructure/object_pool.hpp"

using namespace std;
static std::shared_ptr<ObjectPool<Object>> pool;
ObjectManager* objManager;

static Object* createAudioSource() {
	auto obj = new Object();
	AudioSource* source = obj->AddComponent<AudioSource>();
	source->disableWhileNotPlaying = true;
	return obj;
}

void initializeAudioSourcePool(ObjectManager& manager, size_t size) {
	objManager = &manager;
	pool = std::make_shared<ObjectPool<Object>>(size, &createAudioSource);
}

shared_ptr<Object> acquireAudioSource() {
	auto ptr = pool->Acquire();
	if (!ptr) {
		return {};
	}
	shared_ptr<Object> result = move(ptr);
	objManager->Register(result);
	return result;
}