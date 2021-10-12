
#default start from here 
all: shared_lib_dplist shared_lib_tcpsock sensor_gateway sensor_node file_creator

#compiled as shared libraries
shared_lib_dplist: lib/dplist.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB dplist *****$(NO_COLOR)"
	gcc -c -g lib/dplist.c -Wall -std=c11 -Werror -fPIC -o lib/dplist.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING LIB dplist< *****$(NO_COLOR)"
	gcc lib/dplist.o -o lib/libdplist.so -Wall -shared -lm -fdiagnostics-color=auto

shared_lib_tcpsock: lib/tcpsock.c
	@echo "$(TITLE_COLOR)\n***** COMPILING LIB tcpsock *****$(NO_COLOR)"
	gcc -c -g lib/tcpsock.c -Wall -std=c11 -Werror -fPIC -o lib/tcpsock.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING LIB tcpsock *****$(NO_COLOR)"
	gcc lib/tcpsock.o -o lib/libtcpsock.so -Wall -shared -lm -fdiagnostics-color=auto

#now compile
sensor_gateway : main.c connmgr.c datamgr.c sensor_db.c sbuffer.c lib/libdplist.so lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** CPPCHECK *****$(NO_COLOR)"
	cppcheck --enable=all --suppress=missingIncludeSystem main.c connmgr.c datamgr.c sensor_db.c sbuffer.c lib/libdplist.c lib/libtcpsock.c
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_gateway *****$(NO_COLOR)"
	gcc -c -g  main.c      -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o main.o      -fdiagnostics-color=auto
	gcc -c -g  connmgr.c   -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o connmgr.o   -fdiagnostics-color=auto
	gcc -c -g  datamgr.c   -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o datamgr.o   -fdiagnostics-color=auto
	gcc -c -g  sensor_db.c -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o sensor_db.o -fdiagnostics-color=auto
	gcc -c -g  sbuffer.c   -Wall -std=c11 -Werror -DSET_MIN_TEMP=10 -DSET_MAX_TEMP=20 -DTIMEOUT=5 -o sbuffer.o   -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_gateway *****$(NO_COLOR)"
	gcc main.o connmgr.o datamgr.o sensor_db.o sbuffer.o -ldplist -ltcpsock -lpthread -o sensor_gateway -Wall -L./lib -Wl,-rpath=./lib -lsqlite3 -fdiagnostics-color=auto

file_creator : file_creator.c
	@echo "$(TITLE_COLOR)\n***** COMPILE & LINKING file_creator *****$(NO_COLOR)"
	gcc file_creator.c -o file_creator -Wall -fdiagnostics-color=auto
	./file_creator

sensor_node : sensor_node.c lib/libtcpsock.so
	@echo "$(TITLE_COLOR)\n***** COMPILING sensor_node *****$(NO_COLOR)"
	gcc -c sensor_node.c -Wall -std=c11 -Werror -o sensor_node.o -fdiagnostics-color=auto
	@echo "$(TITLE_COLOR)\n***** LINKING sensor_node *****$(NO_COLOR)"
	gcc sensor_node.o -ltcpsock -o sensor_node -Wall -L./lib -Wl,-rpath=./lib -fdiagnostics-color=auto


run_scalability_test: sensor_gateway sensor_node
	gnome-terminal -- valgrind -v  --leak-check=full --show-reachable=yes ./sensor_gateway 1234
	sleep 2
	gnome-terminal -- ./sensor_node 15 1 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 21 2 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 37 2 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 49 3 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 129 4 127.0.0.1 1234


run_stress_test: sensor_gateway sensor_node
	gnome-terminal -- valgrind -v  --leak-check=full --show-reachable=yes ./sensor_gateway 1234
	sleep 2
	gnome-terminal -- ./sensor_node 15 1 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 21 1 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 37 1 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 49 1 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 129 1 127.0.0.1 1234


run_three_node_test: sensor_gateway sensor_node
	gnome-terminal -- valgrind -v  --leak-check=full --show-reachable=yes ./sensor_gateway 1234
	sleep 2
	gnome-terminal -- ./sensor_node 15 5 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 21 5 127.0.0.1 1234
	gnome-terminal -- ./sensor_node 37 5 127.0.0.1 1234


run_sequence_test: sensor_gateway sensor_node
	gnome-terminal -- valgrind -v  --leak-check=full --show-reachable=yes ./sensor_gateway 1234
	sleep 2
	gnome-terminal -- ./sensor_node 15 4 127.0.0.1 1234
	sleep 2
	gnome-terminal -- ./sensor_node 21 4 127.0.0.1 1234
	sleep 3
	gnome-terminal -- ./sensor_node 37 4 127.0.0.1 1234


clean:
	rm -rf *.o sensor_gateway sensor_node file_creator *~
	rm -rf lib/*.o sensor_gateway *~ 

clean-all: clean
	rm -rf lib/*.so

clean-lib:
	rm -rf lib/*.so

.PHONY : clean clean-all
# do not look for files called clean, clean-all or this will be always a target
