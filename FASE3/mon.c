/***********************************************************
* Sistemas Operativos - 2015/2016 - Simulacao de Discoteca 
*
* Docente: Eduardo Marques
*
* Alunos:
* David Ricardo Cândido nº 2032703
* Luís Bouzas Prego 	nº 2081815
*
* Ficheiro: mon.c
************************************************************/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "unix.h"

typedef struct{
	char nome[20];
	int vip,						// zona apenas para clientes vip (1) ou nao (0)
		abertura,					// hora de abertura da zona
		tempo_funcionamento,		// tempo de funcionamento da zona
		visitas,					// numero de clientes normais que entram na zona
		visitas_vip,				// numero de clientes vip que entram na zona
		total_minutos_visitas,		// minutos que todos os clientes (nao vip) passaram na zona
		total_minutos_visitas_vip,	// minutos passados pelos clientes vip, nesta zona
		minutos_espera_fila,		// minutos de espera na fila normal
		minutos_espera_fila_vip,	// minutos de espera na fila vip
		tentativas_fila,			// desistencias da fila
		tentativas_horario,			// tentativas de acesso quando a zona esta fechada
		desistencias,				// clientes normais que desistiram apos tres tentativas de acesso
		desistencias_vip;			// desistencias de clientes vip, ao fim de tres tentativas de acesso
}zona;

zona zonas[20];

int total_zonas = 0,				// numero de zonas na simulacao
	total_minutos_visitas = 0,		// total de minutos que todos os clientes passaram no interior da discoteca
	total_minutos_visitas_vip = 0,	// total de minutos que os clientes vip passaram no interior da discoteca
	total_minutos_espera = 0,		// total de minutos que os clientes esperaram na fila normal da discoteca
	total_minutos_espera_vip = 0,	// total de minutos que os clientes (vip) esperaram na fila vip
	visitas = 0,					// numero de clientes que entraram na discoteca
	visitas_vip = 0;				// numero de clientes vip que entraram na discoteca

FILE *relatorio;					// ficheiro do relatorio

// apresentacao das estatisticas, sempre que forem requisitadas e no final da simulacao
void stats(){
	printf("apresentacao da estatistica\n");
}


// escuta das mensagens do simulador
void *escuta_comunicacao(void *arg)
{
	int 	sockfd = *((int *) arg),
		 	msg, 					// recepcao do codigo enviado pelo simulador
			num_input,				// quantidade de inputs captados da mensagem do simulador
			tempo,					// hora do acontecimento no simulador
			id,						// id do cliente / zona
			local,					// no caso das zonas, id do local, senao -1 (discoteca)
			resultado,				// variável com vários propósitos
			vip,					// zona destinada apenas a clientes vip
			abertura,				// minuto de abertura de uma zona, caso se aplique
			tempo_funcionamento,	// tempo em que uma zona está aberta, caso se aplique
			i;						// variavel auxiliar para ciclos
	char 	buffer[256],			// contentor do codigo enviado pelo simulador
			cliente[20],			// string para armazenar a personagem no acontecimento SIMULADOR | DISCOTECA | ZONA | CLIENTE
			accao[20];				// onde ocorreu o acontecimento

	// espera pelas mensagens do simulador
	while(1){
		if((msg=recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(msg < 0)
				perror("recv");
		} else {
           	buffer[msg] = '\0';
			fprintf(relatorio,"%s", buffer);
			num_input = sscanf(buffer, "%d %s %d %s %d %d %d %d %d", &tempo, cliente, &id, accao, &local, &resultado, &vip, &abertura, &tempo_funcionamento);
			if(num_input > 0){
				if(!strcmp(cliente,"SIMULACAO")){
					if(!strcmp(accao,"INICIO")){
						printf("inicio da simulacao.\n");
					}
					if(!strcmp(accao,"FIM")){
						printf("simulacao terminada - \"fim\" para sair.\n");
					}
				} else if(!strcmp(cliente,"DISCOTECA")){
					if(!strcmp(accao,"ABERTURA")){
//						printf("abertura da discoteca\n");
					} else if(!strcmp(accao,"FECHO")){
//						printf("fecho da discoteca\n");
					} else {
//						printf("codigo desconhecido: \"%s\"\n", buffer);
					}					
				} else if(!strcmp(cliente,"ZONA")){
					if(!strcmp(accao,"CRIACAO")){
						zonas[total_zonas].vip = vip;
						zonas[total_zonas].abertura = abertura;
						zonas[total_zonas].tempo_funcionamento = tempo_funcionamento;
						zonas[total_zonas].visitas = 0;
						zonas[total_zonas].visitas_vip = 0;
						zonas[total_zonas].total_minutos_visitas = 0;
						zonas[total_zonas].total_minutos_visitas_vip = 0;
						zonas[total_zonas].minutos_espera_fila = 0;
						zonas[total_zonas].minutos_espera_fila_vip = 0;
						zonas[total_zonas].tentativas_fila = 0;
						zonas[total_zonas].tentativas_horario = 0;
						zonas[total_zonas].desistencias = 0;
						zonas[total_zonas].desistencias_vip = 0;
					} else if(!strcmp(accao,"ABERTURA")){
						printf("abertura da zona %d\n", local);
					} else if(!strcmp(accao,"FECHO")){
						printf("fecho da zona %d\n", local);
					} else {
						printf("codigo desconhecido: \"%s\"\n", buffer);
					}
				} else if(!strcmp(cliente,"CLIENTE")){
					if(!strcmp(accao,"ENTRA")){
						if(local<0){
							if(vip == 1)
								visitas_vip++;
							else
								visitas++;
						} else {
							if(vip == 1)
								zonas[local].visitas_vip++;
							else
								zonas[local].visitas++;
						}
					} else if(!strcmp(accao,"ESPERA")){
						// printf("cliente em espera na fila de %s\n", local);
						// apenas quando o cliente sair da fila, para entrar na zona
						// tem que se testar, se for discoteca ou zona
					} else if(!strcmp(accao,"DESISTE")){
						// printf("cliente desiste %s\n", local);
						// se o cliente atingir as três tentativas falhadas
						// apenas para as zonas
					} else if(!strcmp(accao,"SAI")){
						if(local < 0){
							if(vip == 1){
								visitas_vip++;
								total_minutos_visitas_vip += resultado;
							} else {
								visitas++;
								total_minutos_visitas += resultado;
							}
						} else {
								if(vip == 1){
									zonas[local].visitas_vip++;
									zonas[local].total_minutos_espera_vip += resultado;
								} else {
									zonas[local].visitas++;
									zonas[local].total_minutos_espera += resultado;										
								}
							}
						}
					} else if(!strcmp(accao,"TENTA") && local >= 0){

						// printf("cliente tenta %s\n", local);
						// apenas se aplica a zonas, não a discoteca
					} else {
						printf("codigo desconhecido: \"%s\"\n", buffer);
					} 
				} else {
						printf("codigo desconhecido: \"%s\"\n", buffer);
				}
			}
		}
	}
	return NULL;
}

// funcao principal
int main(int argc, char *argv[]){	
	struct sockaddr_un serv_addr, cli_addr;
	int sockfd, servlen, newsockfd;

	int i = 0;								// variável auxiliar para ciclos, etc

	int clilen = sizeof(cli_addr);
	char buffer[256];
	char menu[512] = 
	"\n------------------------------\n comandos disponiveis:\n------------------------------\n \"inicio\" - iniciar simulacao \n \"stats\" - estatisticas    \n \"menu\" - ver comandos    \n \"fim\" - fim da simulacao    \n------------------------------\n";

	// socket UNIX
	if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		perror("cant open socket stream");
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	if(bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		perror("cant bind local address");
	listen(sockfd, 1);

	if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0)
		perror("accept error");

	// tarefa que trata da comunicação com o simulador
	pthread_t thread;
	pthread_create(&thread, NULL, &escuta_comunicacao, &newsockfd);

	// mostra o menu
	printf("%s",menu);

	unlink("relatorio.log"); 					// apaga ficheiro, caso já exista
	relatorio = fopen("relatorio.log", "a"); 	// abre o ficheiro para escrita

	do{
		scanf("%s", buffer);
		if(!strcmp(buffer,"inicio")){
			if(send(newsockfd, buffer, sizeof(buffer), 0) == -1){
				perror("send");
				exit(1);
			}
		}
		if(!strcmp(buffer,"stats")){
			stats();
		}
		if(!strcmp(buffer,"menu")){
			printf("%s", menu);
		}
		if(!strcmp(buffer,"fim")){
			if(send(newsockfd, buffer, sizeof(buffer), 0) == -1){
				perror("send");
				exit(1);
			}
		}
	}while(strcmp(buffer,"fim"));

	close(sockfd);
	fclose(relatorio);
	return 0;
}