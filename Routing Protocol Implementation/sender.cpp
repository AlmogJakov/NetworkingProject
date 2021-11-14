#include "node.hpp"

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// SEND METHODS ///////////////////////////////////
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