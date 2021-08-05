# UDPPingClient.py
import time
from socket import *
from datetime import datetime
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.settimeout(1.0)
sequence = 1
packet_loss = 0
packet_sent = 0
sum_rtt = 0
minimum_rtt = float('inf')
maximum_rtt = float(0)
while sequence <= 10:
    packet_sent = packet_sent + 1
    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    #  current_time = datetime.now()
    start_time = time.time()
    message = 'Ping ' + str(sequence) + ' ' + str(current_time)
    # serverSocket.sendto(message.encode(), (serv_ip, serv_port))
    serverSocket.sendto(message.encode(), ('127.0.0.1', 12000))
    try:
        # message, address = serverSocket.recvfrom(serv_port)
        message, address = serverSocket.recvfrom(12000)
        rtt = time.time() - start_time
        sum_rtt += rtt
        if rtt < minimum_rtt:
            minimum_rtt = rtt
        elif rtt > maximum_rtt:
            maximum_rtt = rtt
        print((message.decode() + " | RTT: %s seconds") % rtt)
        # print("RTT: %s seconds" % (time.time() - start_time))
        print("---------------------------------------------------")
        sequence = sequence + 1
    except:
        print("Request timed out")
        print("---------------------------------------------------")
        packet_loss = packet_loss + 1
print("Average RTT: %s" % (sum_rtt/10))
print("Maximum RTT: %s" % maximum_rtt)
print("Minimum RTT: %s" % minimum_rtt)
print("Packet Loss: %s%s" % ((packet_loss/packet_sent)*100, "%"))
serverSocket.close()
