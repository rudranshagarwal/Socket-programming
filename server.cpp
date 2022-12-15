#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <bits/stdc++.h>

#include <iostream>
#include <assert.h>
#include <tuple>
using namespace std;


typedef long long LL;


///////////////////////////////
int *distances;
int *parent;

int maxclients;

#define PORT_ARG 3080

int portbase;
int c = 0;

int numberofnodes;

map<int, int> ports;
map<pair<int, int>, int> sleeptime;

pthread_mutex_t c_lock;
pthread_mutex_t ports_lock;

const int initial_msg_len = 256;

const LL buff_sz = 1048576;
pair<string, int> read_string_from_socket(const int &fd, int bytes)//read string from socket
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. \n";
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);
    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)//send string on socket
{
    int bytes_sent = write(fd, s.c_str(), s.length());
    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA via socket.\n";
    }

    return bytes_sent;
}


void dijsktra(vector<int> adj[], vector<int> weight[], int n)//run dijsktra to get shortest path
{

    int visited[n];
    distances[0] = 0;
    visited[0] = 0;
    parent[0] = 0;

    for (int i = 1; i < n; i++)
    {
        visited[i] = 0;
        distances[i] = INT_MAX;
    }
    for (int x = 0; x < n; x++)
    {
        int min = INT_MAX, i;
        for (int j = 0; j < n; j++)
        {
            if (distances[j] <= min && !visited[j])
            {
                min = distances[j];
                i = j;
            }
        }
        visited[i] = 1;

        for (int j = 0; j < adj[i].size(); j++)
        {
            if (visited[adj[i][j]] == 1)
                continue;
            else if (distances[adj[i][j]] > weight[i][j] + distances[i])
            {
                distances[adj[i][j]] = weight[i][j] + distances[i];
                parent[adj[i][j]] = i;
            }
        }
    }
}

void handle_connection(int client_socket_fd, int node)// handle connection
{
    // int client_socket_fd = *((int *)client_socket_fd_ptr);
    //####################################################

    int received_num, sent_num;

    /* read message from client */
    int ret_val = 1;
    if (node == 0)//if node is 0 keep connection alive
    {
        while (1)
        {
            string cmd;
            tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);//get the message
            ret_val = received_num;
            // debug(ret_val);
            // printf("Read something\n");
            if (ret_val <= 0)
            {
                // perror("Error read()");
                printf("Server could not read msg sent from client\n");
                close(client_socket_fd);
            }
            else
            {

                // cout << "Client sent : " << cmd << endl;

                if (cmd == "exit")
                {
                    cout << "Exit pressed by client" << endl;
                    // goto close_client_socket_ceremony;
                    close(client_socket_fd);
                }
                else if (cmd == "pt")// print table
                {
                    printf("dest forw delay\n");

                    for (int i = 1; i < numberofnodes; i++)
                    {
                        printf("%4d %4d %5d\n", i, parent[i], distances[i]);
                    }
                    printf("\n----------------\n");
                    string msg_to_send_back = "Ack:" + cmd;
                    send_string_on_socket(client_socket_fd, msg_to_send_back);
                }
                else if (node == 0 && cmd.substr(0, 4) == "send")//if it is from client parse it.
                {
                    int i, j;
                    const char *command = cmd.c_str();
                    int c = 0;
                    for (i = 0; i < strlen(command); i++)
                    {
                        if (command[i] == ' ')
                        {
                            c++;
                            if (c == 2)
                            {
                                i++;
                                break;
                            }
                        }
                    }
                    for (j = i - 2; j >= 0; j--)//get destination node
                    {
                        if (command[j] == ' ')
                        {
                            j++;
                            break;
                        }
                    }
                    char destination[i - j];
                    destination[i - j - 1] = '\0';
                    string s;
                    for (int x = j; x < i - 1; x++)
                    {
                        destination[x - j] = command[x];
                        s += command[x];
                    }
                    int dest = atoi(destination);
                    printf("%s %d\n", &command[i], dest);
                    string msg_to_relay = s + " " + &cmd[i];
                    string msg_to_send_back = "Ack:" + cmd;//send acknowlegement
                    send_string_on_socket(client_socket_fd, msg_to_send_back);
                    int forward = dest;
                    if (dest == 0)
                    {
                        printf("Data received at the final destiantion node: %d\nSource : Client\nDestination : %d\nMessage : %s\n", node, dest, &cmd[i]);
                        printf("\n----------------\n");
                    }
                    else
                    {
                        while (parent[forward] != forward)//get where to forwaed
                        {
                            forward = parent[forward];
                        }
                        printf("Data received at node: %d\nSource: Client\nDestination : %d\nFowarded_Destination : %d\nMessage : %s\n", node, dest, forward, &cmd[i]);
                        printf("\n----------------\n");

                        sleep(sleeptime.find({0, forward})->second);//introduce the delay
                        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);//make this node the client
                        if (socket_fd < 0)
                        {
                            perror("Error in socket creation for CLIENT");
                            exit(-1);
                        }
                        struct sockaddr_in server_obj;

                        int port_num = ports.find(forward)->second;

                        memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
                        server_obj.sin_family = AF_INET;
                        server_obj.sin_port = htons(port_num); // convert to big-endian order

                        if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
                        {
                            perror("Problem in connecting to the server");
                            exit(-1);
                        }
                        send_string_on_socket(socket_fd, msg_to_relay);
                    }
                }
                else// if any other node do the same thing but disconnect at the end
                {

                    int i;
                    for (i = 0; i < cmd.length(); i++)
                    {
                        if (cmd[i] == ' ')
                        {
                            i++;
                            break;
                        }
                    }
                    char destination[i - 1];
                    destination[i - 1] = '\0';
                    for (int x = 0; x < i - 1; x++)
                    {
                        destination[x] = cmd[x];
                    }
                    int dest = atoi(destination);
                    // printf("%d\n",node);
                    // printf("%d\n", dest);
                    if (node == dest)
                    {
                        printf("Data received at the final destiantion node: %d\nOriginalSource : %d\nDestination : %d \nMessage : %s\n", node, 0, dest, &cmd[i]);
                        printf("\n----------------\n");
                    }
                    else
                    {
                        int forward = dest;
                        while (parent[forward] != node)
                        {
                            forward = parent[forward];
                        }
                        printf("Data received at node: %d\nOriginal Source : %d\nDestination : %d\nFowarded_Destination : %d; Message : %s\n", node, 0, dest, forward, &cmd[i]);
                        printf("\n----------------\n");

                        sleep(sleeptime.find({node, forward})->second);
                        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (socket_fd < 0)
                        {
                            perror("Error in socket creation for CLIENT");
                            exit(-1);
                        }
                        struct sockaddr_in server_obj;

                        int port_num = ports.find(forward)->second;

                        memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
                        server_obj.sin_family = AF_INET;
                        server_obj.sin_port = htons(port_num); // convert to big-endian order

                        if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
                        {
                            perror("Problem in connecting to the server");
                            exit(-1);
                        }
                        close(client_socket_fd);
                        // printf(BRED "Disconnected from client %d" ANSI_RESET "\n", node);
                        send_string_on_socket(socket_fd, cmd);
                    }
                    // goto close_client_socket_ceremony;
                    // close_client_socket_ceremony:
                }

                // return NULL;
            }
        }
    }
    else
    {
        string cmd;
        tie(cmd, received_num) = read_string_from_socket(client_socket_fd, buff_sz);
        ret_val = received_num;
        // debug(ret_val);
        // printf("Read something\n");
        if (ret_val <= 0)
        {
            // perror("Error read()");
            printf("Server could not read msg sent from client\n");
            // goto close_client_socket_ceremony;
                                close(client_socket_fd);

        }
        else
        {

            // cout << "Client sent : " << cmd << endl;

            if (cmd == "exit")
            {
                cout << "Exit pressed by client" << endl;
                // goto close_client_socket_ceremony;
                                    close(client_socket_fd);

            }
            if (node == 0 && cmd.substr(0, 4) == "send")
            {
                int i, j;
                const char *command = cmd.c_str();
                int c = 0;
                for (i = 0; i < strlen(command); i++)
                {
                    if (command[i] == ' ')
                    {
                        c++;
                        if (c == 2)
                        {
                            i++;
                            break;
                        }
                    }
                }
                for (j = i - 2; j >= 0; j--)
                {
                    if (command[j] == ' ')
                    {
                        j++;
                        break;
                    }
                }
                char destination[i - j];
                destination[i - j - 1] = '\0';
                string s;
                for (int x = j; x < i - 1; x++)
                {
                    destination[x - j] = command[x];
                    s += command[x];
                }
                int dest = atoi(destination);
                printf("%s %d\n", &command[i], dest);
                string msg_to_relay = s + " " + &cmd[i];
                string msg_to_send_back = "Ack:" + cmd;
                send_string_on_socket(client_socket_fd, msg_to_send_back);
                int forward = dest;
                if (dest == 0)
                {
                    printf("Data received at the final destiantion node: %d\nSource : Client\n Destination : %d\nMessage : %s\n", node, dest, &cmd[i]);
                    printf("\n----------------\n");
                }
                else
                {
                    while (parent[forward] != forward)
                    {
                        forward = parent[forward];
                    }
                    printf("Data received at node: %d\nSource: Client\nDestination : %d\nFowarded_Destination : %d\nMessage : %s\n", node, dest, forward, &cmd[i]);
                    printf("\n----------------\n");

                    sleep(sleeptime.find({0, forward})->second);
                    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    if (socket_fd < 0)
                    {
                        perror("Error in socket creation for CLIENT");
                        exit(-1);
                    }
                    struct sockaddr_in server_obj;

                    int port_num = ports.find(forward)->second;

                    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
                    server_obj.sin_family = AF_INET;
                    server_obj.sin_port = htons(port_num); // convert to big-endian order

                    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
                    {
                        perror("Problem in connecting to the server");
                        exit(-1);
                    }
                    send_string_on_socket(socket_fd, msg_to_relay);
                }
            }
            else
            {
                int i;
                for (i = 0; i < cmd.length(); i++)
                {
                    if (cmd[i] == ' ')
                    {
                        i++;
                        break;
                    }
                }
                char destination[i - 1];
                destination[i - 1] = '\0';
                for (int x = 0; x < i - 1; x++)
                {
                    destination[x] = cmd[x];
                }
                int dest = atoi(destination);
                // printf("%d\n",node);
                // printf("%d\n", dest);
                if (node == dest)
                {
                    printf("Data received at the final destiantion node: %d\nOriginalSource : %d\nDestination : %d \nMessage : %s\n", node, 0, dest, &cmd[i]);
                    printf("\n----------------\n");
                }
                else
                {
                    int forward = dest;
                    while (parent[forward] != node)
                    {
                        forward = parent[forward];
                    }
                    printf("Data received at node: %d\nOriginal Source : %d\nDestination : %d\nFowarded_Destination : %d\nMessage : %s\n", node, 0, dest, forward, &cmd[i]);
                    printf("\n----------------\n");

                    sleep(sleeptime.find({node, forward})->second);
                    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    if (socket_fd < 0)
                    {
                        perror("Error in socket creation for CLIENT");
                        exit(-1);
                    }
                    struct sockaddr_in server_obj;

                    int port_num = ports.find(forward)->second;

                    memset(&server_obj, 0, sizeof(server_obj)); // Zero out structure
                    server_obj.sin_family = AF_INET;
                    server_obj.sin_port = htons(port_num); // convert to big-endian order

                    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)
                    {
                        perror("Problem in connecting to the server");
                        exit(-1);
                    }
                    close(client_socket_fd);
                    // printf(BRED "Disconnected from client %d" ANSI_RESET "\n", node);
                    send_string_on_socket(socket_fd, cmd);
                }
                // goto close_client_socket_ceremony;
                // close_client_socket_ceremony:
            }

            // return NULL;
        }
    }
}

void *serverthread(void *args)//execute nodes
{
    int node = *(int *)args;
    int wel_socket_fd, client_socket_fd, port_number;
    socklen_t clilen;

    struct sockaddr_in serv_addr_obj, client_addr_obj;

    wel_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (wel_socket_fd < 0)
    {
        perror("ERROR creating welcoming socket");
        exit(-1);
    }
    pthread_mutex_lock(&c_lock);
    int currc = c;
    pthread_mutex_unlock(&c_lock);
    //////////////////////////////////////////////////////////////////////
    /* IP address can be anything (INADDR_ANY) */
    bzero((char *)&serv_addr_obj, sizeof(serv_addr_obj));
    pthread_mutex_lock(&c_lock);
    pthread_mutex_lock(&ports_lock);
    port_number = ports.find(node)->second;
    c++;
    ports[node] = port_number;//get portnumber for current node
    pthread_mutex_unlock(&c_lock);
    pthread_mutex_unlock(&ports_lock);
    serv_addr_obj.sin_family = AF_INET;
    serv_addr_obj.sin_addr.s_addr = INADDR_ANY;
    serv_addr_obj.sin_port = htons(port_number); // process specifies port

    if (bind(wel_socket_fd, (struct sockaddr *)&serv_addr_obj, sizeof(serv_addr_obj)) < 0)
    {
        perror("Error on bind on welcome socket: ");
        exit(-1);
    }
    while (1)//keep listening on a socket
    {
        listen(wel_socket_fd, maxclients);
        // cout << "Server has started listening on the LISTEN PORT" << endl;
        clilen = sizeof(client_addr_obj);
        client_socket_fd = accept(wel_socket_fd, (struct sockaddr *)&client_addr_obj, &clilen);
        if (client_socket_fd < 0)
        {
            perror("ERROR while accept() system call occurred in SERVER");
            exit(-1);
        }
        int temp;

        // printf(BGRN "New client connected from port number %d and IP %s \n" ANSI_RESET, ntohs(client_addr_obj.sin_port), inet_ntoa(client_addr_obj.sin_addr));

        handle_connection(client_socket_fd, node);
    }
    return NULL;
}

int main(int argc, char *argv[])
{

    int n, m;
    scanf("%d %d", &n, &m);
    maxclients = 2 * m + 1;
    vector<int> adj[n];
    vector<int> weight[n];
    distances = (int *)malloc(n);
    parent = (int *)malloc(n);
    portbase = PORT_ARG;
    numberofnodes = n;
    pthread_mutex_init(&c_lock, NULL);
    for (int i = 0; i < m; i++)
    {
        int x, y, z;
        scanf("%d %d %d", &x, &y, &z);
        adj[x].push_back(y);
        adj[y].push_back(x);
        weight[x].push_back(z);
        weight[y].push_back(z);
        sleeptime[{x, y}] = z;
        sleeptime[{y, x}] = z;
    }

    dijsktra(adj, weight, n);
    for (int i = 0; i < n; i++)
    {
        if (parent[i] == 0)
            parent[i] = i;
    }
    pthread_t serverthreads[n];
    int server[n];
    for (int i = 0; i < n; i++)
    {
        server[i] = i;
        ports[i] = portbase + c;
        c++;
        pthread_create(&serverthreads[i], NULL, serverthread, (void *)&server[i]);
    }
    for (int i = 0; i < n; i++)
    {
        pthread_join(serverthreads[i], NULL);
    }
}