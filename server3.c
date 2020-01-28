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


#define BUFFSIZE    1024																	//Maximale Anzahl an Characters, die in Buffer geschrieben werden können
#define PORT        33370																	//Port-Nummer, könnte eine Zahl zwischen 1024 und 65535 sein (größer als well-known ports)
#define MAX_CONN 5																			//Maximale Anzahl an möglichen Verbindungen


fd_set activeFDs,readFDs;																	//Initizalisierung von den beiden Filedescriptern
char path[BUFFSIZE];																		//Erstellung eines Buffers für den Dateipfad
char buff[BUFFSIZE]; 																		//Erstellung eines Buffers für den Inhalt der Datei


int main() {
    FILE *file_ptr;
	int cfd = 0;
    int socketfd;																			//socket zum Clienten
    int openRequests = 0; 																	//Initizalisierung einer Zählvariable zur Überprüfung der noch zu Verarbeiteten Dateien
    struct sockaddr_in my_addr, peer_addr; 													//IP socket Adresse
    socklen_t peer_addr_size;																//Größe der Adresse
	

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
	
	
    if(bind(socketfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {				//Verbindung zum Socket überprüfen
		printf("socket bind failed\n");
		exit(0);
    }

    if(listen(socketfd, MAX_CONN) == -1){													//Warten auf den Clienten
		printf("listen failed\n");
		exit(0);
    }

    peer_addr_size = sizeof(struct sockaddr_in);
    FD_ZERO(&activeFDs); 																	//Initizalisierung des file Descriptor set
    FD_SET(socketfd, &activeFDs); 															//Initizalisierung des file Descriptor set

    while(1) {
	readFDs = activeFDs; 
	if (select(FD_SETSIZE, &readFDs, NULL, NULL, NULL)== -1) { 								//Wartet auf Eingabe von einem oder mehrer aktiven Verbindungen
	    printf("select failed\n");
	    exit(0);
	}
		for(int i = 0; i < FD_SETSIZE; ++i) { 												//Bedienung von allen Sockets mit Eingabe
			if(FD_ISSET(i, &readFDs)) {
				if(i == socketfd) {
					cfd = accept(socketfd, (struct sockaddr *) &peer_addr, &peer_addr_size);//Verbindungsanfrage für die eigentliche Socket Verbindung
					if(cfd == -1) {															//Fehlerbehandlung für Scheitern der Verbindung
						printf("accept failed");
						exit(0);
					}
					FD_SET(cfd, &activeFDs);												//füge extrahierten filedescriptor zum aktivem set hinzu
					openRequests++;															//erhöhe die Zahl der offenen Requests, wird bei der Behandlung von einem read fail benötigt
				}else {
                    read(cfd, path, sizeof(path)); 											//Lesen des Eingabepfads
					if ((strncmp(path, "exit", 4)) == 0) { 									//Überprüfung ob exit eingegeben wurde wenn ja Verbindung schließen
						printf("received 'exit'\n");
						close(socketfd);
						return 0;
					}
					
					file_ptr = fopen(strtok(path, "\n"), "r"); 								//Vorhandene Datei im Dateipfad öffnen falls möglich sonst Fehler
					if (file_ptr == NULL) {
						printf("fopen failed\n");
						exit(0);
					}
					
					size_t buff_len = fread(buff, sizeof(char), BUFFSIZE, file_ptr); 		//Dateiinhalt in den dafür vorgesehenen Buffer schreiben
					
					if((int)buff_len < 1) {													//Falls der gelesene String leer war Fehler ausgeben und diese Verbindung schließen
						printf("empty or broken file read\n");
						FD_CLR(i, &activeFDs);
						close(i);
						openRequests -= 1; 													//Anzahl offener Datei zum lesen reduzieren
						if (openRequests== 0) { 											//Wenn keine Datei zum lesen mehr vorhanden ist Verbindung schließen
							close(socketfd);
							return 0;
						}
					}else{
						buff[buff_len+1] = '\n';											//markiere end of file
						printf("%s\n", buff);												//Ausgabe des buffers zur Kontrolle
						write(cfd, buff, sizeof(buff));										//schreib den Inhalt der Datei an client zurück
						memset(buff,0, sizeof(buff));										//clear buffer
						break;
					}
                }
            }
        }
    }
    close(socketfd); 																		//Filedescriptor socketfd schließen
	return 0; 																				//Server beenden
}
