# UDPPingerServer.py
# We will need the following module to generate randomized lost packets
import time
from socket import *
# Create a UDP socket, use of SOCK_DGRAM for UDP packets
serverSocket = socket(AF_INET, SOCK_DGRAM)
# unblocked option for recvfrom() method
serverSocket.settimeout(0)
# Assign IP address and port number to socket
serverSocket.bind(('127.0.0.1', 12000))
current_sequence = 0
last_time = 0.0
connected = False
while True:
    # Receive the client packet along with the address it is coming from
    try:
        # unblocked to indicate if client disconnected
        message, address = serverSocket.recvfrom(1024)
        message_array = message.decode().split(' ')
        message_seq = int(message_array[1])
        message_time = float(message_array[2])
        # if the customer is currently logged in
        if connected is False:
            connected = True
            last_time = message_time
        if message_seq != current_sequence + 1:
            for i in range(current_sequence + 1, message_seq):
                print("Lost Packet %s" % i)
        current_sequence = message_seq
        print("Got Packet %s | Time Difference: %s" % (message_seq, message_time-last_time))
        last_time = message_time
        # Capitalize the message from the client
        message = message.upper()
        serverSocket.sendto(message, address)
    except:
        # if did not receive a packet for more then 5 seconds, assume client disconnected.
        if connected:
            if (time.time() - last_time) > 5:
                print("disconnected")
                current_sequence = 0
                connected = False
                last_time = 0.0