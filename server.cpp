#include<iostream>
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h>
#include<cstring>
#include<cstdlib>
#include<sys/wait.h>


using namespace std;

char **mailboxes;
int *semaphores;
int packetSize;
int killFlag = 0;

void *connection_handler (void *socket_desc);
void write_message (int boxnum, char *message, int &socket);
void read_message (int boxnum, int &socket);
 
int main(int argc , char *argv[])
{
	int numBoxes, boxSize, port;

	if ( argc != 5 )
	{
		cerr << "usage:  dipc <number of boxes> <size of boxes in kbytes> <port> <size of packets in kbytes>" << endl;
		return -1;
	}

	HWND window;
	AllocConsole();
	window = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(window,0);

	int socket_desc , client_sock , c , read_size;
	int *new_sock;
	struct sockaddr_in server , client;
	char client_message[2000];

	numBoxes = atoi(argv[1]);
	boxSize = atoi(argv[2]);
	packetSize = atoi(argv[4]);

	port = atoi(argv[3]);
	cout << port << endl;

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
	server.sin_port = htons( port );

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
		if(killFlag == 1)
			break;
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

	for(int i = 0; i < numBoxes; i++)
		delete mailboxes[i];
	delete mailboxes;
	delete semaphores;

	return 0;
}

void *connection_handler (void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char client_message[2000];
	char *read_error, *usage, *write_error, *success, *packet_error;
	char *tokens;
	int boxnum = 0;

	cout << "Connection to: " << sock << " established... " << endl;


	read_error = "No mailbox provided, please try again.  usage:  r <boxnum> ";
	write_error = "Either no mailbox specified or no message, please try again.  usage: w <boxnum> <message>";
	usage = "Message format:  1) 'q'  - will kill client connection  2) 'r' <boxnum> - read all messages from mailbox[boxnum] 3) 'w' <boxnum> <message> - writes whatever is in message to mailbox[boxnum]";
	success = "Message written succesfully";
	packet_error = "Your message was too large for the packet to send, make it smaller";

	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{	
		if( read_size > packetSize * 1024)
		{
			write(sock, packet_error, strlen(packet_error));
		}
		//Send the message back to client
		//write(sock , client_message , strlen(client_message));

		//THIS IS WHERE WRITING/READING TO SHARED MEMORY GOES
		else
		{	
			tokens = strtok(client_message, " \t");
			if( strcmp(tokens, "dipcdel") == 0)
				killFlag = 1;
			
			else if( strcmp(tokens, "q") == 0)
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
						write_message(boxnum, tokens, sock);
						write(sock, success, strlen(success));
					}
				}
			}
			else
				write(sock, usage, strlen(usage));
		}
	}


	return 0;
}

/*****************************************************************************************
* Function:  write_message
*
* Author:  Mack Smith
*
* Description:  This function takes a message from the client and puts it in the mailbox
* denoted with boxnum.  It replaces any message already in the mailbox.  It then sends a 
* message back to the client when the message is written 
*
* Parameters: 
*	1) int boxnum - the mailbox number used to index to the desired mailbox
*	2) char *message - a pointer to the message from the client
*	3) int &socket - the socket associated with the client, used to send messages back
*
******************************************************************************************/
void write_message (int boxnum, char *message, int &socket)
{	
	char *wait_message = "Waiting to write message";

	while(semaphores[boxnum] == 1){write(socket, wait_message, strlen(wait_message));}
	// check mailbox semaphore
	if(semaphores[boxnum] == 0)
	{
		semaphores[boxnum] = 1;
		strcpy(mailboxes[boxnum], message);
	}
	semaphores[boxnum] = 0;
}

/*****************************************************************************************
* Function:  read_message
*
* Author:  Mack Smith
*
* Description:  This function takes in a box number to identify the desired mailbox to read
* from.  It then uses the socket descriptor to write the message from the mailbox back to
* the client.  
*
* Parameters:
*	1) int boxnum - the mailbox number used to index to the desired mailbox
*	2) int &socket - the socket associated with the client, used to send messages back
*
******************************************************************************************/
void read_message (int boxnum, int &socket)
{
	char *wait_message = "Waiting to read message";

	while(semaphores[boxnum] == 1){write(socket, wait_message, strlen(wait_message));}
	if(semaphores[boxnum] == 0)
	{
		semaphores[boxnum] = 1;
		write(socket, mailboxes[boxnum], strlen(mailboxes[boxnum]));
	}
	semaphores[boxnum] = 0;
}























