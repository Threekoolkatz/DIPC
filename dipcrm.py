from socket import *
HOST = 'localhost'
PORT = 50007
s = socket ( AF_INET , SOCK_STREAM )
s.connect((HOST, PORT))
message = "dipcdel"
s.send(message)
s.close()
