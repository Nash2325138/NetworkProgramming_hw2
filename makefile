CC=g++ -Wall
ser_c=server.cpp
cli_c=client.cpp
all: ${ser_c} ${cli_c}
	${CC} -o client ${cli_c}
	${CC} -o server ${ser_c}
client: ${cli_c}
	${CC} -o client ${cli_c} 
server: ${ser_c}
	${CC} -o server ${ser_c}
