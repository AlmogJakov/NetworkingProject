#include "node.hpp"

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