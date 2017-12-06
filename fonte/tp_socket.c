/* tp_socket.c - funções auxiliares para uso de sockets UDP*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>

#include "tp_socket.h"

#define MTU 1024 

int tp_mtu(void)
{
    fprintf(stderr,"tp_mtu called\n");
    /*******************************************************
     * A alteração do MTU oferecido para o PJD pode ser    *
     * feita alterando-se o valor da macro usada a seguir. *
     *******************************************************/
    return MTU;
}

int tp_sendto(int so, char* buff, int buff_len, so_addr* to_addr)
{
    int count;
    fprintf(stderr,"tp_sendto called (%d bytes)\n", buff_len);
    /*******************************************************
     * Aqui seria um bom lugar para injetar erros a fim de *
     * exercitar a funcionalidade do protocolo da camada   *
     * acima (o PJD).                                      *
     *******************************************************/

    count = sendto(so, (void*)buff, buff_len, 0,
            (struct sockaddr*) to_addr, sizeof(struct sockaddr_in));
    fprintf(stderr,"tp_sendto returning (sent %d bytes)\n", count);
    return count;
    
}

int tp_recvfrom(int so, char* buff, int buff_len, so_addr* from_addr)
{
    int count;
    unsigned int sockaddr_len = sizeof(struct sockaddr_in);
    fprintf(stderr,"tp_recvfrom called (%d bytes)\n",buff_len);
    /*******************************************************
     * Aqui seria um bom lugar para injetar erros a fim de *
     * exercitar a funcionalidade do protocolo da camada   *
     * acima (o PJD).                                      *
     *******************************************************/
    /*
    time_t t;
    srand((unsigned) time(&t));
    int erro = rand() % 50;
    if(erro > 25){
        return 0;
    }
    */


    count = recvfrom(so,(void*)buff,(size_t)buff_len,0,
            (struct sockaddr*) from_addr, &sockaddr_len);
    fprintf(stderr,"tp_recvfrom returning (received %d bytes)\n",count);
    return count;
}

int tp_recvfrom_confiavel(int so, char* buff, int buff_len, so_addr* from_addr)
{
    int count;
    unsigned int sockaddr_len = sizeof(struct sockaddr_in);
    fprintf(stderr,"tp_recvfrom called (%d bytes)\n",buff_len);
    /*******************************************************
     * Aqui seria um bom lugar para injetar erros a fim de *
     * exercitar a funcionalidade do protocolo da camada   *
     * acima (o PJD).                                      *
     *******************************************************/
    count = recvfrom(so,(void*)buff,(size_t)buff_len,0,
            (struct sockaddr*) from_addr, &sockaddr_len);
    fprintf(stderr,"tp_recvfrom returning (received %d bytes)\n",count);
    return count;
}

int tp_init(void)
{
    fprintf(stderr,"tp_init called\n");
    /*********************************************************
     * Exceto para fins de automatizar os testes com versões *
     * alteradas deste código (para injetar erros), não deve *
     * haver motivo para se alterar esta função.             *
     *********************************************************/
    return 0;
}

/*****************************************************
 * A princípio não deve haver motivo para se alterar *
 * as funções a seguir.                              *
 *****************************************************/

int tp_socket(unsigned short port)
{
    int    so;
    struct sockaddr_in local_addr;
    int    addr_len =sizeof(local_addr);

    fprintf(stderr,"tp_socket called\n");
    if ((so=socket(PF_INET,SOCK_DGRAM,0))<0) {
        return -1;
    }
    if (tp_build_addr(&local_addr,INADDR_ANY,port)<0) {
        return -2;
    }
    if (bind(so, (struct sockaddr*)&local_addr, sizeof(local_addr))<0) {
        return -3;
    }
    return so;
}

int tp_build_addr(so_addr* addr, char* hostname, int port)
{
    struct hostent* he;

    fprintf(stderr,"tp_build_addr called\n");
    addr->sin_family = PF_INET;
    addr->sin_port   = htons(port);
    if (hostname==NULL) {
        addr->sin_addr.s_addr = INADDR_ANY;
    } else {
        if ((he=gethostbyname(hostname))==NULL) {
            return -1;
        }
        bcopy(he->h_addr,&(addr->sin_addr.s_addr),sizeof(in_addr_t));
    }
    return 0;
}

/**
* Aloca memoria dinamicamente para criar um novo buffer, com tamanho tam_buffer + 1.
*/
char * cria_buffer(int tam_buffer){
    char *buffer;
    buffer = (char*) malloc((tam_buffer + 1) * sizeof(char));
    memset(buffer, 0, tam_buffer + 1);

    return buffer;
}

/**
* Sair do programa com uma mensagem de erro e codigo de saida 1.
*/
void sair_com_erro(const char *mensagem_erro, const int valor_errno){
    errno = valor_errno;
    perror(mensagem_erro);
    exit(EXIT_FAILURE);
}


char ** cria_janela(int porta_servidor, int tam_buffer, int tam_janela){
    char **janela;
    janela = (char**) malloc(tam_janela * sizeof(char*));
    int i;

    seq_servidor = 0;
    ack_servidor = 0;
    ack_esperado = 0;

    for(i = 0; i<tam_janela; i++){   
        janela[i] = (char*) malloc((tam_buffer+1) * sizeof(char));
        memset(janela[i], 0, tam_buffer + 1);
    }

    return janela;
}
