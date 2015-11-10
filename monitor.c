#include "simulador.h" 

int opcao = 0, sair = 0; 

//espera contacto do simulador
void *EsperaComunicacao(void *arg) {
	int sockfd = *((int *) arg);
	int n, num_comandos;
	int tempo = 0, evento, cod1, cod2, cod3;
	char buffer[256];
	// espera pela mensagem do servidor
	while(1){
		if((n = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0){
			if(n < 0)
				perror("1:recv");
		} else {
       		buffer[n] = '\0';
			switch(evento){
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
			}
		}
	}
	return NULL; 
}
int main(int argc, char **argv) {
	struct sockaddr_un serv_addr, cli_addr;
	int sockfd, servlen, newsockfd;
	int n, num_comandos;
	int clilen=sizeof(cli_addr);
	char buffer[256];
	
	//Criação do socket UNIX, se devolve -1 ocorreu erro
	if((sockfd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("erro na abertura do socket");
		return 0;
	}
	serv_addr.sun_family=AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	unlink(UNIXSTR_PATH);
	
	if(bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
	{
		perror("endereco nao foi atribuido");
		return 0;
	}
	
	listen(sockfd, 1);
	
	// aceita nova conexão com o socket, se devolve -1 ocorreu erro
	if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0){
		perror("erro na conexao");
	}
	// tarefa que trata da comunicação com o simulador
	pthread_t thread;
	pthread_create(&thread, NULL, &EsperaComunicacao, &newsockfd);
	
	system("clear");
	printf("\n**** Discoteca ****\n\n");
	printf("Escolhe as seguintes opcoes:\n");
	printf("1 - começar simulacao \n");
	printf("2 - mostrar estatistica \n");
	printf("3 - sair \n\n");
	do
	{
		fgets(buffer, sizeof(buffer), stdin);
		if(send(newsockfd, buffer, sizeof(buffer), 0) == -1)
		{
			perror("send");
			exit(1);
		}
		switch(atoi(buffer))
		{
			case 1:
				opcao = 1;
				break;
			case 2:
				opcao = 0;
				break;
			case 3:
				opcao = 0;
				sair = 1;
				break;
			default:
				printf("opcao errada.\n");
				break;
		}
	} while(!sair);	

	printf("\nsimulacao terminada.\n");
	//Fechar socket
	close(sockfd); 
}
