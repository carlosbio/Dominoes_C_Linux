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

int fd_serv,fd_cli;			// Identificadores do FIFO do servidor e cliente, respectivamente
int pidchart[4]; 			// Vector global (pra poder ser acedido pela funcao shutdown) com pids dos processos clientes ligados (maximo de 4 clientes)
char *userschart[4]={NULL};	// Vector de ponteiros global com os nomes dos jogadores ligados (máximo de 4 jogadores)
int pidi=0;					// Contador global do numero de jogadores ligados (máximo de 4 jogadores)
int t=5;					// Contador global do intervalo definido após a criação de um jogo e variável global de alarme
RESPOSTA r;					// Declaração de uma variável global do tipo estrutura RESPOSTA

//#define BG

void Cliexit(int sig)
{
	fprintf(stdout,"\n[SERVIDOR] Um jogador abandonou!\n");
}
void Shutdown(int sig)		// Função chamada atraves de um SINAL SIGUSR2
{
	int i;

	fprintf(stdout,"\n[SERVIDOR] Recebido sinal de shutdown! A desligar o servidor...\n\n");

	for (i=0;i<pidi;i++)
		kill(pidchart[i],SIGUSR2);

	for(i=0;i<5;i++)
	{
		fprintf(stderr,". ");
		sleep(1);
	}
	
	printf("\n");

	unlink(FIFO_SERV);

	exit(0);
}

void Tempo(int s)
{
	signal(SIGALRM,Tempo);
	r.tempo=r.tempo-5;
	alarm(t);
	
	/*if(r.tempo>0)
		fprintf(stdout,"Faltam %d segundos para iniciar o jogo!\n\n",r.tempo);*/

	if(r.tempo==0)
	{
		fprintf(stdout,"\n[SERVIDOR] O jogo esta pronto a iniciar! A aguardar o ultimo jogador...\n\n");
		t=0;
	}
	
	if(r.tempo==-5)
		r.tempo=0;
}

void Randomiza(int *alvo,int n)		// Função que baralha as pedras de dominó
{
	int tam=sizeof(int);
    int i;
    int r;
    int origem[28];

    for (i=0;i<28;origem[i]=i++);

    srand((unsigned int)time(0));

    for (i=28;i>0;i--)
    {
        r=rand()%i;
        memcpy(&alvo[28-i],&origem[r],tam);
        memcpy(&origem[r],&origem[i-1],tam);
    }

	for (i=0;i<28;i++)
		alvo[i]=alvo[i]+1;
}

int main(void)
{
	int pid,i,j,k,l,pass,menor,flagjogo=0,pidaux,flagpedra,pedraval,tok,mix[27],tam=28,pote[28],tampote,flagpote;
	int pedras1[14],nump1,v1=0;
	int pedras2[14],nump2,v2=0;
	int pedras3[14],nump3,v3=0;
	int pedras4[14],nump4,v4=0;
	char *str[50]={NULL},jogs[50],*pecas;

	PEDIDO p;
	RESPOSTA *d;

	signal(SIGUSR2,Shutdown);		// Processo ao receber sinal SIGUSR2 executa funcao shutdown
	signal(SIGUSR1,Cliexit);		// Processo que notifica o servidor quando um cliente termina
	signal(SIGINT,Shutdown);		// Processo ao receber sinal SIGINT executa funcao shutdown
	signal(SIGALRM,Tempo);			// Processo que recebe um sinal de alarme e executa a função Tempo
	
	if(access(FIFO_SERV,F_OK)==0)	// Verificacao de server ja a correr
	{
		fprintf(stdout,"[SERVIDOR] ERRO: Ja existe um servidor em execucao! A terminar...\n");
		exit(1);
	}

#ifdef BG
	pid=fork();	// Fork para passar a execução em background

	if(pid==-1)
	{
		fprintf(stdout,"[SERVIDOR] ERRO: Falha ao criar o fork do processo! A terminar...\n");
		exit(1);
	}

	if(pid!=0)
		exit(0);					// Se o pid é diferente de 0 o processo é o pai
#endif

	r.pidserv=getpid();				// Guarda imediatamente na estrutura o pid do servidor para utilização em sinais

	mkfifo(FIFO_SERV,0644);			// Criar FIFO
	fd_serv=open(FIFO_SERV,O_RDWR);	// Abrir FIFO do servidor desbloqueante

	if (fd_serv==-1)				// Verifica erros ao criar/abrir o fifo do servidor
	{
		fprintf(stderr,"\n[SERVIDOR] ERRO: Falha ao criar/abrir o fifo do servidor! A terminar...\n");
		exit(EXIT_FAILURE);
	}	

	fprintf(stderr,"\n[SERVIDOR] Servidor iniciado!\n");
	
	r.flagfim=0;									// Reset da flag que indica o fim de um jogo anterior e permite o login de novos jogadores
	r.tempo=1;										// Inicialização da variável r.tempo associado ao alarme para o timeout do jogo
	pedras1[0]=pedras2[0]=pedras3[0]=pedras4[0]=0;	// Inicialização das variáveis pedras com o primeiro elemento a zero para verificação se um jogador está em jogo ou não
	
	putenv("NUMPECAS=7");					// Criar a variável de ambiente NUMPECAS
	pecas=getenv("NUMPECAS");				// Guardar a variável de ambiente em pecas
	nump1=nump2=nump3=nump4=atoi(pecas);	// Converter o valor da variável de ambiente para inteiro e definir o numero de peças para cada jogador

	while (1)
	{
		if(r.flagfim==0)						// Impede a entrada neste menu a jogadores que já jogaram pelo menos um jogo e que continuam ligados
		{
			do									// Pedido de identificação do jogador
			{			
				r.res=0;						// Reset do campo resposta

				read(fd_serv,&p,sizeof(p));		// Receber pedido
				
				if(strcasecmp(p.com,"show")==0)	// Verifica se o comando foi 'show'
				{
					if(pidi==0)
						fprintf(stdout,"\n\n[SERVIDOR] Nao existem jogadores ligados...\n\n");
					else
					{
						if(r.tempo==0&&flagjogo==1)		// Caso esteja a decorrer um jogo
						{
							fprintf(stdout,"\n\n[SERVIDOR] Nome do jogo a decorrer: %s\n\n\n[SERVIDOR] Lista dos jogadores em jogo:\n\n",r.jogo);	// Nome do jogo
							
							for(i=0;i<pidi;i++)		
								fprintf(stdout,"\n[SERVIDOR] - Jogador %d (%s - PID: %d)\n",i+1,userschart[i],pidchart[i]);	// Imprime a lista de jogadores em jogo
						}
						else							// Caso não exista um jogo criado
						{			
							fprintf(stdout,"\n\n[SERVIDOR] Nao existe um jogo criado...\n\n\n[SERVIDOR] Lista dos jogadores logados:\n\n");
							
							for(i=0;i<pidi;i++)		
								fprintf(stdout,"\n[SERVIDOR] - Jogador %d (%s - PID: %d)\n",i+1,userschart[i],pidchart[i]);	// Imprime a lista de jogadores logados
						}
					}
					
					r.res=-19;	// Envia código de erro (cliente) ou validação (servidor2)
				}
				else
				if(strcasecmp(p.com,"close")==0)	// Verifica se o comando foi 'close'
					r.res=-20;						// Envia código de erro (cliente) ou validação (servidor2)
				else
				if(strcasecmp(p.com,"exit")==0)		// Verifica se o comando foi 'exit'
					r.res=-1;						// Envia código de validação
				else
					if(pidi>0)						// Verifica se já existe algum jogador ligado
						for(i=0;i<pidi;i++)
							if(strcasecmp(userschart[i],p.com)==0)	// Verifica se já existe um user com o mesmo nome
								r.res=-2;							// Envia código de erro
					
					if(pidi<4&&r.res==0)							// Verifica se foi atingido o valor máximo de jogadores, senão adiciona novo
					{
						pidchart[pidi]=atoi(p.fifo);				// Adiciona o PID do cliente à lista
						userschart[pidi]=malloc(sizeof(p.com));		// Aloca espaço para o nome do jogador na tabela de nomes
						strcpy(userschart[pidi],p.com);				// Adiciona o nome do jogador à lista
						pidi++;										// Incrementa o contador do nro de clientes ligados
						r.res=1;									// Envia código de validação
					}
					else
						if(pidi==4)				// Verifica se o nro maximo de jogadores foi atingido e envia código de erro caso se verifique
							r.res=2;			// Envia código de erro

				fd_cli=open(p.fifo,O_WRONLY);	// Abrir FIFO do cliente
				write(fd_cli,&r,sizeof(r));		// Enviar resposta
				close(fd_cli);					// Fechar FIFO do cliente
			}
			while(r.res<=0);	// Enquanto resposta for menor ou igual a zero
		}

		if(pidi>0)									// Menu após registo do utilizador
		{
			do
			{
				read(fd_serv,&p,sizeof(p));			// Receber pedido

				tok=0;								// Reset do contador de tokens
						
				str[tok]=strtok(p.com," ");			// Divide a string em tokens
						
				while(str[tok]!=NULL)
					str[++tok]=strtok(NULL," ");	// Guarda em *str um array para cada token

				if(strcasecmp(p.com,"show")==0)		// Verifica se o comando foi 'show'
				{
					if(pidi==0)						// Caso não existam jogadores ligados
						fprintf(stdout,"\n\n[SERVIDOR] Nao existem jogadores ligados...\n\n");
					else
					{
						if(r.tempo==0&&flagjogo==1)		// Caso esteja a decorrer um jogo
						{
							fprintf(stdout,"\n\n[SERVIDOR] Nome do jogo a decorrer: %s\n\n\n[SERVIDOR] Lista dos jogadores em jogo:\n\n",r.jogo);	// Nome do jogo
							
							for(i=0;i<pidi;i++)		
								fprintf(stdout,"\n[SERVIDOR] - Jogador %d (%s - PID: %d)\n",i+1,userschart[i],pidchart[i]);	// Imprime a lista de jogadores em jogo
						}
						else							// Caso não exista um jogo criado
						{			
							fprintf(stdout,"\n\n[SERVIDOR] Nao existe um jogo criado...\n\n\n[SERVIDOR] Lista dos jogadores logados:\n\n");
							
							for(i=0;i<pidi;i++)		
								fprintf(stdout,"\n[SERVIDOR] - Jogador %d (%s - PID: %d)\n",i+1,userschart[i],pidchart[i]);	// Imprime a lista de jogadores logados
						}
					}
					
					r.res=-19;		// Envia código de erro (cliente) ou validação (servidor2)
				}
				else
				if(strcasecmp(p.com,"close")==0)	// Verifica se o comando foi 'close'
					r.res=-20;						// Envia código de erro (cliente) ou validação (servidor2)
				else
				if(strcasecmp(str[0],"exit")==0)	// Caso seja introduzido o comando 'exit'
				{
					r.res=-1;						// Envia código de validação
					pidi--;							// Decrementa o o contador de user, eliminando o utilizador actual
					fd_cli=open(p.fifo,O_WRONLY);	// Abrir FIFO do cliente
					write(fd_cli,&r,sizeof(r));		// Enviar resposta
					close(fd_cli);					// Fechar FIFO do cliente
					break;
				}
				else
				if(strcasecmp(str[0],"users")==0)	// Caso seja introduzido o comando 'users'
				{
					for(i=0;i<pidi;i++)
					{
						strcat(jogs,userschart[i]);	// Concatena os users actuais num vector
						strcat(jogs," ");			// Concatena um espaço entre os users actuais
						r.pidc[i]=pidchart[i];		// Guarda os PIDs actuais na estrutura de resposta
						
						if(i==0)					// Para o jogador 1
						{					
							r.mao[0]=v1;			// Guarda as vitórias do jogador na estrutura de resposta
							r.play[0]=pedras1[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
						}
						if(i==1)					// Para o jogador 2
						{
							r.mao[1]=v2;			// Guarda as vitórias do jogador na estrutura de resposta
							r.play[1]=pedras2[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
						}
						if(i==2)					// Para o jogador 3
						{
							r.mao[2]=v3;			// Guarda as vitórias do jogador na estrutura de resposta
							r.play[2]=pedras3[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
						}
						if(i==3)					// Para o jogador 4
						{
							r.mao[3]=v4;			// Guarda as vitórias do jogador na estrutura de resposta
							r.play[3]=pedras4[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
						}
					}

					r.pidis=pidi;			// Guarda o nro de users actuais na estrutura de resposta
					strcpy(r.com,jogs);		// Copia a string com os users actuais para a estrutura de resposta
					strcpy(jogs,"");		// Reset da string auxiliar jogs
					r.res=-2;				// Devolve o código de resposta
				}
				else
				if(strcasecmp(str[0],"logout")==0)	// Caso seja introduzido o comando 'logout'
				{
					pidaux=atoi(p.fifo);			// Converte o PID do cliente para inteiro

					for(i=0;i<pidi;i++)				// Corre os PIDS dos jogadores ligados
					{
						if(pidchart[i]==pidaux)		// Determina qual o cliente que está a comunicar
						{
							for(k=i;k<pidi-1;k++)	// Actualiza a tabela de PIDS e nomes dos jogadores
							{
								pidchart[k]=pidchart[k+1];
								userschart[k]=userschart[k+1];
							}

							pidi--;		// Decrementa o contador de jogadores	
						}
					}

					r.res=3;			// Envia código de comando
				}
				else
				if(strcasecmp(str[0],"status")==0)		// Caso seja introduzido o comando 'status'
				{
					if(flagjogo==0)						// Caso não exista um jogo criado
						r.res=-4;
					else
					{
						for(i=0;i<pidi;i++)
						{
							strcat(jogs,userschart[i]);	// Concatena os users actuais num vector
							strcat(jogs," ");			// Concatena um espaço entre os users actuais
							r.pidc[i]=pidchart[i];		// Guarda os PIDs actuais na estrutura de resposta
						}

						r.pidis=pidi;			// Guarda o nro de users actuais na estrutura de resposta
						strcpy(r.com,jogs);		// Copia a string com os users actuais para a estrutura de resposta
						strcpy(jogs,"");		// Reset da string auxiliar jogs
						r.res=-5;				// Devolve o código de resposta
					}
				}
				else
				if(strcasecmp(str[0],"play")==0)	// Caso seja introduzido o comando 'play'
				{
					if(flagjogo==0)
						r.res=-4;					// Código de resposta de jogo não criado
					else
						r.res=2;					// Código de resposta de autorização de entrada em jogo
				}
				else
				if(strcasecmp(str[0],"new")==0)		// Caso seja introduzido o comando 'new'
				{	
					if(flagjogo==1)					// Caso já exista um jogo criado
						r.res=-3;
					else
					{
						r.res=1;				// Código de resposta
						strcpy(r.jogo,str[1]);	// Nome do jogo
						r.tempo=atoi(str[2]);	// Tempo durante qual se aceitam novos participantes no jogo criado
						alarm(t);				// Inicia o alarme da função do tempo restante para iniciar o jogo
						flagjogo=1;				// Flag que indica a criação de um novo jogo
						r.flagfim=0;			// Reset da flag que indica o fim de um jogo anterior e permite o login de novos jogadores
					}	
				}
				else
					r.res=-6;					// Código de resposta caso seja introduzido um comando inválido
		
				fd_cli=open(p.fifo,O_WRONLY);	// Abrir FIFO do cliente
				write(fd_cli,&r,sizeof(r));		// Enviar resposta
				close(fd_cli);					// Fechar FIFO do cliente
			}
			while(r.res<0);
		}

		if(r.tempo==0&&flagjogo==1)		// Menu após criação de um jogo
		{
			r.res=0;			// Reset da variável de comando
			
			pass=0;				// Reset do contador de passagem de turno;

			r.total=0;			// Inicialização da variável que indica o nº de faces de dominó na mesa

			d=stock();			// Cria as pedras de dominó numa lista ordenada de referência

			Randomiza(mix,tam);	// Baralha as pedras de dominó, guardando no array mix valores criados aleatoriamente
			
			if(pidi==2)			// Distribui as pedras no caso de um jogo de 2 jogadores
			{
				for(i=0;i<7;i++)
					pedras1[i]=mix[i];	// Atribui 7 valores de mix ao jogador
			
				for(j=0;i<14;i++,j++)
					pedras2[j]=mix[i];	// Atribui 7 valores de mix ao jogador

				for(j=0;i<28;i++,j++)
					pote[j]=mix[i];		// As restantes ficam no molho

				tampote=14;
			}

			if(pidi==3)			// Distribui as pedras no caso de um jogo de 3 jogadores
			{
				for(i=0;i<7;i++)
					pedras1[i]=mix[i];	// Atribui 7 valores de mix ao jogador
			
				for(j=0;i<14;i++,j++)
					pedras2[j]=mix[i];	// Atribui 7 valores de mix ao jogador

				for(j=0;i<21;i++,j++)
					pedras3[j]=mix[i];	// Atribui 7 valores de mix ao jogador

				for(j=0;i<28;i++,j++)
					pote[j]=mix[i];		// As restantes ficam no molho

				tampote=7;
			}

			if(pidi==4)			// Distribui as pedras no caso de um jogo de 4 jogadores
			{
				for(i=0;i<7;i++)
					pedras1[i]=mix[i];	// Atribui 7 valores de mix ao jogador
			
				for(j=0;i<14;i++,j++)
					pedras2[j]=mix[i];	// Atribui 7 valores de mix ao jogador

				for(j=0;i<21;i++,j++)
					pedras3[j]=mix[i];	// Atribui 7 valores de mix ao jogador

				for(j=0;i<28;i++,j++)
					pedras4[j]=mix[i];	// Atribui 7 valores de mix ao jogador

				tampote=0;				// Com 4 jogadores não existe molho
			}

			do
			{
				if(r.res==0)
					fprintf(stdout,"\n[SERVIDOR] Jogo em curso! Nao sao permitidos mais jogadores!\n");

				for(r.turno=0;r.turno<pidi;)			// Estratégia de mudança de turnos (após atingir o valor máximo, volta a zero e conta novamente)
				{
					read(fd_serv,&p,sizeof(p));			// Receber pedido					
										
					tok=0;								// Reset do contador de tokens
							
					str[tok]=strtok(p.com," ");			// Divide a string em tokens
							
					while(str[tok]!=NULL)
						str[++tok]=strtok(NULL," ");	// Guarda em *str um array para cada token

					if(strcasecmp(p.com,"show")==0)		// Verifica se o comando foi 'show'
					{
						if(pidi==0)
							fprintf(stdout,"\n\n[SERVIDOR] Nao existem jogadores ligados...\n\n");
						else
						{
							if(r.tempo==0&&flagjogo==1)		// Caso esteja a decorrer um jogo
							{
								fprintf(stdout,"\n\n[SERVIDOR] Nome do jogo a decorrer: %s\n\n\n[SERVIDOR] Lista dos jogadores em jogo:\n\n",r.jogo);	// Nome do jogo
								
								for(i=0;i<pidi;i++)		
									fprintf(stdout,"\n[SERVIDOR] - Jogador %d (%s - PID: %d)\n",i+1,userschart[i],pidchart[i]);	// Imprime a lista de jogadores em jogo
							}
							else							// Caso não exista um jogo criado
							{			 
								fprintf(stdout,"\n\n[SERVIDOR] Nao existe um jogo criado...\n\n\n[SERVIDOR] Lista dos jogadores logados:\n\n");
								
								for(i=0;i<pidi;i++)		
									fprintf(stdout,"\n[SERVIDOR] - Jogador %d (%s - PID: %d)\n",i+1,userschart[i],pidchart[i]);	// Imprime a lista de jogadores logados
							}
						}
						
						r.res=-19;		// Envia código de erro (cliente) ou validação (servidor2)
					}
					else
					if(strcasecmp(p.com,"close")==0)	// Verifica se o comando foi 'close'
						r.res=-20;						// Envia código de erro (cliente) ou validação (servidor2)
					else
					if(strcasecmp(str[0],"exit")==0||strcasecmp(str[0],"giveup")==0)	// Verifica se o comando foi 'exit' ou 'giveup'
					{
						pidaux=atoi(p.fifo);			// Converte o PID do cliente para inteiro

						for(i=0;i<pidi;i++)				// Corre os PIDS dos jogadores ligados
						{
							if(pidchart[i]==pidaux)		// Determina qual o cliente que está a comunicar
							{
								if(i==0)				// Verifica as peças do jogador, caso se trate do jogador 1
									for(j=0;j<nump1;j++)			// Corre a mão actual do jogador 1
									{
										pote[tampote]=pedras1[j];	// Passa as peças da mão para o pote
										tampote++;					// Incrementa o numero de peças do pote
										pedras1[j]=0;				// Coloca a zero os campos das peças
									}
								
								if(i==1)				// Verifica as peças do jogador, caso se trate do jogador 2
									for(j=0;j<nump2;j++)			// Corre a mão actual do jogador 2
									{
										pote[tampote]=pedras2[j];	// Passa as peças para o pote
										tampote++;					// Incrementa o numero de peças do pote
										pedras2[j]=0;				// Coloca a zero os campos das peças
									}
									
								if(i==2)				// Verifica as peças do jogador, caso se trate do jogador 3
									for(j=0;j<nump3;j++)			// Corre a mão actual do jogador 3
									{
										pote[tampote]=pedras3[j];	// Passa as peças para o pote
										tampote++;					// Incrementa o numero de peças do pote
										pedras3[j]=0;				// Coloca a zero os campos das peças
									}
								
								if(i==3)				// Verifica as peças do jogador, caso se trate do jogador 3
									for(j=0;j<nump4;j++)			// Corre a mão actual do jogador 3
									{
										pote[tampote]=pedras4[j];	// Passa as peças para o pote
										tampote++;					// Incrementa o numero de peças do pote
										pedras4[j]=0;				// Coloca a zero os campos das peças
									}							
								
								for(k=i;k<pidi-1;k++)	// Actualiza a tabela de PIDS e nomes dos jogadores
								{
									pidchart[k]=pidchart[k+1];
									userschart[k]=userschart[k+1];
								}

								pidi--;		// Decrementa o contador de jogadores	
							}
						}

						r.res=-1;			// Envia código de comando
					}
					else
					if(strcasecmp(str[0],"users")==0)	// Verifica se o comando foi 'users'
					{
						for(i=0;i<pidi;i++)
						{
							strcat(jogs,userschart[i]);	// Concatena os users actuais num vector
							strcat(jogs," ");			// Concatena um espaço entre os users actuais
							r.pidc[i]=pidchart[i];		// Guarda os PIDs actuais na estrutura de resposta
							
							if(i==0)					// Para o jogador 1
							{					
								r.mao[0]=v1;			// Guarda as vitórias do jogador na estrutura de resposta
								r.play[0]=pedras1[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
							}
							if(i==1)					// Para o jogador 2
							{
								r.mao[1]=v2;			// Guarda as vitórias do jogador na estrutura de resposta
								r.play[1]=pedras2[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
							}
							if(i==2)					// Para o jogador 3
							{
								r.mao[2]=v3;			// Guarda as vitórias do jogador na estrutura de resposta
								r.play[2]=pedras3[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
							}
							if(i==3)					// Para o jogador 4
							{
								r.mao[3]=v4;			// Guarda as vitórias do jogador na estrutura de resposta
								r.play[3]=pedras4[0];	// Guarda a informação se o jogador está em jogo ou não com base na primeira peça na mão (zero significa que não está)
							}
						}

						r.pidis=pidi;			// Guarda o nro de users actuais na estrutura de resposta
						strcpy(r.com,jogs);		// Copia a string com os users actuais para a estrutura de resposta
						strcpy(jogs,"");		// Reset da string auxiliar jogs
						r.res=-2;				// Devolve o código de resposta
					}
					else
					if(strcasecmp(str[0],"tiles")==0)	// Verifica se o comando foi 'tiles'
					{
						pidaux=atoi(p.fifo);			// Converte o PID do cliente para inteiro

						for(i=0;i<pidi;i++)				// Corre os PIDS dos jogadores ligados
						{
							if(pidchart[i]==pidaux)		// Procura o PID do cliente que fez o pedido
							{
								if(i==0)				// Jogador 1
								{
									for(j=0,l=0;j<nump1;j++,l=l+3)		// Corre as pedras atribuidas aleatoriamente
									{
										k=pedras1[j];					// Guarda o indice actual
										r.pedras[l]=d[k-1].id;			// k-1 porque, no ponteiro de arrays *d, no indice 0 fica a pedra com o ID 1
										r.pedras[l+1]=d[k-1].face[0];
										r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
									}

									r.numpedras=nump1;					// Devolve o numero de pedras que o jogador possui
								}

								if(i==1)				// Jogador 2
								{
									for(j=0,l=0;j<nump2;j++,l=l+3)		// Corre as pedras atribuidas aleatoriamente
									{
										k=pedras2[j];					// Guarda o indice actual
										r.pedras[l]=d[k-1].id;
										r.pedras[l+1]=d[k-1].face[0];
										r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
									}

									r.numpedras=nump2;					// Devolve o numero de pedras que o jogador possui
								}

								if(i==2)				// Jogador 3
								{
									for(j=0,l=0;j<nump3;j++,l=l+3)		// Corre as pedras atribuidas aleatoriamente
									{
										k=pedras3[j];					// Guarda o indice actual
										r.pedras[l]=d[k-1].id;
										r.pedras[l+1]=d[k-1].face[0];
										r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
									}

									r.numpedras=nump3;					// Devolve o numero de pedras que o jogador possui
								}

								if(i==3)				// Jogador 4
								{
									for(j=0,l=0;j<nump4;j++,l=l+3)		// Corre as pedras atribuidas aleatoriamente
									{
										k=pedras4[j];					// Guarda o indice actual
										r.pedras[l]=d[k-1].id;
										r.pedras[l+1]=d[k-1].face[0];
										r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
									}

									r.numpedras=nump4;					// Devolve o numero de pedras que o jogador possui
								}
							}
						}

						r.res=-3;		// Envia código de comando
					}
					else
					if(strcasecmp(str[0],"game")==0)	// Verifica se o comando foi 'game'
					{
						if(r.total==0)
							r.res=-4;	// Envia código de comando
						else
							r.res=-5;	// Envia código de comando					
					}
					else
					if(strcasecmp(str[0],"play")==0&&tok==3)	// Verifica se o comando foi 'play' e se o numero de tokens está correcto
					{
						pidaux=atoi(p.fifo);					// Converte o PID do cliente para inteiro
						
						if(pidaux!=pidchart[r.turno])			// Verifica se o jogador se encontra no seu turno
							r.res=-15;
						else
						{						
							flagpedra=0;						// Reset da flag de validação de peça candidata a ser jogada

							pedraval=0;							// Reset da flag de validação de peça colocada na mesa

							r.id=atoi(str[1]);					// Converte o ID da peça em inteiro e guarda em r.id

							for(k=0;k<pidi;k++)					// Verifica se a pedra que está a ser jogada pertence de facto à mão do jogador
							{
								if(pidchart[k]==pidaux)			// Determina qual o cliente que está a comunicar actualmente
								{
									if(k==0)					// Verifica as peças do jogador, caso se trate do jogador 1
										for(l=0;l<nump1;l++)
											if(r.id==pedras1[l])
												flagpedra=1;	// Caso o jogador possua esta peça, a flag é assinalada
										
									if(k==1)					// Verifica as peças do jogador, caso se trate do jogador 2
										for(l=0;l<nump2;l++)
											if(r.id==pedras2[l])
												flagpedra=1;	// Caso o jogador possua esta peça, a flag é assinalada

									if(k==2)					// Verifica as peças do jogador, caso se trate do jogador 3
										for(l=0;l<nump3;l++)
											if(r.id==pedras3[l])
												flagpedra=1;	// Caso o jogador possua esta peça, a flag é assinalada

									if(k==3)					// Verifica as peças do jogador, caso se trate do jogador 4
										for(l=0;l<nump4;l++)
											if(r.id==pedras4[l])
												flagpedra=1;	// Caso o jogador possua esta peça, a flag é assinalada
								}					
							}

							if(flagpedra==1)		// Uma vez validado que o jogador tem de facto a peça, coloca-se a mesma na mesa
							{
								for(i=0;i<28;i++)								// Corre o array geral das pedras ordenadas
								{
									if(d[i].id==r.id)							// Procura a pedra no array geral de pedras através do ID
									{
										if(r.total==0)							// Caso a mesa ainda esteja vazia de peças
										{
											r.mesa[r.total]=d[i].face[0];		// Coloca na mesa a primeira face da primeira pedra
											r.mesa[r.total+1]=d[i].face[1];		// Coloca na mesa a segunda face da primeira pedra
											r.total=r.total+2;					// Incremento de 2 (é uma pedra mas são 2 faces jogadas)
											pedraval=1;							// Flag de validação de pedra colocada na mesa
										}
										else		// Caso já exista pelo menos uma peça na mesa
										{
											if(strcasecmp(str[2],"left")==0||strcasecmp(str[2],"right")==0)		// Garante a introdução de um comando válido
											{
												if(strcasecmp(str[2],"left")==0)		// Caso se coloque a peça no lado esquerdo da sequência de peças na mesa
												{
													if(r.mesa[0]==d[i].face[0])			// Compara se o valor é igual à face 1
													{
														for(j=r.total-1;j>=0;j--)
															r.mesa[j+2]=r.mesa[j];		// Assim sendo, move duas posições para direita para todo o conteúdo do array
											
														r.mesa[1]=d[i].face[0];			// Atribui à mesa o valor da face 1
														r.mesa[0]=d[i].face[1];			// Atribui à mesa o valor da face 2
														r.total=r.total+2;				// Incremento da contador de faces
														pedraval=1;						// Flag de validação de pedra colocada na mesa
													}
													else
														if(r.mesa[0]==d[i].face[1])		// Compara se o valor é igual à face 2
														{
															for(j=r.total-1;j>=0;j--)
																r.mesa[j+2]=r.mesa[j];	// Assim sendo, move duas posições para direita para todo o conteúdo do array
											
															r.mesa[1]=d[i].face[1];		// Atribui à mesa o valor da face 2
															r.mesa[0]=d[i].face[0];		// Atribui à mesa o valor da face 1
															r.total=r.total+2;			// Incremento da contador de faces
															pedraval=1;					// Flag de validação de pedra colocada na mesa
														}
														else							// No caso de jogada inválida
															r.res=-6;					// Comando de erro
												}

												if(strcasecmp(str[2],"right")==0)			// Caso se coloque a peça do lado direito
												{
													if(r.mesa[r.total-1]==d[i].face[0])		// Compara se o valor é igual à face 1
													{
														r.mesa[r.total]=d[i].face[0];		// Atribui à mesa o valor da face 1
														r.mesa[r.total+1]=d[i].face[1];		// Atribui à mesa o valor da face 2
														r.total=r.total+2;					// Incremento da contador de faces
														pedraval=1;							// Flag de validação de pedra colocada na mesa
													}
													else
														if(r.mesa[r.total-1]==d[i].face[1])	// Compara se o valor é igual à face 2
														{
															r.mesa[r.total]=d[i].face[1];	// Atribui à mesa o valor da face 2
															r.mesa[r.total+1]=d[i].face[0];	// Atribui à mesa o valor da face 1
															r.total=r.total+2;				// Incremento da contador de faces
															pedraval=1;						// Flag de validação de pedra colocada na mesa
														}
														else								// No caso de jogada inválida
															r.res=-6;						// Comando de erro
												}
											}
											else			// No caso da introdução de um comando inválido
												r.res=-6;	// Comando de erro
										}			
									}
								}

								if(pedraval==1)		// Uma vez validada a colocação da peça na mesa, retira-se a peça jogada da mão do jogador
								{
									r.turno++;							// Confima que o jogador colocou uma pedra válida e passa o turno
									
									pass=0;								// Reset do contador em que se passa o turno sem ter jogado
									
									for(k=0;k<pidi;k++)					// Verifica qual foi a pedra jogada e elimina-a do array de peças da mão do jogador
									{
										if(pidchart[k]==pidaux)			// Determina qual o cliente que está a comunicar actualmente
										{
											if(k==0)					// Verifica as peças do jogador, caso se trate do jogador 1
												for(l=0;l<nump1;l++)
													if(pedras1[l]==r.id)
													{
														for(i=l;i<nump1-1;i++)		// Actualiza o array de peças
															pedras1[i]=pedras1[i+1];
															
														nump1--;			// Decrementa o número de peças

														if(nump1==0)		// Caso acabem as peças da mão
														{
															v1++;			// Adiciona uma vitória conquistada
															r.flagfim=1;	// Flag de final de jogo
															r.turno=4;		// Força a saída do ciclo de turno
															r.res=0;		// Comando de resposta
															break;
														}
														else
														{
															r.res=-7;		// Comando de resposta de peça colocada na mesa
															break;
														}
													}
										
											if(k==1)					// Verifica as peças do jogador, caso se trate do jogador 2
												for(l=0;l<nump2;l++)
													if(pedras2[l]==r.id)
													{
														for(i=l;i<nump2-1;i++)		// Actualiza o array de peças
															pedras2[i]=pedras2[i+1];
															
														nump2--;			// Decrementa o número de peças
														
														if(nump2==0)		// Caso acabem as peças da mão
														{
															v2++;			// Adiciona uma vitória conquistada
															r.flagfim=1;	// Flag de final de jogo
															r.turno=4;		// Força a saída do ciclo de turno
															r.res=0;		// Comando de resposta
															break;
														}
														else
														{
															r.res=-7;		// Comando de resposta de peça colocada na mesa
															break;
														}
													}

											if(k==2)					// Verifica as peças do jogador, caso se trate do jogador 3
												for(l=0;l<nump3;l++)
													if(pedras3[l]==r.id)
													{
														for(i=l;i<nump3-1;i++)		// Actualiza o array de peças
															pedras3[i]=pedras3[i+1];
															
														nump3--;			// Decrementa o número de peças
														
														if(nump3==0)		// Caso acabem as peças da mão
														{
															v3++;			// Adiciona uma vitória conquistada
															r.flagfim=1;	// Flag de final de jogo
															r.turno=4;		// Força a saída do ciclo de turno
															r.res=0;		// Comando de resposta
															break;
														}
														else
														{
															r.res=-7;		// Comando de resposta de peça colocada na mesa
															break;
														}
													}

											if(k==3)					// Verifica as peças do jogador, caso se trate do jogador 4
												for(l=0;l<nump4;l++)
													if(pedras4[l]==r.id)
													{
														for(i=l;i<nump4-1;i++)		// Actualiza o array de peças
															pedras4[i]=pedras4[i+1];
															
														nump4--;			// Decrementa o número de peças
														
														if(nump4==0)		// Caso acabem as peças da mão
														{
															v4++;			// Adiciona uma vitória conquistada
															r.flagfim=1;	// Flag de final de jogo
															r.turno=4;		// Força a saída do ciclo de turno
															r.res=0;		// Comando de resposta
															break;
														}
														else
														{
															r.res=-7;		// Comando de resposta de peça colocada na mesa
															break;
														}
													}
										}					
									}
								}
							}
							else
								r.res=-6;		// Comando de resposta de jogada inválida
						}
					}
					else
					if(strcasecmp(str[0],"get")==0)
					{
						pidaux=atoi(p.fifo);			// Converte o PID do cliente para inteiro
						
						if(pidaux!=pidchart[r.turno])	// Verifica se o jogador se encontra no seu turno
							r.res=-15;
						else						
						if(r.total==0)					// Verifica se já foram jogadas peças
							r.res=-10;
						else					
						if(tampote==0)					// Verifica se existem peças no pote
							r.res=-11;
						else
						{
							flagpote=0;					// Reset da flag de validação do pote							

							for(i=0;i<pidi;i++)
								if(pidchart[i]==pidaux)			// Determina qual o cliente que está a comunicar actualmente
								{
									if(i==0)					// Verifica as peças do jogador, caso se trate do jogador 1
										for(j=0;j<nump1;j++)	// Corre a mão actual do jogador 1
										{
											k=pedras1[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;		// Mal encontre um pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump1-1))	// Ao verificar-se que o jogador precisa mesmo de uma pedra atribui-se uma do pote
												{
													pedras1[nump1]=pote[tampote-1];	// Passa uma pedra do pote para a mão do jogador
													nump1++;						// Incrementa o numero de peças na mão do jogador
													tampote--;						// Decrementa as peças do pote						
													r.res=-12;						// Comando de validação
													break;							// Sai do ciclo
												}
											}
										}
										
									if(i==1)					// Verifica as peças do jogador, caso se trate do jogador 2
										for(j=0;j<nump2;j++)	// Corre a mão actual do jogador 2
										{
											k=pedras2[j];
											
											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;		// Mal encontre um pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump2-1))
												{
													pedras2[nump2]=pote[tampote-1];	// Passa uma pedra do pote para a mão do jogador
													nump2++;						// Incrementa o numero de peças na mão do jogador
													tampote--;						// Decrementa as peças do pote		
													r.res=-12;						// Comando de validação
													break;							// Sai do ciclo
												}
											}
										}
										
									if(i==2)					// Verifica as peças do jogador, caso se trate do jogador 3
										for(j=0;j<nump3;j++)	// Corre a mão actual do jogador 3
										{
											k=pedras3[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;		// Mal encontre um pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump3-1))
												{
													pedras3[nump3]=pote[tampote-1];	// Passa uma pedra do pote para a mão do jogador
													nump3++;						// Incrementa o numero de peças na mão do jogador
													tampote--;						// Decrementa as peças do pote		
													r.res=-12;						// Comando de validação
													break;							// Sai do ciclo
												}
											}
										}
										
									if(i==3)					// Verifica as peças do jogador, caso se trate do jogador 4
										for(j=0;j<nump4;j++)	// Corre a mão actual do jogador 4
										{
											k=pedras4[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;		// Mal encontre um pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump4-1))
												{
													pedras4[nump4]=pote[tampote-1];	// Passa uma pedra do pote para a mão do jogador
													nump4++;						// Incrementa o numero de peças na mão do jogador
													tampote--;						// Decrementa as peças do pote			
													r.res=-12;						// Comando de validação
													break;							// Sai do ciclo
												}
											}
										}								
									}
								}
					}
					else
					if(strcasecmp(str[0],"help")==0)	// Verifica se o comando foi 'help'
					{
						r.numpedras=0;					// Reset do numero de pedras a enviar como resposta ao cliente
						
						if(r.total==0)					// Caso ainda não tenha sido jogado qualquer peça
							r.res=-14;
						else
						{
							pidaux=atoi(p.fifo);

							for(i=0;i<pidi;i++)
								if(pidchart[i]==pidaux)				// Determina qual o cliente que está a comunicar actualmente
								{
									if(i==0)						// Verifica as peças do jogador, caso se trate do jogador 1
									{
										for(l=0,j=0;j<nump1;j++)	// Corre a mão actual do jogador 1
										{
											k=pedras1[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.pedras[l]=d[k-1].id;			// k-1 porque, no ponteiro de arrays *d, no indice 0 fica a pedra com o ID 1
												r.pedras[l+1]=d[k-1].face[0];
												r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
												l=l+3;							// Depois de atribuir os 3 valores, incrementa em 3 o contador
											}

											r.numpedras=(l/3);		// Devolve o numero de pedras que podem ser jogadas
										}
									}
									
									if(i==1)						// Verifica as peças do jogador, caso se trate do jogador 2
									{
										for(l=0,j=0;j<nump2;j++)	// Corre a mão actual do jogador 2
										{
											k=pedras2[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.pedras[l]=d[k-1].id;			// k-1 porque, no ponteiro de arrays *d, no indice 0 fica a pedra com o ID 1
												r.pedras[l+1]=d[k-1].face[0];
												r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
												l=l+3;							// Depois de atribuir os 3 valores, incrementa em 3 o contador
											}

											r.numpedras=(l/3);		// Devolve o numero de pedras que podem ser jogadas
										}
									}
									
									if(i==2)						// Verifica as peças do jogador, caso se trate do jogador 3
									{
										for(l=0,j=0;j<nump3;j++)	// Corre a mão actual do jogador 3
										{
											k=pedras3[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.pedras[l]=d[k-1].id;			// k-1 porque, no ponteiro de arrays *d, no indice 0 fica a pedra com o ID 1
												r.pedras[l+1]=d[k-1].face[0];
												r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
												l=l+3;							// Depois de atribuir os 3 valores, incrementa em 3 o contador
											}

											r.numpedras=(l/3);		// Devolve o numero de pedras que podem ser jogadas
										}
									}
									
									if(i==3)						// Verifica as peças do jogador, caso se trate do jogador 4
									{
										for(l=0,j=0;j<nump4;j++)	// Corre a mão actual do jogador 4
										{
											k=pedras4[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.pedras[l]=d[k-1].id;			// k-1 porque, no ponteiro de arrays *d, no indice 0 fica a pedra com o ID 1
												r.pedras[l+1]=d[k-1].face[0];
												r.pedras[l+2]=d[k-1].face[1];	// Guarda os valores dos 3 inteiros de cada pedra num vector auxiliar
												l=l+3;							// Depois de atribuir os 3 valores, incrementa em 3 o contador
											}

											r.numpedras=(l/3);		// Devolve o numero de pedras que podem ser jogadas
										}
									}								
								}
								
							r.res=-13;		// Comando de resposta
						}
					}				
					else
					if(strcasecmp(str[0],"info")==0)	// Verifica se o comando foi 'info'
					{
						for(i=0;i<pidi;i++)
						{
							strcat(jogs,userschart[i]);	// Concatena os users actuais num vector
							strcat(jogs," ");			// Concatena um espaço entre os users actuais
							r.pidc[i]=pidchart[i];		// Guarda os PIDs actuais na estrutura de resposta
						}

						r.pidis=pidi;			// Guarda o nro de users actuais na estrutura de resposta
						strcpy(r.com,jogs);		// Copia a string com os users actuais para a estrutura de resposta
						strcpy(jogs,"");		// Reset da string auxiliar jogs
						
						r.numpedras=tampote;	// Numero de peças disponíveis no pote
						
						for(j=0;j<pidi;j++)		// Numero de pedras que cada jogador tem na mão
						{
							if(j==0)						
								r.mao[0]=nump1;	// Numero de pedras do jogador 1
							if(j==1)
								r.mao[1]=nump2;	// Numero de pedras do jogador 2
							if(j==2)
								r.mao[2]=nump3;	// Numero de pedras do jogador 3
							if(j==3)
								r.mao[3]=nump4;	// Numero de pedras do jogador 4
						}
						
						r.res=-16;		// Devolve o código de resposta
					}
					else				
					if(strcasecmp(str[0],"pass")==0)	// Verifica se o comando foi 'pass'
					{
						pidaux=atoi(p.fifo);			// Converte o PID do cliente para inteiro
						
						if(pidaux!=pidchart[r.turno])	// Verifica se o jogador se encontra no seu turno
							r.res=-15;
						else
						if(tampote!=0)					// Verifica se ainda existem peças no pote
							r.res=-17;
						else
						{
							for(i=0;i<pidi;i++)
								if(pidchart[i]==pidaux)			// Determina qual o cliente que está a comunicar actualmente
								{
									if(i==0)					// Verifica as peças do jogador, caso se trate do jogador 1
										for(j=0;j<nump1;j++)	// Corre a mão actual do jogador 1
										{
											k=pedras1[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;			// Mal encontre uma pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump1-1))	// Ao verificar-se que o jogador não tem hipótese de jogar, autoriza-se a passagem de turno
												{								
													pass++;
													
													if(pass==4)		// Se houver 4 passagens de turno seguidas, acaba o jogo e ganha quem tem menos peças
													{
														for(i=0;i<pidi;i++)	// Determina quem tem menos peças
														{
															if(i==0)
															{
																menor=nump1;
																l=i;
															}
															else
															if(i==1&&nump2<menor)
															{
																menor=nump2;
																l=i;
															}
															else
															if(i==2&&nump3<menor)
															{
																menor=nump3;
																l=i;
															}
															else
															if(i==3&&nump4<menor)
															{
																menor=nump4;
																l=i;
															}	
														}
														
														if(l==0)	// Adiciona uma vitória conquistada
															v1++;
														if(l==1)
															v2++;
														if(l==2)
															v3++;
														if(l==3)
															v4++;
															
														strcpy(r.com,userschart[l]);	// Nome do vencedor															
														r.flagfim=1;					// Flag de final de jogo
														r.turno=4;						// Força a saída do ciclo de turno
														r.res=6;						// Comando de resposta
														break;
													}
													else			// Caso ainda não se tenham atingido 4 passagens de turno seguidas
													{
														r.turno++;	// Incrementa o turno
														r.res=-18;	// Comando de resposta
														break;
													}
												}
											}
										}
										
									if(i==1)					// Verifica as peças do jogador, caso se trate do jogador 2
										for(j=0;j<nump2;j++)	// Corre a mão actual do jogador 2
										{
											k=pedras2[j];
											
											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;			// Mal encontre uma pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump2-1))	// Ao verificar-se que o jogador não tem hipótese de jogar, autoriza-se a passagem de turno
												{								
													pass++;
													
													if(pass==4)		// Se houver 4 passagens de turno seguidas, acaba o jogo e ganha quem tem menos peças
													{
														for(i=0;i<pidi;i++)	// Determina quem tem menos peças
														{
															if(i==0)
															{
																menor=nump1;
																l=i;
															}
															else
															if(i==1&&nump2<menor)
															{
																menor=nump2;
																l=i;
															}
															else
															if(i==2&&nump3<menor)
															{
																menor=nump3;
																l=i;
															}
															else
															if(i==3&&nump4<menor)
															{
																menor=nump4;
																l=i;
															}	
														}
														
														if(l==0)	// Adiciona uma vitória conquistada
															v1++;
														if(l==1)
															v2++;
														if(l==2)
															v3++;
														if(l==3)
															v4++;
															
														strcpy(r.com,userschart[l]);	// Nome do vencedor															
														r.flagfim=1;					// Flag de final de jogo
														r.turno=4;						// Força a saída do ciclo de turno
														r.res=6;						// Comando de resposta
														break;
													}
													else			// Caso ainda não se tenham atingido 4 passagens de turno seguidas
													{
														r.turno++;	// Incrementa o turno
														r.res=-18;	// Comando de resposta
														break;
													}
												}
											}
										}
										
									if(i==2)					// Verifica as peças do jogador, caso se trate do jogador 3
										for(j=0;j<nump3;j++)	// Corre a mão actual do jogador 3
										{
											k=pedras3[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;			// Mal encontre uma pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump3-1))	// Ao verificar-se que o jogador não tem hipótese de jogar, autoriza-se a passagem de turno
												{								
													pass++;
													
													if(pass==4)		// Se houver 4 passagens de turno seguidas, acaba o jogo e ganha quem tem menos peças
													{
														for(i=0;i<pidi;i++)	// Determina quem tem menos peças
														{
															if(i==0)
															{
																menor=nump1;
																l=i;
															}
															else
															if(i==1&&nump2<menor)
															{
																menor=nump2;
																l=i;
															}
															else
															if(i==2&&nump3<menor)
															{
																menor=nump3;
																l=i;
															}
															else
															if(i==3&&nump4<menor)
															{
																menor=nump4;
																l=i;
															}	
														}
														
														if(l==0)	// Adiciona uma vitória conquistada
															v1++;
														if(l==1)
															v2++;
														if(l==2)
															v3++;
														if(l==3)
															v4++;
															
														strcpy(r.com,userschart[l]);	// Nome do vencedor															
														r.flagfim=1;						// Flag de final de jogo
														r.turno=4;						// Força a saída do ciclo de turno
														r.res=6;						// Comando de resposta
														break;
													}
													else			// Caso ainda não se tenham atingido 4 passagens de turno seguidas
													{
														r.turno++;	// Incrementa o turno
														r.res=-18;	// Comando de respota
														break;
													}
												}
											}
										}
										
									if(i==3)					// Verifica as peças do jogador, caso se trate do jogador 4
										for(j=0;j<nump4;j++)	// Corre a mão actual do jogador 4
										{
											k=pedras4[j];

											if(r.mesa[0]==d[k-1].face[0]||r.mesa[0]==d[k-1].face[1]||r.mesa[r.total-1]==d[k-1].face[0]||r.mesa[r.total-1]==d[k-1].face[1])
											{
												r.res=-10;			// Mal encontre uma pedra que possa ser jogada, envia comando de erro e salta
												break;
											}
											else
											{
												if(j==(nump4-1))	// Ao verificar-se que o jogador não tem hipótese de jogar, autoriza-se a passagem de turno
												{								
													pass++;
													
													if(pass==4)		// Se houver 4 passagens de turno seguidas, acaba o jogo e ganha quem tem menos peças
													{
														for(i=0;i<pidi;i++)	// Determina quem tem menos peças
														{
															if(i==0)
															{
																menor=nump1;
																l=i;
															}
															else
															if(i==1&&nump2<menor)
															{
																menor=nump2;
																l=i;
															}
															else
															if(i==2&&nump3<menor)
															{
																menor=nump3;
																l=i;
															}
															else
															if(i==3&&nump4<menor)
															{
																menor=nump4;
																l=i;
															}	
														}
														
														if(l==0)	// Adiciona uma vitória conquistada
															v1++;
														if(l==1)
															v2++;
														if(l==2)
															v3++;
														if(l==3)
															v4++;
															
														strcpy(r.com,userschart[l]);	// Nome do vencedor															
														r.flagfim=1;					// Flag de final de jogo
														r.turno=4;						// Força a saída do ciclo de turno
														r.res=6;						// Comando de resposta
														break;
													}
													else			// Caso ainda não se tenham atingido 4 passagens de turno seguidas
													{
														r.turno++;	// Incrementa o turno
														r.res=-18;	// Comando de resposta
														break;
													}
												}
											}
										}								
									}							
						}
					}
					else
						r.res=-8;		// Código de resposta caso seja introduzido um comando inválido
						
					fd_cli=open(p.fifo,O_WRONLY);	// Abrir FIFO do cliente
					write(fd_cli,&r,sizeof(r));		// Enviar resposta
					close(fd_cli);					// Fechar FIFO do cliente
				}
			}
			while(r.res<0);		// Enquanto o r.res for inferior a zero
		}
	}

	close(fd_serv);		// Fechar FIFO do servidor
	unlink(FIFO_SERV);	// Apagar FIFO do servidor
	
	exit(0);
}
