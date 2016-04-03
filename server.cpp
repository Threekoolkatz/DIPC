#include<iostream>
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h>
#include<cstring>
#include<cstdlib>

using namespace std;

char **mailboxes;
int *semaphores;

void *connection_handler (void *socket_desc);
void write_message (int boxnum, char *message);
void read_message (int boxnum, int &socket);
 
int main(int argc , char *argv[])
{
	int numBoxes, boxSize, packetSize;
	char port[25];

	if ( argc != 5 )
	{
		cerr << "usage:  dipc <number of boxes> <size of boxes in kbytes> <port> <size of packets in kbytes>" << endl;
		return -1;
	}
	int socket_desc , client_sock , c , read_size;
	int *new_sock;
	struct sockaddr_in server , client;
	char client_message[2000];

	numBoxes = atoi(argv[1]);
	boxSize = atoi(argv[2]);
	packetSize = atoi(argv[4]);


	//Create array of mailboxes
	mailboxes = new(nothrow) char*[numBoxes];
	for( int i = 0; i < numBoxes; i++)
		mailboxes[i] = new(nothrow) char[boxSize * 1024];
	
	//Create and initialize array of semaphores
	semaphores = new(nothrow) int[numBoxes];
	for( int i = 0; i < numBoxes; i++)
		semaphores[i] = 0;

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 50007 );

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");
		 
		pthread_t sniffer_thread;
		new_sock = new(nothrow) int;
		*new_sock = client_sock;
		 
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
		    perror("could not create thread");
		    return 1;
		}
		 
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");
	}

	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
	}

	return 0;
}

void *connection_handler (void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char client_message[2000];
	char *read_error, *usage, *write_error, *success;
	char *tokens;
	int boxnum = 0;

	cout << "Connection to: " << sock << " established... " << endl;


	read_error = "No mailbox provided, please try again.  usage:  r <boxnum> ";
	write_error = "Either no mailbox specified or no message, please try again.  usage: w <boxnum> <message>";
	usage = "Message format:  1) 'q'  - will kill client connection  2) 'r' <boxnum> - read all messages from mailbox[boxnum] 3) 'w' <boxnum> <message> - writes whatever is in message to mailbox[boxnum]";
	success = "Message written succesfully";

	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{	
		//Send the message back to client
		//write(sock , client_message , strlen(client_message));

		//THIS IS WHERE WRITING/READING TO SHARED MEMORY GOES
		tokens = strtok(client_message, " \t");
		if( strcmp(tokens, "q") == 0)
			cout << "Shutdown socket" << endl;
			//shutdown(sock, 2); // shuts down transmission and recieving for this socket

		else if( strcmp(tokens, "r") == 0)
		{
			tokens = strtok(NULL, " \t");
			if(tokens == NULL)
				write(sock, read_error, strlen(read_error));
			// read from mailbox[tokens]
			else
			{	boxnum = atoi(tokens) - 1;
				read_message(boxnum, sock);
			}
		}
		
		else if( strcmp(tokens, "w") == 0)
		{
			tokens = strtok(NULL, " \t");

			// checks if a mailbox number was given
			if(tokens == NULL)
				write(sock, write_error, strlen(write_error));
			else
			{
				boxnum = atoi(tokens) - 1;
				cout << "writing to box " << boxnum << ": " << endl;
				tokens = strtok(NULL, "");
				// checks if there was a message given
				if(tokens == NULL)
					write(sock, write_error, strlen(write_error));

				// write message to mailbox[boxnum]
				else
				{
					cout << "Message: " << tokens << endl;
					write_message(boxnum, tokens);
					write(sock, success, strlen(success));
				}
			}
		}
		else
			write(sock, usage, strlen(usage));
	
	}


	return 0;
}

void write_message (int boxnum, char *message)
{
	// check mailbox semaphore
	if(semaphores[boxnum] == 0)
	{
		semaphores[boxnum] = 1;
		strcpy(mailboxes[boxnum], message);
	}
	semaphores[boxnum] = 0;
}

void read_message (int boxnum, int &socket)
{
	if(semaphores[boxnum] == 0)
	{
		semaphores[boxnum] = 1;
		write(socket, mailboxes[boxnum], strlen(mailboxes[boxnum]));
	}
	semaphores[boxnum] = 0;

}























