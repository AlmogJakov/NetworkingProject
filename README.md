
1. [ Routing Protocol Implementation ](#routing-protocol-implementation)
2. [ UDP IPv4 VS IPv6 ](#udp-ipv4-vs-ipv6)
3. [ UDP Pinger ](#udp-pinger)

-----

# Routing Protocol Implementation
In this implementation we develop a node in a network where communication between different nodes is possible. The nodes do not necessarily communicate directly with each other, and they can communicate through other nodes.
	
</table>

-----


<table align="center">
	
<h3>The main method ('discover') for locating a node in the network:</h3>
<tr><td>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartC/images/discover-animation.gif"/></p>
</td></tr>
</table>

-----
	

<h3> The following actions can be performed: </h3>    

⚪  **setid,id** - setting the id of the current node.  
⚪  **connect,ip:port** - connecting the current node to the node that meets the inputed IP and Port.  
⚪  **send,dest-node-id,data-length,data** - sending data of the given length to the given destination node.  
⚪  **route,node-id** - print the route to the given node if the node exists (if the route is unknown discover it before).  
⚪  **peers** - printing the list of the connected (directly) nodes.  	

-----


<h2> 'Node' (Represents a vertex): </h2>

Represents a network computer.
Contains a number of data structures in order to perform mutual communication operations between computers:

- **int id** - a unique id that represents the computer on the network.  
- **unordered_map <int, unsigned int> sockets** - map of the connected nodes (key - socket id. value - socket fd).  
- **unordered_map <int, string> text** - map of the data (text) that the current node should send to specific node (key - dest node id. value - text).  
- **unordered_map <int, vector <int>> waze** - map of the pathways to certain nodes (key - dest node id. value - the path).  
- **unordered_map <int, vector <int>> checkpoint_waze** - temporary map of the partial pathways to certain nodes obtained during DISCOVER operation (explained below) while (key - dest node id. value - the path).  
- **unordered_map <int, set <int>> nodes_to_request** - map of the remaining neighbors to send information to in order to complete a particular requested action identified by a unique id (key - the unique request id. value - remaining neighbors to send information to).  
- **unordered_map <int, pair <int, int >> node_to_reply** - map representing the neighbor to whom the answer should be returned (key - the unique request id. value - pair {neighbor node to whom the answer should be returned, id of the message sent by this neighbor}).  
	
<!--
```diff 
@@ self.graph; @@ (for receiving any vertex by key).
``` 
```diff 
@@ self.ni; @@ (for receiving any vertex neighbors as keys while value = weight).
``` 
```diff 
@@ self.revers_ni; @@ (for receiving any vertex reversed neighbors as keys while value = weight).
``` 
-->

<h3> The methods implemented in this class: </h3>    
	
- **std_setid(#param)** - determines the current node ID based on the input.  
- **std_connect(#param)** - opening a new socket and sending a request to connect to the IP and the port received at the input. (necessary condition - ID is defined in both nodes).  
- **std_send(#param)** - sending a message of a certain length to the node given in the input. If the path to the node unknown, a DISCOVER operation is activated (as explained below) before sending.  
- **std_route(#param)** - prints the best route to the node given in the input. If the path to the node is not known, the DISCOVER operation is activated (as explained below) before printing the route.  
- **std_peers(#param)** - prints the neighboring nodes (directly connected).  
- **gotmsg(#param)** - using a switch case when receiving a message to redirect to corresponding incoming message function.  
- **input_ack(#param)** - handles the case of incoming ack message (depending on the source message on which the ACK message was returned).  
- **input_nack(#param)** - handles the case of incoming nack message (depending on the source message on which the ACK message was returned).  
- **input_connect(#param)** - accepting SOCKET login (if ID has been placed on both sides) and then adding the SOCKET to the list of neighbors.  
- **input_send(#param)** - print out the input message and sends an ack message back to sender.  
- **input_discover(#param)** - flooding DISCOVER messages to the network for the purpose of finding the best route to a particular node. Intuitive explanation: 1. sending a DISCOVER message to all the vertices (a DISCOVER message is sent to a single neighbor at a time until a reply is returned, then sent to the next neighbor and so on). 2. If a DISCOVER message has been sent to a leaf node or a node we have already visited (closing a circle) - returns a NACK message back to the last node from which a DISCOVER message was sent. If the destination node is found - return the route by ROUTE message.  
- **input_route(#param)** - if there are neighbors for whom a DISCOVER operation has not been performed - continue to perform a DISCOVER operation in order to find the best route. Otherwise, the route is saved in the "waze" data structure (if an internal node has received this message, the saving is performed in a temporary "checkpoint_waze" data structure). If there is already a route - the best route is maintained.  
- **input_relay(#param)** - receiving a RELAY message, receiving the continued chaining of the messages (explained below) in the input and sending it to the next vertex in the route.  
- **input_refresh(#param)** - flooding REFRESH messages to the network when the topology has been changed (node deleted/added to the network) in order to delete existing routes in each node. the algorithm is implemented in a similar way to the DISCOVER algorithm explained above.  
- **send_route(#param)** - this function is used to respond to a DISCOVER message when finding a route. creates the route and sends it back to the node that send the discover message.  
- **send_nack(#param)** - sending a NACK message in response to the message in the input (depending on the type of message).  
- **send_ack(#param)** - sending a ACK message in response to the message in the input (depending on the type of message).  
- **send_discover(#param)** - sending a DISCOVER message in response to the parameters in the input.  
- **send_relay(#param)** - a call to this function is made when there is a path to the target node. the function chains RELAY messages with a SEND message where each RELAY message is intended for an internal node in the track and the SEND message (containing the content) to the destination node. if the content of the message exceeds one SEND message, several SEND messages will be chained.  
- **send_refresh(#param)** - sending a REFRESH message in response to the parameters in the input.  

<h2> 'Select' (Implements listening to sockets): </h2>

In this class the methods help to listen to the user input and also to listen to different sockets in parallel.
  
The following actions can be performed:
- Add fd to monitoring - Add fd to listening list.
- Remove fd from monitoring - Remove fd from listening list. 
- Wait for input - Waiting for input from the user or input from one of the sockets from the listening list. When receiving a certain input the function returns the fd number.

-----

# UDP IPv4 VS IPv6
Diagnosing the difference between the HEADER of IPV4 versus IPV6 under UDP protocol.
	
<h3>Before running the server:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/show-netstat-before.jpg" width="60%"/></p>
	
<h3>After running the server:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/show-netstat-after.jpg" width="60%"/></p>
	
Note that the flag '-n' (netstat -a **-n**) shows network addresses as numbers. When this flag is not specified, the netstat command interprets addresses where possible and displays them symbolically. This flag can be used with any of the display formats. (source: https://www.ibm.com/docs/en/power8/8408-44E?topic=commands-netstat-command)
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/flag-netstat.jpg"/></p>
	
<h3>IPV4 Client Header:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/ipv4-header-client.jpg" width="60%"/></p>
	
<h3>IPV4 Server Header:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/ipv4-header-server.jpg" width="60%"/></p>
	
<h3>IPV6 Client Header:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/ipv6-header-client.jpg" width="60%"/></p>
	
<h3>IPV6 Server Header:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/ipv6-header-server.jpg" width="60%"/></p>
	
<h3>Ping execution:</h3>
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/ping.jpg" width="60%"/></p>
	
<h3>Differences between IPV4 and IPV6:</h3>    
	
- In IPV6 there is no fragmentation. The sender must send packages of the appropriate size and not packages that are too large.  
- In IPV6 there is no checksum because it already exists in the transport layer and the data link layer.  
- The options field does not exist but may have a pointer in the Next Header field.  
- In IPV6 the addresses are 128 bits long as opposed to IPV4 where the addresses are 32 bits long.  
	
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartB/images/ipv4VSipv6.jpg"/></p>
(source: https://www.juniper.net/us/en/research-topics/what-is-ipv4-vs-ipv6.html)

-----

# UDP Pinger
Implementation of sending ping under UDP protocol while losing packets (randomly).
The program prints the values: Average RTT, Maximum RTT, Minimum RTT, Packet Loss.
	
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartA/images/udp-pinger.jpg"/></p>

-----

<h3>UDPHeartBeat</h3>
	
Another implementation is UDPHeartBeat, this implementation is the same as the implementation above and in addition:  
	
<b>Server side:</b>   
	
- report time differences from the last packet sent by the client.  
- reporting loss of packets.  
- estimated reporting if client disconnected.  
	
	

<b>Client side:</b>   
	
- print the input from the server.  
- print the RTT value.  
	
<p align="center"><img src="https://github.com/AlmogJakov/NetworkingProject/blob/main/PartA/images/udp-pinger-heartBeat.jpg"/></p>

-----
Almog Jakov: https://github.com/AlmogJakov  
Tal Somech: https://github.com/TalSomech  
