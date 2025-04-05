#ifndef CONNECTION_STATE_H
#define CONNECTION_STATE_H
#include "server.hpp"

class ConnectionState : public ServerState {
public:
	ConnectionState(Server& server);

	void Init() override;
	void OnEnter() override;
	void OnExit() override;
	std::optional<std::type_index> Step() override;
};

#endif