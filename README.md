
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
⚪  **route,node-id** - printing the route to the given node if the node exists (if the route is unknown discover before printing).  
⚪  **peers** - printing the list of the connected (directly) nodes.  	

-----


<h2> 'Node' Class (Represents a vertex): </h2>

Represents a network computer.
Contains a number of data structures in order to perform mutual communication operations between computers:

- **int id** - a unique id that represents the computer on the network.  
- **unordered_map <int, unsigned int> sockets** - map of the connected nodes (key - socket id. value - socket fd).  
- **unordered_map <int, string> text** - map of the data (text) that the current node should send to specific node (key - dest node id. value - text).  
- **unordered_map <int, vector <int>> waze** - map of the pathways to certain nodes (key - dest node id. value - the path).  
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
	
- **std_setid(#param)** - TODO: Explanations..
- **std_connect(#param)** - TODO: Explanations..
- **std_send(#param)** - TODO: Explanations..
- **std_route(#param)** - TODO: Explanations..
- **std_peers(#param)** - TODO: Explanations..
- **gotmsg(#param)** - TODO: Explanations..
- **ack(#param)** - TODO: Explanations..
- **nack(#param)** - TODO: Explanations..
- **cnct(#param)** - TODO: Explanations..
- **Send(#param)** - TODO: Explanations..
- **discover(#param)** - TODO: Explanations..
- **route(#param)** - TODO: Explanations..
- **relay(#param)** - TODO: Explanations..
- **del(#param)** - TODO: Explanations..
- **send_route(#param)** - TODO: Explanations..
- **send_nack(#param)** - TODO: Explanations..
- **send_ack(#param)** - TODO: Explanations..
- **send_discover(#param)** - TODO: Explanations..
- **send_relay(#param)** - TODO: Explanations..
- **send_Del(#param)** - TODO: Explanations..

<h2> 'Select' Class (Implements listening to sockets): </h2>

In this class the methods help to listen to the user input and also to listen to different sockets in parallel.
  
The following actions can be performed:
- Add fd to monitoring - Add fd to listening list.
- Remove fd from monitoring - Remove fd from listening list. 
- Wait for input - Waiting for input from the user or input from one of the sockets from the listening list. When receiving a certain input the function returns the fd number.

-----

# UDP IPv4 VS IPv6
In this implementation we develop a node in a network where communication between different nodes is possible. The nodes do not necessarily communicate directly with each other, and they can communicate through other nodes.
	
</table>

-----

# UDP Pinger
In this implementation we develop a node in a network where communication between different nodes is possible. The nodes do not necessarily communicate directly with each other, and they can communicate through other nodes.
	
</table>

-----
