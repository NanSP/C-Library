#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>


#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

/* SISTEMA */

#define maxLivro 50
#define maxUsuarios 50
#define maxCaracter 50
#define limiteEmprestimo 2
#define MAX_MESSAGES 100

char messages[MAX_MESSAGES][100];
int message_count = 0;
pthread_mutex_t message_lock;

typedef struct {
	char titulo[maxCaracter],autor[maxCaracter],anoPublicacao[4],status[20];
}Livro;

typedef struct {
	char nome[maxCaracter],listaEmprestados[2][100];
	int id,nbook;
}Usuario;

Livro livros[maxLivro];
int totalLivros = 0;
Usuario usuarios[maxUsuarios];
int totalUsuarios = 0;

//==============================INICIO DAS FUNÇÕES DA BIBLIOTECA==================================
void menu();
//Funções livro
void adicionarLivro(FILE *dadosLivros){
	Livro b, revisao;
	int duplicado=0,ex=0;
	
	do {
		printf("1. Sair\n");
        printf("2. Adicionar Livro\n");
        printf("Escolha uma opção: ");
        scanf("%d", &ex);
        getchar(); 

        if (ex == 1) {
            menu();  
            return;
        }
	
		printf("Digite o título do livro: \n");
		fgets(b.titulo,50,stdin);
		strtok(b.titulo, "\n");
		printf("Informe o ano de publicação: \n");
		fgets(b.anoPublicacao,50,stdin);
		strtok(b.anoPublicacao, "\n");
	
		rewind(dadosLivros);
		while (fread(&revisao,sizeof(Livro),1,dadosLivros)){
			if(strcmp(revisao.titulo, b.titulo)==0 &&
				strcmp(revisao.anoPublicacao,b.anoPublicacao)==0){
				printf("Livro existente no sistema.\n");
				duplicado=1;
				break;
			}
		}
		if(!duplicado){
			printf("Digite o nome do autor: \n");
			fgets(b.autor,50,stdin);
			strtok(b.autor, "\n");
			strcpy(b.status,"Disponivel");
	
	
			fwrite(&b,sizeof(Livro),1,dadosLivros);
			printf("Livro adicionado.\n");
		}
	} while (ex != 1);
}

void emprestarLivro(FILE **dadosLivros, FILE **dadosUsuario) {
	Livro b;
	Usuario user;
	char sLivro[50];
	int idU = 0, resultado = 0, encontrado = 0, ex = 0;
	FILE *temp, *tempUser;

	do {
		printf("1. Sair\n");
        printf("2. Emprestar livro\n");
        printf("Escolha uma opção: ");
        scanf("%d", &ex);
        getchar();

        if (ex == 1) {
            menu();
            return;
        }

		printf("Informe o ID do usuário: \n");
		scanf("%d", &idU);
		getchar();

		rewind(*dadosUsuario);
		tempUser = fopen("tuser.dat", "wb");
		if (!tempUser) {
			printf("Erro ao abrir o arquivo: tuser.dat\n");
			return;
		}

		temp = fopen("tp.dat", "wb");
		if (!temp) {
			printf("Erro ao abrir o arquivo: tp.dat\n");
			fclose(tempUser);
			return;
		}

		
		while (fread(&user, sizeof(Usuario), 1, *dadosUsuario)) {
			if (user.id == idU) {
				encontrado = 1;
				if (user.nbook >= 2) {
					printf("O usuário já possui 2 livros emprestados.\n");
					fclose(tempUser);
					fclose(temp);
					return;
				}

				
				printf("Informe o livro que será emprestado: \n");
				fgets(sLivro, sizeof(sLivro), stdin);
				strtok(sLivro, "\n");
				

				rewind(*dadosLivros);
				resultado = 0;

				
				while (fread(&b, sizeof(Livro), 1, *dadosLivros)) {
					if (strcmp(b.titulo, sLivro) == 0) {
						resultado = 1;
						if (strcmp(b.status, "Disponivel") == 0) {
							strcpy(b.status, "Emprestado");
							printf("Livro emprestado com sucesso!\n");

							
							strcpy(user.listaEmprestados[user.nbook], sLivro);
							user.nbook++;
						} else {
							printf("Livro indisponível\n");
						}
					}
					fwrite(&b, sizeof(Livro), 1, temp); 
				}

				if (!resultado) {
					printf("Livro não localizado.\n");
				}
			}
			
			fwrite(&user, sizeof(Usuario), 1, tempUser);
		}

		if (!encontrado) {
			printf("Usuário não encontrado.\n");
		}

		
		fclose(*dadosLivros);
		fclose(temp);
		fclose(*dadosUsuario);
		fclose(tempUser);

		remove("book.dat");
		rename("tp.dat", "book.dat");
		remove("user.dat");
		rename("tuser.dat", "user.dat");

		
		*dadosUsuario = fopen("user.dat", "r+b");
		*dadosLivros = fopen("book.dat", "r+b");

	} while (ex != 1);
}


void devolverLivro(FILE **dadosLivros, FILE **dadosUsuario) {
	Livro b;
	Usuario user, tempUser;
	char dLivro[50];
	int resultado = 0, ex = 0, idU = 0, encontrado = 0;
	FILE *temp;

	do {
		printf("1. Sair\n");
        printf("2. Devolver livro\n");
        printf("Escolha uma opção: ");
        scanf("%d", &ex);
        getchar();

        if (ex == 1) {
            menu();
            return;
        }

		printf("Informe o ID do usuário: \n");
		scanf("%d", &idU);
		getchar();

		rewind(*dadosUsuario);
		FILE *tpUser = fopen("tpuser.dat", "wb");
		if (!tpUser) {
			printf("Erro ao abrir o arquivo: tpuser.dat\n");
			return;
		}

		encontrado = 0;
		while (fread(&tempUser, sizeof(Usuario), 1, *dadosUsuario)) {
			if (tempUser.id == idU) {
				user = tempUser; 
				encontrado = 1;
				break;
			}
		}

		if (!encontrado) {
			printf("Usuário não encontrado.\n");
			fclose(tpUser);
			return;
		}

		printf("Informe o livro que será devolvido: \n");
		fgets(dLivro, sizeof(dLivro), stdin);

		temp = fopen("tp.dat", "wb");
		if (!temp) {
			printf("Erro ao abrir o arquivo: tp.dat.\n");
			fclose(tpUser);
			return;
		}

		rewind(*dadosLivros);
		resultado = 0;

		while (fread(&b, sizeof(Livro), 1, *dadosLivros)) {
			if (strcmp(b.titulo, dLivro) == 0) {
				resultado = 1;

				if (strcmp(b.status, "Emprestado") == 0) {
					strcpy(b.status, "Disponivel");
					printf("Livro devolvido com sucesso!\n");

					for (int i = 0; i < user.nbook; i++) {
						if (strcmp(user.listaEmprestados[i], dLivro) == 0) {
							for (int j = i; j < user.nbook - 1; j++) {
								strcpy(user.listaEmprestados[j], user.listaEmprestados[j + 1]);
							}
							user.nbook--;
							break;
						}
					}
				} else {
					printf("O livro já está disponível.\n");
				}
			}
			fwrite(&b, sizeof(Livro), 1, temp); 
		}

		if (!resultado) {
			printf("Livro não localizado.\n");
		}

		rewind(*dadosUsuario);
		while (fread(&tempUser, sizeof(Usuario), 1, *dadosUsuario)) {
			if (tempUser.id == idU) {
				fwrite(&user, sizeof(Usuario), 1, tpUser); 
			} else {
				fwrite(&tempUser, sizeof(Usuario), 1, tpUser);
			}
		}

		fclose(*dadosLivros);
		fclose(temp);
		fclose(*dadosUsuario);
		fclose(tpUser);

		remove("book.dat");
		rename("tp.dat", "book.dat");
		*dadosLivros = fopen("book.dat", "r+b");

		remove("user.dat");
		rename("tpuser.dat", "user.dat");
		*dadosUsuario = fopen("user.dat", "r+b");

	} while (ex != 1);
}


void listarLivros(FILE *dadosLivros){
	Livro b;
	
	rewind(dadosLivros);
	
	while(fread(&b, sizeof(Livro),1,dadosLivros)){
		printf("==================================\n");
		printf("Título: %s\n", b.titulo);
		printf("Autor: %s\n", b.autor);
		printf("Ano de publicação: %s\n",b.anoPublicacao);
		printf("Status: %s\n",b.status);
		printf("==================================\n");
	}
}

//Funções usuario

void cadastrarUsuario(FILE *dadosUsuario){
	srand(time(NULL));
	Usuario user;
	int ex=0;
	user.nbook=0;	
	
	do {
		printf("1. Sair\n");
        printf("2. Cadastrar usuário\n");
        printf("Escolha uma opção: ");
        scanf("%d", &ex);
        getchar(); 

        if (ex == 1){
            menu();  
            return;
        }
        
		printf("Digite o nome: \n");
		fgets(user.nome,50,stdin);
		user.id=rand();
	
		fwrite(&user, sizeof(Usuario),1,dadosUsuario);
		printf("Usuário cadastrado!\n");
	} while (ex != 1);
}

void listarUsuarios(FILE **dadosUsuario){

	Usuario user;
	
	rewind(*dadosUsuario);
	
	while (fread(&user,sizeof(Usuario),1,*dadosUsuario)){
		printf("==========================\n");
		printf("Nome do usuário: %s",user.nome);
		printf("ID: %d\n", user.id);
		printf("Livros emprestados: \n");
		
		if (user.nbook > 0) {
            for (int i = 0; i < user.nbook; i++) {
                printf(" - %s\n", user.listaEmprestados[i]);
            }
        } else {
            printf("Nenhum livro emprestado.\n");
  		}
		printf("==========================\n");
	}

 }

//Função Menu
 void menu(){
	int inicio;
	FILE *dadosLivros;
	FILE *dadosUsuario;
	
	dadosLivros = fopen("book.dat", "r+b");
	if (dadosLivros == NULL) {
		dadosLivros = fopen("book.dat", "w+b");
		if (dadosLivros == NULL) {
			printf("Erro ao abrir ou criar o arquivo: book.dat.\n");
			return;
		}
	}

	dadosUsuario = fopen("user.dat", "r+b");
	if (dadosUsuario == NULL) {
		dadosUsuario = fopen("user.dat", "w+b");
		if (dadosUsuario == NULL) {
			printf("Erro ao abrir ou criar o arquivo: user.dat.\n");
			return;
		}
	}
	
	do{
		printf("==============BIBLIOTECA==============");
		printf("Informe o que deseja acessar: \n");
		printf("1. Adicionar livro\n");
		printf("2. Cadastrar usuário\n");
		printf("3. Emprestar livro\n");
		printf("4. Devolver livro\n");
		printf("5. Listar livros\n");
		printf("6. Listar usuários\n");
		printf("7. Sair\n");
		printf("======================================");
		scanf("%d", &inicio);
		getchar();
		
		switch(inicio){
			case 1:
				adicionarLivro(dadosLivros);
				break;
			case 2:
				cadastrarUsuario(dadosUsuario);
				break;
			case 3:
				emprestarLivro(&dadosLivros, &dadosUsuario);
				break;
			case 4:
				devolverLivro(&dadosLivros, &dadosUsuario);
				break;
			case 5:
				listarLivros(dadosLivros);
				break;
			case 6:
				listarUsuarios(&dadosUsuario);
				break;
			case 7: 
				printf("Retornando...\n");
				break;
			default:
				printf("Opção inválida.\n");
		}
		// VERIFICAÇÂO DE MENSAGENS
		pthread_mutex_lock( & message_lock);
		int i;
		for (i = 0; i < message_count; i++) {
			printf("%s\n", messages[i]);
		}
		message_count = 0;

		Sleep(1500);
		pthread_mutex_unlock( & message_lock);
	} while (inicio != 7);
	
	
	fclose(dadosLivros);
	fclose(dadosUsuario);
 }
//==============================FIM DAS FUNÇÕES DA BIBLIOTECA==================================

void error_exit(const char * message) {
	perror(message);
	exit(EXIT_FAILURE);
}

void * receive_updates(void * socket_desc) {
	int client_socket = * (int * )socket_desc;
	char buffer[1024];
	while (1) {
		int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
		buffer[bytes_received] =  '\0';
		if (bytes_received <= 0) {
			printf("Conexão encerrada pelo servidor.\n");

			return 0;
			break;
		}

		//ARMAZENAR A MENSAGEM RECEBIDA
		pthread_mutex_lock( & message_lock);
		if (message_count < MAX_MESSAGES) {
			strncpy(messages[message_count++], buffer, sizeof(messages[0]));
		}
		pthread_mutex_unlock( & message_lock);
	}
}

int main() {
	setlocale(LC_ALL, "portuguese");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), & wsaData) != 0) {
		printf("Erro ao inicializar Winsock.\n");
		return 1;
	}

	int client_socket;
	struct sockaddr_in server_addr;

	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Erro ao criar socket.\n");
		WSACleanup();
		return 1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);//USO DO inet_addr para compatibilidade com biblioteca winsock2
	
	
	if (server_addr.sin_addr.s_addr == INADDR_NONE) {
		error_exit("Endereço inválido ou não suportado.\n");
	}

	// CONEXÃO COM O SERVIDOR
	if (connect(client_socket, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
		error_exit("Erro ao conectar ao servidor");
	}

	printf("Conectado ao servidor.\n");

	char nome[50];
	printf("Digite seu nome: ");
	fgets(nome, sizeof(nome), stdin);
	nome[strcspn(nome, "\n")] = '\0';
	send(client_socket, nome, strlen(nome), 0);

	
	printf("CARREGANDO DADOS DA BIBLIOTECA..\n");
	recv(client_socket, (char * ) & totalLivros, sizeof(int), 0);
	recv(client_socket, (char * ) & livros, sizeof(Livro) * totalLivros, 0);
	recv(client_socket, (char * ) & totalUsuarios, sizeof(int), 0);
	recv(client_socket, (char * ) & usuarios, sizeof(Usuario) * totalUsuarios, 0);

	char msg[1024];
	int fila = recv(client_socket, msg, sizeof(msg) - 1, 0);
	msg[fila] = '\0';
	printf("%s\n", msg);

	pthread_mutex_init( & message_lock, NULL);

	pthread_t receive_thread;
	if (pthread_create( & receive_thread, NULL, receive_updates, (void * ) & client_socket) != 0) {
		perror("Erro ao criar thread de recebimento");
		closesocket(client_socket);
		WSACleanup();
		return 1;
	}


	Sleep(1500);

	menu();


	/* Salvar dados
	FILE * arquivo = fopen("dados.dat", "wb");
	if (arquivo) {
		fwrite( & totalLivros, sizeof(int), 1, arquivo);
		fwrite(livros, sizeof(Livro), totalLivros, arquivo);
		fwrite( & totalUsuarios, sizeof(int), 1, arquivo);
		fwrite(usuarios, sizeof(Usuario), totalUsuarios, arquivo);
		fclose(arquivo);
		printf("Dados salvos com sucesso.\n");
	} else {
		printf("Erro ao salvar os dados.\n");
	}*/

	closesocket(client_socket);
	WSACleanup();

	return 0;
}
