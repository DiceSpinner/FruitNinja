#ifndef FRUIT_H
#define FRUIT_H
#include "object.hpp"

class Fruit : public Object {
public:
	Fruit(Model& model);
	void update() override;
};

#endif