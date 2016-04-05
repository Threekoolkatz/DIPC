#----------------------------------------------------------------------------

# Use the GNU C/C++ compiler.

CC = gcc
CPP = g++

# COMPILER OPTIONS

CFLAGS = -c

LDFLAGS = -pthread

# OBJECT FILES
OBJS = server.o

server: server.o
	${CPP} -lm ${LDFLAGS} ${OBJS} -o server

server.o: server.cpp

#---------------------------------------------------------------------------
