# UDPPingClient.py
import time
import random
from socket import *
from datetime import datetime
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.settimeout(1.0)
sequence = 0
packet_loss = 0
packet_sent = 0
sum_rtt = 0
minimum_rtt = float('inf')
maximum_rtt = float(0)
while sequence < 10:
    sequence = sequence + 1
    rand = random.randint(0, 10)
    # If rand is less is than 4, we consider the packet lost
    if rand < 4:
        packet_loss = packet_loss + 1
        continue
    packet_sent = packet_sent + 1
    now = datetime.now()
    start_time = time.time()
    message = 'Ping ' + str(sequence) + ' ' + str(start_time)
    serverSocket.sendto(message.encode(), ('127.0.0.1', 12000))
    try:
        # blocked until we get response from the server
        message, address = serverSocket.recvfrom(12000)
        message_array = message.decode().split(' ')
        message_text = str(message_array[0])
        message_seq = int(message_array[1])
        rtt = time.time() - start_time
        sum_rtt += rtt
        if rtt < minimum_rtt:
            minimum_rtt = rtt
        elif rtt > maximum_rtt:
            maximum_rtt = rtt
        print((message_text + " " + str(message_seq) + " " + " | RTT: %s seconds") % rtt)
        print("---------------------------------------------------")
    except:
        print("Request timed out")
        print("---------------------------------------------------")
print("Average RTT: %s" % (sum_rtt/10))
print("Maximum RTT: %s" % maximum_rtt)
print("Minimum RTT: %s" % minimum_rtt)
print("Packet Loss: %s%s" % ((packet_loss/10)*100, "%"))
serverSocket.close()
