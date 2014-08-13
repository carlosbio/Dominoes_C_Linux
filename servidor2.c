// --------------------------------------------------------------------------------------------------------------------------------------------------------	//
//																																							//
// Descrição e implementação de um programa em C direccionado para sistemas UNIX para a construção de um sistema de jogos de dominó online em tempo real	//
//																																							//
// Trabalho realizado por Carlos da Silva (21220319) e Feliciano Figueira (21150144) no âmbito da unidade curricular de Sistemas Operativos					//
//																																							//
// Curso de Engenharia Informática, ano lectivo 2013/2014 - Instituto Superior de Engenharia de Coimbra														//
//																																							//
// --------------------------------------------------------------------------------------------------------------------------------------------------------	//

#include "util.h"

int main(int argc,char *argv[])
{
	int fd_serv,fd_cli;		// Identificadores do FIFO do servidor e cliente, respectivamente
	RESPOSTA r;
	PEDIDO p;
	
	if(access(FIFO_SERV,F_OK)!=0)
	{
		fprintf(stdout,"ERRO: Nao existe um servidor em execucao!\n\n");
		exit(1);
	}
	
	sprintf(p.fifo,"%d",getpid());	
	mkfifo(p.fifo,0644);				// Criar FIFO cliente	
	fd_serv=open(FIFO_SERV,O_WRONLY);	// Abrir FIFO do servidor
	
	if (fd_serv==-1)
	{
		fprintf(stderr,"\nErro ao criar/abrir o FIFO\n\n");
		exit(EXIT_FAILURE);
	}
	
	strcpy(p.com,argv[1]);			// Copia o argumento para o pedido
	
	write(fd_serv,&p,sizeof(p));	// Enviar pedido
	fd_cli=open(p.fifo,O_RDONLY);	// Abrir FIFO do cliente
	read(fd_cli,&r,sizeof(r));		// Receber resposta
	
	if(r.res==-19)
		fprintf(stdout,"\n\nA obter informacoes acerca do estado interno do servidor...\n\n");
	
	if(r.res==-20)
	{
		fprintf(stdout,"\nA desligar o servidor...\n\n");
		kill(r.pidserv,SIGUSR2);
	}	
	
	close(fd_cli);
	
	close(fd_serv);	// Fechar FIFO do servidor
	unlink(p.fifo);	// Apagar FIFO cliente
	
	exit(0);
}
