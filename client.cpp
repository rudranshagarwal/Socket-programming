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

#define SERVER_PORT 3080

const long long int buff_sz = 1048576;
pair<string, int> read_string_from_socket(int fd, int bytes)//read from socket
{
    std::string output;
    output.resize(bytes);

    int bytes_received = read(fd, &output[0], bytes - 1);
    if (bytes_received <= 0)
    {
        cerr << "Failed to read data from socket. Seems server has closed socket\n";
        exit(-1);
    }

    output[bytes_received] = 0;
    output.resize(bytes_received);

    return {output, bytes_received};
}

int send_string_on_socket(int fd, const string &s)//write to socket
{
    int bytes_sent = write(fd, s.c_str(), s.length());

    if (bytes_sent < 0)
    {
        cerr << "Failed to SEND DATA on socket.\n";
        // return "
        exit(-1);
    }

    return bytes_sent;
}


int main(int argc, char *argv[])
{
    struct sockaddr_in server_obj;

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("%d\n", socket_fd);
    if (socket_fd < 0)
    {
        perror("Error in socket creation for CLIENT\n");
        exit(-1);
    }
    int port_num = SERVER_PORT;

    memset(&server_obj, 0, sizeof(server_obj));
    server_obj.sin_family = AF_INET;
    server_obj.sin_port = htons(port_num);

    if (connect(socket_fd, (struct sockaddr *)&server_obj, sizeof(server_obj)) < 0)//establish connection
    {
        perror("Problem in connecting to the server\n");
        exit(-1);
    }

    printf("Connected to server\n");

    while (true)
    {
        string to_send;
        cout << "Enter msg: ";
        getline(cin, to_send);
        send_string_on_socket(socket_fd, to_send);//write message
        int num_bytes_read;
        string output_msg;
        tie(output_msg, num_bytes_read) = read_string_from_socket(socket_fd, buff_sz);//wait for acknowlegement
        cout << "Received: " << output_msg << endl;
        cout << "====" << endl;
    }
    close(socket_fd);

    return 0;
}