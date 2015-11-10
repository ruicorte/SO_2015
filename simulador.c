#include "simulador.h" 
#include <time.h> 
#include <pthread.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#define FICHEIRO "configuracao.conf" 

struct sockaddr_un serv_addr; 
int sockfd, servlen; 

struct configuracao_stru config; 
cliente_stru *clientes; 
time_t tempo;
int opcao = 0, sair = 0; 
int temp_atual_simu = 0, temp_inicial_simu = 0, temp_final_simu = 0, num_clientes = 0; 

// escrita no ficheiro
void RelatorioSimulacao(int num_cliente) {
	//Abrir ficheiro
	int fd = open("relatorio.txt", O_CREAT | O_WRONLY | O_APPEND , S_IWUSR | S_IRUSR);
	char temp[100];
	sprintf(temp, "Chegou um cliente a fila da guiché. Numero %d.\n", num_cliente);
	int n = strlen(temp);
	// escreve no ficheiro
	write (fd, temp, n);
	// fecha ficheiro
	close(fd);
}

/**
	METÓDO PARA CRIAR TAREFA CLIENTE 
**/ 
// criacao de tarefa cliente
void *criarTarefaCliente(void *cli) {

	cliente_stru *NovoCliente = (cliente_stru *)cli;
	// escreve no ficheiro
	RelatorioSimulacao(NovoCliente->NUMERO);
	printf("\nChegou um cliente a fila da guiché. Numero %d.", NovoCliente->NUMERO);
	
	pthread_exit(NULL); 
} 

// gera clientes
void gerarClientes() {
	srand(time(NULL));
	tempo = time(0);
	time(&tempo);
	printf("\n** [%d:%d:%d] ABRIU A MONTANHA RUSSA **\n", localtime(&tempo)->tm_hour,localtime(&tempo)->tm_min,localtime(&tempo)->tm_sec);
	
	temp_atual_simu = ((localtime(&tempo)->tm_hour)*3600) + ((localtime(&tempo)->tm_min)*60) + localtime(&tempo)->tm_sec;
	
	// inicio e final da simulacao
	temp_inicial_simu = temp_atual_simu;
	config.TEMPO_SIMULACAO *= 60;
	temp_final_simu = config.TEMPO_SIMULACAO + temp_inicial_simu;
 
	pthread_t tarefas[500];
	clientes = (cliente_stru *) malloc(500*sizeof(cliente_stru));
	
	// criacao de clientes enquanto dura a simulacao
	while(temp_atual_simu < temp_final_simu)
	{
		// gera tempo de chegada
		int aleatorio = rand() % (config.TEMPO_CHEGADA_CLIENTES+1);
		// passa para segundos
		aleatorio *= 60;
		// actualiza tempo actual
		temp_atual_simu += aleatorio;
		num_clientes++;
		
		// define cliente
		clientes[num_clientes-1].NUMERO = num_clientes;
		clientes[num_clientes-1].INSTANTE_CHEGADA_GUICHE = temp_atual_simu;
		clientes[num_clientes-1].PROB_DESIS_FILA_GUICHE = rand() % 100;
		
		//printf("\nCriou: %d %d %d", num_clientes, temp_atual_simu, clientes[num_clientes-1].PROB_DESIS_FILA_GUICHE);
		
		// criacao de tarefa cliente
		if(pthread_create(&tarefas[num_clientes-1], NULL, criarTarefaCliente, (void *) &clientes[num_clientes-1]))
		{
			printf("erro na tarefa cliente\n");
			exit(1);
		}
	} 
} 

// trata pedidos do monitor
void *RecebeMonitor(void *arg) {
	struct sockaddr_un cli_addr;
	int n, id;
	int sockfd=*((int *) arg), clilen=sizeof(cli_addr);
	char buffer[256];
	// espera pelos pedidos do monitor
	while(1){
		if((n = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(n < 0)
				perror("recv error");
		}
		buffer[n] = '\0';
		
		switch(atoi(buffer)) {
			case 1:
				opcao = 1;
				gerarClientes();
				break;
			case 2:
				opcao = 0;
				break;
			case 3:
				opcao = 0;
				sair = 1;
				exit(1);
				break;
			default:
				printf("opcao invalida.\n");
				break;
		}
	}
	return NULL; 
} 

// resultado das configuracoes
void VerResultado() {
	printf("Tamanho maximo da fia guiche:  %d\n", config.TAMANHO_MAX_FILA_GUICHE);
	printf("Tamanho maximo da fila de embarque:  %d\n", config.TAMANHO_MAX_FILA_EMBARQUE);
	printf("Tempo medio numa viagem: %d\n", config.TEMPO_MEDIO_VIAGEM);
	printf("Tempo medio na fila do guiche: %d\n", config.TEMPO_ESPERA_FILA_GUICHE);
	printf("Tempo medio na fila de embarque: %d\n", config.TEMPO_ESPERA_FILA_EMBARQUE);
	printf("Tempo de chegada dos clientes:  %d\n", config.TEMPO_CHEGADA_CLIENTES);
	printf("Tempo de chegada dos carros: %d\n", config.TEMPO_CHEGADA_CARROS);
	printf("Numero maximo de carros:  %d\n", config.NUMERO_DE_CARROS);
	printf("Numero maximo de passageiros por carro:  %d\n", config.NUMERO_DE_PASSAGEIROS_CARRO);
	printf("Probabilidade de disistencias da fila guiche:  %d\n", config.PROB_DESISTENCIA_GUICHE);
	printf("Probabilidade de disistencias da fila de embarque:  %d\n", config.PROB_DESISTENCIA_EMBARQUE);
	printf("Tempo de simulação:  %d\n", config.TEMPO_SIMULACAO); 
} 

int main(int argc, char **argv) {
	// leitura do ficheiro de configuracao
	int *conf = LerConfiguracao(FICHEIRO);
	config.TAMANHO_MAX_FILA_GUICHE = conf[0];
	config.TAMANHO_MAX_FILA_EMBARQUE = conf[1];
	config.TEMPO_MEDIO_VIAGEM = conf[2];
	config.TEMPO_ESPERA_FILA_GUICHE = conf[3];
	config.TEMPO_ESPERA_FILA_EMBARQUE = conf[4];
	config.TEMPO_CHEGADA_CLIENTES = conf[5];
	config.TEMPO_CHEGADA_CARROS = conf[6];
	config.NUMERO_DE_CARROS = conf[7];
	config.NUMERO_DE_PASSAGEIROS_CARRO = conf[8];
	config.PROB_DESISTENCIA_GUICHE = conf[9];
	config.PROB_DESISTENCIA_EMBARQUE = conf[10];
	config.TEMPO_SIMULACAO = conf[11];
	
	// cria socket UNIX, se devolver -1 ocorreu erro
	if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("erro na abertura do socket");
		return 0;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	// ligacao ao socket, caso devolva -1, ocorreu erro
	if(connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
	{
		perror("erro na ligacao ao socket");
	}
			
	// criacao da tarefa que trata do pedidos do monitor
	pthread_t thread;
	pthread_create(&thread, NULL, &RecebeMonitor, &sockfd);
		
	while(!opcao);

	char buffer[256];
	char input[200];
	tempo = time(0);
	send(sockfd, buffer, sizeof(buffer), 0);
	
	do{
		fgets(input, sizeof(input), stdin);
		sprintf(buffer, "%d %s", (int)tempo, input);
		send(sockfd, buffer, sizeof(buffer), 0);
	}while(sair != 1);
	
	printf("simulacao terminada\n");
	close(sockfd); 
}
