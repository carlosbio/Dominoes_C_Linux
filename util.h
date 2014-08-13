// --------------------------------------------------------------------------------------------------------------------------------------------------------	//
//																																							//
// Descrição e implementação de um programa em C direccionado para sistemas UNIX para a construção de um sistema de jogos de dominó online em tempo real	//
//																																							//
// Trabalho realizado por Carlos da Silva (21220319) e Feliciano Figueira (21150144) no âmbito da unidade curricular de Sistemas Operativos					//
//																																							//
// Curso de Engenharia Informática, ano lectivo 2013/2014 - Instituto Superior de Engenharia de Coimbra														//
//																																							//
// --------------------------------------------------------------------------------------------------------------------------------------------------------	//

#include <string.h>
#include <strings.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define FIFO_SERV "serv"
#define TAM 28

typedef struct		// Estrutura pedido
{
	char com[50];	// Comando pedido
	char fifo[10];	// Nome do fifo cliente

} PEDIDO;

typedef struct		// Estrutura resposta
{
	int tempo;		// Tempo que o servidor vai aguardar durante o qual aceita novos participantes no jogo criado
	int pidserv;	// PID do servidor
	int res;		// Comando resposta (int)
	int pidis;		// Nro de PIDs activos
	int turno;		// Turno do jogador
	int flagfim;	// Flag que indica o fim de um jogo anterior e permite o login de novos jogadores
	int pidc[4];	// PIDs dos clientes
	int mao[4];		// Numero de pedras que cada jogador tem na mão (esta variável também pode servir para enviar as vitórias conquistadas)
	int play[4];	// Armazena os jogadores que estão em jogo ou não
	int id;			// ID da peça
	int face[2];	// Faces da peça
	int pedras[84];	// Vector com os valores das pedras (id, face 1, face 2 -> 28 * 3 = 84)
	int numpedras;	// Quantidade de pedras que o jogador possui (esta variável também pode servir para outros comandos de resposta)
	int mesa[56];	// Array das pedras dispostas na mesa
	int total;		// Total de faces dispostas na mesa (faces = nº de pedras * 2, portanto max = 56)
	char com[50];	// Comando resposta (char)
	char jogo[20];	// Nome do jogo
	
} RESPOSTA;

RESPOSTA baralho[28];				// Declaração de um variável do tipo estruturado DOMINO

RESPOSTA *stock()					// Função que gera todas as peças do dominó
{
	int i,j,n=0;

	for(i=0;i<7;i++)
	{
		for(j=i;j<7;j++)
		{
			baralho[n].id = n+1;	// Atribui um id à peça
			baralho[n].face[0]=i;	// Atribui um valor a uma das faces
			baralho[n].face[1]=j;	// Atribui um valor à outra face
			n++;
		}
	}

	return baralho;					// Devolve o baralho completo
}
