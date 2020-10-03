/*
 * This program is used to test the scanning functionality of the sanitizer module of AntibIoTic.
 * It starts listening on port PORT and waits indefinitely.
 * */

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h> 
#include <errno.h>

#define PORT 1111

int main(int argc, char **args)
{
	int socket_fd, socket_clt, err;
	struct sockaddr_in srv_addr;
	int port;
	int addrlen = sizeof(srv_addr);
	// Create socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
	{
		perror("socket()");
		printf("Failed to create socket. Error:%d\n", errno);
		return socket_fd;
	}

	// Bind the socket to localhost:PORT
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(argc > 1)
		port = atoi(args[1]);
	else
		port = PORT;
	srv_addr.sin_port = htons(port);
	err = bind(socket_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr));
	if (err == -1)
	{
		perror("bind()");
		printf("Failed to bind socket to localhost:%d. Error: %d\n",port, errno);
		return errno;
	}

	// Start listening for one connection
	err = listen(socket_fd, 1);
	if (err == -1)
	{
		perror("listen()");
		printf("Failed to listen for new connections.\n");
		return errno;
	}

	while(1)
	{
		printf("Program waiting for connections on port %d...\n", port);
		// Wait for connections
		socket_clt = accept(socket_fd, (struct sockaddr*) &srv_addr, (socklen_t*) &addrlen);
		if (socket_clt == -1)
		{
			perror("accept()");
			printf("Failed to accept connection. Error: %d\n",errno);
			return errno;
		}else
		{
			printf("Connection established with the client.\n");
			printf("Closing connection...\n");
			err = close(socket_clt);
			if (err == -1)
			{
				perror("close()");
				printf("Failed to close the connection. Error: %d\n",errno);
				return errno;
			}
			printf("Connection closed.\n");
		}
	}
	printf("Exiting...\n");
	return 0;
}

