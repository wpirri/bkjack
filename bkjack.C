/* *****************************************************************************
*	BLACKJACK	
*	bkjack.c
*
***************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include "bkjack.h"

#include "bkjugador.h"
#include "bkbanca.h"

void ExitHandler(int sig);
void ChildExit(int sig);
void ShowError();
void ShowMessage(const char* msg);


CBkBanca *pBanca;
CBkJugador *pJugador;
int admin = 0;

int main(int argc, const char *argv[])
{
	int help = 0;
	char *debug = NULL;
	int i;
	int key;

	pBanca = NULL;
	pJugador = NULL;

/* Capturo las señales que necesito */
	signal(SIGALRM,	SIG_IGN);
	signal(SIGPIPE,	SIG_IGN);

	signal(SIGKILL,	ExitHandler);
	signal(SIGTERM,	ExitHandler);
	signal(SIGSTOP,	ExitHandler);
	signal(SIGABRT,	ExitHandler);
	signal(SIGQUIT,	ExitHandler);
	signal(SIGPWR,	ExitHandler);
	signal(SIGINT,	ExitHandler);
	signal(SIGILL,	ExitHandler);
	signal(SIGFPE,	ExitHandler);
	signal(SIGSEGV,	ExitHandler);
	signal(SIGBUS,	ExitHandler);
	signal(SIGSTKFLT,	ExitHandler);

	signal(SIGUSR1,	SIG_IGN);
	signal(SIGUSR2,	SIG_IGN);

	signal(SIGCHLD,	ChildExit);

	if(argc > 1)
	{
		for(i = 1; i < argc; i++)
		{
			if(strcmp("-b", argv[i]) == 0)
			{
				admin = 1;
			}
			else if(strcmp("-d", argv[i]) == 0)
			{
				debug = (char*)malloc(strlen(argv[++i]) + 1);
				strcpy(debug, argv[i]);
			}
			else if(strcmp("-h", argv[i]) == 0)
			{
				help = 1;
			}
		}
	}
	if(help)
	{
		if(debug) free(debug);
		printf("Use: %s [-b] [-d filename] [-h]\n", argv[0]);
		printf("     por defecto arranca como JUGADOR\n");
		printf("     -b: arranca en modo BANCA\n");
		printf("     -d filename: mensajes de depuración al archivo 'filename'\n");
		printf("     -h: muestra la ayuda\n");
		return 0;
	}
	key = ftok(argv[0], 1); 
	if(admin)
	{
		ShowMessage("Arrancando BANCA");	
		pBanca = new CBkBanca(key);
		if(pBanca->Open() == 0)
		{
			while(pBanca->Run());
		}
		else
		{
			ShowError();
		}
	}
	else
	{
		pJugador = new CBkJugador(key);
		if(pJugador->Open() == 0)
		{
			while(pJugador->Run());
		}
		else
		{
			ShowError();
		}
	}
	ExitHandler(0);
	return 0;
}

void ShowError()
{
	extern int errno;
	ShowMessage(sys_errlist[errno]);
}

void ExitHandler(int sig)
{
	if(pBanca) delete pBanca;
	if(pJugador) delete pJugador;
	
	printf("Exit on signal %i\n", sig);
	exit(0);
}

void ChildExit(int sig)
{
	int st;
	wait(&st);
}

void ShowMessage(const char* msg)
{
#ifndef _DEBUG
	if(admin)
#endif /*_DEBUG*/
	{
		FILE *outto = stderr;

		fputs(msg, outto);
		fputs("\n", outto);
		fflush(outto);
	}
}

