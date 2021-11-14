#pragma once

#include <iostream> /* basic actions */
#include <bits/stdc++.h> /* INT_MIN */
#include <numeric> /* accumulate */
#include <set> /* e.g nodes_to_request set */
#include <unordered_map> /* e.g sockets unordered_map */
#include <vector> /* e.g waze vector */
#include <sstream> /* stdin input to sstream */
#include <cmath> /* floor/ceil */
#include <fcntl.h> /* set socket to unblocked/blocked */
#include <unistd.h> /* read/write/close */
#include <arpa/inet.h> /* inet_ntop */

using namespace std;

#include "select.hpp"

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// VARIABLES & DATA STRUCTURES ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/* current node id */
extern int id;
/* key - neighbor id. value - socket.
   Note! to get the neighbors we can iterate thru this map keys. */
extern unordered_map<int,unsigned int> sockets;
/* save data that should be sent to each node 
   key - dest. value - data to send. */
extern unordered_map<int,string> text;
/* save path to each node */
extern unordered_map<int,vector<int>> waze;
/* temporary routes to save the partial route between the source and destination */
extern unordered_map<int,vector<int>> checkpoint_waze;
/* storing the nodes left to the router from this node (usable by methods like discover method) */
extern unordered_map<int,set<int>> nodes_to_request;
/* saves the data of the node to which an answer should be returned.
   key: source id (e.g general_request_id). 
   value: pair (first - the node id to reply. second - last message id) */
extern unordered_map<int,pair<int, int>> node_to_reply;
/* key - neighbor id. value - ip:port */
extern unordered_map<int,string> ip_port;

extern int r_port;

/* basic structure of a packet */
typedef struct packetMSG {
    int id;
    int src;
    int dest;
    int trailMSG;
    int funcID;
    char payload[492] = {0};
} message;

////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// STD INPUT METHODS /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void std_setid(stringstream& ss,string splited[]);
void std_send(stringstream& ss,string splited[]);
void std_connect(stringstream& ss,string splited[]);
void std_route(stringstream& ss,string splited[]);
void std_peers(stringstream& ss,string splited[]);
void std_refresh(int general_req_id);

////////////////////////////////////////////////////////////////////////////////////
////////////////////////// EXTERNAL SOCKETS INPUT METHODS //////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void gotmsg(message* msg, int ret);
void input_ack(message* msg, int ret);
void input_nack(message* msg, int ret);
void input_connect(message* msg, int ret);
void input_discover(message* msg, int ret);
void input_route(message* msg, int ret);
void input_send(message* msg, int ret);
void input_relay(message* msg, int ret);
void input_refresh(message *msg);

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// SEND METHODS ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void send_refresh(int general_request_id);
void send_nack(message* msg, int ret = -1);
void send_ack(message* msg);
void send_route(message* msg);
void send_discover(int dst, int discover_id);
void send_relay(int destination,int original_id);

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// GLOBAL METHODS //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void print_ip_info(int r_port);
string get_ip_port();

// /* returns the type name of the message */
// string message_type(message* msg) {
//     //int func_id = *(int*)(&msg->funcID);
//     int func_id;
//     memcpy(&func_id, &msg->funcID, sizeof(int));
//     if (func_id==1) return "Ack";
//     else if (func_id==2) return "Nack";
//     else if (func_id==4) return "Connect";
//     else if (func_id==8) return "Discover";
//     else if (func_id==16) return "Route";
//     else if (func_id==32) return "Send";
//     else if (func_id==64) return "Relay";
//     else if (func_id==128) return "Delete";
//     return "(Can not identify)";
// }