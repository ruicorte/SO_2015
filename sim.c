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

/************************************************************************************************************
*	variaveis globais
*	descricao: podem ser acedidas em qualquer momento e local durante a execucao do simulador
************************************************************************************************************/

int num_carro;

time_t start;

struct sockaddr_un serv_addr;
int sockfd, servlen;

//variaveis com informacao da configuracao
int TEMPO_SIMULACAO, TEMPO_MEDIO_CRIACAO_CARROS;

int corre=0, pausa=0, controlo_pausa;

/************************************************************************************************************
*	funcao: tarefa_carro
*	argumentos: ponteiro do tipo void
*	devolve: ponteiro do tipo void
*	descricao: comportamento de uma thread que representa um carro
************************************************************************************************************/
void *tarefa_carro(void *ptr)
{
	int id = num_carro++;
	char buffer_c[256];

	int i;
			
	printf("O carro %d foi criado.\n", id);
	sprintf(buffer_c, "CARRO %d\n", id);
	send(sockfd,buffer_c,sizeof(buffer_c),0);

	return NULL;
}

// Funcao que trata dos pedidos vindos do Monitor 
void *recebe_comandos_monitor(void *arg)
{
	struct sockaddr_un cli_addr;
	int done, n, id;

	int sockfd=*((int *) arg), clilen=sizeof(cli_addr);

	char buffer[256];
	
	// Ciclo que fica a espera dos pedidos do monitor, para lhes dar resposta
	while(1)
	{
		done=0;	
		if((n=recv(sockfd, buffer, sizeof(buffer), 0)) <= 0)
		{
			if(n < 0) 
				perror("recv error");
			done=1;
		}
		buffer[n]='\0';

		if(!strcmp(buffer, "fim\n"))
		{
			corre=0;
			exit(1);
		}
		else
		{	
			if(!strcmp(buffer, "inicio\n"))
				corre = 1;
			if(!strcmp(buffer, "carro\n"))
			{
				pthread_t thread;
				pthread_create(&thread, NULL, &tarefa_carro, &sockfd);
			}
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	
	if(argc<2)
	{
		printf("Ficheiro de configuração em falta. Não é possível continuar.");
		return 1;
	}
	else
	{
		//interpretacao do ficheiro de configuraçao
		int *conf = leitura_configuracao(argv[1]);
		num_carro = conf[0];
		
		//criacao do socket e ligação
		if((sockfd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
			perror("Simulador: cant open socket stream");
		serv_addr.sun_family=AF_UNIX;
		strcpy(serv_addr.sun_path, UNIXSTR_PATH);
		servlen=strlen(serv_addr.sun_path)+sizeof(serv_addr.sun_family);
		if(connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
			perror("connect error");

		//Criacao da tarefa que ira tratar dos pedidos enviados pelo Monitor
		pthread_t thread;
		pthread_create(&thread, NULL, &recebe_comandos_monitor, &sockfd);
		
		while(!corre);

		char buffer[256];
		
		sprintf(buffer, "INICIO\n");
		send(sockfd,buffer,sizeof(buffer),0);

		while(corre);
		
		printf("\nSimulação terminada.\n");
		sprintf(buffer,"FIM \n");
		send(sockfd,buffer,sizeof(buffer),0);
		
		close(sockfd);
	}
	return 0;
}
