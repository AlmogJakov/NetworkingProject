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
#include <netdb.h> // addrinfo
#include <list>
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <numeric>
#include "select.hpp"
#include "node.hpp"

int id; /* my id */
unordered_map<int,const unsigned int> sockets;
unordered_map<int,int> msgs;
unordered_map<int,list<int>> waze;
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
    cout << "MY LOCAL IP: " << "192.168.190.129" << endl;
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
//    if(waze.find(splited[1])==waze.end()){
//        send_discover();
//    }
//
//    for(int i = 0; i < waze[splited[1]].size(); ++i) {
//
//    }
    message outgoing;
    outgoing.id = rand();
    outgoing.src = id;
    outgoing.dest = stoi(splited[1]);
    outgoing.trailMSG = waze[splited[1]].size();
    outgoing.funcID = 32; /* send function id is 32 */
    memcpy(outgoing.payload, splited[3].c_str(), stoi(splited[2])); /* set the payload */
    char outgoing_buffer[512];
    memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
    send(sockets.at(outgoing.dest), outgoing_buffer, sizeof(outgoing_buffer), 0);
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
            nack(msg->id);
            break;}
       case 4:{ /* CONNECT */
           cnct(msg, ret);
           break;}
        case 8:{ /* DISCOVER */
            discover(ret,msg->dest);
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

            break;
        }
        case 64:{break;}
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
void nack(int messagenum){
    int msgid=5;
    message ack;
    ack.id=msgid;
    ack.src=id;
}

/* --------------------------------- CONNECT ------------------------------------ */
void cnct(message* msg, int ret){
    message rply; /* ack */
    rply.id=random();
    rply.src=id;
    memcpy(rply.payload, (char*)&msg->id,sizeof(int));
    rply.trailMSG=0;
    rply.funcID=1;
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
void discover(int ret,message * msg){
    int dest,num_sent,init_id;
    message * reply;
    memcpy(&dest,msg->payload,sizeof(int));
    msgs.emplace(msg->id,msg->src);
    memcpy(&init_id,msg->payload+2*sizeof(int),sizeof(int));
    memcpy(&num_sent,msg->payload+sizeof(int),sizeof(int));
    msgs.emplace({init_id,msg->src});
    list<int> visited;
    read(ret,&visited,(sizeof(int)*num_sent))
    if(sockets.find(dest)!=sockets.end()){
        reply->id=random();
        reply->src=id;
        reply->funcID=16;
        reply->dest=msg->src;
        memcpy(&reply->payload, &msg->id, sizeof(int));
        //reply->payload[5]=2;
        memcpy(reply->payload + sizeof(int), (char*)2, sizeof(int));
        memcpy(reply->payload + 2*sizeof(int), (char*)&id, sizeof(int));
        memcpy(reply->payload + 3*sizeof(int), (char*)&dest, sizeof(int));
        write(sockets.at(reply->dest),&reply,sizeof(&reply));
        //wait_for_input();
    }
    else{
        for(auto & neighbor:sockets){
            if(neighbor.first!=msg->src){
                send_discover(msg,neighbor.first);
                //msgs.emplace({msg->id,msg->src});
            }
        }
    }
};
/* --------------------------------- ROUTE -------------------------------------- */
void route(message* msg,int message_num,int length,int * way){
   if(length<waze[msg->src].size()){
       waze[msg->src].clear();
       for(int i=0;i<length;i++){
           waze.at(msg->src).push_back(way[i]);
       }
   }
   else {
       if (accumulate(way, way + length, 0) < accumulate(waze[msg->src].begin(), waze[msg->src].end(), 0)) {
           waze[msg->src].clear();
           for (int i = 0; i < length; i++) {
               waze.at(msg->src).push_back(way[i]);
           }
       }
   }
   message * reply;
   int f_msg;
   memcpy(&f_msg,msg->payload,sizeof(int));
   reply->src=id;
   reply->dest=msgs[f_msg];

}
/* --------------------------------- RELAY -------------------------------------- */

void relay(int message_num,message * msg){
    message* relay;
    int ret;
    int dest;
    if(msg->dest!=id){
        send_nack(msg);
    }
    else{
        for(int i=0;i<msg->trailMSG;i++) {
           // ret=wait_for_input();
            read(ret, &relay, 512);
            memcpy(relay->payload,&dest,sizeof(int));
            write(sockets.at(dest),&relay,sizeof(relay));
        }
    }
};

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

void send_discover(message * msg,int dest,int * visited){
    message *disc;
    disc->funcID=8;
    disc->dest=dest;
    disc->src=id;
    disc->id=random();
    msgs.insert({disc->id,msg->src});
    memcpy(&disc->payload,msg->payload,sizeof(int));
    int num_sent;
    memcpy()
    memcpy(&disc->payload+sizeof(int),)
    write(sockets[disc->dest],&disc,sizeof(&disc));
}
//void discover(int message_num){};
//void route(int message_num,int length,int * way){}
//void Send(int length,char*){}
//void relay(int message_num){};
//



