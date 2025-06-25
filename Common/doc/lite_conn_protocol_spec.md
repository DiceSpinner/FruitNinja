## LiteConn Protocol Specification

### Purpose and Motivation

LiteConn is a lightweight, connection-oriented protocol built on top of UDP. It was designed to support low-latency peer-to-peer communication, particularly for real-time multiplayer games and simulations. Game state updates and user interactions often require selective reliability — some packets must be delivered reliably (e.g. state synchronization, game events), while others (such as frequent position updates) can be dropped without consequence.

TCP, with its strict in-order delivery and stream-based model, introduces head-of-line blocking and latency that are unsuitable for most game data. UDP, while fast and lightweight, lacks native support for reliability, ordering, or session management.

This protocol addresses those gaps by implementing:

- Connection handshakes and teardown
- Reliable message delivery (with retransmissions)
- Asynchronous request-response messaging
- Heartbeat-based timeout detection
- Per-packet metadata including sequencing and session IDs

Compared to TCP, LiteConn gives developers fine-grained control over which messages require reliability, while retaining the performance benefits of UDP. LiteConn is a datagram-oriented protocol — it preserves message boundaries and does not guarantee in-order delivery or continuous streams like TCP. It is implemented using the Winsock API, and therefore currently only works on Windows systems.

### 1. Usage Guide



This protocol exposes four main abstractions for use in applications:

- **LiteConnManager**: Maintains a single UDP socket and manages multiple simultaneous logical connections. It accepts or initiates connections and handles background routing and timeouts.
- **LiteConnConnection**: Represents a single peer-to-peer connection. It provides APIs for sending and receiving messages, both reliable and unreliable. It also supports connection lifecycle operations including state querying, teardown, and timeout detection.
- **LiteConnResponse**: Returned when you send a request. Lets you wait for or cancel the response.
- **LiteConnRequest**: Returned when you receive a request. Lets you respond, reject, or start a follow-up conversation.
- **LiteConnMessage**: Returned when you receive a message. Contains data as well as an optional handle to **LiteConnRequest** if the message is a request.

This section explains how to use the `LiteConnManager` and `LiteConnConnection` interfaces for establishing and managing peer-to-peer UDP communication.

#### 1.1 Establishing a Connection

The `LiteConnManager` constructor accepts several arguments that define its behavior:

- **Port number** (`USHORT`): The local port to bind. If omitted, a random ephemeral port is chosen.
- **Connection limit** (`size_t`): The maximum number of simultaneous connections that can be maintained.
- **Packet queue capacity** (`size_t`): Maximum number of packets that each connection may buffer before new packets are dropped.
- **Maximum packet size** (`DWORD`): Upper limit for the size of incoming UDP packets.
- **Update interval** (`std::chrono::duration`): Frequency at which the background thread wakes to process incoming packets and update connection state.

The background thread continuously polls the OS message queue using `select()`, dispatches incoming packets to their associated connections (using a unique identifier called `sessionID`, described later), and manages retransmissions and timeouts. The following steps describe how to establish connections between peers using this system.

In a typical client-server architecture, the client must know the server's public IP address. This IP is assigned by the server's internet service provider. However, due to network address translation (NAT), the public-facing port may differ from the internal port specified in the server's code. In such cases, the router administrator must configure port forwarding to route traffic from the public port to the internal port used by the server. Without port forwarding, incoming connections from external clients will not reach the correct destination inside the local network.

- Each peer must create a `LiteConnManager` instance to initialize its UDP communication layer. This manager owns the UDP socket and provides the interface for accepting or initiating connections.
- On the server side, call `Accept()` to wait for incoming connections. This will return a `std::shared_ptr<LiteConnConnection>` when a peer initiates a connection. Note that the `isListening` flag on the server's `LiteConnManager` must be set to `true`; otherwise, connection requests will be ignored. The returned `LiteConnConnection` enters a handshake phase and is not immediately established — you may call `WaitForConnectionComplete()` to block until the connection status is resolved. This is optional; you can also poll using `IsConnected()` or `IsDisconnected()`.
- On the client side, use `ConnectPeer()` with the target address to initiate a connection. This returns a `std::shared_ptr<LiteConnManager>`. Like the server side, the connection is not immediately established and must complete a handshake. You may call `WaitForConnectionComplete()` or check status manually using `IsConnected()` or `IsDisconnected()`.

```cpp
// Server
LiteConnManager server(30000, 4, 64, 1400, 50ms);
server.isListening = true;
auto serverConn = server.Accept(timeout);
if (serverConn && serverConn->WaitForConnectionComplete()) {
    // serverConn is ready
}

// Client
LiteConnManager client(64, 4, 1400, 50ms);
auto clientConn = client.ConnectPeer(serverAddr, timeout);
if (clientConn && clientConn->WaitForConnectionComplete()) {
    // clientConn is ready
}
```

#### 1.2 Sending and Receiving Messages

The following calls are provided by the `LiteConnConnection` class:

- Use `SendData()` to send unreliable messages.
- Use `SendReliableData()` to ensure delivery with retries.
- Use `SendRequest()` to send a reliable message expecting a reply. It returns a `LiteConnRequest` which you can block on.
- Use `Receive()` to get incoming messages. If the message is a request, the `reqestHandle` field is set.

Note: All public functions of `LiteConnConnection` and `LiteConnManager`, including message-sending functions like `SendRequest()`, return `std::optional` results when appropriate. These return `std::nullopt` if the connection is no longer valid, such as after being disconnected. This design allows all API calls to be safely used from multiple threads and during connection teardown. Always check the return value before using any handle or result.

#### 1.3 Responding and Conversing

- When you receive a message via `Receive()`, it may carry a `LiteConnRequest` in its `requestHandle` field if it originated from a peer's request.
- Call `.Respond()` to respond to the request.
- Call `.Converse()` to send a follow-up request and wait for reply.
- Call `.Reject()` to discard the request.
- The destructor of `LiteConnRequest` will reject the response if it has not been fufilled.

```cpp
if (msg.requestHandle) {
    msg.requestHandle->Respond(...);
    // or
    auto followUp = msg.requestHandle->Converse(...);
    if (followUp) auto reply = followUp->GetResponse();
}
```

#### 1.4 Disconnecting and Cleanup

- Call `Disconnect()` to terminate the connection.
- All pending `LiteConnRequest`s are completed with `nullopt`.

---

### 1.5 Lifetime and Thread Safety

- All public interfaces of `LiteConnConnection` and `LiteConnManager` are thread-safe.
- Access to `LiteConnResponse` and `LiteConnRequest` is not thread safe.
- The `LiteConnManager` owns the `UDPSocket` used by all `LiteConnConnection` instances it creates. When the manager is destroyed, the socket is closed, and all active connections will be disconnected.
- When `LiteConnConnection` is destroyed, if the connection is active (whether connected or establishing connection),the connection will be teared down.

### 2. Protocol Details (Implementation)

### 2.1 Core Abstractions

The key abstractions in LiteConn are introduced in [Section 1 – Usage Guide](#1-usage-guide), which focuses on how they are used in practice. For reference:

- `LiteConnManager`: Manages a shared UDP socket and oversees multiple logical connections.
- `LiteConnConnection`: Represents a reliable or unreliable connection between peers.
- `LiteConnRequest` and `LiteConnResponse`: Track request/response pairs in a conversation.
- `LiteConnMessage`: A message delivered to the application layer, possibly carrying a `LiteConnRequest`.

This section focuses on internal behaviors and interactions among these abstractions. Each object is implemented to support low-latency communication over unreliable networks, using features like retransmission, timeout monitoring, and explicit request handling.

### 2.2 Packet Format

Each packet consists of:

- A serialized `LiteConnHeader`
- A serialized `uint64_t` representing the new request created by the response (Optional)
- Application payload (optional)

#### LiteConnHeader Fields

| Field       | Type       | Description                                                |
| ----------- | ---------- | ---------------------------------------------------------- |
| `sessionID` | `uint32_t` | Identifies the connection                                  |
| `index`     | `uint32_t` | Sequence number for ordering                               |
| `flag`      | `uint8_t`  | Bitfield defining packet behavior                          |
| `id32`      | `uint32_t` | Used for identifying packets that requires reliable delivery and session negotiation |
| `id64`      | `uint64_t` | Unique ID for request/response tracking                    |

### 2.3 Flags and Semantics

LiteConn internally uses packet-level flags to implement its request/reply, reliability, and connection features. These flags are completely hidden from the application and are set or interpreted by the protocol based on the public API calls such as `SendRequest`, `SendReliableData`, `Receive`, and `Disconnect`.

#### Internal Data and Control Flags (not visible to application)

| Flag   | Description                                                |
| ------ | ---------------------------------------------------------- |
| `DATA` | Marks packet as containing user payload                    |
| `IMP`  | Packet must be reliably delivered (with retry/ACK)         |
| `REQ`  | Packet expects a response (request)                        |
| `ACK`  | Acknowledges a packet identified by `id32` or `id64`       |
| `CXL`  | Signals cancellation of a request or late response discard |
| `SYN`  | Starts connection handshake                                |
| `FIN`  | Requests peer to close connection                          |
| `HBT`  | Heartbeat ping to verify peer liveness                     |

### 2.4 Connection Lifecycle

#### Handshake

The LiteConn handshake establishes a reliable connection between two peers over UDP using a custom three-way exchange. This ensures both parties agree on a session identifier used for routing and acknowledgement. The client begins by sending a probe with a temporary identifier, and the server responds with session information to complete the association.

1. **Client sends SYN** using `LiteConnManager::ConnectPeer()`. This call creates a `LiteConnConnection` in the `Connecting` state. The initial packet includes `sessionID = 0` and a randomly generated checksum in `id32`. This checksum acts as a temporary identifier to allow routing until the server assigns the final sessionID. The client will repeatedly resend this SYN message until it receives the server's SYN-ACK response or until the connection times out.
2. **Server replies with SYN | ACK** in response to a client connection attempt initiated through `LiteConnManager::Accept()`. This creates a `LiteConnConnection` in the `Pending` state. The server echoes the client's checksum in the `sessionID` field so the packet can be correctly routed on the client side. Meanwhile, the server's own assigned sessionID is embedded in the `id32` field. This response initiates the final leg of the handshake and waits for the client to confirm.
3. **Client replies with ACK**, updates its internal sessionID to the value received in `id32`, and confirms the connection by responding to the server

```plaintext
Client                                      Server
  |                                           |
  | ------ SYN (id32 = client checksum) ----> |
  |                                           |
  | <--- SYN + ACK (id32 = session ID,
  |                sessionID = checksum) ---- |
  |                                           |
  | ------------- ACK (sessionID) ----------> |
  |                                           |
Connection established                 Connection established
```


#### Timeout and Retransmission Behavior

LiteConn employs a robust retry and timeout strategy to ensure connections can be established and maintained over unreliable networks.

Each connection’s timeout behavior is coordinated by the routing thread within `LiteConnManager`, which periodically wakes at a frequency set by the manager’s `updateInterval` parameter. This thread checks for expired timers and determines when to retry handshakes or retransmit messages.

Timeouts and retransmissions in LiteConn are coordinated by the background routing thread in `LiteConnManager`. Every active connection is checked for expired timers and required actions during each iteration of this thread.

#### Handshake Retry Behavior

LiteConn employs robust retry logic on both sides of the connection to tolerate packet loss during the handshake phase:

- **Client Retries**: After sending the initial `SYN`, the client enters the `Connecting` state and resends `SYN` packets at regular intervals (as defined by `connectionRetryInterval`) until it receives a `SYN | ACK` from the server or the `connectionTimeout` is reached.

- **Server Retries**: After sending a `SYN | ACK`, the server enters the `Pending` state. If it does not receive the client's final `ACK`, it alternates between:
  - Re-sending the original `SYN | ACK` using the client's checksum as `sessionID`
  - Sending a `SYN` packet using the final session ID to reintroduce the session to a client that may have completed the handshake

This dual retry approach ensures that connection setup is resilient to unidirectional or bidirectional packet loss.

Once the client sends its `ACK`, it immediately enters the `Connected` state. The server, however, remains in `Pending` until it receives this confirmation.

#### Handshake Retry Flow

```plaintext
Client                                      Server
  |                                           |
  | ------ SYN (id32 = checksum) ---------->  |
  |     [lost or delayed]                     |
  |                                           |
  | ------ SYN (retry) -------------------->  |
  |                                           |
  | <--- SYN + ACK (id32 = sessionID,
  |                sessionID = checksum) ---  |
  |                                           |
  | ----------- ACK (sessionID) ----------->  |
  |     [client becomes Connected]            |
  |     [ACK is lost]                         |
  |                                           |
  |                                   Server unsure if ACK received:
  | <--- SYN + ACK (retry) -----------------  |
  | <--- SYN (sessionID) -------------------  |
  |                                           |
  | (Client responds to either retry)         |
  | ----------- ACK (sessionID) ----------->  |
  |                                           |
Connection established                 Connection established
```

These features ensure LiteConn remains responsive and resilient even under packet loss or intermittent network conditions.



#### Timeout Setting

Each `LiteConnConnection`'s timers are monitored by the background routing thread of the `LiteConnManager`. The frequency at which this thread activates is determined by the `updateInterval` parameter passed to the `LiteConnManager` constructor. These timers are configured via the `TimeoutSetting` structure, which governs how often heartbeats are sent, how long to retry important messages, and when to declare a connection as dead.

- `connectionTimeout`: If no valid message is received within this duration, the connection is marked as disconnected. The expiration timer is reset every time a valid packet is received, ensuring that active connections are not dropped due to inactivity between heartbeats or application messages.
- `connectionRetryInterval`: Determines how often the protocol resends handshake or heartbeat packets.
- `impRetryInterval`: Controls the retransmission rate of important (`IMP`) or request (`REQ`) packets until they are acknowledged.
- `replyKeepDuration`: Duration for which recently acknowledged messages are remembered. This helps detect duplicates and provide retransmission feedback.

The router thread invokes these timers regularly and cleans up expired state. Heartbeat (`HBT`) packets are used to maintain session liveness, while retry logic ensures delivery of critical messages without relying on TCP.



#### Teardown

LiteConn provides a straightforward mechanism for cleanly closing a connection. Either peer can initiate the teardown process by sending a `FIN` control packet. This signals the intention to close the session and release any associated resources.

- Sending `FIN` marks the connection as closed
- Peer receiving `FIN` will close immediately and fulfill all pending futures with `nullopt`
- No `FIN` retransmission is performed; timeouts must handle cleanup

### 2.5 Request/Response Internals

#### Sending Requests

- `SendRequest()` sends a packet flagged with `REQ | DATA` and includes a unique `id64` with reliability to track the request.

- This transport-level acknowledgment only confirms receival by the peer — it does not include the application-level response.

- The application itself must handle the response by calling `Respond()` on the corresponding `LiteConnRequest` handle.

#### Receiving Requests

- `REQ` messages create a `LiteConnRequest`
- Peer may `Respond()`, `Reject()`, or `Converse()`
- When using `Converse()`, the response message itself is flagged as a new `REQ` to continue the conversation. In this case, the new request must include its `id64` (request ID) in the payload so the peer can extract it upon receipt. This ID is used to track and match the follow-up request with its response. The protocol handles this transparently, but it is important to note that conversational replies embed this tracking ID as part of the application-visible message data

```plaintext
  Peer A                                Peer B
   |                                      |
   | --- REQ + DATA(id64 = 1001) -------> |
   |                                      |
   | <--- ACK + DATA (id64 = 1001) ------ |
   |                                      |
   | [Response received]                  |

Optional follow-up (conversation):
   | --- ACK + DATA + REQ (id64 = 1001, new id64 = 1002 in payload) -> |
   |                                                                   |
   | <--- ACK + DATA (id64 = 1002) ----------------------------------- |
```

#### Cancellation

Request and response cancellation is explicitly communicated via the `CXL` flag to ensure both peers clean up associated resources.

- If a `LiteConnResponse` is cancelled by calling `Cancel()` or destroyed without completion, a `CXL` packet is sent to the peer.
- If a `LiteConnRequest` is rejected via `Reject()` or destroyed without being responded to, a `CXL | ACK` packet is sent. This indicates that the cancellation refers to a request originally sent by the peer.
- If a response arrives for a request that is no longer tracked (expired or already cancelled), a `CXL` is sent back to inform the sender that the original request is no longer valid.

All `CXL` messages, including both `CXL` and `CXL | ACK`, are delivered reliably. They are retransmitted until acknowledged to ensure the peer receives definitive notice of cancellation.

These signals ensure both sides can promptly discard any retained state for cancelled requests or conversations.

### 2.6 Message Delivery and ACKs

Reliable delivery in LiteConn is handled transparently by the transport layer using an auto-resend mechanism. When a packet is marked for reliable delivery (such as with `SendReliableData` or `SendRequest`), it is registered for retransmission until acknowledged. This behavior is only enabled after the connection has reached the `Connected` state — reliability features are not active during the handshake phase.

Each reliably sent packet is assigned a unique `id32`, which is used to track acknowledgments. This value is embedded in the `LiteConnHeader` and serves as the key identifier for the peer to confirm receipt. Unlike `index`, which continues to increment for every outgoing packet to assist in freshness checks and address updates, `id32` is used solely for identifying which packet should be acknowledged.

As long as no matching `ACK` is received for a packet's `id32`, the packet will be automatically retransmitted at regular intervals defined by `impRetryInterval`. Once acknowledged, the resend entry is removed and the packet is considered successfully delivered.

```plaintext
Sender                      Receiver
  |                            |
  | --- IMP + DATA (id32=5) -->|
  |                            |
  | <-------- ACK (id32=5) ----|
  |                            |
  [If no ACK within timeout:]
  |                            |
  | --- retry IMP (id32=5) --->|
  |                            |

```

To prevent duplicate processing, the peer that receives and acknowledges a reliable packet will remember the `id32` of that packet for a duration specified by `replyKeepDuration` in `TimeoutSetting`. If a duplicate of the same packet is received before this duration expires—such as when the original ACK was lost—the peer will recognize it, suppress reprocessing, and resend the appropriate ACK response.