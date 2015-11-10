/****************************************************************************************************
* sistemas operativos - 2015/2016 - simulacao de discoteca
*
* Docente: Eduardo Marques
*
* Alunos:
* David Ricardo Cândido nº 2032703
* Luís Bouzas Prego 	nº 2081815
*
* Ficheiro: sim.c
****************************************************************************************************/

#include <time.h>
#include <stdlib.h>

#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "unix.h"
#include "util.h"

// variaveis globais
int num_cliente;
time_t start;
struct sockaddr_un serv_addr;
int sockfd, servlen;

int corre=0;

// tarefa cliente
void *tarefa_cliente(void *ptr){
	int id = num_cliente++;
	char buffer_c[256];
	int i;			
	printf("Sou o cliente %d e cheguei a discoteca.\n", id);
	sprintf(buffer_c, "CHEGADA CLIENTE %d\n", id);
	send(sockfd,buffer_c,sizeof(buffer_c),0);

	return NULL;
}

// tratamento dos pedidos do monitor
void *recebe_comandos_monitor(void *arg){
	struct sockaddr_un cli_addr;
	int done, n, id;

	int sockfd=*((int *) arg), clilen=sizeof(cli_addr);

	char buffer[256];
	
	// espera pelos pedidos do monitor
	while(1){
		done=0;	
		if((n=recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(n < 0) 
				perror("recv error");
			done=1;
		}
		buffer[n]='\0';

		if(!strcmp(buffer, "fim\n")){
			corre=0;
			exit(1);
		} else {	
			if(!strcmp(buffer, "inicio\n")){
				corre = 1;
			}
			if(!strcmp(buffer, "cliente\n")){
				pthread_t thread;
				pthread_create(&thread, NULL, &tarefa_cliente, &sockfd);
			}
		}
	}
	return NULL;
}

int main(int argc, char *argv[]){
	if(argc<2){
		printf("falta ficheiro de configuracao");
		return 1;
	} else {
		// interpretacao do ficheiro de configuraçao
		int *conf = leitura_configuracao(argv[1]);
		num_cliente = conf[0];
		
		// socket
		if((sockfd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
			perror("simulador: erro na abertura do socket");
		serv_addr.sun_family=AF_UNIX;
		strcpy(serv_addr.sun_path, UNIXSTR_PATH);
		servlen=strlen(serv_addr.sun_path)+sizeof(serv_addr.sun_family);
		if(connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
			perror("erro de ligacao");

		// tarefa que trata dos pedidos enviados pelo monitor
		pthread_t thread;
		pthread_create(&thread, NULL, &recebe_comandos_monitor, &sockfd);
		
		while(!corre);

		char buffer[256];
		
		sprintf(buffer, "INICIO\n");
		send(sockfd,buffer,sizeof(buffer),0);

		while(corre);
		
		sprintf(buffer,"FIM \n");
		send(sockfd,buffer,sizeof(buffer),0);
		printf("\nfim da simulacao.\n");
		
		close(sockfd);
	}
	return 0;
}