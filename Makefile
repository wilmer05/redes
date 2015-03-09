CC = g++
HEAD = auxiliares.h
OBJ = cliente.o auxiliares.o
OBJ2 = auxiliares.o servidor.o
OPT = -lpthread
PROG = cliente
PROG2 = servidor

all: $(PROG) $(PROG2) 

$(PROG2): $(HEAD) $(OBJ2)
	$(CC) $(OBJ2) -o $(PROG2) $(OPT)

$(PROG): $(HEAD) $(OBJ)
	$(CC) $(OBJ) -o $(PROG) $(OPT)

cliente.o: $(HEAD) cliente.cpp
	$(CC) -c cliente.cpp $(OPT)
	
servidor.o: $(HEAD) servidor.cpp
	$(CC) -c servidor.cpp $(OPT)
	
auxiliares.o:
	$(CC) -c auxiliares.cpp $(OPT)
	
clean:
	rm cliente.o $(PROG) $(OBJ2) $(PROG2)
