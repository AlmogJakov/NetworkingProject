#pragma once
typedef struct packetMSG {
    int id;
    int src;
    int dest;
    int trailMSG;
    int funcID;
    char payload[492] = {0};
}message;
void gotmsg(message* msg);
// Parameters: The message number on it Sent the ack (4 bytes First in payload)
// Returns Func: none
void ack(message* msg);

// Parameters: The message number on it Sent the nack (4 bytes First in payload)
// Returns Func: none
void nack(int message_num);

// Parameters: none
// Returns Func: ack or nack
//void Cnct();
void cnct(const unsigned int new_socket, message * msg);
// Parameters: Destination Junction (4 bytes First in payload)
// Returns Func: none
void discover(int message_num);

// Parameters:
// 1. Message number - discover the original (4 first bytes In payload)
// 2. The length of the answer (4 seconds bytes In payload).
//     in extent And no route was found The length of the answer will be 0.
// 3. The nodes in sending order (4 bytes per node).
// Returns Func: none
void route(int message_num,int length,int * way);

// Parameters:
// 1. The length of the message (4 First bytes in payload of The first message Only).
// 2. The message itself (Starting from The fifth byte)
// Returns Func: ack or nack
void Send(int length,char*);

// Parameters:
// 1. next node In the path (4 First bytes in payload).
// 2. Number of subsequent messages that has to be relayed (seconds 4 bytes in payload).
// Returns Func: ack or nack
void relay(int message_num);