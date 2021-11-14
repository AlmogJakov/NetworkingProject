#include "node.hpp"

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
            } else if (splited[0].compare("setid")==0) {std_setid(ss,splited);}
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