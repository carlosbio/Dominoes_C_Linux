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

int fd_serv,fd_cli;		// Identificadores do FIFO do servidor e cliente, respectivamente
char fifocli[10];		// Variável global usada no tratamento do sinal shutdown

void Shutdown(int sig)	// Função que recebe o sinal de shutdown quando o servidor desliga
{
	int i;

	fprintf(stdout,"\nO servidor desligou! A encerrar sessao dentro de 5 segundos...\n\n");

	close(fd_cli);		// Fechar FIFO do cliente
	close(fd_serv);		// Fechar FIFO do servidor
	unlink(fifocli);	// Apagar FIFO cliente

	for(i=0;i<5;i++)	// Imprime no ecrã um temporizador de 5 segundos
	{
		fprintf(stderr,". ");
		sleep(1);
	}
	
	printf("\n");

	exit(0);
}

int main(void)
{
	int flag_jog=0,i,j,flag_exit=0;
	char *str[50]={NULL};

	RESPOSTA r;
	PEDIDO p;

	signal(SIGUSR2,Shutdown);		// Recebe o sinal SIGUSR2 e executa funcao shutdown
	signal(SIGINT,SIG_IGN);			// Recebe o sinal SIGINT e ignora-o
	
	if(access(FIFO_SERV,F_OK)!=0)	// Verifica se já existe um servidor em execução
	{
		fprintf(stdout,"ERRO: Nao existe um servidor em execucao!\n\n");
		exit(1);
	}
	
	sprintf(p.fifo,"%d",getpid());	
	mkfifo(p.fifo,0644);				// Criar FIFO cliente	
	fd_serv=open(FIFO_SERV,O_WRONLY);	// Abrir FIFO do servidor

	strcpy(fifocli,p.fifo);				// Copia o nome do fifo do cliente para esta variável global usada no tratamento do sinal shutdown
	
	if (fd_serv==-1)
	{
		fprintf(stderr,"\nErro ao criar/abrir o FIFO\n\n");
		exit(EXIT_FAILURE);
	}

	while(flag_exit==0) 		// Condição que conserva o jogador em jogo e condição de inicio do menu de pedido de identificação do jogador
	{
		if(flag_jog==0)			// Flag que indica que ainda não existe um registo de novo jogador criado
		{
			do
			{
				system("clear");
				fprintf(stdout,"MENU DE ENTRADA\n");
				fprintf(stdout,"Registar novo nome de jogador (inserir nome) | Sair (exit)\n\n[> ");
				scanf(" %19[^\n]",p.com);		// Comando de pedido
		
				write(fd_serv,&p,sizeof(p));	// Enviar pedido
				fd_cli=open(p.fifo,O_RDONLY);	// Abrir FIFO do cliente
				read(fd_cli,&r,sizeof(r));		// Receber resposta
		
				if(r.res==-1)	// Código de resposta ao comando 'exit'
				{
					fprintf(stdout,"\nA terminar o programa...\n\n");
					kill(r.pidserv,SIGUSR1);	// Sinaliza o servidor que o cliente vai encerrar
					close(fd_cli);				// Fechar FIFO do cliente
					flag_exit=1;				// Flag que assinala a saida do ciclo de jogo
					sleep(1);
					break;
				}

				if(r.res==-2)	// Código de resposta quando se tenta registar um nome que já existe
				{
					fprintf(stdout,"\nNOME INVALIDO - Ja existe um jogador com este nome!\n\nAguarde...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
				}
				
				if(r.res==-19||r.res==-20)	// Código de resposta aos comandos 'show' e 'close' (exclusivos do servidor2)
				{
					fprintf(stdout,"\nComando invalido! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
				}

				if(r.res==1)	// Código de resposta após registo de um novo jogador
				{
					fprintf(stdout,"\nBem-vindo %s!\n\nAguarde...\n\n",p.com);
					flag_jog=1;			// Flag de registo de novo jogador
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
					break;
				}

				if(r.res==2)	// Código de resposta após atingir-se o numero máximo de jogadores
				{
					fprintf(stdout,"\nFoi atingido o numero maximo de jogadores!\n\nAguarde...\n\n");
					flag_jog=1;;		// Flag de registo de novo jogador
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
					break;
				}
			}
			while(r.res<0);	// Enquanto resultado for menor que zero
		}

		if(flag_jog==1)		// Se existir pelo menos um jogador ligado, tem a possibilidade de criar um novo jogo
		{
			do				// Menu após registo do jogador
			{	
				system("clear");
				fprintf(stdout,"MENU PRINCIPAL\n");
				fprintf(stdout,"Jogadores (users) | Info (status) | Criar jogo (new 'nome' 'intervalo')\n");
				fprintf(stdout,"Entrar num jogo (play) | Terminar (logout) | Sair (exit)\n\n[> ");
				scanf(" %19[^\n]",p.com);

				write(fd_serv,&p,sizeof(p));	// Enviar pedido
				fd_cli=open(p.fifo,O_RDONLY);	// Abrir FIFO do cliente
				read(fd_cli,&r,sizeof(r));		// Receber resposta

				if(r.res==-1)	// Código de resposta ao comando 'exit'
				{
					fprintf(stdout,"\nA terminar o programa...\n\n");
					kill(r.pidserv,SIGUSR1);	// Sinaliza o servidor que o cliente vai encerrar
					close(fd_cli);				// Fechar FIFO do cliente
					flag_exit=1;				// Flag de condição de saida
					sleep(1);
					break;
				}
	
				if(r.res==-2)	// Código de resposta ao comando 'users'
				{
					i=0;							// Reset do contador
						
					str[i]=strtok(r.com," ");		// Divide a string em tokens
						
					while(str[i]!=NULL)
						str[++i]=strtok(NULL," ");	// Guarda em *str um array para cada token

					fprintf(stdout,"\nLista de jogadores ligados:\n\n");

					for(i=0;i<r.pidis;i++)
						if(r.play[i]==0)
							fprintf(stdout,"\n - Jogador %d (%s - PID: %d) - %d vitorias\n",i+1,str[i],r.pidc[i],r.mao[i]);	// Imprime a lista de jogadores logados
						else
							fprintf(stdout,"\n - Jogador %d (%s - PID: %d) - %d vitorias  <--  EM JOGO\n",i+1,str[i],r.pidc[i],r.mao[i]);	// Imprime a lista de jogadores logados

					fprintf(stdout,"\n\nLista concluida! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(3);
				}
				
				if(r.res==-19||r.res==-20)	// Código de resposta ao comando 'show' e 'close' (exclusivos do servidor2)
				{
					fprintf(stdout,"\nComando invalido! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
				}

				if(r.res==-3)	// Código de resposta após a tentativa de criar um jogo quando já exite um criado
				{
					fprintf(stdout,"\nJa existe um jogo criado! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(2);
				}

				if(r.res==-4)	// Código de resposta após a tentativa de entrar num jogo quando ainda não existe um criado
				{
					fprintf(stdout,"\nAinda nao existe um jogo criado! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(2);
				}

				if(r.res==-5)	// Código de resposta ao comando 'status'
				{
					fprintf(stdout,"\nJa existe um jogo criado! Nome do jogo: %s\n\n",r.jogo);

					i=0;							// Reset do contador
						
					str[i]=strtok(r.com," ");		// Divide a string em tokens
						
					while(str[i]!=NULL)
						str[++i]=strtok(NULL," ");	// Guarda em *str um array para cada token

					fprintf(stdout,"\nLista dos jogadores participantes:\n\n");

					for(i=0;i<r.pidis-1;i++)		// Considera-se r.pidis-1 para não incluir o cliente actual
						fprintf(stdout,"\n - Jogador %d (%s - PID: %d)\n",i+1,str[i],r.pidc[i]);	// Imprime a lista de jogadores logados

					fprintf(stdout,"\n\nLista concluida! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(3);			
				}

				if(r.res==-6)	// Código de resposta após introdução de um comando inválido
				{
					fprintf(stdout,"\nComando invalido! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
				}

				if(r.res==1)	// Código de resposta ao comando 'new'
				{
					fprintf(stdout,"\n---> NOVO JOGO CRIADO <---\n\n");
					fprintf(stdout,"Nome do jogo: %s\n\n",r.jogo);
					fprintf(stdout,"Inicia dentro de %d segundos!\n\nAguarde enquanto os aperitivos vao sendo servidos...\n\n",r.tempo);
					flag_jog=2;		// Flag que sinaliza jogo criado
					close(fd_cli);	// Fechar FIFO do cliente
					
					for(i=r.tempo;i>=0;i--)		// Imprime um timer no ecrã enquanto o timeout é diferente de zero
					{
						fprintf(stderr,". ");
						sleep(1);
					}
				}

				if(r.res==2)	// Código de resposta ao comando 'play'
				{
					fprintf(stdout,"\nBem-vindo!\n\nNome do jogo: %s\n\n",r.jogo);
					fprintf(stdout,"Aguarde enquanto os aperitivos vao sendo servidos...\n\n");
					flag_jog=2;		// Flag que sinaliza jogo criado
					close(fd_cli);	// Fechar FIFO do cliente
					
					for(i=r.tempo;i>=0;i--)
					{
						fprintf(stderr,". ");
						sleep(1);
					}
				}

				if(r.res==3)	// Código de resposta ao comando 'logout'
				{
					fprintf(stdout,"\nA terminar sessao...\n\n");
					flag_jog=0;
					close(fd_cli);	// Fechar FIFO do cliente
					sleep(1);			
				}
			}
			while(r.res<0);
		}

		if(flag_jog==2)		// Flag que sinaliza jogo criado
		{
			system("clear");

			do
			{							
				fprintf(stdout,"MENU JOGO\n");
				fprintf(stdout,"Jogadores (users) | Pedras (tiles) | Mesa (game) | Info (info)\n");
				fprintf(stdout,"Jogar (play 'num' 'lado') | Molho (get) | Passar (pass) | Desistir (exit)\n\n[> ");
				scanf(" %19[^\n]",p.com);
				
				if(r.flagfim==1)	// Indica o fim de um jogo após a tentativa de jogar e já exitir um vencedor
				{
					fprintf(stdout,"\nO JOGO TERMINOU! VENCEDOR: %s \n\nA regressar ao menu principal...\n\n",r.com);
					flag_jog=1;			// Força o regresso ao menu principal
					sleep(2);
					break;
				}
				else	// Caso a flag de fim de jogo esteja a zero, o jogo decorre normalmente
				{
					write(fd_serv,&p,sizeof(p));	// Enviar pedido
					fd_cli=open(p.fifo,O_RDONLY);	// Abrir FIFO do cliente
					read(fd_cli,&r,sizeof(r));		// Receber resposta
					
					if(r.res==0)	// Código de resposta após condição de vitória
					{
						fprintf(stdout,"\nPARABENS! VENCEU O JOGO! A regressar ao menu principal...\n\n");
						flag_jog=1;			// Força o regresso ao menu principal
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}

					if(r.res==-1)	// Código de resposta ao comando 'exit'
					{
						fprintf(stdout,"\nA terminar o programa...\n\n");
						kill(r.pidserv,SIGUSR1);				// Sinaliza o servidor que o cliente vai encerrar
						close(fd_cli);							// Fechar FIFO do cliente
						flag_exit=1;
						sleep(1);
						break;
					}

					if(r.res==-2)	// Código de resposta ao comando 'users'
					{
						i=0;							// Reset do contador
							
						str[i]=strtok(r.com," ");		// Divide a string em tokens
							
						while(str[i]!=NULL)
							str[++i]=strtok(NULL," ");	// Guarda em *str um array para cada token

						fprintf(stdout,"\nLista de jogadores ligados:\n\n");

						for(i=0;i<r.pidis;i++)
							if(r.play[i]==0)
								fprintf(stdout,"\n - Jogador %d (%s - PID: %d) - %d vitorias\n",i+1,str[i],r.pidc[i],r.mao[i]);	// Imprime a lista de jogadores logados
							else
								fprintf(stdout,"\n - Jogador %d (%s - PID: %d) - %d vitorias  <--  EM JOGO\n",i+1,str[i],r.pidc[i],r.mao[i]);	// Imprime a lista de jogadores logados

						fprintf(stdout,"\n\nLista concluida! A regressar ao menu...\n\n");
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(1);
					}

					if(r.res==-3)	// Código de resposta ao comando 'tiles'
					{
						fprintf(stdout,"\nO seu jogo:\n\n");

						for(j=0;j<(3*r.numpedras);j=j+3)
							fprintf(stdout,"%d[%d:%d] ",r.pedras[j],r.pedras[j+1],r.pedras[j+2]);

						printf("\n\n");
						close(fd_cli);							// Fechar FIFO do cliente
						sleep(1);
					}

					if(r.res==-4)	// Código de resposta ao comando 'game' e caso ainda não existam peças na mesa
					{
						fprintf(stdout,"\nAinda nao foi jogada qualquer pedra! A regressar ao menu...\n\n");
						close(fd_cli);							// Fechar FIFO do cliente
						sleep(1);
					}

					if(r.res==-5)	// Código de resposta ao comando 'exit' e já existem pedras na mesa
					{
						fprintf(stdout,"\nVisualizacao da mesa:\n\n");

						for(i=0;i<r.total;i=i+2)
								fprintf(stdout,"[%d:%d]",r.mesa[i],r.mesa[i+1]);

						printf("\n\n");

						close(fd_cli);							// Fechar FIFO do cliente
						sleep(1);
					}

					if(r.res==-6)	// Código de resposta após uma jogada inválida
					{
						fprintf(stdout,"\nJogada invalida! Repetir...\n\n");
						close(fd_cli);							// Fechar FIFO do cliente
						sleep(1);
					}

					if(r.res==-7)	// Código de resposta ao comando 'play'
					{
						fprintf(stdout,"\nPedra jogada! Proximo jogador...\n\n");
						close(fd_cli);							// Fechar FIFO do cliente
						sleep(1);
					}

					if(r.res==-8)	// Código de resposta após a introdução de um comando inválido
					{
						fprintf(stdout,"\nComando invalido! A regressar ao menu...\n\n");
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}

					if(r.res==6)	// Código de resposta após se verificar a condição de vitória
					{
						fprintf(stdout,"\nO JOGO TERMINOU! VENCEDOR: %s \n\nA regressar ao menu principal...\n\n",r.com);
						flag_jog=1;			// Força o regresso ao menu principal
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}

					if(r.res==-10)	// Código de resposta ao comando 'get' quand ainda tem peças válidas para jogar
					{
						fprintf(stdout,"\nJogada invalida! Ainda possui pelo menos uma pedra possivel de ser jogada...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}

					if(r.res==-11)	// Código de resposta ao comando 'get' mas já não existem peças disponíveis
					{
						fprintf(stdout,"\nMolho vazio! A regressar ao menu...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}

					if(r.res==-12)	// Código de resposta ao comando 'get'
					{
						fprintf(stdout,"\nPedra recolhida do molho! A aguardar jogada...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}
					
					if(r.res==-13)	// Código de resposta ao comando 'help'
					{
						if(r.numpedras==0)				
							fprintf(stdout,"\nNao possui pedras que possam ser jogadas! Tem de ir ao pote...\n\n");
						else
						{
							fprintf(stdout,"\nPedras que podem ser jogadas! A aguardar jogada...\n\n");
							
							for(j=0;j<(3*r.numpedras);j=j+3)
								fprintf(stdout,"%d[%d:%d] ",r.pedras[j],r.pedras[j+1],r.pedras[j+2]);

							printf("\n\n");
						}
						 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}
					
					if(r.res==-14)	// Código de resposta ao comando 'help' mas ainda não existem peças na mesa
					{
						fprintf(stdout,"\nMesa vazia, pode jogar qualquer pedra! A aguardar jogada...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}
					
					if(r.res==-15)	// Código de resposta após a tentativa de jogar fora do turno
					{
						fprintf(stdout,"\nAguarde o seu turno! A regressar ao menu...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}
					
					if(r.res==-16)	// Código de resposta ao comando 'info'
					{
						i=0;							// Reset do contador
							
						str[i]=strtok(r.com," ");		// Divide a string em tokens
							
						while(str[i]!=NULL)
							str[++i]=strtok(NULL," ");	// Guarda em *str um array para cada token

						fprintf(stdout,"\nLista dos jogadores participantes:\n\n");

						for(i=0;i<r.pidis;i++)
						{
							if(i==r.turno)						
								fprintf(stdout,"\n - Jogador %d (%s - PID: %d) - %d pedras  <--  PROXIMO A JOGAR\n",i+1,str[i],r.pidc[i],r.mao[i]);	// Jogador em turno
							else
								fprintf(stdout,"\n - Jogador %d (%s - PID: %d) - %d pedras\n",i+1,str[i],r.pidc[i],r.mao[i]);	// Jogadores participantes
						}

						fprintf(stdout,"\n\nExistem %d pedras no molho!\n",r.numpedras);
						fprintf(stdout,"\n\nLista concluida! A regressar ao menu...\n\n");
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(3);
					}
					
					if(r.res==-17)	// Código de resposta ao comando 'pass' quando ainda existem peças no molho
					{
						fprintf(stdout,"\nAinda existem pedras no molho! A regressar ao menu...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}
					
					if(r.res==-18)	// Código de resposta ao comando 'pass'
					{
						fprintf(stdout,"\nTurno passado! A regressar ao menu...\n\n"); 
						close(fd_cli);		// Fechar FIFO do cliente
						sleep(2);
					}
					
					if(r.res==-19||r.res==-20)	// Código de resposta ao comando 'show' e 'close' (exclusivos do servidor2)
					{
					fprintf(stdout,"\nComando invalido! A regressar ao menu...\n\n");
					close(fd_cli);		// Fechar FIFO do cliente
					sleep(1);
					}
				}
			}
			while(r.res<0);
		}
	}

	close(fd_serv);	// Fechar FIFO do servidor
	unlink(p.fifo);	// Apagar FIFO cliente
	
	exit(0);
}
