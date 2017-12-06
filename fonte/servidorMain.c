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
* Imprime resultado.
*/
void imprime_resultado(struct timeval tempo_inicio, struct timeval tempo_final) {
	printf("O tempo de execucao do servidor foi de %.6f seconds\n", ((tempo_final.tv_sec * 1000000 + tempo_final.tv_usec)
			- (tempo_inicio.tv_sec * 1000000 + tempo_inicio.tv_usec))/ 1000000.0);
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

	//Verifica se o numero de argumentos e' valido, ou seja, igual a 3.
    if(argc != 3) {
        sair_com_erro("Numero de argumentos invalido! Informe: porta_do_servidor tam_buffer\n Erro", EINVAL);     
    }

    //Variaveis com os argumentos da linha de comando
    int porta_servidor = atoi(argv[1]);
    int tam_buffer = atoi(argv[2]);
    int tam_dados = tam_buffer-TAMANHO_CABECALHO;

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
    char *dados = cria_buffer(tam_dados); //o dados represeta a parte do buffer que contem os dados, sem o cabeçalho

    //Variaveis relacionadas aos sockets
    so_addr endereco_cliente;
    int socket_servidor;

    //Variaveis relacionadas aos bytes
    int bytes_recebidos=0;
    int bytes_enviados=0;

    //Variaveis relacionadas ao protocolo de confirmacao
    //int seq_servidor=0; //ID de cada pacote
    //int ack_servidor=0; //ack de cada pacote
    //int ack_esperado=0; //representa o ultimo ID criado (quantos IDs eu já criei)

    //Variaveis relacionadas a janela
    //Cria a janela
    char **janela = cria_janela(porta_servidor,tam_buffer,TAMANHO_JANELA);
    int deslocamento_janela=0;
    int tam_atual_janela=0;

    FILE *arquivo;

    //Faz abertura passiva do servidor e Liga o socket do servidor com o endereco do servidor
    socket_servidor = tp_socket(porta_servidor);

    if(socket_servidor < 0){
    	sair_com_erro("Erro ao criar o socket do sevidor\n Erro", EINVAL);
    }

    //Inicializa o temporizador
    mysethandler(); 
    mysettimer(espera);


    //Recebe o nome do arquivo
	//CONSIDERAR QUE O NOME DO ARQUVIO É RECEBIDO SEM PERDA
	while(1){
		code = tp_recvfrom_confiavel(socket_servidor, buffer_ack, tam_buffer, &endereco_cliente);
		if(code < 0){
			sair_com_erro("Erro ao receber o nome do arquivo\n Erro", EINVAL);
		}
		else{
			bytes_recebidos += code;
            
            //inicia temporizador
            pause();

			//Abre o arquivo requisitado pelo cliente para leitura
  			arquivo = fopen(buffer_ack, "r");

  			if(arquivo == NULL){
				sair_com_erro("Erro ao abrir o arquivo para leitura\n Erro", EINVAL);
  			}
  			else{
  				break;
  			}

		}
	}

    //Começa a enviar os dados para o cliente
    //Vai lendo o arquivo e enviando um buffer por vez, até chegar ao final do arquivo
    while(1) {
        code = fread(dados, sizeof(*dados), tam_dados, arquivo);
        bytes_enviados += code;
        //continua lendo e mandando..
        if(code>0){
            //CRIAR O PACOTE
            //Limpa os buffers
            memset(buffer, 0, tam_buffer + 1);
            memset(buffer_ack, 0, tam_buffer + 1);

            buffer[0] = seq_servidor;
            buffer[1] = ack_servidor;
            memcpy(buffer+2, dados, code);

            //Checar se tem espaço na janela
            //janela cheia
            if(TAMANHO_JANELA == tam_atual_janela){

                code = tp_recvfrom(socket_servidor, buffer_ack, tam_buffer, &endereco_cliente);

                //Servidor não recebeu nenhum ACK. É preciso reenviar todos os pacotes do buffer antes de mover a janela deslizante
                while(code==-1){  
                    int i;
                    for(i = deslocamento_janela; i < TAMANHO_JANELA; i++){

                        /*
                        id_pacote=idPacote(janela_[i%tamjanela_]);
                        criaPacote(pacotereenvio,id_pacote,TIPO_DADOS,TAM_CABECALHO+janela_[i],tam_buffer);
                        tp_sendto(idsocket_, pacotereenvio, tam_buffer+TAM_CABECALHO,(so_addr*)&endereco_);
                        */
                        
                        code = tp_sendto(socket_servidor, janela[i%TAMANHO_JANELA], tam_buffer, &endereco_cliente);
                        if(code<0){
                            sair_com_erro("Erro ao enviar o buffer\n Erro", EINVAL);
                        }

                    }
                    code = tp_recvfrom(socket_servidor, buffer_ack, tam_buffer, &endereco_cliente);
                    if(code<0){
                        sair_com_erro("Erro ao enviar o buffer\n Erro", EINVAL);
                    }
                }

                //Servidor recebe o ACK de um pacote não esperado
                if(buffer_ack[0]!=ack_esperado){
                    tam_atual_janela -= buffer_ack[0] - ack_esperado -1;
                    ack_esperado = buffer_ack[0];
                }

                //Desliza Janela e copia pacote para a janela
                ack_esperado++;    
                strcpy(janela[deslocamento_janela], buffer);
                deslocamento_janela=(deslocamento_janela+1)%TAMANHO_JANELA;

            }
            //Se a Janela tem espaço livre para colocar o pacote
            else{
                tam_atual_janela++;
                strcpy(janela[(tam_atual_janela-1+deslocamento_janela)%TAMANHO_JANELA], buffer); //Copia o pacote para a primeira posição vazia da janela
            }

            //Envia o pacote para o cliente depois de inserir na janela

            code = tp_sendto(socket_servidor, buffer, tam_buffer, &endereco_cliente);
            if(code<0){
                sair_com_erro("Erro ao enviar o buffer\n Erro", EINVAL);
            }
            else{
                //bytes_enviados += strlen(dados);
            }
            //inicia temporizador
            pause();

            seq_servidor = (seq_servidor + 1)%127;
            ack_servidor = (ack_servidor + 1)%127;
        }

        else if(code < 0){
            sair_com_erro("Erro ao ler do arquivo\n Erro", EINVAL);
        }

        //acabou o arquivo
        else{
            //sai do while
            break;
        }
    }
    //Acabou o arquivo

    //Envia final de arquivo
    memset(buffer, 0, tam_buffer + 1);
    memset(buffer_ack, 0, tam_buffer + 1);

    buffer[0] = seq_servidor;
    buffer[1] = ack_servidor;
    memcpy(buffer+2, "FIM", strlen("FIM"));
    buffer[5] = '\0';

    seq_servidor = (seq_servidor + 1)%127;
    (ack_servidor++)%127;

    code = tp_sendto(socket_servidor, buffer, tam_buffer, &endereco_cliente); 

    if(code<0){
        sair_com_erro("Erro ao enviar o fim da comunicacao\n Erro", EINVAL);
    }
    pause();


    //aguarda o ack do fim do arquivo
    while(1){
        code = tp_recvfrom_confiavel(socket_servidor, buffer, tam_buffer, &endereco_cliente);
        //se eu nao recebi nada, envia de novo sem mudar nada
        if (code == 0) {
            code = tp_sendto(socket_servidor, buffer, tam_buffer, &endereco_cliente);
            
            if(code<0){
                sair_com_erro("Erro ao enviar o buffer\n Erro", EINVAL);
            }
            //inicia temporizador
            pause();
        } 

        //se deu erro
        else if(code < 0){
            sair_com_erro("Erro ao receber ack do cliente\n Erro", EINVAL);
        }

        else {
         break;
        }     

    }

/*
    do{
        tp_recvfrom_confiavel(socket_servidor, buffer_ack, tam_buffer, &endereco_cliente);
        
    }while(buffer_ack[2]!='F'); //Espera a confirmação do ack final
  
    pause();
    */

     //Chama gettimeofday para pegar o tempo de termino, para calcular o tempo do servidor.
    if(gettimeofday(&tempo_final, NULL) == -1) {
        perror("Nao foi possivel executar a funcao gettimeofday corretamente\n Erro");
    }

    //Imprime resultado
    imprime_resultado(tempo_inicial, tempo_final);
    printf("Quantidade de dados enviados: %d\n", bytes_enviados);

    fclose(arquivo);
    free(buffer);
    free(buffer_ack);
    free(dados);
    close(socket_servidor);
	return 0;
}
