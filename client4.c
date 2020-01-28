#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define BUFFSIZE    1024																	//Maximale Anzahl an Characters, die in Buffer geschrieben werden können
#define PORT        33370																	//Port-Nummer, könnte eine Zahl zwischen 1024 und 65535 sein (größer als well-known ports)
#define BILLION 	1000000000																//BIlLION-Makro für Abfangung von Überlauf der nanosekunden in Zeitmessung
#define NLOOPS		1000																	//wie oft die Zeitmessung eines reads durchgeführt werden soll


int main(int argc, char *argv[]) {
    char buff_in[BUFFSIZE];																	//buffer für Inhalt der Datei, vom Server gelesen
	char buff_out[BUFFSIZE];																//buffer für filepath/name der Datei, wird an Server geschickt
	struct timespec end, start;																//end und start punkte der Zeitmessung
	clockid_t clk_id = CLOCK_MONOTONIC_RAW;													//benutzte Uhr, für Nanosekunden Auflösung 
	
    int socketfd;																			//socket zum Server
    struct sockaddr_in server_address;														//IP socket Adresse

    socketfd = socket(AF_INET, SOCK_STREAM, 0);												//creates an endpoint for communication and returns a file
																							//descriptor that refers to that endpoint
    if (socketfd == -1) {
        printf("socket creation failed\n");
        exit(0);
    }

    memset(&server_address, 0, sizeof(server_address));										//setze alle Adress-Werte auf 0
    server_address.sin_family      = AF_INET;												//protocol family IPv4
    server_address.sin_port        = htons(PORT);											//Konvertiert Port-Nummer zu Network-Byte-Order
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");								//IP Adresse,in der unsigned long int Network-Byte-Order

   
	
    if (connect(socketfd, (struct sockaddr *) &server_address, 								//connects the socket referred to by the file descriptor socketfd
				sizeof(struct sockaddr_in)) != 0) {											//to the address specified by server_address
        printf("connect failed\n");
        exit(0);
    }
    unsigned long accum = 0;																//Variable, benutzt für Messung der gesamt benötigten Zeit
    unsigned long z = 0;																	//Variable, für einzelne Messung
	unsigned long avg_time = 0;																//Gemittelte Zeit der Gesamtzeit über Anzahl der Durchläufe
	while(1) {														
		memset(buff_out, 0, BUFFSIZE);
		memset(buff_in, 0, BUFFSIZE);
		int n =0;
		
		while ((buff_out[n++] = getchar()) != '\n');										//Datei-pfad in buff_out einlesen
		if (strncmp(buff_out, "exit", 4) != 0) 												//gebe nur aus, wenn nicht 'exit' eingegeben wurde
			printf("Request contents of file: %s\n", buff_out);
			
		for(int i = 0; i<NLOOPS; i++) {														//nloops-mal die in der Konsole angegebene Datei anfragen
			if(clock_gettime(clk_id, &start) < 0) {											//Startpunkt der Zeitmessung
				printf("clock fail\n");
				exit(0);
			}
			if (write(socketfd, buff_out, sizeof(buff_out)) == -1) {						//Schicke Inhalt von buff_out an fd socketfd
				printf("write to socket failed\n");
				exit(0);
			}
			if (read(socketfd, buff_in, sizeof(buff_in)) == -1) {							//Lese vom Server zum fd socketfd geschriebene Werte
				printf("read from socket failed\n");
				exit(0);
			}
			if(clock_gettime(clk_id, &end) < 0) {											//Endpunkt der Zeitmessung
				printf("clock fail\n");
				exit(0);
			}														
			if(start.tv_nsec > end.tv_nsec) { 												//Überlauf abfangen, indem die Sekunden-Werte auf Größe der Nanosekunden erweitert werden
				z = (((end.tv_sec -1) - start.tv_sec) * BILLION) + 							//und dann die Startwerte von den Endwerten abgezogen werden
				((end.tv_nsec + BILLION)- start.tv_nsec);
				accum += z;
			} else {																	
				z = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);			//Berechnung der Zeitmessung, wenn kein Überlauf aufgetreten ist
				accum += z;
			}
		}
		
		avg_time = accum / NLOOPS;															//Mittelwert der Messungen bestimmen
		printf("Asynchroner Read braucht: %ld nanosec\n", avg_time);						//Mittelwert ausgeben
		printf("From server: %s\n", buff_in);												//Datei-Inhalt ausgeben
		
		if (strncmp(buff_out, "exit", 4) == 0) {											//Wenn exit in Konsole eingegeben wird, aus Eventschleife ausbrechen
            printf("client exit...\n");
            break;
        }
	}
    close(socketfd);																		//filedescriptor socketfd schließen
    return 0;																				//client beenden
}