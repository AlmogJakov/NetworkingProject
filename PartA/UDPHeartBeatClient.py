# UDPPingClient.py
import time
import random
from socket import *
from datetime import datetime
# my_ip = '5.29.33.158'
# my_port = 1024
# serv_ip = '109.66.22.227'
# serv_port = 12000
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.settimeout(1.0)
# serverSocket.bind((my_ip, my_port))
serverSocket.bind(('127.0.0.1', 1024))
# serverSocket.connect((my_ip, my_port))
# serverSocket.connect((serv_ip, serv_port))
sequence = 0
packet_loss = 0
packet_sent = 0
sum_rtt = 0
minimum_rtt = float('inf')
maximum_rtt = float(0)
while sequence < 10:
    sequence = sequence + 1
    rand = random.randint(0, 10)
    if rand < 4:
        packet_loss = packet_loss + 1
        continue
    packet_sent = packet_sent + 1
    now = datetime.now()
    # current_time = now.strftime("%H:%M:%S")
    current_time = round(time.time())
    start_time = time.time()
    message = 'Ping ' + str(sequence) + ' ' + str(current_time)
    # serverSocket.sendto(message.encode(), (serv_ip, serv_port))
    serverSocket.sendto(message.encode(), ('127.0.0.1', 12000))
    try:
        # message, address = serverSocket.recvfrom(serv_port)
        # blocked until we get response from the server
        message, address = serverSocket.recvfrom(12000)
        message_array = message.decode().split(' ')
        message_text = str(message_array[0])
        message_seq = int(message_array[1])
        message_time = time.strftime('%H:%M:%S', time.gmtime(int(message_array[2])))
        rtt = time.time() - start_time
        sum_rtt += rtt
        if rtt < minimum_rtt:
            minimum_rtt = rtt
        elif rtt > maximum_rtt:
            maximum_rtt = rtt
        print((message_text + " " + str(message_seq) + " " + message_time + " | RTT: %s seconds") % rtt)
        # print("RTT: %s seconds" % (time.time() - start_time))
        print("---------------------------------------------------")
    except:
        print("Request timed out")
        print("---------------------------------------------------")
print("Average RTT: %s" % (sum_rtt/10))
print("Maximum RTT: %s" % maximum_rtt)
print("Minimum RTT: %s" % minimum_rtt)
print("Packet Loss: %s%s" % ((packet_loss/10)*100, "%"))
serverSocket.close()
