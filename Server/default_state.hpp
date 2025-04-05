#ifndef DEFAULT_STATE_H
#define DEFAULT_STATE_H
#include "server.hpp"
class DefaultState : public ServerState {
public:
	DefaultState(Server& server);
	void Init() override;
	void OnEnter() override;
	std::optional<std::type_index> Step() override;
};

#endif