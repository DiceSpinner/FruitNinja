#ifndef FRUIT_H
#define FRUIT_H
#include "../core/component.hpp"
#include "../rendering/model.hpp"

class Fruit : public Component {
private:
	bool CursorInContact();
	std::shared_ptr<Model> slice1;
	std::shared_ptr<Model> slice2;
public:
	int reward;
	float radius;

	Fruit(std::unordered_map<std::type_index, std::unique_ptr<Component>>& collection, Transform& transform, Object* object, 
		float radius, int score, std::shared_ptr<Model> slice1, std::shared_ptr<Model> slice2);
	void Update() override;
};

#endif