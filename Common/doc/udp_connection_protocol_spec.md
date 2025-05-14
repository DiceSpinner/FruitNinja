## UDPConnection Protocol Specification

### Overview
This document defines the message-level protocol used by the `UDPConnection` class for optionally reliable, optionally bi-directional communication over UDP. It includes packet format, flag semantics, delivery guarantees, and guidelines for application-level usage.

---

### Connection Handshake

UDPConnection establishes a session between peers using a handshake protocol that ensures both sides agree on a shared session identifier (`sessionID`). This process is initiated when a peer attempts to connect.

#### Handshake Packet Exchange:
1. **Client sends SYN**
   - A packet with the `SYN` flag and a randomly generated sessionID is sent to initiate the connection.

2. **Server replies with SYN|ACK**
   - The server responds with a `SYN | ACK` packet containing the confirmed sessionID (it may echo or generate its own).

3. **Client sends ACK**
   - The client acknowledges the handshake with an `ACK` packet. After this, the connection is considered established.

#### SessionID Handling:
- The server has the final authority on the sessionID. The client generates a temporary sessionID when initiating the handshake, but it must adopt the sessionID provided by the server in the `SYN | ACK` reply.
- The `sessionID` is included in all subsequent packets to associate them with the active connection.
- Packets with incorrect or mismatched sessionIDs are discarded.

---

### Packet Structure
Each packet consists of:

- **Header** (`UDPHeader`): Serialized at the beginning of the payload
- **Payload**: Application-defined data

#### `UDPHeader` Fields:
| Field       | Type      | Description |
|-------------|-----------|-------------|
| `index`     | `uint32_t` | Unique packet index per connection (used for ordering, deduplication, and response matching). Handled automatically by the transport layer. |
| `sessionID` | `uint32_t` | Session identifier assigned during connection handshake. Managed by the transport layer. |
| `ackIndex`  | `uint32_t` | Index of packet this message acknowledges (used in ACK packets) |
| `flag`      | `uint8_t`  | See flag definitions below |

---

### Flag Semantics (`UDPHeaderFlag`)

#### Application-level Flags:
| Flag  | Description |
|-------|-------------|
| `None` | No special handling |
| `REQ`  | Indicates this is a request expecting a response. Retries until a response is received or timeout. No ACK is required. |
| `ASY`  | Used with `REQ`. The response will arrive asynchronously. |
| `IMP`  | Indicates reliable delivery is required. An ACK is expected; otherwise, the packet will be retransmitted. Cannot be used with `REQ`. |
| `ACK` | Acknowledges receipt of a packet identified by `ackIndex`. May be used by application logic to acknowledge `REQ` packets. ACKs for `IMP` packets are handled automatically by the `UDPConnection`. |

#### Internal-use Flags (DO NOT USE directly):
| Flag  | Description |
|-------|-------------|
| `SYN` | Used in connection setup |
| `FIN` | Used to notify connection termination |
| `HBT` | Heartbeat packet to check if connection is alive |

#### Invalid Combinations:
- `REQ | IMP` → **Invalid**. `REQ` implies retry-on-no-response; adding `IMP` is redundant and semantically conflicting.
- `IMP | ACK` → **Invalid** for application use. This combination is reserved by the transport layer to acknowledge receipt of an `IMP` packet and is generated automatically by `UDPConnection`.
