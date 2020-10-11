/* *****************************************************************************
*	BLACKJACK - JUGADOR	
*	bkjugador.c
*
***************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "bkjack.h"

//#include "status.h"

#include "bkjugador.h"

extern void ShowMessage(const char* msg);

CBkJugador::CBkJugador(int key)
{
    m_pSincro = NULL;
    m_pMesa = NULL;
    m_key = key;
}

CBkJugador::~CBkJugador()
{
	if(m_pMesa) m_pMesa->Close();
    if(m_pSincro) delete m_pSincro;
    if(m_pMesa) delete m_pMesa;
}

int CBkJugador::Open()
{
	char str[80];

	// me engancho a los semforos
    m_pSincro = new CSincro(m_key);
    if(m_pSincro->Open(TOT_SEM) != 0)
    {
        printf("[CBkJugador::Open] Error al abrir semaforos del juego"
					", la banca no esta\n");
        //delete m_pSincro;
        //m_pSincro = NULL;
        return (-1);
    }
	// le doy un ratito por si la banca recien se levanta
	sleep(1);
	// trato de abrir la memoria compartida
	m_pMesa = new CMesa(m_pSincro); 
	if(m_pMesa->Open(getenv("LOGNAME"))!= 0)
	{
		//delete m_pMesa;
		//m_pMesa = NULL;
        //delete m_pSincro;
        //m_pSincro = NULL;
		printf("[CBkJugador::Open]"
					" El area de memoria compartida no existe\n");
		return (-1);
	}
	// le pego un toquesito a la banca si esta esperando jugadores
/*	muevo esto a un lugar donde lo pueda sincronizar para que no haga lios
	if(m_pMesa->Estado() == PST_ESPERANDO)
	{
		m_pSincro->Signal(SEM_PARTIDA);
	}
	else
	{
		sprintf(str, "[CBkJugador::Open] Etado del juego %i",
														m_pMesa->Estado());
		ShowMessage(str);
	}
*/
	if(m_pMesa->Toque() != 0) return (-1);
    return 0;
}

int CBkJugador::Run()
{
	while(m_pMesa->Juego() == 0)
	{
		// algo para hacer desde acá entre mano y mano


	}
	// al final de la partida...


	// pregunto si sigue jugando
	return m_pMesa->SeguirJugando();
}

