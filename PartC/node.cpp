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

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "select.hpp"
#include "node.hpp"

using namespace std;

struct packetMSG {
    int id;
    int src;
    int dest;
    int trailMSG;
    int funcID;
    char payload[492];
};

int id;

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
    outerfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(r_port); 

    bind(innerfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_port = htons(r_port+1);  
    bind(outerfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    printf("adding fd1(%d) to monitoring\n", innerfd);
    add_fd_to_monitoring(innerfd);
    printf("adding fd2(%d) to monitoring\n", outerfd);
    add_fd_to_monitoring(outerfd);
    listen(innerfd, 10);
    listen(outerfd, 10); 

    printf("---------------------------------\n");
    // print my ip
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&inet_addr;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
    printf("MY IP: %s\n", str);
    // print my port
    printf("MY PORTS: %d, %d\n", r_port, r_port+1);
    printf("---------------------------------\n");

    

    for (i=0; i<10; ++i) {
        memset(&buff, '\0', sizeof(buff));
	    printf("waiting for input...\n");
        //cout << "hey!!!" << endl;
	    ret = wait_for_input();
	    printf("fd: %d is ready. reading...\n", ret);
	    read(ret, buff, 1025);
        // At the file descriptor level, stdin is defined to be file descriptor 0,
        // stdout is defined to be file descriptor 1,
        // and stderr is defined to be file descriptor 2.

        

        if (ret < 2) { // std input
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



            
	        

            if (splited[0].compare("connect")==0) {
                getline(ss,splited[1],':'); // ip
                getline(ss,splited[2],':'); // port
                uint16_t port{}; // = stoul(splited[2]);
                ss >> port;
                char const* destip = splited[1].c_str();
                cout << "ip: " << splited[1] << ". port: " << splited[2] << endl;
                //struct sockaddr_in serv_addr;
                struct sockaddr_in destAddress;
                memset(&destAddress, 0, sizeof(destAddress));
	            destAddress.sin_family = AF_INET;
                destAddress.sin_port = htons(port);
                if(inet_pton(AF_INET, "127.0.0.1", &destAddress.sin_addr)<=0) {
                    printf("\nInvalid address/ Address not supported \n");
                    return -1;
                }
                if (connect(innerfd, (struct sockaddr*)&destAddress, sizeof(destAddress)) < 0) {
                    printf("\nConnection Failed \n");
                    return -1;
                }
                // send(sock , hello , strlen(hello) , 0 );
	            // int rval = inet_pton(AF_INET, (const char*)destip, &destAddress.sin_addr);
	            // if (rval <= 0) {
		        //     printf("inet_pton() failed\n");
		        //     return -1;
	            // } else {
                //     cout << "rval: " << rval << endl;
                // }
            }
        } else { // we got a packet
            
            // if ((accept(outerfd, (struct sockaddr*)&destAddress, (socklen_t*)sizeof(destAddress)))<0) {
            //         perror("accept");
            //         exit(EXIT_FAILURE);
            // }
            //struct packetMSG pckt = (struct packetMSG)(buff);
            struct packetMSG* pckt = (struct packetMSG*)malloc(sizeof(struct packetMSG));
            if (pckt->funcID == 4) {
                cout << "Got Connection!" << endl;
            }
            //struct cooked_packet* reply_packet = (struct cooked_packet*)malloc(sizeof(struct cooked_packet));
        }
	}
}
