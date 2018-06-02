SRC=./src/server.cpp ./src/main.cpp  ./src/common.cpp  ./src/cdbparam.cpp
INCLUDE= -I./include  
LIB=  -levent -lc -lrt -lcurl -lpthread 
APP= relay
CFLAG=-std=c++11
DEBUG=-g
server:
	g++ $(CFLAG) $(DEBUG) $(SRC) $(INCLUDE) -o $(APP) $(LIB)  

clean:
	rm -rf $(APP)
