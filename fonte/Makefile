all: servidorMain clienteMain

clean: 
	@rm -f *.o 

servidorMain: servidorMain.c tp_socket.o temporizador.o 
	gcc servidorMain.c tp_socket.o temporizador.o -o servidorFTP

clienteMain: clienteMain.c tp_socket.o temporizador.o
	gcc clienteMain.c tp_socket.o temporizador.o -o clienteFTP

tp_socket.o: tp_socket.c tp_socket.h

temporizador.o: temporizador.c temporizador.h