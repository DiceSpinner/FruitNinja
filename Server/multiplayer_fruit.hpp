#ifndef MULTIPLAYER_FRUIT_H
#define MULTIPLAYER_FRUIT_H
#include "multiplayer_game.hpp"

class Fruit : public Component {
private:
	void PlayVFX() const;
	PlayerContext& context;
public:
	int point;
	float radius;

	Fruit(
		std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object,
		float radius, int score, PlayerContext& context
	);
	void Update(const Clock& clock) override;
};

#endif