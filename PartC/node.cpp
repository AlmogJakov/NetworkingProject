#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <netdb.h> /* addrinfo */

#include <set>
#include <list>
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "select.hpp"
#include "node.hpp"

/* my id */
int id;
/* key - neighbor id. value - socket. */
/* Note! to get the neighbors we can iterate thru this map keys. */
unordered_map<int,const unsigned int> sockets;
unordered_map<int,int> msgs;

/* save path to each node */
unordered_map<int,set<int>> my_stack; 
/* save path to each node */
unordered_map<int,list<int>> waze; 

// /* key - id of discover. value - distance till this node */
// unordered_map<int,int> discover_distance;
/* key - id of discover. value - destination to doscover */
unordered_map<int,int> discover_to_dest;
unordered_map<int,int> node_to_reply;
// /* key - id of discover. value - set of nodes that sent to us discover message */
// unordered_map<int,set<int>> discover_input; 
// /* key - id of discover. value - set of nodes that we sent to them discover message */
// unordered_map<int,set<int>> discover_output;
// /* key - id of discover. value - set of nodes that returned us an answer for the discover message */
// unordered_map<int,int> discover_num_of_answers;
// /* if discover_input.size() + discover_num_of_answers.size() == neighbors size
//    so we done handling the current vertex and we can return an answer */



//message * main_msg;
int main(int argc, char *argv[]) {
    int innerfd = 0, outerfd=0;
    struct sockaddr_in serv_addr; 
    int ret, i;
    int r_port;
    printf("Please choose a port: ");
    scanf("%d", &r_port);
    char buff[1025];
    // time_t ticks;
    innerfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(r_port);
    bind(innerfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("adding fd(%d) to monitoring\n", innerfd);
    add_fd_to_monitoring(innerfd);
    listen(innerfd, 10);
    printf("---------------------------------\n");
    cout << "USE THE COMMAND: connect,192.168.190.129:12350" << endl;
    /* Print my ip */
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&inet_addr;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    printf("MY IP: %s\n", str);
    /* Print my port */
    printf("MY PORTS: %d\n", r_port);
    printf("---------------------------------\n");

    while(true){
        memset(&buff, '\0', sizeof(buff));
	    printf("waiting for input...\n");
	    ret = wait_for_input();
	    printf("fd: %d is ready. reading...\n", ret);
        /* At the file descriptor level, stdin is defined to be file descriptor 0,
           stdout is defined to be file descriptor 1,
           and stderr is defined to be file descriptor 2. */
        if (ret < 2) { // std input
            read(ret, buff, 1025);
            if (buff[strlen(buff)-1]=='\n') buff[strlen(buff)-1] = '\0';
            stringstream ss;
            ss << buff;
            string splited[4]; // split std input
            getline(ss,splited[0],',');
            if (splited[0].compare("setid")==0) {std_setid(ss,splited);}
            else if (splited[0].compare("connect")==0) {std_connect(ss,splited);}
            else if (splited[0].compare("send")==0) {std_send(ss,splited);}
        } else { /* we got a packet */
            int addrlen;
            if (ret==innerfd) { /* if ret==innerfd then we got a new connection! */
                if ((ret = accept(ret, (struct sockaddr *)&serv_addr, (socklen_t*)&addrlen))<0) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
            }
            /* get the message to struct */
            message* incoming = (message*)malloc(sizeof(message));
            read(ret ,incoming, 512);
            /* print message type */
            cout << "\033[1;36m"; /* print in color */
            cout << "Got " << message_type(incoming) << " message type!" << endl;
            cout << "\033[0m"; /* end print in color */
            /* handle the message */
            gotmsg(incoming, ret);
            delete(incoming);
        }
	}
}

////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// STD INPUT METHODS /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/* --------------------------------- SETID -------------------------------------- */
void std_setid(stringstream& ss,string splited[]) {
    getline(ss,splited[1],','); /* id */
    id = stoi(splited[1]);
    cout << "\033[1;36m"; /* print in color */
    cout << "MY ID: " << id << endl;
    cout << "\033[0m"; /* end print in color */
}

/* -------------------------------- CONNECT ------------------------------------- */
void std_connect(stringstream& ss,string splited[]) {
    getline(ss,splited[1],':'); /* ip */
    getline(ss,splited[2],':'); /* port */
    uint16_t port = stoul(splited[2]);
    char const* destip = splited[1].c_str();
    struct sockaddr_in destAddress;
    int new_sock = socket(AF_INET, SOCK_STREAM, 0);
    bind(new_sock, (struct sockaddr*)&destAddress, sizeof(destAddress));
    memset(&destAddress, 0, sizeof(destAddress));
	destAddress.sin_family = AF_INET;
    destAddress.sin_port = htons(port);
    if(inet_pton(AF_INET, destip, &destAddress.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return;}
    if (connect(new_sock, (struct sockaddr*)&destAddress, sizeof(destAddress)) < 0) {
        printf("\nConnection Failed \n");
        return;}
    add_fd_to_monitoring(new_sock);
    message outgoing;
    outgoing.id = rand();
    outgoing.src = id;
    outgoing.dest = 0;
    outgoing.trailMSG = 0;
    outgoing.funcID = 4; /* connect function id is 4 */
    char outgoing_buffer[512];
    memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
    send(new_sock, outgoing_buffer, sizeof(outgoing_buffer), 0);
}

/* ---------------------------------- SEND -------------------------------------- */
void std_send(stringstream& ss,string splited[]) {
    getline(ss,splited[1],','); /* destination */
    getline(ss,splited[2],','); /* message length */
    getline(ss,splited[3],','); /* message itself */
    if (sockets.find(stoi(splited[1]))!=sockets.end()) { /* connected directly. send the message! */
        message outgoing;
        outgoing.id = rand();
        outgoing.src = id;
        outgoing.dest = stoi(splited[1]);
        outgoing.trailMSG = 0;
        outgoing.funcID = 32; /* send function id is 32 */
        memcpy(outgoing.payload, splited[3].c_str(), stoi(splited[2])); /* set the payload */
        char outgoing_buffer[512];
        memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
        send(sockets.at(outgoing.dest), outgoing_buffer, sizeof(outgoing_buffer), 0);
    } else { /* not connected directly. we need to discover/relay&send. */
        if(waze.find(stoi(splited[1]))==waze.end()){ /* no path! lets discover */
            send_discover(stoi(splited[1]));}
        else {

        }
    }
}

/*
~   ~   ~   ~   ~   ~   ~   ~   DISCOVER HEADER   ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |               PAYLOAD                |
|---------------------------------------------------------------------------------------|
|                                                | DST | DISCOVER ID | DISTANCE         |
|_______________________________________________________________________________________|
*/
void send_discover(int dst) {
    message outgoing;
    outgoing.id = rand();
    outgoing.src = id;
    outgoing.dest = dst;
    outgoing.trailMSG = 0;
    outgoing.funcID = 8; /* discover function id is 8 */
    int init_distance = 0;
    memcpy(outgoing.payload, &dst, sizeof(int)); /* set the payload */
    memcpy(outgoing.payload+sizeof(int), &outgoing.id, sizeof(int)); /* add discover id to payload */
    char outgoing_buffer[512];
    memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
    for(auto nei : sockets) {
        my_stack[outgoing.id].insert(nei.first);
    }
    int first_nei = *my_stack[outgoing.id].begin();
    my_stack[outgoing.id].erase(first_nei);
    cout << "discovering to " << first_nei << endl;
    send(sockets[first_nei], outgoing_buffer, sizeof(outgoing_buffer), 0);
    discover_to_dest.insert({outgoing.id,dst});
}

////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// EXTERNAL SOCKETS METHODS /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void gotmsg(message* msg, int ret){
    switch(msg->funcID){
        case 1:{ /* ACK */
            ack(msg,ret);
            break;}
        case 2:{ /* NACK */
            nack(msg, ret);
            break;}
       case 4:{ /* CONNECT */
           cnct(msg, ret);
           break;}
        case 8:{ /* DISCOVER */
            discover(msg, ret);
            break;}
        case 16:{ /* ROUTE */
            break;}
        case 32:{ /* SEND */
            Send(msg, ret);
            break;}
        case 64: { /* RELAY */
            break;}
    }
}

/* ----------------------------------- ACK -------------------------------------- */
void ack(message* msg, int ret){//,int messagenum){
    /* if the socket is unknown then the ack is for a connect message!
       therfore add the socket to connected sockets list! */
    if (sockets.count(msg->src)==0) {
        sockets.insert({msg->src,ret});
    }
}

/* ---------------------------------- NACK -------------------------------------- */
void nack(message* msg, int ret) {
    /* nack type is the function id that the nack returns for */
    int nack_type;
    memcpy(&nack_type, msg->payload+sizeof(int), sizeof(int)); /* save nack_type from msg payload */
    if (nack_type==8) { /* got nack for discover function! */
        int discover_id;
        memcpy(&discover_id, msg->payload+2*sizeof(int), sizeof(int)); /* save discover_id from msg payload */
        int destination = discover_to_dest[discover_id];
        my_stack[discover_id].erase(msg->src);
        if (my_stack[discover_id].size()>0) { /* can discover more! */
            int first_nei = *my_stack[discover_id].begin();
            cout << "got nack. keep discovering to " << first_nei << endl;
            message dis; /* discovering forward */
            dis.id=random();
            dis.src=id;
            dis.dest=first_nei;
            memcpy(dis.payload, &destination,sizeof(int));
            memcpy(dis.payload+sizeof(int), &discover_id,sizeof(int));
            dis.trailMSG=0;
            dis.funcID=8;
            write(sockets[first_nei],&dis,sizeof(dis));
        } else {
            if (waze.count(discover_id)!=0) {
                cout << "done searching! should return route!" << endl;
            } else {
                cout << "done searching! should return nack!" << endl;
            }
        }
    }
}

/* --------------------------------- CONNECT ------------------------------------ */
void cnct(message* msg, int ret){
    message rply; /* ack */
    rply.id=random();
    rply.src=id;
    memcpy(rply.payload, (char*)&msg->id,sizeof(int));
    rply.trailMSG=0;
    rply.funcID=1;
    //sockets[msg->src] = ret;
    sockets.insert({msg->src,ret});
    add_fd_to_monitoring(ret);
    write(ret,&rply,sizeof(rply));
}

/* ---------------------------------- SEND -------------------------------------- */
void Send(message* msg, int ret) {
    cout << msg->payload << endl;
    message rply; /* ack */
    rply.id=random();
    rply.src=id;
    rply.dest=msg->src;
    memcpy(rply.payload, (char*)&msg->id,sizeof(int));
    rply.trailMSG=0;
    rply.funcID=1;
    write(ret,&rply,sizeof(rply));
}

/* -------------------------------- DISCOVER ------------------------------------ */
/*
~   ~   ~   ~   ~   ~   ~   ~   DISCOVER HEADER   ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |               PAYLOAD                |
|---------------------------------------------------------------------------------------|
|                                                | DST | DISCOVER ID | DISTANCE         |
|_______________________________________________________________________________________|

~   ~   ~   ~   ~   ~   ~   ~ DISCOVER NACK HEADER ~   ~   ~   ~   ~   ~   ~   ~
 _______________________________________________________________________________________________
| MSG ID | SRC ID | DST ID | TRAIL MSG | FUNC ID |                   PAYLOAD                    |
|-----------------------------------------------------------------------------------------------|
|                                                | LAST MSG ID | DISCOVER FUNC ID | DISCOVER ID |
|_______________________________________________________________________________________________|
*/
void discover(message* msg, int ret) {
    /* Note! If discover method was activated so dest is not an neighbor! */
    int destination;
    int discover_id;
    memcpy(&destination, msg->payload, sizeof(int)); /* save destination from msg payload */
    memcpy(&discover_id, msg->payload+sizeof(int), sizeof(int)); /* save discover_id from msg payload */
    discover_to_dest.insert({discover_id,destination});
    node_to_reply.insert({discover_id,msg->src});
    if (sockets.find(destination)!=sockets.end()) { /* if destination is a neighbor we found the node! */
        cout << "found the socket " << destination << "! return route" << endl;
    } else if (my_stack[discover_id].size()>0) { /* circle! cannot continue discovering! */
        cout <<  msg->src <<" send to me (my id:" << id << ") and closed circle! return nack" << endl;
        message rply; /* nack */
        rply.id=random();
        rply.src=id;
        rply.dest=msg->src;
        int discover_function_id = 8;
        memcpy(rply.payload, &msg->id,sizeof(int));
        memcpy(rply.payload+sizeof(int), &discover_function_id,sizeof(int));
        memcpy(rply.payload+2*sizeof(int), &discover_id,sizeof(int));
        rply.trailMSG=0;
        rply.funcID=2;
        write(ret,&rply,sizeof(rply));
    } else { /* we can continue discovering */
        cout << "continue discovering" << endl;
            /* if a node sent us discover message we would not want
               to return to this node an discover message */
            for(auto nei : sockets) {
                if (nei.first!=msg->src) {my_stack[discover_id].insert(nei.first);}
            }
            int first_nei = *my_stack[discover_id].begin();
            my_stack[discover_id].erase(first_nei);
            cout << "discovering to " << first_nei << endl;
            message dis; /* discovering forward */
            dis.id=random();
            dis.src=id;
            dis.dest=first_nei;
            memcpy(dis.payload, &destination,sizeof(int));
            memcpy(dis.payload+sizeof(int), &discover_id,sizeof(int));
            dis.trailMSG=0;
            dis.funcID=8;
            write(sockets[first_nei],&dis,sizeof(dis));
    }
};
/* --------------------------------- ROUTE -------------------------------------- */
void route(int message_num,int length,int * way){}
/* --------------------------------- RELAY -------------------------------------- */
void relay(int message_num){};

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// GLOBAL METHODS //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

string message_type(message* msg) {
    if (msg->funcID==1) return "Ack";
    else if (msg->funcID==2) return "Nack";
    else if (msg->funcID==4) return "Connect";
    else if (msg->funcID==8) return "Discover";
    else if (msg->funcID==16) return "Route";
    else if (msg->funcID==32) return "Send";
    else if (msg->funcID==64) return "Relay";
    return "(Can not identify)";
}

void send_nack(message * msg){}
void send_ack(message * msg){
    auto* rply=new message;
    rply->id=random();
    rply->src=id;
    rply->dest=msg->src;
    rply->trailMSG=msg->trailMSG==0?0:msg->trailMSG-1;
    rply->funcID=1;
    memcpy(rply->payload, (char*)&msg->id,sizeof(int));
    write(sockets.at(msg->src),&rply,sizeof(rply));
    delete rply;
    //rply->payload=(char*)msg->id;
}