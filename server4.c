#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <aio.h>
#include <signal.h>
#include <errno.h>


#define BUFFSIZE    1024																	//Maximale Anzahl an Characters, die in Buffer geschrieben werden können
#define PORT        33370																	//Port-Nummer, könnte eine Zahl zwischen 1024 und 65535 sein (größer als well-known ports)
#define MAX_CONN 5																			//Maximale Anzahl an möglichen Verbindungen


fd_set activeFDs,readFDs;																	//Initizalisierung von den beiden Filedescriptern
char path[BUFFSIZE];																		//Erstellung eines Buffers für den Dateipfad
char buff[BUFFSIZE]; 																		//Erstellung eines Buffers für den Inhalt der Datei


int main(int argc, char *argv[]) {
    int cfd = 0;
    int socketfd;																			//socket zum Clienten
    int openRequests = 0; 																	//Initizalisierung einer Zählvariable zur Überprüfung der noch zu Verarbeiteten Dateien
    struct sockaddr_in my_addr, peer_addr; 													//IP socket Adresse
    socklen_t peer_addr_size;																//Größe der Adresse
    struct aiocb *aiocbList;																//aiocb Struct definiert Parameter, die eine I/O Ausführung kontrollieren
	
	
    if((aiocbList = malloc(sizeof(struct aiocb))) == NULL) {
		printf("malloc of aiocbList failed\n");
		exit(0);
	}

	socketfd = socket(AF_INET, SOCK_STREAM, 0);												//Erzeugt ein Ende der Kommunikation und gibt den Inhalt der Datei wieder
																							//descriptor that refers to that endpoint
    if (socketfd== -1) {																	//Fehlerbehandlung falls die Erzeugung des Sockets fehl schlug
		printf("socket creation failed\n");
		exit(0);
    }

    memset(&my_addr, 0, sizeof(struct sockaddr_in));										//Setzt alle Adresswerte auf 0
    my_addr.sin_family = AF_INET;															//Protokoll zur family IPv4
    my_addr.sin_port = htons(PORT); 														//Konvertiert Port-Nummer zu Network-Byte-Order
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);											//Akzeptiert verschiedene IP-Adressen(Network-Byte-Order)
	
	
    if(bind(socketfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {				//Verbindung zum Socket überprüfen, Adresse my_addr dem socket socketfd zuordnen
		printf("socket bind failed\n");
		exit(0);
    }

    if(listen(socketfd, MAX_CONN) == -1){													//Warten auf Clienten, markieren als passiven socket, damit accept() auf ihn angewendet werden kann			
		printf("listen failed\n");
		exit(0);
    }
	
	
    peer_addr_size = sizeof(struct sockaddr_in);											
    FD_ZERO(&activeFDs);																	//Initizalisierung des file Descriptor set
    FD_SET(socketfd, &activeFDs);															//Initizalisierung des file Descriptor set
	
    while(1) {
	readFDs = activeFDs;
	if (select(FD_SETSIZE, &readFDs, NULL, NULL, NULL)== -1) {								//Wartet auf Eingabe von einem oder mehreren aktiven Verbindungen
		printf("select failed\n");															//filedescriptors in readFDs werden überwacht, ob Zeichen zum Lesen zur Verfügung stehen 
	    exit(0);
	}
		for(int i = 0; i < FD_SETSIZE; ++i) {												//Bedienung von allen Sockets mit Eingabe
			if(FD_ISSET(i, &readFDs)) {
				if(i == socketfd) {
					cfd = accept(socketfd, (struct sockaddr *) &peer_addr, &peer_addr_size);//extract first request in queue of pending conections for listening socket, socketfd 
					if(cfd == -1) {															//Fehlerbehandlung für Scheitern der Verbindung
						printf("accept failed");
						exit(0);
					}
					FD_SET(cfd, &activeFDs);												//füge extrahierten filedescriptor zum aktivem set hinzu
					openRequests++;															//erhöhe die Zahl der offenen Requests, wird bei der Behandlung von einem read fail benötigt
				}else {
					read(i, path, sizeof(path));											//Lesen des Eingabepfads
					if ((strncmp(path, "exit", 4)) == 0) {									//Überprüfung ob exit eingegeben wurde, wenn ja Verbindung schließen
						printf("received 'exit'\n");
						close(socketfd);
						return 0;
					}

					struct aiocb *my_aiocb = malloc(sizeof(struct aiocb));
					my_aiocb->aio_fildes = open(strtok(path, "\n"), O_RDONLY); 	            //Filedescriptor auf welchem die E/A Interaktion durchgeführt wird
					my_aiocb->aio_nbytes = BUFFSIZE;                            			//Größe des Buffers
					my_aiocb->aio_buf = buff;                                   			//Buffer 
					my_aiocb->aio_offset = 0;                                   			//File offset
					my_aiocb->aio_reqprio = 0;                                  			//Priorisierung in der auszuführenden Reihenfolge wird festgelegt 
					my_aiocb->aio_lio_opcode = LIO_READ;                        			//Die Operation Lesen wird durchgeführt

					if (aio_read(my_aiocb) != 0) {											//Asynchroner Lesevorgang vom Filedescriptor in den buffer
						printf("aio_read failed");				
						free(my_aiocb);
						FD_CLR(i, &activeFDs);
						close(i);
						openRequests -= 1;													//Anzahl offener Datein zum Lesen reduzieren
						if (openRequests== 0) {												//Wenn keine Datei zum lesen mehr vorhanden ist Verbindung schließen
							close(socketfd);
							return 0;
						}
					}
					int err;
					while ((err = aio_error(my_aiocb)) == EINPROGRESS); 					//Mit EINPROGRESS als Wiedergabe wird induziert, dass die Anfrage noch nicht fertig ist -> 0 wenn fertig
					if (err == 0) {
						printf("%s\n", buff);												//Da die Anfrage fertig ist, wird sie ausgegeben und danach der Buffer gecleared
						write(i, buff, sizeof(buff));
						bzero(buff, sizeof(buff));
						break;
					} else {
						printf("aio_error failed\n");
						return -1;
					}		
				}
			}
		}
	}
	close(socketfd);																		//Filedescriptor socketfd schließen
	return 0;																				//Server beenden
}
