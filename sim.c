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
	char 	nome[20]; 				// nome da zona
	int 	lotacao, 				// a lotacao maxima sera a soma da lotacao de todas as zonas
			hora_abertura,			// comeco da contagem do tempo - em principio sera 00
			duracao, 				// duracao da simulacao - cada segundo corresponde a um minuto
			vip;					// entrada restrita - só para VIPs
	sem_t 	porteiro; 				// porteiro da entrada do recinto - mantem o recinto abaixo do limite de lotacao
} zona;

// variaveis globais
zona 	zonas[20];					// array que contem todas as zonas - com o limite maximo de 20 mas pode ser alterado manualmente 
int 	total_zonas = 0,			// numero de zonas
	 	simula = 0,					// controlo da execucao da simulacao - provavelmente nao sera necessario
	 	lotacao_maxima = 0,			// lotacao total do recinto (a soma das lotacoes de todas as zonas) - quando estiver este numero de clientes no recinto, nao serao permitidas mais entradas
	 	tempo_simulacao = 0,		// duracao da simulacao
	 	tempo_actual = 0,			// tempo actual da simulacao
	 	tempo_maximo_chegada = 0,	// intervalo maximo entre as chegadas de dois clientes ao recinto
	 	tempo_maximo_zona = 0,		// tempo máximo em que um cliente permanece numa zona
	 	prob_desiste_filas = 0,		// probabilidade de desistencia nas filas
	 	prob_vip = 0,				// probabilidade de um novo cliente ser vip
		num_cliente = 1,			// numeracao dos clientes
		clientes_dentro = 0;		// clientes que entraram no recinto e ainda não sairam

sem_t 	porteiro_entrada; 			// semaforo que se destina a controlar a lotacao máxima de todo o espaco

pthread_mutex_t mutex;				// mutex - exclusao mutua

time_t 	start; 						// controlo do tempo - por enquanto apenas serve como semente para gerar random

struct 	sockaddr_un serv_addr;		// socket
int 	sockfd, servlen;			// socket

// funcao que devolve o numero de zonas que o cliente nao visitou
int num_zonas_a_visitar(int *arr_cli){
	int i;
	int zonas_a_visitar = 0;
//	printf("\n");
	for(i=0; i<total_zonas; i++){
		if(arr_cli[i]>0)
			zonas_a_visitar++;
//		printf("%d ", arr_cli[i]);
	}
//	printf("\n");
//	return zonas_a_visitar;
}

// tarefa cliente
void *cliente(void *ptr){
	pthread_detach(pthread_self()); 						// liberta recursos quando a thread termina
	char 	buffer[256];									// array de caracteres auxiliar - para envio de mensagens ao monitor
	int 	zonas_cliente[total_zonas], 					// array para referência das zonas visitadas pelo cliente
			vip = 0,										// cliente vip
			c = 0,											// variavel auxiliar para ciclos
			tempo_chegada = rand() % tempo_maximo_chegada,	// tempo ate chegar ao recinto para entrar
			zona_actual = 0,								// zona que o cliente visita em determinada altura
			estado_semaforo = 0;							// variavel para verificar estado do semaforo de cada zona
int temp = 0;
	pthread_mutex_lock(&mutex);
	int id = num_cliente++; 								// variavel global - referencia unica de cada cliente
	pthread_mutex_unlock(&mutex);

	if(rand()%100 <= prob_vip)								// atribuicao de estatuto vip ao cliente, segundo a probabilidade global
		vip = 1;
	for(c=0; c<total_zonas; c++){							// geracao aleatoria do tempo que o cliente permanece em cada uma das zonas 
		if(vip==1 || zonas[c].vip==0)
			zonas_cliente[c] = rand()%tempo_maximo_zona;
		else
			zonas_cliente[c] = 0;
	}
	if((tempo_actual + tempo_chegada) < tempo_simulacao){
		sleep(tempo_chegada);								// compasso de espera até que o cliente chegue à entrada do recinto
		pthread_t proximo_cliente;							// criacao do proximo cliente da cadeia
		pthread_create(&proximo_cliente, NULL, &cliente, &sockfd);

		sem_getvalue(&porteiro_entrada, &estado_semaforo);
		temp = rand() % 100;
//printf("*****************%d: cliente: %d estado semaforo: %d tolerancia: %d aleatorio: %d (entrada!!!)\n", tempo_actual, id, estado_semaforo, prob_desiste_filas, temp);
		if(((estado_semaforo < 1) && (prob_desiste_filas < temp)) || (estado_semaforo > 0)){
			sem_wait(&porteiro_entrada); 					// passagem pela entrada do recinto
			clientes_dentro++;

			pthread_mutex_lock(&mutex);

			printf("%d: cliente %d entrou na discoteca.\n", tempo_actual, id);
			sprintf(buffer, "%d CLIENTE %d ENTRA DISCOTECA\n", tempo_actual, id);
			send(sockfd,buffer,sizeof(buffer),0);
			pthread_mutex_unlock(&mutex);

			// aqui e feita a escolha da proxima zona a visitar
			// enquanto houver zonas disponiveis que ainda nao foram visitadas (ou rejeitadas) pelo cliente e o recinto estiver aberto
			int encontrou_zona;
			while(num_zonas_a_visitar(zonas_cliente) > 0 && tempo_actual < tempo_simulacao){
				encontrou_zona = 0;
				while(num_zonas_a_visitar(zonas_cliente) > 0 && tempo_actual < tempo_simulacao){
					zona_actual = rand() % total_zonas;
					if(zonas_cliente[zona_actual]>0){
						if(zonas[zona_actual].hora_abertura <= tempo_actual){
							if(zonas[zona_actual].duracao != 0 && (tempo_actual > (zonas[zona_actual].hora_abertura + zonas[zona_actual].duracao))){
								zonas_cliente[zona_actual] = 0;
								/*
								pthread_mutex_lock(&mutex);
								printf("%d: cliente %d nao entrou na zona %s porque está fechada\n", tempo_actual, id, zonas[zona_actual].nome);
								sprintf(buffer, "%d CLIENTE %d NAO_ENTRA %s\n", tempo_actual, id, zonas[zona_actual].nome);
								send(sockfd,buffer,sizeof(buffer),0);
								pthread_mutex_unlock(&mutex);
								*/
							} else {
								encontrou_zona = 1;
								break;
							}
						}
					}
				}
				if(encontrou_zona>0){			
					temp = rand()%100;
					sem_getvalue(&zonas[zona_actual].porteiro, &estado_semaforo);
//printf("*****************%d: cliente: %d estado semaforo: %d tolerancia: %d aleatorio: %d\n", tempo_actual, id, estado_semaforo, prob_desiste_filas, temp);

					if(((estado_semaforo < 1) && (prob_desiste_filas < temp)) || (estado_semaforo > 0)){
						pthread_mutex_lock(&mutex);
						printf("%d: o cliente %d entrou numa zona: %s\n", tempo_actual, id, zonas[zona_actual].nome);
						sprintf(buffer, "%d CLIENTE %d ENTRA %s\n", tempo_actual, id, zonas[zona_actual].nome);
						send(sockfd,buffer,sizeof(buffer),0);
						pthread_mutex_unlock(&mutex);

						sem_wait(&zonas[zona_actual].porteiro);
						sleep(zonas_cliente[zona_actual]);

						pthread_mutex_lock(&mutex);
						sem_post(&zonas[zona_actual].porteiro);
						printf("%d: o cliente %d saiu de uma zona: %s\n", tempo_actual, id, zonas[zona_actual].nome);
						sprintf(buffer, "%d CLIENTE %d SAI %s\n", tempo_actual, id, zonas[zona_actual].nome);
						send(sockfd,buffer,sizeof(buffer),0);
						pthread_mutex_unlock(&mutex);
					} else {
						pthread_mutex_lock(&mutex);
						printf("%d: o cliente %d desistiu porque não quis esperar na fila da %s\n", tempo_actual, id, zonas[zona_actual].nome);
						sprintf(buffer, "%d CLIENTE %d DESISTE %s\n", tempo_actual, id, zonas[zona_actual].nome);
						send(sockfd,buffer,sizeof(buffer),0);
						pthread_mutex_unlock(&mutex);
					}

					zonas_cliente[zona_actual] = 0;
				}
			}
			pthread_mutex_lock(&mutex);
			sem_post(&porteiro_entrada);
			clientes_dentro--;
			printf("%d: cliente %d saiu da discoteca.\n", tempo_actual, id);
			sprintf(buffer, "%d CLIENTE %d SAI DISCOTECA\n", tempo_actual, id);
			send(sockfd,buffer,sizeof(buffer),0);
			pthread_mutex_unlock(&mutex);
		} else {

			pthread_mutex_lock(&mutex);
			printf("%d: o cliente %d não quis esperar na fila para entrar na discoteca.\n", tempo_actual, id);
			sprintf(buffer, "%d CLIENTE %d DESISTE DISCOTECA\n", tempo_actual, id);
			send(sockfd,buffer,sizeof(buffer),0);
			pthread_mutex_unlock(&mutex);
		}
	} else {
		printf("%d: o cliente %d não entrou, a discoteca esta fechada!\n", tempo_actual, id);
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
	while(1){										// espera pelos pedidos do monitor
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
		printf("erro: tem que fornecer um ficheiro de configuracao\n");
		abort();
	}
	char 	name[50],
			buff[500],
			*item;

	while(fgets( buff, sizeof buff, fp) != NULL){
		item = strtok(buff," ");
		if(strcmp(item,"#")){ // ignora comentarios no ficheiro de configuracao
			if(strcmp(item,"ZONA")==0){	// criacao de zona
                strcpy(zonas[total_zonas].nome,strtok(NULL," "));
				zonas[total_zonas].lotacao = atoi(strtok(NULL," "));
				zonas[total_zonas].hora_abertura = atoi(strtok(NULL," "));
				zonas[total_zonas].duracao = atoi(strtok(NULL," "));
				zonas[total_zonas].vip = atoi(strtok(NULL," "));
				sem_init(&zonas[total_zonas].porteiro, 0, zonas[total_zonas].lotacao);
				lotacao_maxima += zonas[total_zonas].lotacao;
				total_zonas++;
			} else { // atribuicao de parametro
				if(strcmp(item,"TEMPO_SIMULACAO")==0){
					tempo_simulacao = atoi(strtok(NULL," "));
				} else if(strcmp(item,"PROB_DESISTE_FILAS")==0){
					prob_desiste_filas = atoi(strtok(NULL," "));
				} else if(strcmp(item,"TEMPO_MAXIMO_CHEGADA")==0){
					tempo_maximo_chegada = atoi(strtok(NULL," "));
				} else if(strcmp(item,"TEMPO_MAXIMO_ZONA")==0){
					tempo_maximo_zona = atoi(strtok(NULL," "));
				} else if(strcmp(item,"PROB_VIP")==0){
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
		srand((unsigned) time(&start)); 					// random seed
		sem_init(&porteiro_entrada, 0, lotacao_maxima);
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

		// criacao do primeiro cliente na cadeia (de clientes) que visita o recinto
		// um cliente cria o proximo ate que chegue a hora de fecho
		pthread_t primeiro_cliente;
		pthread_create(&primeiro_cliente, NULL, &cliente, &sockfd);

		while((tempo_actual < tempo_simulacao || clientes_dentro > 0) && simula){
			sleep(1);
			tempo_actual++;
		}

		sprintf(buffer,"%d FIM \n", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);

		close(sockfd);
	}
	return 0;
}