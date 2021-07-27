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

#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "select.hpp"
#include "node.hpp"


int id;
unordered_map<int,const unsigned int> sockets;
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
    /* print my local ip */
    // addrinfo *addr_info = NULL;
    // char hostName[100];
    // char ip[30] = {0};
    // gethostname(hostName, sizeof(hostName));
    // cout << hostName << endl;;
    // int err = getaddrinfo(hostName,NULL,NULL,&addr_info);
    // in_addr ipaddr =(((sockaddr_in*)(addr_info->ai_addr))->sin_addr);
    // strcpy(ip,inet_ntoa(ipaddr));
    // cout << "MY LOCAL IP: " << ip << endl;
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
            string splited[4]; // split
            int i = 0;
            getline(ss,splited[0],',');
            if (splited[0].compare("setid")==0) {
                getline(ss,splited[1],','); // id
                id = stoi(splited[1]);
                cout << "\033[1;36m"; // print in color
                cout << "MY ID: " << id << endl;
                cout << "\033[0m"; // end print in color
            } else if (splited[0].compare("connect")==0) {
                getline(ss,splited[1],':'); // ip
                getline(ss,splited[2],':'); // port
                uint16_t port = stoul(splited[2]);
                char const* destip = splited[1].c_str();
                // cout << "ip: " << splited[1] << ". port: " << splited[2] << endl;
                struct sockaddr_in destAddress;
                int new_sock = socket(AF_INET, SOCK_STREAM, 0);
                bind(new_sock, (struct sockaddr*)&destAddress, sizeof(destAddress));
                memset(&destAddress, 0, sizeof(destAddress));
	            destAddress.sin_family = AF_INET;
                destAddress.sin_port = htons(port);
                if(inet_pton(AF_INET, destip, &destAddress.sin_addr)<=0) {
                    printf("\nInvalid address/ Address not supported \n");
                    return -1;
                }
                if (connect(new_sock, (struct sockaddr*)&destAddress, sizeof(destAddress)) < 0) {
                    printf("\nConnection Failed \n");
                    return -1;
                }
                add_fd_to_monitoring(new_sock);
                message outgoing;
                outgoing.id = rand();
                outgoing.src = id;
                outgoing.dest = 0;
                outgoing.trailMSG = 0;
                outgoing.funcID = 4; // connect function num is 4
                memcpy(outgoing.payload, "hey there!", 10); // set the payload
                char outgoing_buffer[512];
                memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
                send(new_sock, outgoing_buffer, sizeof(outgoing_buffer), 0);
            }
        } else { /* we got a packet */
            int addrlen;
            /* if ret==innerfd then we got a new connection! */
            if (ret==innerfd) {
                if ((ret = accept(ret, (struct sockaddr *)&serv_addr, (socklen_t*)&addrlen))<0) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                add_fd_to_monitoring(ret);
            }
            message* incoming = (message*)malloc(sizeof(message));
            read(ret ,incoming, 512);
            cout << "\033[1;36m"; // print in color
            cout << "Got " << message_type(incoming) << " message type!" << endl;
            cout << "\033[0m"; // end print in color
            if (incoming->funcID == 4) {
                cnct(ret, incoming);
                continue;
            }
            //gotmsg(incoming);
            delete(incoming);
        }
	}
}
void gotmsg(message* msg){
    switch(msg->funcID){
        case 1:{
            ack(msg);
            break;}
        case 2:{
            nack(msg->id);
            break;}
//        case 4:{
//            Connect();
//            break;
//        }
        case 8:{
            discover(msg->dest);
            break;
        }
        case 16:{
//            int f_msg;
//            int length;
//            memcpy(&f_msg,main_msg->payload,sizeof(f_msg) );
//            memcpy(&length,main_msg->payload+sizeof(f_msg),sizeof(length));
//            int * way;
//            int i=0;
//            while(i<length){
//                memcpy(reinterpret_cast<void *>(way[i]), main_msg->payload + 8 + 4 * i, sizeof(int));
//            }
//            route(f_msg,length,way);
            break;
        }
        case 32:{
            int length;
            //memcpy(length, main_msg->payload,sizeof(length));

            break;
        }
        case 64:{break;}
    }
}

void ack(message * msg){//,int messagenum){
    cout<<"Ack"<<endl;
//    message rply;
//    rply.id=random();
//    rply.src=id;
//    memcpy(rply.payload, (char*)&msg->id,sizeof(int));
//    rply.trailMSG=0;
//    rply.funcID=1;
//
    //rply->payload=(char*)msg->id;

}

void nack(int messagenum){
    int msgid=5;
    message ack;
    ack.id=msgid;
    ack.src=id;

}

void cnct(const unsigned int new_socket, message * msg){
    message rply;
    rply.id=random();
    rply.src=id;
    memcpy(rply.payload, (char*)&msg->id,sizeof(int));
    rply.trailMSG=0;
    rply.funcID=1;
    sockets.insert(make_pair(msg->src,new_socket));
    add_fd_to_monitoring(new_socket);
    write(new_socket,&rply,sizeof(rply));
}

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
void discover(int message_num){};
void route(int message_num,int length,int * way){}
void Send(int length,char*){}
void relay(int message_num){};




