#ifndef AUDIOLISTENER_H
#define AUDIOLISTENER_H
#include "../core/component.hpp"

class AudioListener : public Component {

};

template<>
class ComponentFactory<AudioListener> {
	static unsigned char numListeners;

	template<typename... Args>
	static std::unique_ptr<AudioListener> Construct(Args&&... args)
	{
		if (numListeners == 0) {
			numListeners++;
			return std::make_unique<AudioListener>(std::forward<Args>(args)...);
		}
		std::cout << "Cannot have more than 1 listener\n";
		return {};
	}
};

#endif