#include "tp_socket.h"
#include "temporizador.h"

#define TAMANHO_CABECALHO 2*sizeof(char)
#define espera 1000
#define TAMANHO_JANELA 16

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>

/**
* Imprime o resultado.
*/
void imprime_resultado(struct timeval tempo_inicial, struct timeval tempo_final, int bytes_recebidos, int tam_buffer){
    float tempo_total = ( (tempo_final.tv_sec * 1000000 + tempo_final.tv_usec) - (tempo_inicial.tv_sec * 1000000 + tempo_inicial.tv_usec) )/1000000.0;
    float bytes_por_segundo = (bytes_recebidos/tempo_total)/1000;

    printf("Buffer = %5u byte(s), %10.2f kbps (%u bytes em %.6f s)\n", tam_buffer, bytes_por_segundo, bytes_recebidos, tempo_total);
}

int main(int argc, char *argv[]){

	//Chama a funcao tp_init antes de qualquer outra funcao
	int resultado = tp_init();
	if(resultado == 0){
		fprintf(stderr, "Funcao de inicializacao executada com sucesso\n");
	}
	else{
		sair_com_erro("Erro na funcao de inicializacao\n Erro", EINVAL);     
		
	}

	//Verifica se o numero de argumentos e' valido, ou seja, igual a 5.
    if(argc != 5) {
        sair_com_erro("Numero de argumentos invalido! Informe: host_do_servidor porta_do_servidor nome_arquivo tam_buffer\n Erro", EINVAL);  	
    }

    //Variaveis com os argumentos da linha de comando
    char *host_servidor = argv[1];
    int porta_servidor = atoi(argv[2]);
    char *nome_arquivo = argv[3];
    int tam_buffer = atoi(argv[4]);

    int code=0;

    //Variaveis com as medidas do tempo inicial e final
    struct timeval tempo_inicial;
    struct timeval tempo_final;

    //Chama gettimeofday para pegar o tempo de inicio, para calcular o tempo do servidor.
    if(gettimeofday(&tempo_inicial, NULL) == -1) {
        perror("Nao foi possivel executar a funcao gettimeofday corretamente\n Erro");
    }

    //Variaveis relacionadas ao buffer
    char *buffer = cria_buffer(tam_buffer); //o buffer representa os pacotes que a gente envia
    char *buffer_ack = cria_buffer(tam_buffer); //o buffer_ack representa o pacote que a gente recebe do cliente
    char *dados;

    //Variaveis relacionadas aos sockets
    so_addr endereco_servidor;
    int socket_cliente;

    //Variaveis relacionadas aos bytes
    int bytes_recebidos=0;
    int bytes_enviados=0;

    //Variaveis relacionadas ao protocolo de confirmacao
    int seq_cliente=0; //ID de cada pacote
    int ack_cliente=0; //ack de cada pacote
    int ack_esperado=0; //representa o ultimo ID criado (quantos IDs eu já criei)

    FILE *arquivo;

    //Cria o socket do cliente
    socket_cliente = tp_socket(0);

    if(socket_cliente < 0){
    	sair_com_erro("Erro ao criar o socket do cliente\n Erro", EINVAL);
    }

    tp_build_addr(&endereco_servidor, host_servidor, porta_servidor);
    

    //CONSIDERAR QUE O NOME DO ARQUVIO É ENVIADO SEM PERDA
    //Envia o nome do arquivo
    code = tp_sendto(socket_cliente, nome_arquivo, strlen(nome_arquivo), &endereco_servidor);

	if(code<0){
		sair_com_erro("Erro ao enviar o nome do arquivo\n Erro", EINVAL);
	}
	else{
		bytes_enviados += code;
	}

    //Abrir arquivo que vai ser gravado
    strcat(nome_arquivo, ".out"); //para nao dar conflito no diretorio
    arquivo = fopen(nome_arquivo,"wb");

    if(arquivo == NULL){
        sair_com_erro("Erro ao abrir o arquivo para escrita\n Erro", EINVAL);
    }

    //----------------------------------------------
    	
	//Começa a receber os dados do arquivo
	//Le o arquivo por bytes e escreve em arquivo

	while (1) {
		code = tp_recvfrom(socket_cliente, buffer, tam_buffer, &endereco_servidor);
      
		//Se eu recebi alguma coisa
        if (code > 0) {

            //O cliente recebeu uma mensagem 
            dados = &(buffer[2]);

            //Cliente recebeu novamente um pacote que já foi recebido (ou seja, o servidor não recebeu o ACK daquele pacote)
            while(buffer[0] != seq_servidor){
                code = tp_sendto(socket_cliente, buffer, sizeof(char)*5, &endereco_servidor);
                if(code<0){
                    sair_com_erro("Erro ao enviar o ack para o servidor\n Erro", EINVAL);
                }       
                pause();

                code = tp_recvfrom(socket_cliente, buffer, tam_buffer, &endereco_servidor);
            }

            //Se o pacote recebido for o pacote de fim de arquivo
            if (strcmp(dados, "FIM") == 0){
                ack_cliente = (seq_servidor + 1)%127;
                seq_cliente = ack_servidor;
                
                memset(buffer_ack, 0, tam_buffer + 1);

                buffer_ack[0] = buffer[0];
                buffer_ack[1] = ack_cliente;
                buffer_ack[2] = 'F';
                code = tp_sendto(socket_cliente, buffer_ack, sizeof(char)*5, &endereco_servidor);
                if(code<0){
                    sair_com_erro("Erro ao enviar o ack para o servidor\n Erro", EINVAL);
                }       
                break;              
            }

            //Se é uma mensagem: Cliente recebe pacote esperado e envia um pacote ACK para o servidor
            else{
                ack_cliente = (seq_servidor + 1)%127;
                seq_cliente = ack_servidor;
                
                memset(buffer_ack, 0, tam_buffer + 1);

                buffer_ack[0] = buffer[0];
                buffer_ack[1] = ack_cliente;
                buffer_ack[2] = 'A';
                code = tp_sendto(socket_cliente, buffer_ack, sizeof(char)*5, &endereco_servidor);
                if(code<0){
                    sair_com_erro("Erro ao enviar o ack para o servidor\n Erro", EINVAL);
                }       
            }

            seq_servidor = (seq_servidor + 1)%127;
            (ack_servidor++)%127;
            
            //escreve no arquivo

            bytes_recebidos += strlen(dados);

            //Escrever a mensagem no arquivo
            fwrite(dados, sizeof(*dados), strlen(dados), arquivo);
            
    	}

        else if(code < 0){
        	sair_com_erro("Erro ao receber dados do arquivo\n Erro", EINVAL);
        }

	}	    

    //----------------------------------------------

    //Chama gettimeofday para pegar o tempo de termino, para calcular o tempo do servidor.
    if(gettimeofday(&tempo_final, NULL) == -1) {
        perror("Nao foi possivel executar a funcao gettimeofday corretamente\n Erro");
    }

    //Imprime resultado
    imprime_resultado(tempo_inicial, tempo_final, bytes_recebidos, tam_buffer);
    printf("Quantidade de dados recebidos: %d\n", bytes_recebidos);

    fclose(arquivo);
    free(buffer);
    free(buffer_ack);
    close(socket_cliente);

	return 0;
}
