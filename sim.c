/***********************************************************
* Sistemas Operativos - 2015/2016 - Simulacao de Discoteca 
*
* Docente: Eduardo Marques
*
* Alunos:
* David Ricardo Cândido nº 2032703
* Luís Bouzas Prego 	nº 2081815
*
* Ficheiro: sim.c
************************************************************/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include "unix.h"

// estrutura com os parametros de cada zona
typedef struct{
	char 	*nome; 					// nome da zona
	int 	lotacao, 				// a lotacao maxima sera a soma da lotacao de todas as zonas
			hora_abertura,			// comeco da contagem do tempo - em principio sera 00
			duracao, 				// duracao da simulacao - cada segundo corresponde a um minuto
			limite_alcool,			// caso o cliente não possa entrar alcoolizado nesta zona
			vip;					// entrada restrita - só para VIPs
	sem_t 	porteiro; 				// porteiro da entrada do recinto - mantem o recinto abaixo do limite de lotacao
} zona;

// variaveis globais
zona 	zonas[20];					// array que contem todas as zonas - com o limite maximo de 20 mas pode ser alterado manualmente 
int 	total_zonas = 0,			// numero de zonas
	 	simula = 0,					// controlo da execucao da simulacao
	 	lotacao_maxima = 0,			// lotacao total do recinto (a soma das lotacoes de todas as zonas)
	 	tempo_simulacao = 0,		// tempo da simulacao
	 	tempo_actual = 0,			// tempo actual da simulacao
	 	tempo_maximo_chegada = 0,	// tempo médio de chegada dos clientes ao recinto
	 	prob_desiste_filas = 0,		// probabilidade de desistencia nas filas
	 	prob_vip = 0,				// probabilidade de um novo cliente ser vip
		num_cliente = 1;			// numeracao dos clientes


int clientes_dentro = 0;
int ultimo_cliente = 0;

sem_t 	porteiro_entrada, 			// semaforo que se destina a controlar a lotacao máxima de todo o espaco
		sem_sim;
pthread_mutex_t mutex;				// mutex

time_t 	start; 						// controlo do tempo

struct 	sockaddr_un serv_addr;
int 	sockfd, servlen;









// tarefa cliente
void *cliente(void *ptr){
	pthread_detach(pthread_self()); 	// liberta recursos quando termina

	char 	buffer[256];				// array de caracteres auxiliar
	int 	zonas_cliente[total_zonas]; // referência das zonas visitadas pelo cliente
	//printf("\n\ntempo maximo chegada: %d\n\n",tempo_chegada);
	int 	tempo_chegada = rand()%tempo_maximo_chegada;

pthread_mutex_lock(&mutex);
	int id = num_cliente++; 			// referencia de cada cliente
	//printf("\n\ncliente criado: %d\n\n",id);
pthread_mutex_unlock(&mutex);
	
	sleep(tempo_chegada);

pthread_mutex_lock(&mutex);
	tempo_actual += tempo_chegada;
pthread_mutex_unlock(&mutex);


sem_wait(&porteiro_entrada);

pthread_mutex_lock(&mutex);
	if(tempo_actual <= tempo_simulacao){
		printf("\n*****tempo actual: %d\n",tempo_actual);
		pthread_t proximo_cliente;
		pthread_create(&proximo_cliente, NULL, &cliente, &sockfd);
		printf("\ncliente criou o proximo: %d\n", id);
	} else {
		printf("\n*****tempo actual: %d\n",tempo_actual);
		printf("\nentrou o ultimo cliente: %d\n", id);
		ultimo_cliente = id;
		/*
pthread_mutex_lock(&mutex);
		simula = 0;
		sprintf(buffer,"%d FIM\n", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);
pthread_mutex_unlock(&mutex);*/
	}
pthread_mutex_unlock(&mutex);

int i_cli;
pthread_mutex_lock(&mutex);
sem_getvalue(&porteiro_entrada, &i_cli);
printf("\n\nno semaforo: %d\n\n",i_cli);
pthread_mutex_unlock(&mutex);




pthread_mutex_lock(&mutex);
	clientes_dentro++;
//	printf("cliente %d entrou na discoteca.\n", id);
	sprintf(buffer, "%d CLIENTE %d ENTROU\n", tempo_actual, id);
	send(sockfd,buffer,sizeof(buffer),0);
pthread_mutex_unlock(&mutex);

sleep(rand()%10);

pthread_mutex_lock(&mutex);
sem_post(&porteiro_entrada);
	clientes_dentro--;
//	printf("cliente %d saiu da discoteca.\n", id);
	sprintf(buffer, "%d CLIENTE %d SAIU\n", tempo_actual, id);
	send(sockfd,buffer,sizeof(buffer),0);
pthread_mutex_unlock(&mutex);
	
	if(id == ultimo_cliente){
pthread_mutex_lock(&mutex);
		sem_post(&sem_sim);
pthread_mutex_unlock(&mutex);
	}
	return NULL;
}









// tratamento dos pedidos do monitor
void *recebe_comandos_monitor(void *arg){
	struct 	sockaddr_un cli_addr;
	int 	done,
			n,
			id;
	int 	sockfd = *((int *) arg), clilen=sizeof(cli_addr);

	char buffer[256];

	// espera pelos pedidos do monitor
	while(1){
		done = 0;
		if((n=recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(n < 0)
				perror("recv error");
			done = 1;
		}
		buffer[n] = '\0';

		if(!strcmp(buffer, "fim\n")){
			simula = 0;
		} else if(!strcmp(buffer, "inicio\n")){
			simula = 1;
		}
	}
	return NULL;
}

// interpretacao do ficheiro de configuracao
int configuracao(char *file)
{
	int 	*a 	= (int *)malloc(sizeof(int) * 20);
	FILE 	*fp = fopen (file, "r");
	if(fp == NULL ){
		printf("erro: abertura do ficheiro de configuracao\n");
		abort();
	}
	char 	name[50],
			buff[500],
			*item;

	while(fgets( buff, sizeof buff, fp) != NULL){
		item = strtok(buff," ");
		if(strcmp(item,"#")){
			if(strcmp(item,"ZONA")==0){ 
				// criacao de uma zona
				total_zonas++;
				zonas[total_zonas].nome = strtok(NULL," ");
				zonas[total_zonas].lotacao = atoi(strtok(NULL," "));
				zonas[total_zonas].hora_abertura = atoi(strtok(NULL," "));
				zonas[total_zonas].duracao = atoi(strtok(NULL," "));
				zonas[total_zonas].limite_alcool = atoi(strtok(NULL," "));
				zonas[total_zonas].vip = atoi(strtok(NULL," "));
				sem_init(&zonas[total_zonas].porteiro, 0, zonas[total_zonas].lotacao);
				lotacao_maxima += zonas[total_zonas].lotacao;
			} else { 
				// atribuicao de um parametro
				if(strcmp(item,"TEMPO_SIMULACAO")==0){
					tempo_simulacao = atoi(strtok(NULL," "));
				} else if(strcmp(item,"PROB_DESISTE_FILAS")==0) {
					prob_desiste_filas = atoi(strtok(NULL," "));
				} else if(strcmp(item,"TEMPO_MAXIMO_CHEGADA")==0) {
					tempo_maximo_chegada = atoi(strtok(NULL," "));
				} else if(strcmp(item,"PROB_VIP")==0) {
					prob_vip = atoi(strtok(NULL," "));
				}
			}
		}
	}
	fclose(fp);
	if(total_zonas < 1 || tempo_simulacao < 1)
		return 0;
	return 1;
}

// funcao principal do programa
int main(int argc, char *argv[]){
	if(argc<2){
		printf("falta ficheiro de configuracao");
		return 1;
	} else {
		// sai do programa caso alguma coisa corra mal
		if(!configuracao(argv[1])){
			printf("erro: ficheiro de configuracao invalido.\n");
			return 1;
		}


		srand((unsigned) time(&start)); // random seed
		sem_init(&porteiro_entrada, 0, lotacao_maxima);
		sem_init(&sem_sim, 0, 0);

		// criacao do socket
		if((sockfd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
			perror("erro na abertura do socket");
		serv_addr.sun_family = AF_UNIX;
		strcpy(serv_addr.sun_path, UNIXSTR_PATH);
		servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
		if(connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
			perror("erro de ligacao");

		// tarefa que trata dos pedidos enviados pelo monitor
		pthread_t thread;
		pthread_create(&thread, NULL, &recebe_comandos_monitor, &sockfd);
		
		while(!simula);
		char buffer[256];
		
		sprintf(buffer, "%d INICIO\n", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);

		pthread_t primeiro_cliente;
		pthread_create(&primeiro_cliente, NULL, &cliente, &sockfd);

		sleep(tempo_simulacao);
		
//		while(clientes_dentro);
		sem_wait(&sem_sim);
		
		sprintf(buffer,"%d FIM \n", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);

		close(sockfd);
	}
	return 0;
}