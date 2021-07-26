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
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "select.hpp"
#include "node.hpp"

using namespace std;


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
    //stringstream ss;
    time_t ticks; 

    innerfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(r_port);
    bind(innerfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("adding fd(%d) to monitoring\n", innerfd);
    add_fd_to_monitoring(innerfd);
    //printf("adding fd2(%d) to monitoring\n", outerfd);
    //add_fd_to_monitoring(outerfd);
    listen(innerfd, 10);
    //listen(outerfd, 10); 

    printf("---------------------------------\n");
    // print my ip
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&inet_addr;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    printf("MY IP: %s\n", str);
    // print my port
    printf("MY PORTS: %d\n", r_port);
    printf("---------------------------------\n");

    

    //for (i=0; i<10; ++i) {
    while(true){
        memset(&buff, '\0', sizeof(buff));
	    printf("waiting for input...\n");
        //cout << "hey!!!" << endl;
	    ret = wait_for_input();
	    printf("fd: %d is ready. reading...\n", ret);
	    
        // At the file descriptor level, stdin is defined to be file descriptor 0,
        // stdout is defined to be file descriptor 1,
        // and stderr is defined to be file descriptor 2.
        if (ret < 2) { // std input
            read(ret, buff, 1025);
            if (buff[strlen(buff)-1]=='\n') buff[strlen(buff)-1] = '\0';
            //string str(buff);
            stringstream ss;
            ss << buff;
            // split
            string splited[4];
            // printf("------------------------\n");
            // printf("SPLITED WORDS: \n");
            int i = 0;
            // while(getline(ss,splited[i],',')) {
            //     //cout << splited[i++] << endl;
            // };
            getline(ss,splited[0],',');
            // printf("------------------------\n");
            if (splited[0].compare("setid")==0) {
                getline(ss,splited[1],','); // id
                id = stoi(splited[1]);
                cout << "MY ID: " << id << endl;
            }
            if (splited[0].compare("connect")==0) {
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
            printf("%s\n",incoming->payload);
            

            message outgoing;
            outgoing.id = rand();
            outgoing.src = id;
            outgoing.dest = 0;
            outgoing.trailMSG = 0;
            outgoing.funcID = 4; // connect function num is 4
            memcpy(outgoing.payload, "hey thereee!", 10); // set the payload
            char outgoing_buffer[512];
            memcpy(outgoing_buffer, &outgoing, sizeof(outgoing));
            send(ret, outgoing_buffer, sizeof(outgoing_buffer), 0);
            // message incming_msg;
            // int n = read(new_socket, (void*)&incming_msg, 512);
            // if(incming_msg.funcID==4){
            //     cout<<"MSG"<<endl;
            //     cnct(new_socket,&incming_msg);
            //     //write(new_socket, (void*)&incming_msg, 512);
            //     continue;
            // }
            // cout << n << endl;
            //add_fd_to_monitoring(new_socket);
            //gotmsg(&incming_msg);
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
void discover(int message_num){};
void route(int message_num,int length,int * way){}
void Send(int length,char*){}
void relay(int message_num){};




