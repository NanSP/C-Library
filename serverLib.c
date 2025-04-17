#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <locale.h>


#define maxUsuarios 50
#define maxCaracter 50
#define maxLivro 50



typedef struct {
	char titulo[maxCaracter],autor[maxCaracter],anoPublicacao,status[20];
}Livro;

typedef struct {
	char nome[maxCaracter],listaEmprestados[2][100];
	int id,nbook;
}Usuario;

Livro livros[maxLivro];
int totalLivros = 0;
Usuario usuarios[maxUsuarios];
int totalUsuarios = 0;

//==SERVIDOR==
#define PORT 8080
#define MAX_QUEUE 10

typedef struct {
    int socket;
    char nome[50];
} UserClient;

UserClient * queue[MAX_QUEUE];
int queue_count = 0;
pthread_mutex_t queue_lock;

void save_queue_to_file() {
    FILE* file = fopen("fila.txt", "w");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return;
    }

	int i;
    for (i = 0; i < queue_count; i++) {
        fprintf(file, "Posição: %d \nNome: %s\n", i + 1, queue[i]->nome);
    }

    fclose(file);
}

// Adicionar usuário na fila
void add_to_queue(int client_socket) {
    pthread_mutex_lock(&queue_lock);
    if (queue_count >= MAX_QUEUE) {
        send(client_socket, "A fila esta cheia! Tente novamente mais tarde.\n", 30, 0);
        pthread_mutex_unlock(&queue_lock);
        return;
    }
    UserClient *new_user = (UserClient *) malloc(sizeof(UserClient));
    new_user->socket = client_socket;
    queue[queue_count++] = new_user;
    
	recv(client_socket, new_user->nome, sizeof(new_user->nome), 0);
	
	printf("Socket %d [%s] se conectou com o server! \n", client_socket, new_user->nome);
	printf("Enviando informações da biblioteca para [%d]\n", client_socket);
	send(client_socket, (const char * ) & totalLivros, sizeof(int), 0);
	send(client_socket, (const char * )livros, sizeof(Livro) * totalLivros, 0);
	send(client_socket, (const char * ) & totalUsuarios, sizeof(int), 0);
	send(client_socket, (const char * )usuarios, sizeof(Usuario) * totalUsuarios, 0);
	
	

    char message[100];
    sprintf(message, "Você está na posição %d da fila.\n", queue_count);
    send(client_socket, message, strlen(message), 0);

    // Notificar todos os outros clientes
    int i;
    for (i = 0; i < queue_count - 1; i++) {
    	printf("%d, %d \n", i, queue[i]->socket);
        send(queue[i]->socket, "Nova posição na fila!\n", 25, 0);
    }
    
	save_queue_to_file();
	
    pthread_mutex_unlock(&queue_lock);
}

// Função para lidar com clientes
void* handle_client(void* arg) {
	
    int client_socket = *(int*)arg;
    free(arg);

    add_to_queue(client_socket);
    
	while (1) {
        char buffer[256];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Cliente desconectado: %d\n", client_socket);
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Mensagem recebida de %d: %s\n", client_socket, buffer);
    }
    
	pthread_mutex_lock(&queue_lock);
	int i, j;
    for (i = 0; i < queue_count; i++) {
        if (queue[i]->socket == client_socket) {
            free(queue[i]); 
            for (j = i; j < queue_count - 1; j++) {
                queue[j] = queue[j + 1];
            }
            queue_count--;
            break;
        }
    }
    
    
    char message[100];
    int x;
    for (x = 0; x < queue_count; x++) {
        sprintf(message, "Nova posição: %d da fila.\n", i + 1);
        send(queue[i]->socket, message, strlen(message), 0);
    }
    
    save_queue_to_file();
    
    FILE * arquivo = fopen("informaçoes.dat", "rb");
	if (arquivo) {
		fread( & totalLivros, sizeof(int), 1, arquivo);
		fread(livros, sizeof(Livro), totalLivros, arquivo);
		fread( & totalUsuarios, sizeof(int), 1, arquivo);
		fread(usuarios, sizeof(Usuario), totalUsuarios, arquivo);
		fclose(arquivo);
		printf("Informações da biblioteca foram atualizadas com sucesso!\n");
	} else {
		printf("Erro ao abrir o arquivo.\n");
	}
    
    pthread_mutex_unlock(&queue_lock);
    
    closesocket(client_socket);
    return NULL;
}

int main() {
	setlocale(LC_ALL, "portuguese");
	
    WSADATA wsaData;
    int server_socket, *new_client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    pthread_mutex_init(&queue_lock, NULL);

    // Inicializar
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("Erro ao connectar ao socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) == SOCKET_ERROR) {
        perror("Erro ao escutar");
        exit(EXIT_FAILURE);
    }

	// Carregar dados
	FILE * arquivo = fopen("informaçoes.dat", "rb");
	if (arquivo) {
		fread( & totalLivros, sizeof(int), 1, arquivo);
		fread(livros, sizeof(Livro), totalLivros, arquivo);
		fread( & totalUsuarios, sizeof(int), 1, arquivo);
		fread(usuarios, sizeof(Usuario), totalUsuarios, arquivo);
		fclose(arquivo);
		printf("Dados da biblioteca carregados com sucesso!\n");
	}
	
    printf("SERVIDOR EM EXECUÇÃO NA PORTA: %d\n", PORT);
    printf("=======REGISTROS=======\n");
    while (1) {
        new_client_socket = (int*)malloc(sizeof(int));
        *new_client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (*new_client_socket == INVALID_SOCKET) {
            perror("Erro ao aceitar conexão!");
            free(new_client_socket);
            continue;
        }
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, new_client_socket) != 0) {
            perror("Erro ao criar thread!");
            free (new_client_socket);
        }
    }
    pthread_mutex_destroy(&queue_lock);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
