#ifndef SERVER_STATE_H
#define SERVER_STATE_H
#include <iostream>
#include <unordered_map>
#include <optional>
#include <typeindex>
#include <memory>
#include <concepts>
#include "networking/socket.hpp"
#include "infrastructure/coroutine.hpp"

constexpr auto SERVER_SOCKET_CAPACITY = 30;

class ServerState;
class Server;

template<typename T>
concept TState = std::derived_from<T, ServerState>;

class ServerState {
private:
	std::unordered_map<std::type_index, std::unique_ptr<ServerState>> states = {};
	ServerState* currSubState = {};
	CoroutineManager coroutineManager = {};
	std::optional<std::type_index> transition = {};
	bool terminated = false;
	ServerState* enterSubState = {};
	void Terminate();
protected:
	Server& server;

	template<TState T>
	std::optional<std::type_index> EnterSubState() {
		if (terminated || currSubState) {
			std::cout << "Cannot enter sub state while terminated/has active sub state" << std::endl;
			return {};
		}
		auto type = std::type_index(typeid(T));
		auto state = states.find(type);
		if (state == states.end()) {
			std::cout << "Cannot find substate" << std::endl;
			return {};
		}
		enterSubState = state->second.get();
		OnEnterSubState();
		return Self();
	}
public:
	ServerState(Server& server);

	template<TState T, typename... Args>
	T* AddSubState(Args&&... args) {
		auto type = std::type_index(typeid(T));
		auto state = states.find(type);
		if (state != states.end()) {
			return {};
		}
		auto& inserted = states.emplace(type, std::make_unique<T>(server, std::forward<Args>(args)...)).first->second;
		inserted->Init();

		return static_cast<T*>(inserted.get());
	}

	std::type_index Self() const;
	std::optional<std::type_index> Run();
	void StartCoroutine(Coroutine&& coroutine);
	virtual void Init();
	virtual void ProcessInput();
	virtual std::optional<std::type_index> Step();
	virtual void BroadCastState();
	virtual void OnEnterSubState();
	virtual void OnExitSubState();
	virtual void OnEnter();
	virtual void OnExit();
};

class Server {
	friend class ServerState;
private:
	std::unique_ptr<ServerState> state = {};
	struct {
		UDPSocket socket = { SERVER_SOCKET_CAPACITY   };
		bool playerOneConnected = false;
		bool playerTwoConnected = false;
	} network;
	
public:
	Server();
	void CleanUp();

	template<TState T>
	void SetInitialState() {
		if (state) return;
		auto type = std::type_index(typeid(T));
		if (std::type_index(typeid(state.get())) == type) return;

		state = std::make_unique<T>(*this);
		state->Init();
		state->OnEnter();
	}

	void ProcessInput();
	void Step();
	void BroadCastState();
};

#endif