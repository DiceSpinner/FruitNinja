#ifndef AUDIOLISTENER_H
#define AUDIOLISTENER_H
#include <iostream>
#include "../core/component.hpp"

class AudioListener : public Component {
public:
	AudioListener(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object);
	void FixedUpdate() override;
};

template<>
class ComponentFactory<AudioListener> {
	static unsigned char numListeners;

public:
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