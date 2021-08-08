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

#include "select.hpp"
#include "node.hpp"

/* HEADERS:

~   ~   ~   ~   ~   ~   ~   ~   ~ ACK HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 _________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                     PAYLOAD                    |
|-------------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | FUNC ID TO ACK TO |              |
|_________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   ~ RELAY/SEND ACK HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                        PAYLOAD                       |
|-------------------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | FUNC ID TO ACK TO | GENERAL REQUEST ID |
|_______________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   ~ NACK HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 _________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                     PAYLOAD                    |
|-------------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | FUNC ID TO NACK TO |             |
|_________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   ~ REFRESH NACK HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 ________________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                       PAYLOAD                         |
|--------------------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | FUNC ID TO NACK TO | GENERAL REQUEST ID |
|________________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   ~ ROUTE HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 ____________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                     PAYLOAD                       |
|----------------------------------------------------------------------------------------------------|
|                                                | FIRST GENERAL REQUEST ID | PATH LEN | PATH . . .  |
|____________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   ~ RELAY HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 ______________________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                          PAYLOAD                            |
|--------------------------------------------------------------------------------------------------------------|
|                                                | NEXT NODE ID | NUM OF MESSAGES TO SEND | GENERAL REQUEST ID |
|______________________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   DISCOVER HEADER   ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |               PAYLOAD                |
|---------------------------------------------------------------------------------------|
|                                                | DEST | GENERAL REQUEST ID |          |
|_______________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   CONNECT HEADER   ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |               PAYLOAD                |
|---------------------------------------------------------------------------------------|
|                                                | IP:PORT LEN | IP:PORT |              |
|_______________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   ~ CONNECT ACK HEADER ~   ~   ~   ~   ~   ~   ~   ~   ~
 __________________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                         PAYLOAD                         |
|----------------------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | FUNC ID TO ACK TO | IP:PORT LEN | IP:PORT |
|__________________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~ DISCOVER NACK HEADER ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                           PAYLOAD                            |
|---------------------------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | FUNC ID TO NACK TO | GENERAL REQUEST ID | DEST |
|_______________________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   SEND HEADER   ~   ~   ~   ~   ~   ~   ~   ~
 _____________________________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                          PAYLOAD                           |
|-------------------------------------------------------------------------------------------------------------|
|                                                | TEXT LEN | TEXT . . . | NODE TO REPLY | GENERAL REQUEST ID |
|_____________________________________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~   REFRESH HEADER   ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |               PAYLOAD                |
|---------------------------------------------------------------------------------------|
|                                                | GENERAL REQUEST ID |                 |
|_______________________________________________________________________________________|
*/

/* current node id */
int id = INT_MIN;
/* key - neighbor id. value - socket.
   Note! to get the neighbors we can iterate thru this map keys. */
unordered_map<int,unsigned int> sockets;
/* save data that should be sent to each node 
   key - dest. value - data to send. */
unordered_map<int,string> text;
/* save path to each node */
unordered_map<int,vector<int>> waze;
/* temporary routes to save the partial route between the source and destination */
unordered_map<int,vector<int>> checkpoint_waze;
/* storing the nodes left to the router from this node (usable by methods like discover method) */
unordered_map<int,set<int>> nodes_to_request;
/* saves the data of the node to which an answer should be returned.
   key: source id (e.g general_request_id). 
   value: pair (first - the node id to reply. second - last message id) */
unordered_map<int,pair<int, int>> node_to_reply;
/* key - neighbor id. value - ip:port */
unordered_map<int,string> ip_port;

int r_port;

int main(int argc, char *argv[]) {
    int innerfd = 0, outerfd=0;
    struct sockaddr_in serv_addr;
    int ret, i;
    r_port = 12345; /* default Port */
    if (argc>1) {r_port = stoi(argv[1]);} /* set user input Port if received */
    if (argc>2) { /* set user input ID if received */
        id = stoi(argv[2]);
    } 
    //print_ip_info(r_port);
    char buff[256];
    innerfd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    if (setsockopt(innerfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("\nInvalid address/ Address not supported \n");
        return 1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(r_port);
    bind(innerfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    //printf("adding fd(%d) to monitoring\n", innerfd); /* TODO: remove line */
    add_fd_to_monitoring(innerfd);
    listen(innerfd,100);

    while(true){
        memset(&buff, '\0', sizeof(buff));
	    //printf("waiting for input...\n");
	    ret = wait_for_input();
	    //printf("fd: %d is ready. reading...\n", ret);
        /* At the file descriptor level, stdin is defined to be file descriptor 0,
           stdout is defined to be file descriptor 1,
           and stderr is defined to be file descriptor 2. */
        if (ret < 2) { // std input
            /* unblock read from socket ret for the purpose of ending the next while loop */
            fcntl(ret, F_SETFL, SOCK_NONBLOCK); 
            stringstream ss;
            read(ret, buff, 256);
            while(1) {
                ss << buff;
                memset(&buff, '\0', sizeof(buff));
                if (read(ret, buff, 256)<=0) break;
            }
            /* set back socket ret to blocked */
            ret = ret & (~O_NONBLOCK);
            string splited[4]; /* split std input */
            getline(ss,splited[0],',');
            if (splited[0].at(splited[0].size()-1)=='\n') {splited[0].pop_back();} /* peers'\n' to peers */
            /* if set id == NULL the only command that could be executed is 'setid' */
            if (splited[0].compare("setid")!=0 && id == INT_MIN) {
                cout << "nack" << endl;
                continue;
            }else if (splited[0].compare("setid")==0) {std_setid(ss,splited);}
            else if (splited[0].compare("connect")==0) {std_connect(ss,splited);}
            else if (splited[0].compare("send")==0) {std_send(ss,splited);}
            else if (splited[0].compare("route")==0) {std_route(ss,splited);}
            else if (splited[0].compare("peers")==0) {std_peers(ss,splited);}
        } else { /* we got a packet */
            socklen_t addrlen;
            addrlen = sizeof(serv_addr);
            if (ret==innerfd) { /* if ret==innerfd then we got a new connection! */
                if ((ret = accept(ret, (struct sockaddr *)&serv_addr, &addrlen))<0) {
                    //perror("accept");
                    //exit(EXIT_FAILURE);
                    continue;
                }
            }
            /* get the message to struct */
            message* incoming = (message*)malloc(sizeof(message));
            int bytes_readed = read(ret ,incoming, 512);
            /* id is not setted! return nack */
            if (id==INT_MIN) {
                send_nack(incoming,ret);
                continue;
            }
            /* if 0 bytes were called from the socket. The socket has disconnected! */
            if (bytes_readed==0) {
                /* remove the socket from "sockets"! */
                int removed_id;
                for(auto it = sockets.begin(); it != sockets.end(); it++) {
	                if((it->second) == ret) {
                        removed_id = it->first; /* save the id of the removed node */
		                sockets.erase(it->first); /* remove fd of the socket */
                        ip_port.erase(it->first); /* remove ip&port */
		                break;}
                }
                /* when node disconnect, the general request id is the id of the deleted node.
                   (as opposed to added node where the general request id is random). */
                node_to_reply[removed_id] = {-1, -1};
                /* remove the socket from monitoring! */
                remove_fd_from_monitoring(ret);
                std_refresh(removed_id);
                continue;
                // TODO: call std_refresh
            }
            /* print message type */
            // cout << "\033[1;36m"; /* print in color */
            // cout << "Got " << message_type(incoming) << " message type!" << endl;
            // cout << "\033[0m"; /* end print in color */
            /* handle the message */
            gotmsg(incoming, ret);
            delete(incoming);
        }
	}
}

////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// STD INPUT METHODS /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/* ----------------------------- SETID (STDIN) ---------------------------------- */
void std_setid(stringstream& ss,string splited[]) {
    if (id!=INT_MIN) { /* ID has already been set */
        cout << "nack" << endl;
        return;
    }
    getline(ss,splited[1],','); /* id */
    id = stoi(splited[1]);
    cout << "ack" << endl;
}

/* ---------------------------- CONNECT (STDIN) --------------------------------- */
void std_connect(stringstream& ss,string splited[]) {
    getline(ss,splited[1],':'); /* ip */
    getline(ss,splited[2],':'); /* port */
    uint16_t port = stoul(splited[2]);
    char const* destip = splited[1].c_str();
    struct sockaddr_in destAddress;
    int new_sock = socket(AF_INET, SOCK_STREAM, 0);
    /* set reuse option to enable */
    int enable = 1;
    if (setsockopt(new_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("nack\n"); /* Invalid address/ Address not supported */
        return;
    }
    /* bind socket to destAddress */
    bind(new_sock, (struct sockaddr*)&destAddress, sizeof(destAddress));
    memset(&destAddress, 0, sizeof(destAddress));
	destAddress.sin_family = AF_INET;
    destAddress.sin_port = htons(port);
    if(inet_pton(AF_INET, destip, &destAddress.sin_addr)<=0) {
        printf("nack\n"); /* Invalid address/ Address not supported */
        return;}
    if (connect(new_sock, (struct sockaddr*)&destAddress, sizeof(destAddress)) < 0) {
        printf("nack\n"); /* Connection Failed */
        return;}
    add_fd_to_monitoring(new_sock);
    message outgoing;
    outgoing.id = rand();
    outgoing.src = id;
    outgoing.dest = 0;
    outgoing.trailMSG = 0;
    outgoing.funcID = 4; /* connect function id is 4 */
    /* send ip & port */
    string ip_port = get_ip_port();
    int ip_port_len = ip_port.size();
    memcpy(outgoing.payload, &ip_port_len, sizeof(int));
    memcpy(outgoing.payload+sizeof(int), ip_port.c_str(), ip_port_len);
    char outgoing_buffer[512];
    memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
    send(new_sock, outgoing_buffer, sizeof(outgoing_buffer), 0);
}

/* ------------------------------ SEND (STDIN) ---------------------------------- */
void std_send(stringstream& ss,string splited[]) {
    getline(ss,splited[1],','); /* destination */
    getline(ss,splited[2],','); /* message length */
    if (splited[2].at(splited[2].size()-1)=='\n') {splited[2].pop_back();}
    //getline(ss,splited[3],','); /* message itself */
    splited[3].resize(stoi(splited[2]));
    getline(ss, splited[3]); /* message itself */
    if (sockets.size()==0) { /* No neighbors at all!. TODO: send to myself */
        cout << "NACK" << endl;
        return;
    }
    int general_req_id = rand();
    if (sockets.find(stoi(splited[1]))!=sockets.end()) { /* connected directly. send the message! */
        waze[stoi(splited[1])].push_back(stoi(splited[1]));
        text[stoi(splited[1])] = splited[3];
        node_to_reply[general_req_id] = {-1, -1}; /* source node! stop condition */
        send_relay(stoi(splited[1]),general_req_id);
    } else { /* not connected directly. we need to discover/relay&send. */
        text[stoi(splited[1])] = splited[3];
        if(waze.find(stoi(splited[1]))==waze.end()){ /* no path! lets discover */
            node_to_reply[general_req_id] = {-1, -1}; /* source node! stop condition */
            for(auto nei : sockets) {
                nodes_to_request[general_req_id].insert(nei.first);
            }
            send_discover(stoi(splited[1]),general_req_id);}
        else { /* if there is already a path */
            node_to_reply[general_req_id] = {-1, -1}; /* source node! stop condition */
            send_relay(stoi(splited[1]),general_req_id);
        }
    }
}

/* ------------------------------ ROUTE (STDIN) --------------------------------- */
void std_route(stringstream& ss,string splited[]) {
    getline(ss,splited[1],','); /* destination */
    if (stoi(splited[1])==id) { /* This is the current node! */
        cout << "ack" << endl;
        cout << id << endl;
        return;
    }
    if (sockets.find(stoi(splited[1]))!=sockets.end()) { /* This is a neighbor! */
        cout << "ack" << endl;
        cout << id << "->" << stoi(splited[1]) << endl;
        return;
    }
    if (waze.find(stoi(splited[1]))!=waze.end()) { /* we have the path! print it! */
        int len = waze[stoi(splited[1])].size();
        cout << "ack" << endl;
        cout << id << "->";
        for (int i = 0; i < len-1; i++) {
            cout << waze[stoi(splited[1])][i] << "->";
        }
        cout << waze[stoi(splited[1])][len-1] << endl;
        return;
    }
    if (sockets.size()==0) { /* No neighbors at all! */
        cout << "nack" << endl;
        return;
    }
    int general_req_id = rand();
    node_to_reply[general_req_id] = {-1, -1}; /* source node! stop condition */
    for(auto nei : sockets) {
        nodes_to_request[general_req_id].insert(nei.first);
    }
    send_discover(stoi(splited[1]),general_req_id);
}

/* ------------------------------ PEERS (STDIN) --------------------------------- */
void std_peers(stringstream& ss,string splited[]) {
    cout << "ack" << endl;
    if (sockets.size()==0) {return;} /* no neighbors at all! */
    auto it = sockets.begin();
    cout << it->first;
    it++;
    for(; it != sockets.end(); it++) {
        cout << "," << it->first;
    }
    cout << endl;
}

/* --------------------------------- DELETE ------------------------------------ */
void std_refresh(int general_req_id) {
    if (!waze.empty()) {
        waze.clear();
    }
    if (sockets.empty()) { /* no neighbors to send delete message */
        return;
    }
    for (auto nei : sockets) {
        nodes_to_request[general_req_id].insert(nei.first);
    }
    send_refresh(general_req_id);
}

////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// EXTERNAL SOCKETS METHODS /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void gotmsg(message* msg, int ret){
    switch(msg->funcID){
        case 1:{ /* ACK */
            input_ack(msg,ret);
            break;}
        case 2:{ /* NACK */
            input_nack(msg, ret);
            break;}
       case 4:{ /* CONNECT */
            input_connect(msg, ret);
            break;}
        case 8:{ /* DISCOVER */
            input_discover(msg, ret);
            break;}
        case 16:{ /* ROUTE */
            input_route(msg, ret);
            break;}
        case 32:{ /* SEND */
            input_send(msg, ret);
            break;}
        case 64: { /* RELAY */
            input_relay(msg, ret);
            break;}
        case 128: {
            input_refresh(msg);
            break;}
    }
}

/* -------------------------------- ACK INPUT ----------------------------------- */
void input_ack(message *msg, int ret) {//,int messagenum){
    int ack_type;
    memcpy(&ack_type, msg->payload + sizeof(int), sizeof(int)); /* save ack_type from msg payload */
    /* if the socket is unknown then the ack is for a connect message!
       therfore add the socket to connected sockets list! */
    if (ack_type == 4) { /* respond to ack of connect message */
        /* when adding a node the general request id is random
           (as opposed to the disconnected node where the general request id is the id of the deleted node) */
        int general_req_id = random();
        node_to_reply[general_req_id] = {-1, -1};
        sockets[msg->src] = ret;
        cout << "ack" << endl;
        cout << msg->src << endl;
        /* save ip & port */
        string ip_and_port;
        int ip_port_len;
        memcpy(&ip_port_len, msg->payload + 2 * sizeof(int), sizeof(int));
        ip_and_port.resize(ip_port_len);
        memcpy((char*)ip_and_port.data(), msg->payload + 3 * sizeof(int), ip_port_len);
        ip_port[msg->src] = ip_and_port;
        std_refresh(general_req_id);
    }
    if (ack_type == 32) { /* respond to ack of send message */
        int general_request_id;
        memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
        if (node_to_reply[general_request_id].first == -1) {
            cout << "ack" << endl;
        } else {
            send_ack(msg);
        }
    }
    if (ack_type == 128) { /* respond to ack of a delete message */
        int general_request_id;
        memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
        nodes_to_request[general_request_id].erase(msg->src); /* remove the current node from neighbors */
        if (!nodes_to_request[general_request_id].empty()) { /* there are more neighbors. keep discovering! */
            send_refresh(general_request_id);
            return;
        } else { /* finished all the neighbors! returning */
            if (node_to_reply[general_request_id].first == -1) {
                node_to_reply.erase(general_request_id);
                return;
            }
            send_ack(msg);
        }
    }
}

/* ------------------------------- NACK INPUT ----------------------------------- */
void input_nack(message* msg, int ret) {
    /* nack type is the function id that the nack returns for */
    int nack_type;
    memcpy(&nack_type, msg->payload+sizeof(int), sizeof(int)); /* save nack_type from msg payload */
    if (nack_type==4) {
        //close(ret);
        cout << "nack" << endl;
        return;
    } else if (nack_type==8) { /* respond to discover message */
        int general_request_id;
        int destination;
        memcpy(&general_request_id, msg->payload+2*sizeof(int), sizeof(int)); /* save general_request_id from msg payload */
        memcpy(&destination, msg->payload+3*sizeof(int), sizeof(int)); /* save destination from msg payload */
        if (checkpoint_waze.count(destination)!=0||waze.count(destination)!=0) { /* found path! */ /* ???????????????????????????????? */
            if (node_to_reply[general_request_id].first==-1) { /* were on source node! */
                waze[destination] = checkpoint_waze[destination]; /* copy temp to permanent list */
                checkpoint_waze.erase(destination); /* we found the route! delete partial routes */
                if (!text[destination].empty()) { /* discover called by send! send relay! */
                   send_relay(destination,general_request_id);
                } else { /* discover called by route! print the path! */
                    int len = waze[destination].size();
                    cout << "ack" << endl;
                    cout << id << "->";
                    for (int i = 0; i < len-1; i++) {
                        cout << waze[destination][i] << "->";
                    }
                    cout << waze[destination][len-1] << endl;
                }
            } else {
                send_route(msg);
                checkpoint_waze.erase(destination); /* we found the route! delete partial routes */
            }
        } else { /* done searching! but didnt find path :( */
            if (node_to_reply[general_request_id].first==-1) { /* were on source node! */
                cout << "nack" << endl;
            } else {
                send_nack(msg);
            }
               
        }
    } else if (nack_type == 64 || nack_type == 32) { /* respond to relay/send message */
        int general_request_id;
        memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
        if (node_to_reply[general_request_id].first == -1) {
            cout << "nack" << endl;
        } else {
            send_nack(msg);
        }
    } else if (nack_type == 128) { /* respond to delete message */
        int general_request_id;
        memcpy(&general_request_id, msg->payload + 2 * sizeof(int),
               sizeof(int)); /* save general_request_id from msg payload */
        nodes_to_request[general_request_id].erase(msg->src); /* remove the current node from neighbors */
        if (nodes_to_request[general_request_id].size() > 0) {/* there are more neighbors. keep discovering! */
            send_refresh(general_request_id);
            return;
        } else { /* done searching! */
            if (node_to_reply[general_request_id].first == -1) {
                node_to_reply.erase(general_request_id);
                return;
            }
            send_nack(msg);
        }
    }
}

/* ------------------------------ CONNECT INPUT --------------------------------- */
void input_connect(message* msg, int ret){
    if (id==INT_MIN) {
        send_nack(msg);
        return;
    }
    if (msg->src==id) {
        cout << "nack" << endl;
        return;
    }
    message rply; /* ack */
    rply.id=random();
    rply.src=id;
    int connect_id = 4;
    memcpy(rply.payload, (char*)&msg->id,sizeof(int));
    memcpy(rply.payload+sizeof(int), &connect_id,sizeof(int));
    rply.trailMSG=0;
    rply.funcID=1;
    rply.dest=msg->src;
    sockets[msg->src] = ret;
    /* get ip & port */
    int ip_port_len;
    string ip_and_port;
    memcpy(&ip_port_len, msg->payload, sizeof(int));
    ip_and_port.resize(ip_port_len);
    memcpy((char*)ip_and_port.data(), msg->payload+sizeof(int) ,ip_port_len);
    ip_port[msg->src] = ip_and_port;
    add_fd_to_monitoring(ret);
    write(ret,&rply,sizeof(rply));
}

/* ------------------------------- SEND INPUT ----------------------------------- */
void input_send(message *msg, int ret) {
    // TODO: In all parts of the send message the node to reply value is the node before the last
    int gen_r_id;
    int node_to_rply;
    int length;
    int trail;
    memcpy(&length, &msg->payload, sizeof(int));
    memcpy(&trail, &msg->trailMSG, sizeof(int));
    memcpy(&gen_r_id, msg->payload + sizeof(int) + 484, sizeof(int));
    memcpy(&node_to_rply, msg->payload + sizeof(int) + 480, sizeof(int));
    node_to_reply[gen_r_id] = {node_to_rply, msg->id};
    string newString;
    newString.resize(length);
    memcpy((char *)newString.data(), msg->payload + sizeof(int), length);
    cout << newString;
    message input_msg;
    while (trail > 0) {
        read(ret, &input_msg, sizeof(input_msg));
        memcpy(&length, &input_msg.payload, sizeof(int));
        string new_string;
        new_string.resize(length);
        memcpy((char*)new_string.data(), input_msg.payload + sizeof(int), length);
        cout << new_string;
        trail--;
    }
    cout << endl;
    send_ack(msg);
}

/* ----------------------------- DISCOVER INPUT --------------------------------- */
void input_discover(message* msg, int ret) {
    /* Note! If discover method was activated so dest is not an neighbor! */
    int destination;
    int general_request_id;
    memcpy(&destination, msg->payload, sizeof(int)); /* save destination from msg payload */
    memcpy(&general_request_id, msg->payload+sizeof(int), sizeof(int)); /* save general_request_id from msg payload */
    /* reached to node in discover process. closed circle! */
    if (nodes_to_request[general_request_id].size()>0) { 
        send_nack(msg);
        return;
    }
    /* update node_to_reply (overwrite if exists) */
    node_to_reply[general_request_id] = {msg->src,msg->id};
    if (sockets.find(destination)!=sockets.end()) { /* if destination is a neighbor we found the node! */
        send_route(msg);
    } else if (sockets.size()==1) { /* leaf! cannot continue discovering! */
        send_nack(msg);
    } else if (waze.find(destination)!=waze.end()) { /* if there is already a route! return it */
        send_route(msg);
        return;
    } else { /* we can continue discovering */
        /* add all neighbors to nodes_to_request & keep discovering forward (to random neighbor) */
        for(auto nei : sockets) { 
            /* if a node sent us discover message we would not want
               to return to this node a discover message */
            if (nei.first!=msg->src) {
                nodes_to_request[general_request_id].insert(nei.first);
            }
        }
        send_discover(destination,general_request_id);
    }
}

/* ------------------------------ ROUTE INPUT ----------------------------------- */
void input_route(message* msg, int ret) {
    int length ,general_request_id;
    memcpy(&general_request_id, msg->payload, sizeof(int));
    memcpy(&length, msg->payload+(1)*sizeof(int), sizeof(int)); /* save path length from msg payload */
    vector<int> way;
    for (int i = 0; i < length; i++) {
        int element;
        memcpy(&element, msg->payload+(2+i)*sizeof(int), sizeof(int));
        way.push_back(element);
    }
    int destination = way[length-1];
    if(checkpoint_waze.count(destination)==0||length<checkpoint_waze[destination].size()){
        checkpoint_waze[destination].clear();
        for(int i=0;i<length;i++){
            checkpoint_waze[destination].push_back(way[i]);
        }
    } else if (length==checkpoint_waze[destination].size()) { // there is a path
        //if (accumulate(way.begin(),way.end(),0)<accumulate(checkpoint_waze[destination].begin(),checkpoint_waze[destination].end(), 0)) {
        for (int j = 0; j < length; j++) { /* lexicographic order */
            if (way[j]<checkpoint_waze[destination][j]) {
                checkpoint_waze[destination].clear();
                for (int i = 0; i < length; i++) {
                    checkpoint_waze[destination].push_back(way[i]);
                }
                break;
            }
        }
    }
    nodes_to_request[general_request_id].erase(msg->src); /* remove the current node from neighbors */
    if (nodes_to_request[general_request_id].size()>0) { /* there are more neighbors. keep discovering! */
        send_discover(destination,general_request_id);
        return;
    } else { /* done discovering */
        if (node_to_reply[general_request_id].first==-1) { /* were on source node! */
            waze[destination] = checkpoint_waze[destination];
            checkpoint_waze.erase(destination); /* we found the route! delete partial routes */
            /* there is text to send so discover called by relay! call relay! */
            if (!text[destination].empty()) {
                send_relay(destination,general_request_id);
            } else { /* discover called by route! print the path! */
                cout << "ack" << endl;
                int len = waze[destination].size();
                cout << id << "->";
                for (int i = 0; i < len-1; i++) {
                    cout << waze[destination][i] << "->";
                }
                cout << waze[destination][len-1] << endl;
            }
        } else { /* were on inner node. should return route to prev node */
            send_route(msg);
            checkpoint_waze.erase(destination);
        }
    }
}

/* ------------------------------ RELAY INPUT ----------------------------------- */
void input_relay(message *msg, int ret) {
    int trail, dest, src, general_request_id;
    memcpy(&trail, &msg->trailMSG, sizeof(int));
    memcpy(&dest, &msg->payload, sizeof(int));
    if (sockets.find(dest) == sockets.end()) {
        send_nack(msg);
        return;
    }
    memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
    memcpy(&src, &msg->src, sizeof(int));
    node_to_reply[general_request_id] = {msg->src, msg->id};
    char pipe[512 * (trail)];
    read(ret, pipe, sizeof(pipe));
    write(sockets[dest], &pipe, sizeof(pipe));
}

/* ------------------------------ DELETE INPUT ----------------------------------- */
void input_refresh(message *msg) {
    if (!waze.empty()) {
        waze.clear();
    }
    int general_request_id;
    memcpy(&general_request_id, msg->payload,sizeof(int)); /* save general_request_id from msg payload */
    if (nodes_to_request[general_request_id].size() > 0) { /* closed circle! return ack */
        send_ack(msg);
        return;
    }
    /* update node_to_reply (overwrite if exists) */
    node_to_reply[general_request_id] = {msg->src, msg->id};
    if (sockets.size() == 1) { /* leaf! cannot continue discovering! */
        send_ack(msg);//TODO: change send_ack to include 128
        node_to_reply.erase(general_request_id);
    } else { /* we can continue discovering */
        /* add all neighbors to nodes_to_request & keep discovering forward (to random neighbor) */
        for (auto nei : sockets) {
            /* if a node sent us discover message we would not want
               to return to this node a discover message */
            if (nei.first != msg->src) {
                nodes_to_request[general_request_id].insert(nei.first);
            }
        }
        send_refresh(general_request_id);
    }
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// GLOBAL METHODS //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/* ------------------------------- SEND ROUTE ------------------------------------ */
void send_route(message *msg) {
    int general_request_id, destination, length;
    message rply;
    rply.id = random();
    rply.src = id;
    rply.funcID = 16;
    rply.trailMSG = 0;
    if (msg->funcID == 8) { /* need to respond to discover message */
        memcpy(&destination, msg->payload, sizeof(int));
        memcpy(&general_request_id, msg->payload + sizeof(int), sizeof(int));
        /* if a route message responds to a discover message there are 2 conditions:
            1. We found the destination now! return message!
            2. We did not find the destionation but we reached a node that has a route to it.
               therefore, skip to the continuation of the function and chain the route. */
        if(sockets.find(destination)!=sockets.end()) {
            int path_length = 2;
            rply.dest = node_to_reply[general_request_id].first;
            memcpy(rply.payload, &general_request_id, sizeof(int));
            memcpy(rply.payload + sizeof(int), &path_length, sizeof(int));
            memcpy(rply.payload + 2*sizeof(int), (&id), sizeof(int));
            memcpy(rply.payload + 3*sizeof(int), &destination, sizeof(int));
            write(sockets[rply.dest], &rply, sizeof(rply));
            return;
        }
    } else if (msg->funcID == 2) { /* need to respond to nack message */
        memcpy(&general_request_id, msg->payload + 3 * sizeof(int), sizeof(int));//get general_request_id
        memcpy(&destination, msg->payload + 4 * sizeof(int), sizeof(int));//get destination
    } else if (msg->funcID == 16) { /* need to respond to route message */
        memcpy(&general_request_id, msg->payload, sizeof(int));
        memcpy(&length, msg->payload + sizeof(int), sizeof(int));
        memcpy(&destination, msg->payload + (2 + length - 1) * sizeof(int),
               sizeof(int));//destination is the last in the path
    }
    vector<int>::iterator it;
    /* found the continuation of the route at an inner vertex! return it from waze */
    if(waze.find(destination)==waze.end()) {
        length = checkpoint_waze[destination].size() +1;//the path exists beacuse it was added in route function,the +1 is the current node
        it = checkpoint_waze[destination].begin();
    } else {
        length = waze[destination].size() +1;//the path exists beacuse it was added in route function,the +1 is the current node
        it = waze[destination].begin();
    }
    memcpy(rply.payload, &general_request_id, sizeof(int)); /* adding original id to the payload */
    memcpy(rply.payload + sizeof(int), &length, sizeof(int)); /* adding length to the payload */
    memcpy(rply.payload + 2 * sizeof(int), &id, sizeof(int)); /* adding current node to path */
    rply.dest = node_to_reply[general_request_id].first;
    int node_id;
    for (int i = 0; i < length - 1; ++i) {
        node_id = *it++;
        memcpy(rply.payload + (i + 3) * sizeof(int), &node_id, sizeof(int));
    }
    write(sockets[rply.dest], &rply, sizeof(rply));
}

/* ------------------------------- SEND NACK ------------------------------------ */
void send_nack(message* msg, int ret) {
    int destination, general_request_id;
    message rply; /* nack */
    rply.id=random();
    rply.src=id;
    rply.dest = msg->src;
    rply.trailMSG=0;
    rply.funcID=2;
    if (msg->funcID==4) { /* respond to connect message */
        /* id = INT_MIN. return nack! */
        memcpy(rply.payload, &msg->id, sizeof(int));
        memcpy(rply.payload+sizeof(int), &msg->funcID, sizeof(int));
        write(ret,&rply,sizeof(rply));
        close(ret);
        return;
    } else if (msg->funcID==2) { /* respond to nack message */
        int nack_type;
        memcpy(&nack_type, msg->payload+sizeof(int), sizeof(int));
        /* read and write data to payload in accordance to the nack type */
        if (nack_type==8) { /* the nack message we received responds to discover message */
            memcpy(&general_request_id, msg->payload+2*sizeof(int), sizeof(int));
            memcpy(&destination, msg->payload+3*sizeof(int), sizeof(int));
            int prev_node = node_to_reply[general_request_id].first;
            rply.dest = prev_node;
            /* write to the new message */
            memcpy(rply.payload, &node_to_reply[general_request_id].second,sizeof(int));
            memcpy(rply.payload+1*sizeof(int), &nack_type,sizeof(int));
            memcpy(rply.payload+2*sizeof(int), &general_request_id,sizeof(int));
            memcpy(rply.payload+3*sizeof(int), &destination,sizeof(int));
        } else if (nack_type == 64) { /* the nack message we received responds to relay message */
             int message_type;
            memcpy(&message_type,msg->payload+sizeof(int),sizeof(int));
            memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
            memcpy(rply.payload, &node_to_reply[general_request_id].second,sizeof(int));//adding nack on what message
            memcpy(rply.payload + sizeof(int), &message_type, sizeof(int));//adding the message type
            memcpy(rply.payload + 2 * sizeof(int), &general_request_id,sizeof(int));//adding the general request id
            rply.dest = node_to_reply[general_request_id].first;
        } else if (nack_type == 128) { /* the nack message we received responds to delete message */
            memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
            //memcpy(&destination, msg->payload + 3 * sizeof(int), sizeof(int));
            int prev_node = node_to_reply[general_request_id].first;
            rply.dest = prev_node;
            /* write to the new message */
            //memcpy(rply.payload, &prev_node, sizeof(int));
            memcpy(rply.payload, &node_to_reply[general_request_id].second,sizeof(int));
            memcpy(rply.payload + 1 * sizeof(int), &nack_type, sizeof(int));
            memcpy(rply.payload + 2 * sizeof(int), &general_request_id, sizeof(int));
        }
    } else if (msg->funcID==8) { /* respond to discover message */
        int nack_type = 8;
        memcpy(&destination, msg->payload, sizeof(int));
        memcpy(&general_request_id, msg->payload+sizeof(int), sizeof(int));
        rply.dest = msg->src;
        /* write to the new message */
        memcpy(rply.payload, &rply.dest,sizeof(int));
        memcpy(rply.payload+1*sizeof(int), &nack_type,sizeof(int));
        memcpy(rply.payload+2*sizeof(int), &general_request_id,sizeof(int));
        memcpy(rply.payload+3*sizeof(int), &destination,sizeof(int));
    } else if (msg->funcID==64) { /* respond to  relay message */
        memcpy(&general_request_id, msg->payload + 2 * sizeof(int), sizeof(int));
        memcpy(rply.payload, &node_to_reply[general_request_id].second, sizeof(int));//writing the last msg id
        memcpy(rply.payload + sizeof(int), (void *) 64, sizeof(int));//writing func id
        memcpy(rply.payload + 2 * sizeof(int), &general_request_id, sizeof(int));//writing the gen_r_id
        rply.dest = node_to_reply[general_request_id].first;
    } else if (msg->funcID == 128) { /* respond to delete message */
        memcpy(&general_request_id, msg->payload, sizeof(int));
        rply.dest = msg->src;
        /* write to the new message */
        memcpy(rply.payload, &msg->id, sizeof(int));
        memcpy(rply.payload + 1 * sizeof(int), &msg->funcID, sizeof(int));
        memcpy(rply.payload + 2 * sizeof(int), &general_request_id, sizeof(int));
    }
    write(sockets[rply.dest],&rply,sizeof(rply));
}

/* ------------------------------- SEND ACK ------------------------------------ */
void send_ack(message *msg) {
    int gen_r_id;
    message reply;
    reply.id = random();
    reply.src = id;
    reply.funcID = 1;
    reply.trailMSG = 0;
    if (msg->funcID == 4) { /* connect */
            /* send ip & port */
            string ip_and_port = get_ip_port();
            int ip_port_len = ip_and_port.size();
            int connect_type = 4;
            memcpy(reply.payload, &msg->id, sizeof(int));//writing the last msg id
            memcpy(reply.payload + sizeof(int), &connect_type, sizeof(int));//writing func id
            memcpy(reply.payload + 2*sizeof(int), &ip_port_len ,sizeof(int));
            memcpy(reply.payload + 3*sizeof(int), ip_and_port.c_str(), ip_port_len);
    } else if (msg->funcID == 32) { /* send */
        int length;
        memcpy(&length, msg->payload, sizeof(int));
        memcpy(&gen_r_id, msg->payload + sizeof(int) + 484, sizeof(int));
        memcpy(reply.payload, &node_to_reply[gen_r_id].second, sizeof(int));//writing the last msg id
        memcpy(reply.payload + sizeof(int), &msg->funcID, sizeof(int));//writing func id
        memcpy(reply.payload + 2 * sizeof(int), &gen_r_id, sizeof(int));//writing the gen_r_id
        // if (sockets.count(msg->src)!=0) reply.dest = msg->src;
        reply.dest = node_to_reply[gen_r_id].first; // didnt saved it! junk data!
    } else if (msg->funcID == 128) {
        memcpy(&gen_r_id, msg->payload, sizeof(int));
        memcpy(reply.payload, &node_to_reply[gen_r_id].second, sizeof(int));
        memcpy(reply.payload + sizeof(int), &msg->funcID, sizeof(int));
        memcpy(reply.payload + 2 * sizeof(int), &gen_r_id, sizeof(int));
        if (nodes_to_request[gen_r_id].size() > 0) { /* if we closed circle return the ack to msg->src */
            reply.dest = msg->src;
        } else {
            reply.dest = node_to_reply[gen_r_id].first;
        }
    } else if (msg->funcID == 1) { /* ack */
        int message_type;
        memcpy(&message_type, msg->payload + sizeof(int), sizeof(int));
        if (message_type == 32) { /* ack to send */
            memcpy(&gen_r_id, msg->payload + 2 * sizeof(int), sizeof(int));
            memcpy(reply.payload, &node_to_reply[gen_r_id].second, sizeof(int));
            memcpy(reply.payload + sizeof(int), &message_type, sizeof(int));
            memcpy(reply.payload + 2 * sizeof(int), &gen_r_id, sizeof(int));
            reply.dest = node_to_reply[gen_r_id].first;
        } else if (message_type == 128) { /* ack to delete */
            memcpy(&gen_r_id, msg->payload + 2 * sizeof(int), sizeof(int));
            memcpy(reply.payload, &node_to_reply[gen_r_id].second, sizeof(int));
            memcpy(reply.payload + sizeof(int), &message_type, sizeof(int));
            memcpy(reply.payload + 2 * sizeof(int), &gen_r_id, sizeof(int));
            if (nodes_to_request[gen_r_id].empty()) {
                reply.dest = node_to_reply[gen_r_id].first;
                node_to_reply.erase(gen_r_id);
            } else {
                reply.dest = *nodes_to_request[gen_r_id].begin();
            }
        }
    }
    write(sockets[reply.dest], &reply, sizeof(reply));
}

/* ----------------------------- SEND DISCOVER ---------------------------------- */
void send_discover(int dst, int general_request_id) { /* first discover from the terminal */
    message outgoing;
    outgoing.id = rand();
    int first_nei;
    /* already setted node_to_reply[outgoing.id] = {-1, -1} outside this method */
    first_nei = *nodes_to_request[general_request_id].begin();
    /* add discover id to payload */
    memcpy(outgoing.payload+sizeof(int), &general_request_id, sizeof(int));
    outgoing.src = id;
    outgoing.dest = first_nei;
    outgoing.trailMSG = 0;
    outgoing.funcID = 8; /* discover function id is 8 */
    memcpy(outgoing.payload, &dst, sizeof(int)); /* set the payload */
    char outgoing_buffer[512];
    memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
    send(sockets[first_nei], outgoing_buffer, sizeof(outgoing_buffer), 0);
}

/* ------------------------------ SEND RELAY ----------------------------------- */
void send_relay(int destination, int original_id) {
    if (sockets.find(waze[destination][0]) == sockets.end()) {
        cout << "nack" << endl;
        return;
    }
    int length = waze[destination].size(); 
    message relays;
    relays.funcID = 64;
    int msg_id = random(); /* general request id since we call send_relay once */
    node_to_reply[msg_id] = {-1, -1};
    int txt_length;
    txt_length = strlen(text[destination].c_str());
    int num_of_messages = ceil(txt_length / 480.0)-1;//number of send messages after first send message
    char pipe[(512 * (length+num_of_messages))];
    /* starting from 1 to length-1.
       from 1 because the first message is read by the first node to which it is sent
       to length-1 because the last message is "send" message and not "relay" message */
    for (int i = 1; i <= length - 1; ++i) {
        relays.id = msg_id + i;
        if (i==1) relays.src = id; /* the source of the first relay message is the source itdelf */
        else relays.src = waze[destination][i-2];
        relays.dest = waze[destination][i]; /* assuming node i-1. prev = i-2. next = i */
        int len = length - i + num_of_messages;
        relays.trailMSG = len;
        memcpy(relays.payload, &waze[destination][i], sizeof(int));
        memcpy(relays.payload + sizeof(int), &len, sizeof(int));
        memcpy(relays.payload + 2 * sizeof(int), &msg_id, sizeof(int));
        memcpy(pipe + ((i - 1) * sizeof(relays)), &relays, sizeof(relays));
    }
    string txt_to_send = text[destination];
    message msg;
    msg.src = id;
    msg.dest = destination;
    int text_len = 480;
    /* "for" loop for writing all parts of send messages */
    for (int i = 0; i < num_of_messages+1; i++) {
        msg.funcID = 32;
        /* if destination is a neighbor node_to_reply = current node */
        if (sockets.find(destination) != sockets.end()) {memcpy(msg.payload + 484, &id, sizeof(int));
        /* else, the route>=2 so node_to_reply = waze[destination][length-2] */
        } else {memcpy(msg.payload + 484, &waze[destination][length-2], sizeof(int));} /* last node before dest */
        /* build the messages depending on their length */
        if (i != num_of_messages) {//if i is not the last message put 480 in the message length
            memcpy(msg.payload + sizeof(int), txt_to_send.substr(i * 480,  480).c_str(), 480);
            memcpy(msg.payload, &text_len, sizeof(int));
            memcpy(msg.payload + 484 + sizeof(int), &msg_id, sizeof(int)); /* general request id */
        } else { /* if i is the last message put the remaining chars into the message */
            int remaininglen=txt_length%480!=0?txt_length%480:480;
            memcpy(msg.payload + sizeof(int), txt_to_send.substr(i * 480, remaininglen).c_str(), remaininglen);
            memcpy(msg.payload, &remaininglen, sizeof(int));
            memcpy(msg.payload + 484 + sizeof(int), &msg_id, sizeof(int)); /* general request id */
        }
        msg.trailMSG = num_of_messages-i;
        memcpy(pipe + (i + length - 1) * sizeof(message), &msg, sizeof(msg));
    }
    int dest = waze[destination][0];
    text[destination].erase(); /* we wrote the text to the message! remove the text from data structure */
    write(sockets[dest], &pipe, sizeof(pipe));
}

/* ------------------------------ SEND REFRESH ----------------------------------- */
void send_refresh(int general_request_id) {
    message del_msg;
    del_msg.id = random();
    del_msg.src = id;
    int first_nei;
    first_nei = *nodes_to_request[general_request_id].begin();
    del_msg.dest = first_nei;
    del_msg.trailMSG = 0;
    del_msg.funcID = 128; /* discover function id is 128 */
    memcpy(del_msg.payload, &general_request_id, sizeof(int));
    write(sockets[first_nei], &del_msg, sizeof(del_msg));
}

string message_type(message* msg) {
    //int func_id = *(int*)(&msg->funcID);
    int func_id;
    memcpy(&func_id, &msg->funcID, sizeof(int));
    if (func_id==1) return "Ack";
    else if (func_id==2) return "Nack";
    else if (func_id==4) return "Connect";
    else if (func_id==8) return "Discover";
    else if (func_id==16) return "Route";
    else if (func_id==32) return "Send";
    else if (func_id==64) return "Relay";
    else if (func_id==128) return "Delete";
    return "(Can not identify)";
}

void print_ip_info(int r_port) {
    printf("---------------------------------\n");
    cout << "USE THE COMMAND: connect,192.168.190.129:X" << endl;
    /* Print my ip */
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&inet_addr;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    printf("MY IP: %s\n", str);
    /* Print my port */
    printf("MY PORT: %d\n", r_port);
    printf("---------------------------------\n");
}

string get_ip_port() {
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&inet_addr;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char my_ip[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, my_ip, INET_ADDRSTRLEN );
    stringstream ss;
    ss << string(my_ip) << ":" << r_port;
    return ss.str();
}