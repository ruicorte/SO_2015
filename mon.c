/****************************************************************************************************
* sistemas operativos - 2015/2016 - simulacao de discoteca
*
* Docente: Eduardo Marques
*
* Alunos:
* David Ricardo Cândido nº 2032703
* Luís Bouzas Prego 	nº 2081815
*
* Ficheiro: mon.c
****************************************************************************************************/

#include <pthread.h>
#include <string.h>
#include "unix.h"
#include "util.h"

// ficheiro para relatorio
FILE *relatorio;

// variaveis globais
int corre=0;

// escuta das mensagens do simulador
void *escuta_comunicacao(void *arg)
{
	int sockfd=*((int *) arg);
	int msg, num_comandos, id, com;
	char buffer[256],comando[20];

	// ciclo que espera mensagens do simulador
	while(1){
		if((msg=recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(msg < 0)
				perror("recv");
		} else {
           		buffer[msg]='\0';
			fprintf(relatorio,"%s", buffer);
		}
	}
	return NULL;
}


// funcao principal
int main(int argc, char *argv[]){	
	struct sockaddr_un serv_addr, cli_addr;
	int sockfd, servlen, newsockfd;

	int clilen = sizeof(cli_addr);
	char buffer[256];
	char menu[512] = "----------------------------------\n| comandos disponiveis no monitor|\n----------------------------------\n| inicio - Comecar simulacao     |\n| cliente - cria cliente         |\n| menu - mostra opçoes           |\n| fim - Sair da aplicação        |\n----------------------------------\n";

	// socket UNIX
	if((sockfd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		perror("cant open socket stream");
	serv_addr.sun_family=AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen=strlen(serv_addr.sun_path)+sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	if(bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		perror("cant bind local address");
	listen(sockfd, 1);

	if((newsockfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0)
		perror("accept error");

	// tarefa que trata da comunicação com o simulador
	pthread_t thread;
	pthread_create(&thread, NULL, &escuta_comunicacao, &newsockfd);

	// menu
	printf("%s",menu);

	unlink("relatorio.log"); // apaga ficheiro	
	relatorio=fopen("relatorio.log", "a"); // abre o ficheiro

	do{
		fgets(buffer, sizeof(buffer), stdin);

		if(send(newsockfd, buffer, sizeof(buffer), 0) == -1){
			perror("send");
			exit(1);
		}

		if(!strcmp (buffer, "inicio\n")){
			corre=1;
		}
		if(!strcmp (buffer, "menu\n")){
			printf("%s",menu);
		}
	}
	while(strcmp (buffer, "fim\n"));
	
	corre = 0;

	printf("\nsimulacao terminada\n\n");
	fclose(relatorio);
	close(sockfd);

	return 0;
}