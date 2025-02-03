#ifndef FRUIT_SLICE_H
#define FRUIT_SLICE_H
#include "../core/component.hpp"

class FruitSlice : public Component {
public:
	FruitSlice(std::unordered_map<std::type_index, std::unique_ptr<Component>>& components, Transform& transform, Object* object);
	void Update() override;
};

#endif