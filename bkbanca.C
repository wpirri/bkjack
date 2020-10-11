/* *****************************************************************************
*	BLACKJACK - BANCA	
*	bkbanca.c
*
***************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "bkjack.h"

//#include "status.h"
#include "bkbanca.h"

extern void ShowMessage(const char* msg);

CBkBanca::CBkBanca(int key)
{
	m_pSincro = NULL;
	m_pMesa = NULL;
	m_key = key;
}

CBkBanca::~CBkBanca()
{
	if(m_pMesa)	m_pMesa->Close();
	if(m_pSincro) delete m_pSincro;
	if(m_pMesa)	delete m_pMesa;
}

int CBkBanca::Open()
{
	// primero creo los semáforos necesarios
	m_pSincro = new CSincro(m_key);
	if(m_pSincro->Create(TOT_SEM) != 0)
	{
		ShowMessage("[CBkBanca::Open] Error al crear semaforos del juego");
		delete m_pSincro;
		m_pSincro = NULL;
		return (-1);
	}
	// limito la cantidad de jugadores que pueden entrar a la vez
	m_pSincro->Set(SEM_INGRESO, MAX_JUGADORES);
	// ahora creo el area de memoria compartida
	m_pMesa = new CMesa(m_pSincro);
	if(m_pMesa->Create() != 0)
	{
		ShowMessage("[CBkBanca::Open] Error al crear memoria compartida");
		delete m_pSincro;
		delete m_pMesa;
		m_pSincro = NULL;
		m_pMesa = NULL;
		return (-1);
	}
	// habilito una entrada a memoria compartida
	m_pSincro->Set(SEM_MESA, 1);
	m_pMesa->Estado(PST_0);
	return 0;
}

int CBkBanca::Run()
{
	int Jugador = 0, Anterior = 0, vuelta = 0;;
	char str[40];
	// espera pasiva de jugadores
	while(m_pMesa->Jugadores() < 2)
	{
		m_pMesa->Estado(PST_ESPERANDO);
		ShowMessage("[CBkBanca::Run] Esperando jugadores...");
		if(m_pMesa->Mensaje("Esperando jugadores") != 0) return 0;
		if(m_pSincro->Wait(SEM_PARTIDA) != 0)
		{
			// error de sincronismo
			return 0;
		}
		ShowMessage("[CBkBanca::Run] Entro jugador");
	}
	sleep(10);
	// si ya había jugadores de antes les pregunto si van a seguir jugando
	// y les borro el tablero
	if(m_pMesa->Restart() != 0) return 0;
	// hago un control mas por si se fué alguno
	if(m_pMesa->Jugadores(1) < 2)
	{
		ShowMessage("[CBkBanca::Run] Ups, no hay nadie jugando, en 20 seg vuelvo...");
		sleep(20);
		return 1;
	}
	if(m_pMesa->Mensaje("Iniciando juego") != 0) return 0;
	// inicio una partida
	if(m_pMesa->Iniciar() != 0)
	{
		// algo habra pasado
		ShowMessage("[CBkBanca::Run] No se pudo iniciar una partida");
		return 0;
	}
	ShowMessage("[CBkBanca::Run] Listo para empezar, mesclando cartas...");
	m_pMesa->Mesclar();
	// partida
	while(1)
	{
		if((Jugador = m_pMesa->Next(Anterior)) < 0)
		{
			ShowMessage("[CBkBanca::Run] Error al obtener secuencia");
			break;
		}
		if(Jugador == 0 && Anterior == 0)
		{
			// se deben haber ido todos
			ShowMessage("[CBkBanca::Run] Se fueron todos los jugadores"
						"\no estan todos plantados");
			break;
		}
		// cuento las manos al pasar por la banca
		if(Jugador == 0)
		{
			vuelta++;
		}
		sprintf(str, "[CBkBanca::Run] Proximo: %i, vuelta: %i", Jugador, vuelta);
		ShowMessage(str);
		if(Jugador == 0 && vuelta == 3)
		{
			// en la tercera vuelta le doy la segunda carta a la banca
			// y controlo los NATURALES
			if(m_pMesa->DarCarta(0) != 0) return 0;
			if(m_pMesa->ControlNatural() != 0) return 0;
		}
		else if(vuelta < 3 || Jugador != 0)
		{
			// la banca solo entra en las primeras vueltas
			ShowMessage("[CBkBanca::Run] Juego");
			if(m_pMesa->Juego(Jugador) != 0) return 0;
			ShowMessage("[CBkBanca::Run] Refresh");
			if(m_pMesa->Refresh() != 0) return 0;
		}
		sleep(1);
		Anterior = Jugador;
	}
	// controlo las manos de los jugadores que quedan
	ShowMessage("[CBkBanca::Run] Fin partida, a pagar y a cobrar...");
	if(m_pMesa->Mensaje("Fin de la partida") != 0) return 0;
	sleep(5);
	if(m_pMesa->Liquidacion() != 0) return 0;
	sleep(5);
	if(m_pMesa->Mensaje("Reiniciando el juego") != 0) return 0;
	// fin de la partida
	ShowMessage("[CBkBanca::Run] Iniciando una nueva partida");
	return 1;
}

