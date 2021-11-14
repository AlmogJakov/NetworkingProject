#include "node.hpp"

////////////////////////////////////////////////////////////////////////////////////
////////////////////////// EXTERNAL SOCKETS INPUT METHODS //////////////////////////
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
    /* if the id is not defined */
    if (id==INT_MIN) {
        send_nack(msg);
        return;
    }
    /* since assuming that each node has a different id,
       if msg->src==id then sending the message is from node to itself */
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