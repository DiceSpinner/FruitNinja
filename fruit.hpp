#ifndef FRUIT_H
#define FRUIT_H
#include "core/object.hpp"

class Fruit : public Object {
public:
	Fruit(std::shared_ptr<Model>& model);
	void Update() override;
};

#endif