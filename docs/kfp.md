# Karma Frame Protocol(SBP) v0.1

Karma Frame Protocol is a frame base protocol.
All frames begin with a fixed 12-octet header, followed by an extended header and payload.

## Frame Header 
The Frame Header is a fixed 12-octet header that appears at the beginning of every frame. It contains fields for the Frame Length, Magic Code, Operation Code, Flags, and Frame Identifier.


### Frame Length
The length of the frame is expressed as a 32-bit integer. `Frame Length` = 16 + `Header.length` + `Payload.length`.
### Magic Code
A fixed value representing the protocol self. Currently, the value is 23. This field is used to detect the presence of the SBP protocol, and implementations MUST discard any frame that does not contain the correct magic number.
### Operation Code
The 16-bit opcode of the frame. The frame opcode determines the format and semantics of the frame. Implementations MUST ignore and discard any frame with an unknown opcode.
### Flags
Flags apply to this frame. The flags have the following meaning (described by the mask that allows selecting them):
- 0x01: Response flag. If set, the frame contains the response payload to a specific request frame identified by a frame identifier. If not set, the frame represents a request frame.

The rest of the flags are currently unused and ignored.

### Frame Identifier
A unique identifier for a request frame. That is, it is used to support request-response. 
The frame identifier is expressed as a 32-bit integer in network byte order.

### Extended Header
The extended header starts with the length fields. The length field is used to determine the length of the extended header. The length field is expressed as a 32-bit integer in network byte order. 
The extended header is followed by the payload.


## Frame Definitions
This specification outlines various types of frames, each with a unique 16-bit opcode to identify them. Each frame type has its own specific extended header and payload.

The table below shows all the supported frame types along with a preallocated opcode.

| Opcode | Frame Type | Description |
|--------|------------|-------------|
| 0x0001 | PING | Measure a minimal round-trip time from the sender. |
| 0x0002 | HEARBEAT | To keep clients alive through periodic heartbeat frames. |
| 0x0003 | APPEND_ENTRY
| 0x0004 | APPEND_ENTRY_REPLY
| 0x0005 | REQUEST_VOTE
| 0x0006 | REQUEST_VOTE_REPLY



The below sub-sections describe the details of each frame type, including their usage, their binary format, and the meaning of their fields.

### APPEND_ENTRY
**Request Frame**
然后再client发送的rpc函数时，需要指定target id，但frame里面不需要



=> client_id
=> group_id(raft group id)
=> append entry request 
    => current_term
    => leader_id
    => prev_log_idx
    => prev_log_term
    => leader_commit_idx
=> payloads
    => entries

另一边主机收到frame后，解析header和payload，组成append entry requst，找到相关的raft group的fsm，丢进去。

极端上来说，只需要由一个group id就行。因为，服务端收到frame后，只需要查找group id就能知道把请求丢给哪个raft group的fsm的step。


**Response Frame**
=> group_id
=> append entry response 




## 