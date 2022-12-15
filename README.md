# Client

## read_string_from_socket
> reads the string from the given socket file descriptor

## send_string_on_socket
> sends the string on the given socket descriptor

## main
> initializes socket file descriptor, connects it to the server at node 0 by taking the port number.

---
# Server

## read_string_from_socket
> reads the string from the given socket file descriptor

## send_string_on_socket
> sends the string on the given socket descriptor

## dijkstra 
> runs the dijkstra algorithm to get the minimum delay path from node 0 to any other node. We only run dijkstra from node 0 because in order to get the minimum delay path it is sufficient that it is from node 0 as all messages come from there.

## handle connection
> It takes the socket_file descriptor, if the node is 0 and the connection is with the client it runs a while loop to keep the connection alive. It takes input from client, parses string to get the required message, gets the path along which message has to be sent, makes the current node as client gets forward destination node as server and passes the message to it. Path is derived by storing the parent for a node on its path.

## serverthread
> It makes a socket for every node in the server and keeps listening on that socket, whenever connection is made it calls handle connection to do its task.

## main 
> Takes input and runs dijsktra, makes serverthreads and executes them.

--- 
# Server Failures
> In case of one of the servers failing, we can make the delay from it to all nodes as infinity, in that case, the sortest path for any node will never pass from that server and hence we could avoid the failed server.