#include "node.hpp"

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
    /* if id not defined */
    if (id==INT_MIN) {
        cout << "nack" << endl;
        return;
    }
    if (splited[2].at(splited[2].size()-1)=='\n') {splited[2].pop_back();}
    //getline(ss,splited[3],','); /* message itself */
    splited[3].resize(stoi(splited[2]));
    getline(ss, splited[3]); /* message itself */
    /* send from node to itself */
    if (stoi(splited[1])==id) {
        cout << splited[3] << endl;
        cout << "ack" << endl;
        return;
    }
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