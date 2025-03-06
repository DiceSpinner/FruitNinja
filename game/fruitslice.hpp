#ifndef FRUIT_SLICE_H
#define FRUIT_SLICE_H
#include "fruit.hpp"
#include "../core/component.hpp"

class FruitSlice : public Component {
private:
	FruitChannel& channel;
	bool isUI;
public:
	FruitSlice(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object, FruitChannel& control, bool isUI);
	void Update() override;
};

#endif