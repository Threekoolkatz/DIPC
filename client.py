from socket import *
HOST = 'localhost'
PORT = 50007
s = socket ( AF_INET , SOCK_STREAM )
s.connect((HOST, PORT))
while True :
  message = raw_input("> ")
  if message == 'q': break
  if message != '':
    s.send(message)
    data = s.recv(1024)
    print 'Received' , repr(data)
s.close()
