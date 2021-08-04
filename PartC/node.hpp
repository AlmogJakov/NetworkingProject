#pragma once
using namespace std;

typedef struct packetMSG {
    int id;
    int src;
    int dest;
    int trailMSG;
    int funcID;
    char payload[492] = {0};
}message;
void gotmsg(message* msg, int ret);
// Parameters: The message number on it Sent the ack (4 bytes First in payload)
// Returns Func: none
void input_ack(message* msg, int ret);

// Parameters: The message number on it Sent the nack (4 bytes First in payload)
// Returns Func: none
void input_nack(message* msg, int ret);

// Parameters: none
// Returns Func: ack or nack
//void Cnct();
void input_connect(message* msg, int ret);
// Parameters: Destination Junction (4 bytes First in payload)
// Returns Func: none
void input_discover(message* msg, int ret);

// Parameters:
// 1. Message number - discover the original (4 first bytes In payload)
// 2. The length of the answer (4 seconds bytes In payload).
//     in extent And no route was found The length of the answer will be 0.
// 3. The nodes in sending order (4 bytes per node).
// Returns Func: none
void input_route(message* msg, int ret);

// Parameters:
// 1. The length of the message (4 First bytes in payload of The first message Only).
// 2. The message itself (Starting from The fifth byte)
// Returns Func: ack or nack
void input_send(message* msg, int ret);

// Parameters:
// 1. next node In the path (4 First bytes in payload).
// 2. Number of subsequent messages that has to be relayed (seconds 4 bytes in payload).
// Returns Func: ack or nack
void input_relay(message* msg, int ret);

// returns the type name of the message
string message_type(message* msg);

void std_setid(stringstream& ss,string splited[]);
void std_send(stringstream& ss,string splited[]);
void std_connect(stringstream& ss,string splited[]);
void std_route(stringstream& ss,string splited[]);
void std_peers(stringstream& ss,string splited[]);

void std_refresh(int general_req_id);
void send_refresh(int general_request_id);
void input_refresh(message *msg);

void send_nack(message* msg, int ret = -1);
void send_ack(message* msg);
void send_route(message* msg);
void send_discover(int dst, int discover_id);
void send_relay(int destination,int original_id);