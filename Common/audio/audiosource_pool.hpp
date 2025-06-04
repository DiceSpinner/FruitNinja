#ifndef AUDIOSOURCE_POOL_H
#define AUDIOSOURCE_POOL_H
#include "audiosource.hpp"

constexpr size_t AUDIOSOURCE_POOL_SIZE = 50;

void initializeAudioSourcePool(ObjectManager& manager, size_t size = AUDIOSOURCE_POOL_SIZE);
std::shared_ptr<Object> acquireAudioSource();

#endif