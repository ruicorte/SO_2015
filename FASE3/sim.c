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
			vip,					// entrada restrita - só para VIPs
			vips_em_espera;		
	sem_t 	porteiro, 				// porteiro da entrada do recinto - mantem o recinto abaixo do limite de lotacao
			porteiro_vip;			// porteiro para clientes vip - os clientes nesta fila têm prioridade
} zona;

// variaveis globais
zona 	zonas[20];					// array que contem todas as zonas - com o limite maximo de 20 mas pode ser alterado manualmente 
int 	total_zonas = 0,			// numero de zonas
	 	simula = 0,					// controlo da execucao da simulacao - provavelmente nao sera necessario
	 	lotacao_maxima = 0,			// lotacao total do recinto (a soma das lotacoes de todas as zonas) - quando estiver este numero de clientes no recinto, nao serao permitidas mais entradas
	 	tempo_simulacao = 0,		// duracao da simulacao
	 	tempo_actual = -1,			// tempo actual da simulacao
	 	tempo_maximo_chegada = 0,	// intervalo maximo entre as chegadas de dois clientes ao recinto
	 	tempo_maximo_zona = 0,		// tempo máximo em que um cliente permanece numa zona
	 	prob_desiste_filas = 0,		// probabilidade de desistencia nas filas
	 	prob_vip = 0,				// probabilidade de um novo cliente ser vip
	 	clientes_vip_em_espera = 0,	// variavel de controlo destinada a clientes vip em espera na entrada da discoteca
		num_cliente = 1,			// numeracao dos clientes
		clientes_dentro = 0;		// clientes que entraram no recinto e ainda não sairam

sem_t 	porteiro_entrada, 			// semaforo que se destina a controlar a lotacao máxima de todo o espaco
		porteiro_entrada_vip;

pthread_mutex_t mutex;				// mutex - exclusao mutua

time_t 	start; 						// controlo do tempo - por enquanto apenas serve como semente para gerar random

struct 	sockaddr_un serv_addr;		// socket
int 	sockfd, servlen;			// socket

// funcao que devolve o numero de zonas que o cliente nao visitou
int num_zonas_a_visitar(int *arr_cli, int *arr_cli_tent, int id){
	int i;
	int zonas_a_visitar = 0;
	for(i=0; i<total_zonas; i++){
		if(arr_cli[i]>0){
			if(arr_cli_tent[i]>2)
				arr_cli[i] = 0;
			else
				zonas_a_visitar++;
		}
	}
	return zonas_a_visitar;
}

/*
void mensagem(char *mensagem, char *codigo){
	printf("%s", mensagem);
	send(sockfd,codigo,sizeof(codigo),0);				
}
*/

// tarefa cliente
void *cliente(void *ptr){
	pthread_detach(pthread_self()); 						// liberta recursos quando a thread termina
	char 	buffer[256];									// array de caracteres auxiliar - para envio de mensagens ao monitor
	int 	zonas_cliente[total_zonas], 					// array para referência das zonas visitadas pelo cliente
			tentativas_zonas_cliente[total_zonas],			// array para guardar o numero de tentativas de acesso do cliente, a cada zona
			vip = 0,										// cliente vip
			c = 0,											// variavel auxiliar (para ciclos, etc)
			temp = 0,										// variavel auxiliar
			encontrou_zona = 0,								// variavel auxiliar para escolha da proxima zona a visitar
			tempo_chegada = rand() % tempo_maximo_chegada,	// tempo ate chegar ao recinto para entrar
			zona_actual = 0,								// zona que o cliente visita em determinada altura
			estado_semaforo = 0,							// variavel auxiliar para verificar estado do semaforo de cada zona
			hora_entrada_discoteca = 0,						// variavel auxiliar que regista a hora de entrada na discoteca
			hora_entrada_fila = 0;							// variavel auxiliar que regista a entrada do cliente na fila da discoteca ou de qualquer das zonas
	
	pthread_mutex_lock(&mutex);
	int 	id = num_cliente++; 							// variavel global - referencia unica de cada cliente
	pthread_mutex_unlock(&mutex);

	if((rand() % 100) <= prob_vip)							// atribuicao de estatuto vip ao cliente, segundo a probabilidade global
		vip = 1;

	for(c = 0; c < total_zonas; c++){						// geracao aleatoria do tempo que o cliente permanece em cada uma das zonas 
		tentativas_zonas_cliente[c] = 0;
		if(vip==1 || zonas[c].vip==0){
			do{
				temp = rand() % tempo_maximo_zona;
			} while(temp < 1);
			zonas_cliente[c] = temp;
		}
		else
			zonas_cliente[c] = 0;
	}

	if((tempo_actual + tempo_chegada) < tempo_simulacao){
		sleep(tempo_chegada);								// compasso de espera até que o cliente chegue à entrada do recinto

		pthread_t proximo_cliente;							// criacao do proximo cliente da cadeia
		pthread_create(&proximo_cliente, NULL, &cliente, &sockfd);

		pthread_mutex_lock(&mutex);
		sem_getvalue(&porteiro_entrada, &estado_semaforo);
		if(((estado_semaforo < 1) && (prob_desiste_filas < (rand() % 100))) || (estado_semaforo > 0)){
			hora_entrada_fila = tempo_actual;
			if(vip > 0 && estado_semaforo < 1){
				clientes_vip_em_espera++;					// incrementada a variavel que controla os clientes vip em espera
				pthread_mutex_unlock(&mutex);
				sem_wait(&porteiro_entrada_vip);
				pthread_mutex_lock(&mutex);
				clientes_vip_em_espera--;					// desfeita a acção anterior
				pthread_mutex_unlock(&mutex);
			} else {
				pthread_mutex_unlock(&mutex);
				sem_wait(&porteiro_entrada);
			}

			if(tempo_actual - hora_entrada_fila > 0)
			{
				pthread_mutex_lock(&mutex);
				printf("%d: cliente %d esperou na fila da DISCOTECA.\n", tempo_actual, id);
				sprintf(buffer, "%d CLIENTE %d ENTRA DISCOTECA\n", tempo_actual, id);
				send(sockfd,buffer,sizeof(buffer),0);
				pthread_mutex_unlock(&mutex);				
			}

			pthread_mutex_lock(&mutex);
			clientes_dentro++;
			if(tempo_actual < tempo_simulacao){
				printf("%d: cliente %d entrou na DISCOTECA.\n", tempo_actual, id);
				sprintf(buffer, "%d CLIENTE %d ENTRA DISCOTECA\n", tempo_actual, id);
				send(sockfd,buffer,sizeof(buffer),0);				
			} else {
				printf("%d: cliente %d esperou mas nao entrou na DISCOTECA.\n", tempo_actual, id);
				sprintf(buffer, "%d CLIENTE %d ENTRA DISCOTECA\n", tempo_actual, id);
				send(sockfd,buffer,sizeof(buffer),0);				
			}
			pthread_mutex_unlock(&mutex);

			// escolha da proxima zona a visitar
			// enquanto houver zonas disponiveis que ainda nao foram visitadas pelo cliente (nem excluidas) e o recinto estiver aberto
			do{
				zona_actual = rand() % total_zonas;
				tentativas_zonas_cliente[zona_actual] += 1;
				if(tentativas_zonas_cliente[zona_actual] < 4 && zonas_cliente[zona_actual]>0){
					if((zonas[zona_actual].hora_abertura <= tempo_actual) && ((zonas[zona_actual].duracao == 0) || ((zonas[zona_actual].duracao + zonas[zona_actual].hora_abertura)>tempo_actual))){
						pthread_mutex_lock(&mutex);
						sem_getvalue(&zonas[zona_actual].porteiro, &estado_semaforo);
						if(((estado_semaforo < 1) && (prob_desiste_filas < (rand() % 100))) || (estado_semaforo > 0)){
							if(vip > 0 && estado_semaforo < 1){
								zonas[zona_actual].vips_em_espera++;		// incrementa a variável que controla os clientes VIP em espera
								pthread_mutex_unlock(&mutex);
								sem_wait(&zonas[zona_actual].porteiro_vip);	// fica em espera na fila para VIPs
								pthread_mutex_lock(&mutex);
								zonas[zona_actual].vips_em_espera--;		// desfaz a operação anterior - sai da fila prioritaria
								pthread_mutex_unlock(&mutex);
							} else {
								pthread_mutex_unlock(&mutex);
								sem_wait(&zonas[zona_actual].porteiro);		// cliente normal - fica em espera na fila principal
							}

							if(tempo_actual < tempo_simulacao){
								pthread_mutex_lock(&mutex);
								printf("%d: cliente %d entrou na zona: %s.\n", tempo_actual, id, zonas[zona_actual].nome);
								sprintf(buffer, "%d CLIENTE %d ENTRA %s\n", tempo_actual, id, zonas[zona_actual].nome);
								send(sockfd,buffer,sizeof(buffer),0);
								pthread_mutex_unlock(&mutex);

								sleep(zonas_cliente[zona_actual]);

								pthread_mutex_lock(&mutex);
								if(zonas[zona_actual].vips_em_espera > 0){
									sem_post(&zonas[zona_actual].porteiro_vip);
								} else {
									sem_post(&zonas[zona_actual].porteiro);
								}
								printf("%d: cliente %d saiu da zona: %s.\n", tempo_actual, id, zonas[zona_actual].nome);
								sprintf(buffer, "%d CLIENTE %d SAI %s\n", tempo_actual, id, zonas[zona_actual].nome);
								send(sockfd,buffer,sizeof(buffer),0);
								pthread_mutex_unlock(&mutex);
			
								zonas_cliente[zona_actual] = 0;
							} else {
								pthread_mutex_lock(&mutex);
								if(zonas[zona_actual].vips_em_espera > 0){
									sem_post(&zonas[zona_actual].porteiro_vip);
								} else {
									sem_post(&zonas[zona_actual].porteiro);
								}
								pthread_mutex_unlock(&mutex);
							}
						} else {
							printf("%d: cliente %d tentou entrar na zona %s.\n", tempo_actual, id, zonas[zona_actual].nome);
							sprintf(buffer, "%d CLIENTE %d TENTA %s\n", tempo_actual, id, zonas[zona_actual].nome);
							send(sockfd,buffer,sizeof(buffer),0);
							pthread_mutex_unlock(&mutex);
						}
					}
				} else {
					zonas_cliente[zona_actual] = 0;
				}
			} while((num_zonas_a_visitar(zonas_cliente, tentativas_zonas_cliente, id) > 0) && (tempo_actual < tempo_simulacao));

			pthread_mutex_lock(&mutex);
			if(clientes_vip_em_espera > 0){
				sem_post(&porteiro_entrada_vip);
			} else {
				sem_post(&porteiro_entrada);
			}
			clientes_dentro--;
			printf("%d: cliente %d saiu da DISCOTECA.\n", tempo_actual, id);
			sprintf(buffer, "%d CLIENTE %d SAI DISCOTECA\n", tempo_actual, id);
			send(sockfd,buffer,sizeof(buffer),0);
			pthread_mutex_unlock(&mutex);
		} else {
			printf("%d: cliente %d desistiu da fila para DISCOTECA.\n", tempo_actual, id);
			sprintf(buffer, "%d CLIENTE %d DESISTE DISCOTECA\n", tempo_actual, id);
			send(sockfd,buffer,sizeof(buffer),0);
			pthread_mutex_unlock(&mutex);
		}
	} else {
		pthread_mutex_lock(&mutex);
		printf("%d: cliente %d não entrou, discoteca fechada!\n", tempo_actual, id);
		sprintf(buffer, "%d ENCERRAMENTO\n", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);
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
	while(1){										// espera pelos pedidos do monitor
		done = 0;
		if((n=recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(n < 0)
				perror("recv error");
			done = 1;
		}
		buffer[n] = '\0';
		if(!strcmp(buffer, "fim")){
			simula = 0;
		} else if(!strcmp(buffer, "inicio")){
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
	char	buff[500],
			*item,
			buffer[256];

	while(fgets( buff, sizeof buff, fp) != NULL){
		item = strtok(buff," ");
		if(strcmp(item,"#")){ // ignora comentarios no ficheiro de configuracao
			if(strcmp(item,"ZONA")==0){	// criacao de zona
				zonas[total_zonas].vips_em_espera = 0;
                strcpy(zonas[total_zonas].nome,strtok(NULL," "));
				zonas[total_zonas].lotacao = atoi(strtok(NULL," "));
				zonas[total_zonas].hora_abertura = atoi(strtok(NULL," "));
				zonas[total_zonas].duracao = atoi(strtok(NULL," "));
				zonas[total_zonas].vip = atoi(strtok(NULL," "));
				sem_init(&zonas[total_zonas].porteiro, 0, zonas[total_zonas].lotacao);	// semaforo inicializado com a lotacao da zona, aqui se faz o controlo da lotacao
				sem_init(&zonas[total_zonas].porteiro_vip, 0, 0);						// destina-se a barrar clientes vip, caso tenha sida atingida a lotacao da zona
				lotacao_maxima += zonas[total_zonas].lotacao;

				pthread_mutex_lock(&mutex);
				printf("%d: criacao de %s (zona) %d (lotacao) %d (hora de abertura) %d (duracao) %d (vip)\n", tempo_actual, zonas[total_zonas].nome, zonas[total_zonas].lotacao, zonas[total_zonas].hora_abertura, zonas[total_zonas].duracao, zonas[total_zonas].vip);
				sprintf(buffer, "%d ZONA %d CRIACAO %s %d %d %d %d\n", tempo_actual, total_zonas, zonas[total_zonas].nome, zonas[total_zonas].lotacao, zonas[total_zonas].vip, zonas[total_zonas].hora_abertura, zonas[total_zonas].duracao);
				send(sockfd,buffer,sizeof(buffer),0);
				pthread_mutex_unlock(&mutex);

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
				} else if(strcmp(item,"CAPACIDADE_ESPACOS_COMUNS")==0){
					lotacao_maxima += atoi(strtok(NULL," "));
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
		// sai do programa caso o ficheiro de configuracao seja invalido
		if(!configuracao(argv[1])){
			printf("erro: ficheiro de configuracao invalido.\n");
			return 1;
		}
		srand((unsigned) time(&start)); 					// random seed

		// o semáforo vip destina-se apenas a barrar VIPs quando a fila normal da discoteca está cheia (se encher, significa que se atingiu a lotacao maxima)
		sem_init(&porteiro_entrada, 0, lotacao_maxima);		// inicializacao do semaforo de entrada com a lotacao maxima da discoteca
		sem_init(&porteiro_entrada_vip, 0, 0); 				// inicializacao do semaforo de entrada para clientes vip (inicializacao a zero)

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

		sprintf(buffer, "%d SIMULACAO 0 INICIO\n", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);

		// criacao do primeiro cliente na cadeia (de clientes) que visita o recinto
		// um cliente cria o proximo ate a hora do fecho
		pthread_t primeiro_cliente;
		pthread_create(&primeiro_cliente, NULL, &cliente, &sockfd);
		
		int i;
		char acontecimento[100];
		char codigo[256];
		while((tempo_actual < tempo_simulacao || clientes_dentro > 0) && simula){
			sleep(1);
			pthread_mutex_lock(&mutex);
			tempo_actual++;
			for(i = 0; i < total_zonas; i++){
				if((zonas[i].hora_abertura == tempo_actual) || ((tempo_actual == 0) && (zonas[i].hora_abertura == 0))){
					printf("%d: abertura da zona %s\n", tempo_actual, zonas[i].nome);
					sprintf(codigo,"%d %s %d ABERTURA ZONA 0 %d 0 0\n",tempo_actual, zonas[i].nome, i, zonas[i].vip);
					send(sockfd,codigo,sizeof(codigo),0);

				}
				if((zonas[i].hora_abertura + zonas[i].duracao == tempo_actual) || ((zonas[i].duracao == 0) && (tempo_actual == tempo_simulacao))){
					printf("%d: fecho da zona %s\n", tempo_actual, zonas[i].nome);
					sprintf(codigo,"%d %s %d FECHO ZONA 0 %d 0 0\n",tempo_actual, zonas[i].nome, i, zonas[i].vip);
					send(sockfd,codigo,sizeof(codigo),0);
				}
			}
			pthread_mutex_unlock(&mutex);
		}
		printf("%d: simulacao terminada", tempo_actual);
		sprintf(buffer,"%d SIMULACAO 0 FIM", tempo_actual);
		send(sockfd,buffer,sizeof(buffer),0);

		close(sockfd);
	}
	return 0;
}