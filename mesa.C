/* ****************************************************************************
*	mesa
*
*	funciones para el manejode memoria compartida
*
**************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include <sys/shm.h>
#include <sys/ipc.h>

#include "bkjack.h"

//#include "bkbanca.h"
//#include "status.h"

#include "mesa.h"

#define KEY_GBUFFER	0x0000029a

extern void ShowMessage(const char* msg);

CMesa::CMesa(CSincro* sem)
{
	m_pSincro = sem;
	m_gIndex = (-1);
	m_pMesa = NULL;
	m_pDisplay = NULL;
	m_own = (-1);
	m_juegoIniciado = 0;
}

CMesa::~CMesa()
{
//	Close();	
}

// crea un nuevo objeto de memoria compartida
int CMesa::Create()
{
	Close();
	// creo el area de memoria global con acceso al duenio y a los del grupo
	if((m_gIndex = shmget(KEY_GBUFFER, sizeof(MESA),
										IPC_CREAT|IPC_EXCL|0660)) == (-1))
	{
		// no se pudo crear el area de memoria global
		ShowMessage("[CMesa::Create]"
					" No se pudo crear el area de memoria compartida");
		return (-1);
	}
	if((m_pMesa = (MESA*)shmat(m_gIndex, (void*)0, 0)) == (MESA*)(-1))
	{
		// no se pudo acceder a la memoria compartida
		ShowMessage("[CMesa::Create]"
					" No se pudo acceder a la memoria compartida");
		shmctl(m_gIndex, IPC_RMID, NULL);
		return (-1);
	}
	// limpio la memoria
	memset(m_pMesa, 0, sizeof(MESA));
	m_own = 0;	// el dueño es la banca
	return 0;
}
	
// Abre un objeto de memoria compartida existente
int CMesa::Open(const char *nombre)
{
	Close();
	ShowMessage("[CMesa::Open] Control de ingreso");
	// controlo la entrada maxima de jugadores al area de juego
	if(m_pSincro->Wait(SEM_INGRESO) != 0)
	{
		// puede pasar que la banca se valla mientras un jugador
		// esta esperando para entrar
		ShowMessage("[CMesa::Open] Error de sincronizacion");
		return (-1);
	}
    // accedo al bloque de memoria compartida 
    if((m_gIndex = shmget(KEY_GBUFFER, sizeof(MESA), 0660)) == (-1))
    {
        // no se pudo acceder al area de memoria global
		ShowMessage("[CMesa::Open] No se pudo tomar la memoria compartida");
        return (-1);
    }
    if((m_pMesa = (MESA*)shmat(m_gIndex, (void*)0, 0)) == (MESA*)(-1))
    {
        // no se pudo acceder a la memoria compartida
		ShowMessage("[CMesa::Open] No se pudo acceder a la memoria compartida");
        return (-1);
    }
	ShowMessage("[CMesa::Open] Registracion de jugador");
	// busco una silla
	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	for(m_own = 1; m_own <= MAX_JUGADORES; m_own++)
	{
		if(m_pMesa->jugador[m_own].estado == JST_VACIO)
		{
			break;
		}
	}
	if(m_own > MAX_JUGADORES)
	{
		// no había lugar (esto no debería pasar,
		// si no había lugar se debería haber trabado en el semaforo anterior,
		// pero por las dudas...)
		ShowMessage("[CMesa::Open] Error al buscar lugar de jugador"
					" (esto no deberia pasar)");
		m_own = (-1);
		m_gIndex = (-1);
		m_pSincro->Signal(SEM_MESA);
		return (-1);
	}
	ShowMessage("[CMesa::Open] Esperando un juego");
	m_pMesa->jugador[m_own].estado = JST_ESPERANDO;
	strncpy(m_pMesa->jugador[m_own].nic, nombre,
										sizeof(m_pMesa->jugador[m_own].nic)-1);
	m_pSincro->Signal(SEM_MESA);
	// creo la ventana del juego
    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	m_pDisplay = new CBkDisplay(m_own, m_pMesa);
    m_pSincro->Signal(SEM_MESA);
	// controlo la resolucion de la terminal
	if(!m_pDisplay->Check()) return (-1);
	return 0;
}

void CMesa::Close()
{
	if(m_own == 0)
	{
		// si soy la banca libero la memoria compartida
		shmctl(m_gIndex, IPC_RMID, NULL);
	}
	else if(m_own > 0)
	{
		// si soy un jugador libero una silla
		if(m_pSincro->Wait(SEM_MESA) == 0)
		{
	        m_pMesa->jugador[m_own].estado = JST_VACIO;
			m_pSincro->Signal(SEM_MESA);
		}
		// y libero una vacante
		m_pSincro->Signal(SEM_INGRESO);
	}
	if(m_pMesa)
	{
		shmdt(m_pMesa);
		m_pMesa = NULL;
	}
	m_gIndex = (-1);
	m_own = (-1);
	if(m_pDisplay)
	{
		delete m_pDisplay;
		m_pDisplay = NULL;
	}
}

int CMesa::Jugadores(int jugando = 0)
{
    // cuenta la cantidad de jugadores jugando (valga la redundancia)
    int i;
    int jugadores = 0;
    if(!m_pMesa) return (-1);
    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
    for(i = 1; i <= MAX_JUGADORES; i++)
    {
		if(jugando)
		{
        	if((m_pMesa->jugador[i].estado >= JST_ESPERANDO) && (m_pMesa->jugador[i].estado < JST_ESPECTADOR))
			{
				jugadores++;
			}
		}
		else
		{
        	if(m_pMesa->jugador[i].estado >= JST_ESPERANDO) jugadores++;
		}
    }
    m_pSincro->Signal(SEM_MESA);
    return jugadores;
}

int CMesa::Next(int player)
{
	int next = (-1);
	int i;
    if(!m_pMesa) return (-1);
    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	for(i = 0; i <= MAX_JUGADORES; i++)
	{
		if((i+player+1) > MAX_JUGADORES)
		{
			next = (i+player-MAX_JUGADORES);
			if((m_pMesa->jugador[next].estado == JST_JUGANDO) || !next)
			{
				break;
			}
			next = (-1);
		}
		else
		{
			next = (i+player+1);
			if(m_pMesa->jugador[next].estado == JST_JUGANDO)
			{
				break;
			}
			next = (-1);
		}
	}
    m_pSincro->Signal(SEM_MESA);
	return next;
}

int CMesa::Juego(int player = 0)
{
	int cartas;
	int puntos;
	char str[40];

	// para la banca
	if(m_own == 0)
	{
		// para la banca es pasarle el turno a un jugador
		if(player < 0)	return (-1);
		// veo que hago con este tipo
		ShowMessage(".");
		if((cartas = Cartas(player)) <  0)
		{
			return (-1);
		}
		else if(cartas == 0)
		{
			// primera carta
			sprintf(str, "Primera carta a %i", player);
			ShowMessage(str);
			if(DarCarta(player) != 0) return (-1);
			if(Refresh(player) != 0) return (-1);
		}
		else if(cartas < 2 && player != 0)
        {
			if(Apostado(player) == 0)
			{
				// para todos menos la banca
				sprintf(str, "Pidiendo apuesta a %i", player);
				ShowMessage(str);
				if(Apuesta(player) > 0) // en esta parte tiene que apostar si o si
				{
					if(Refresh(player) != 0) return (-1);
				}
				else
				{
					sprintf(str, "Jugador %i no aposto", player);
					ShowMessage(str);
					if(Estado(player, JST_PERDIO) != 0) return (-1);
				}
			}
			else
			{
				sprintf(str, "Segunda carta a %i", player);
				ShowMessage(str);
				if(DarCarta(player) != 0) return (-1);
				if(Refresh(player) != 0) return (-1);
			}
		}
		else if(cartas < 8 && player != 0)
		{
			sprintf(str, "Carta para %i", player);
			ShowMessage(str);
			// le pregunto si quiere carta
			if((puntos = SumPuntos(player)) < 21)
			{
				sprintf(str, "Jugador %i con %i puntos", player, puntos);
				ShowMessage(str);
				if(Carta(player) > 0)
				{
					// si quiere le doy
					if(DarCarta(player) != 0) return (-1);
					if(Refresh(player) != 0) return (-1);
					// controlo si se pasó
					if((puntos = SumPuntos(player)) > 21)
					{
						sprintf(str, "Jugador %i se paso", player);
						ShowMessage(str);
						Cobrar(player, 0);
					}
					else if(puntos == 21)
					{
						sprintf(str, "Jugador %i servido", player);
						ShowMessage(str);
						if(Estado(player, JST_PLANTADO) != 0) return (-1);
						SendMessage(player, "Servido");
					}
				}
				else
				{
					sprintf(str, "Jugador %i plantado", player);
					ShowMessage(str);
					// si no quiere carta -> se planta
					if(Estado(player, JST_PLANTADO) != 0) return (-1);
				}
			}
		}
		else if(player != 0)
		{
			if(Refresh(player) != 0) return (-1);
		}
	}
	// para los jugadores
	else if(m_own > 0)
	{
		// para un jugador es tomar su turno
		if(m_pSincro->Wait(m_own) != 0)
		{
			return (-1);
		}
		if(Turno() != 0) return (-1);
		// cuando vuelvo libero a la banca
		if(m_pSincro->Signal(SEM_PARTIDA) != 0)
		{
			return (-1);
		}
	}
	else
	{
		return (-1);
	}
	return 0;
}

int CMesa::Iniciar()
{
	int i;
	char str[80];

	if(m_own == 0)
	{
		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		// inicializo los jugadores
		for(i = 1; i <= MAX_JUGADORES; i++)
		{
			if(m_pMesa->jugador[i].estado == JST_ESPERANDO)
			{
				// jugador nuevo, le doy fichas
				m_pMesa->jugador[i].estado = JST_JUGANDO;
				m_pMesa->jugador[i].monedero = INIT_FICHAS;
				m_pMesa->jugador[i].apostado = 0;
			}
		}
		if(m_pSincro->Signal(SEM_MESA) != 0) return (-1);
		return 0;
	}
	else
	{
		// solo para la banca
		return (-1);
	}
}

/*
int CMesa::Estado()	// devuelve estdo de la mesa
{
	int st;
    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	st = m_pMesa->banca.estado;
    m_pSincro->Signal(SEM_MESA);
	return st;
}
*/

int CMesa::Estado(int st)	// setea estado de la mesa
{
    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	m_pMesa->banca.estado = st;
    m_pSincro->Signal(SEM_MESA);
	return 0;
}

int CMesa::Estado(int jugador, int st)	// setea estado del jugador
{
	if(jugador <= 0) return (-1);
    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	m_pMesa->jugador[jugador].estado = st;
    m_pSincro->Signal(SEM_MESA);
	return 0;
}

int CMesa::Mesclar()
{
	time_t t;
	int carta;
	int palo;
	int i;
	char str[80];

    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	// inicializo la secuencia random
	t = time(&t);
	srand((int)t);
	// genero el mazo mesclado
	memset(&m_pMesa->mazo, 0, sizeof(MAZO));
	for(i = 0; i < CARTAS_MAZO; i++)
	{
		do
		{
			carta = RndCarta();
			palo = RndPalo();
		} while(EnMazo(carta, palo));
		m_pMesa->mazo.carta[i].numero = carta;
		m_pMesa->mazo.carta[i].palo = palo;
	}

	// SACAR
	//m_pMesa->mazo.carta[0].numero = 1;
	//m_pMesa->mazo.carta[0].palo = 1;
	//m_pMesa->mazo.carta[1].numero = 1;
	//m_pMesa->mazo.carta[1].palo = 2;
	//m_pMesa->mazo.carta[2].numero = 1;
	//m_pMesa->mazo.carta[2].palo = 2;
	//m_pMesa->mazo.carta[3].numero = 10;
	//m_pMesa->mazo.carta[3].palo = 3;
	//m_pMesa->mazo.carta[4].numero = 10;
	//m_pMesa->mazo.carta[4].palo = 4;
	//m_pMesa->mazo.carta[5].numero = 10;
	//m_pMesa->mazo.carta[5].palo = 4;

	for(i = 0; i < CARTAS_MAZO; i++)
	{
		sprintf(str, "Mazo      [%2i]-[%i]", m_pMesa->mazo.carta[i].numero,
											m_pMesa->mazo.carta[i].palo);
		ShowMessage(str);
	}
	ShowMessage("----------------------");
    m_pSincro->Signal(SEM_MESA);
	return 0;
}

int CMesa::Carta(int jugador)
{
	// ¿otra carta?
	int resp = 0;
	char nombre[40];
	char str[40];

	// pide la apuesta y devuelve el total apostado acumulado
	if(jugador <= 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	strcpy(nombre, m_pMesa->jugador[jugador].nic);
   	m_pSincro->Signal(SEM_MESA);
	sprintf(str, "¿Carta? a %s (%i)", nombre, jugador);
	ShowMessage(str);
	if(Mensaje(str) != 0) return (-1);
	// interrogo al jugador
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	if(m_pMesa->jugador[jugador].estado == JST_JUGANDO)
	{
		m_pMesa->jugador[jugador].cmd = ACC_CONSULTA;
		strcpy(m_pMesa->jugador[jugador].msg, "¿Desea otra carta y/n?");
   		m_pSincro->Signal(SEM_MESA);
   		m_pSincro->Signal(jugador);
		// ahora espero a que conteste
   		if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
		// controlo la respuesta
   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		resp = m_pMesa->jugador[jugador].dat;
   		m_pSincro->Signal(SEM_MESA);
		return resp;
	}
	else
	{
   		m_pSincro->Signal(SEM_MESA);
		return (-1);
	}
}

int CMesa::RndCarta()
{
	// una carta al azar
	return ((rand() % 12) + 1);
}

int CMesa::RndPalo()
{
	// un palo al azar
	return ((rand() % 4) + 1);
}

int CMesa::EnMazo(int carta, int palo)
{
	int i;
	//char str[80];

	for(i = 0; i < CARTAS_MAZO; i++)
	{
		if(m_pMesa->mazo.carta[i].numero == carta &&
			m_pMesa->mazo.carta[i].palo == palo)
		{
			//sprintf(str, "Carta     <%2i>-<%i> encontrada en poc. %i",
			//				carta, palo, i);
			//ShowMessage(str);
			return 1;
		}
	}
	return 0;
}

int CMesa::Refresh(int player = 0)
{
	int i;

	if(player)
	{
    	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		if(m_pMesa->jugador[player].estado >= JST_JUGANDO)
		{
			m_pMesa->jugador[player].cmd = ACC_REFRESH;
    		m_pSincro->Signal(SEM_MESA);
    		m_pSincro->Signal(player);
    		if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
		}
		else
		{
    		m_pSincro->Signal(SEM_MESA);
		}
	}
	else
	{
		for(i = 1; i <= MAX_JUGADORES; i++)
		{
	    	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
			if(m_pMesa->jugador[i].estado >= JST_JUGANDO)
			{
				m_pMesa->jugador[i].cmd = ACC_REFRESH;
	    		m_pSincro->Signal(SEM_MESA);
	    		m_pSincro->Signal(i);
	    		if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
			}
			else
			{
	    		m_pSincro->Signal(SEM_MESA);
			}
		}
	}
	return 0;
}

int CMesa::Turno()
{
	int cmd;
	char str[80];
	int dat;

    if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	cmd = m_pMesa->jugador[m_own].cmd;
	m_pMesa->jugador[m_own].cmd = 0;
    m_pSincro->Signal(SEM_MESA);

	switch(cmd)
	{
	case ACC_REFRESH:
		ShowMessage("Refresh");
		m_pDisplay->Update();
		break;
	case ACC_APOSTAR:
		GetApuesta();
		break;
	case ACC_MENSAJE:
    	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		strcpy(str, m_pMesa->jugador[m_own].msg);
    	m_pSincro->Signal(SEM_MESA);
		m_pDisplay->MessageBox("Black-Jack", str, 2);
		break;
	case ACC_CONSULTA:
    	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		strcpy(str, m_pMesa->jugador[m_own].msg);
		m_pMesa->jugador[m_own].dat = m_pDisplay->QuestionBox("Black-Jack", str);
    	m_pSincro->Signal(SEM_MESA);
		break;
	case ACC_RESTART:
    	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		m_pDisplay->Restart();
    	m_pSincro->Signal(SEM_MESA);
		break;
	case ACC_DESTAPAR:
    	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		dat = m_pMesa->jugador[m_own].dat;
    	m_pSincro->Signal(SEM_MESA);
		m_pDisplay->Mostrar(dat);
		break;
	default:
		break;
	}
	return 0;
}

int CMesa::Cartas(int jugador)
{
	int cartas = 0;
	int i, j;
	int desde, hasta;

	if(jugador < 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	desde = m_pMesa->mazo.desde;
	hasta = m_pMesa->mazo.hasta;

	for(i = 0; i < CARTAS_MAZO; i++)
	{
		if((i + desde) > CARTAS_MAZO)
		{
			j = i + desde - CARTAS_MAZO;
		}
		else
		{
			j = i + desde;
		}
		if(j == hasta) break;
		if(m_pMesa->mazo.carta[j].poseedor == jugador)
		{
			cartas++;
		}
	}
   	m_pSincro->Signal(SEM_MESA);
	return cartas;
}

int CMesa::DarCarta(int jugador)
{
	char str[40];

   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	if(m_pMesa->mazo.hasta > CARTAS_MAZO)
	{
		if(m_pMesa->mazo.desde == 0)
		{
			// se termino el mazo
			ShowMessage("Se termino el mazo");
			return (-1);
		}
		m_pMesa->mazo.hasta = 0;
	}
	else
	{
		if((m_pMesa->mazo.hasta + 1) == m_pMesa->mazo.desde)
		{
			// se termino el mazo
			ShowMessage("Se termino el mazo");
			return (-1);
		}
	}
	m_pMesa->mazo.carta[m_pMesa->mazo.hasta].poseedor = jugador;
	sprintf(str, 	"Carta <%2i>-<%i> a jugador %i",
					m_pMesa->mazo.carta[m_pMesa->mazo.hasta].numero,
					m_pMesa->mazo.carta[m_pMesa->mazo.hasta].palo,
					jugador);
	ShowMessage(str);
	m_pMesa->mazo.hasta++;
   	m_pSincro->Signal(SEM_MESA);
	return 0;
}

int CMesa::SeguirJugando()
{
	m_pDisplay->MessageBox("Black-Jack", "Final de la partida", 5);
	return 0;
}

int CMesa::Apuesta(int jugador)
{
	int apostado = 0;
	char nombre[40];
	char str[40];

	// pide la apuesta y devuelve el total apostado acumulado
	if(jugador <= 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	strcpy(nombre, m_pMesa->jugador[jugador].nic);
   	m_pSincro->Signal(SEM_MESA);
	sprintf(str, "¿Apuesta? %s (%i)", nombre, jugador);
	ShowMessage(str);
	if(Mensaje(str) != 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	if(m_pMesa->jugador[jugador].estado == JST_JUGANDO)
	{
		m_pMesa->jugador[jugador].cmd = ACC_APOSTAR;
   		m_pSincro->Signal(SEM_MESA);
   		m_pSincro->Signal(jugador);
		// ahora espero a que apueste
   		if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
		// controlo la validez de la apuesta
   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		apostado = m_pMesa->jugador[jugador].apostado;
   		m_pSincro->Signal(SEM_MESA);
		return apostado;
	}
	else
	{
   		m_pSincro->Signal(SEM_MESA);
		return (-1);
	}
}

void CMesa::GetApuesta()
{
	int apostado;
	char str[40];

   	if(m_pSincro->Wait(SEM_MESA) != 0) return;
	apostado = m_pDisplay->QuestionBox("Apuesta", "Indique el monto de su apuesta");
	m_pSincro->Signal(SEM_MESA);
	if((apostado + Apostado(m_own)) == 0)
	{
		m_pDisplay->MessageBox("Black-Jack", "Perdio el juego", 5);
		return;
	}
	if(apostado)
	{
		sprintf(str, "Apostado %i", apostado);
		ShowMessage(str);
		Apuesta(m_own, apostado);
		// una vez hecha la apuesta le descubro el tablero
		m_pDisplay->Mostrar();
	}
}

int CMesa::Apostado(int jugador)
{
	int apostado;
	char str[80];

	if(jugador <= 0) return 0;
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	apostado = m_pMesa->jugador[jugador].apostado;
	m_pSincro->Signal(SEM_MESA);
	sprintf(str, "Apuesta cumulada jugador %i = %i ", jugador, apostado);
	ShowMessage(str);
	return apostado;
}

int CMesa::Apuesta(int jugador, int apostado)
{
	if(jugador <= 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	m_pMesa->jugador[jugador].apostado += apostado;
	m_pMesa->jugador[jugador].monedero -= apostado;
	m_pSincro->Signal(SEM_MESA);
	return 0;
}

int CMesa::Restart()
{
	int i;
	int stat, resp;
	char nombre[40];
	char str[40];

	for(i = 1; i <= MAX_JUGADORES; i++)
	{
   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		stat = m_pMesa->jugador[i].estado;
		strcpy(nombre, m_pMesa->jugador[i].nic);
		m_pSincro->Signal(SEM_MESA);
		if(stat > JST_JUGANDO)
		{
			sprintf(str, "? %s", nombre);
			if(Mensaje(str) != 0) return(-1);
	   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
			m_pMesa->jugador[i].cmd = ACC_RESTART;
	   		m_pSincro->Signal(SEM_MESA);
	   		m_pSincro->Signal(i);
			// ahora espero a que vuelva
	   		if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
			// le doy la opción de salir antes de que empiece la partida
			m_pMesa->jugador[i].cmd = ACC_CONSULTA;
			strcpy(m_pMesa->jugador[i].msg, "¿Desea participar de otra partida y/n?");
			sprintf(str, "Participa? %i", i);
			ShowMessage(str);
	   		m_pSincro->Signal(SEM_MESA);
	   		m_pSincro->Signal(i);
			// ahora espero a que conteste
	   		if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
			// controlo la respuesta
	   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
			resp = m_pMesa->jugador[i].dat;
	   		m_pSincro->Signal(SEM_MESA);
			if(resp)
			{
				// entra al nuevo juego
				m_pMesa->jugador[i].estado = JST_JUGANDO;
				m_pMesa->jugador[i].apostado = 0;
			}
			else
			{
				// lo pongo como espectador, primero tuvo que haber jugado una mano
				m_pMesa->jugador[i].estado = JST_ESPECTADOR;
				m_pMesa->jugador[i].apostado = 0;
			}
		}
	}
	return 0;
}

int CMesa::Natural(int jugador)
{
	int i, j, desde, hasta;
	int carta1 = 0, carta2 = 0;

	if(Cartas(jugador) != 2) return 0; // solo para cuando tiene dos cartas

   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	desde = m_pMesa->mazo.desde;
	hasta = m_pMesa->mazo.hasta;
	for(i = 0; i < CARTAS_MAZO; i++)
	{
		if((i + desde) > CARTAS_MAZO)
		{
			j = i + desde - CARTAS_MAZO;
		}
		else
		{
			j = i + desde;
		}
		if(j == hasta) break;
		if(m_pMesa->mazo.carta[j].poseedor == jugador)
		{
			if(!carta1)
			{
				carta1 = m_pMesa->mazo.carta[j].numero;
			}
			else
			{
				carta2 = m_pMesa->mazo.carta[j].numero;
			}
		}
	}
   	m_pSincro->Signal(SEM_MESA);
	if((ValCarta(carta1) == 1) && (ValCarta(carta2) == 10))
	{
		return 1;
	}
	else if((ValCarta(carta1) == 10) && (ValCarta(carta2) == 1))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int CMesa::ControlNatural()
{
	int i;
	int pstat;

	for(i = 1; i <= MAX_JUGADORES; i++)
	{
   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		pstat = m_pMesa->jugador[i].estado;
   		m_pSincro->Signal(SEM_MESA);
		if(pstat == JST_JUGANDO)
		{
			if(Natural(0))
			{
				ShowMessage("La banca tiene NATURAL");
				if(Mensaje("La banca tiene Natural") != 0) return (-1);
				// si la banca tiene natural
				if(Natural(i))
				{
					// a los que tienen natural les cobro solo lo que apostaron
					if(Cobrar(i, 0) != 0) return (-1);
				}
				else
				{
					// los que no tienen natural -> chau... les cobro el doble
					if(Cobrar(i, 1) != 0) return (-1);
				}
			}
			else
			{
				if(Natural(i))
				{
					// a los que tienen natural les pago el doble
					if(Pagar(i, 1) != 0) return (-1);
				}
				// con el resto sigo jugando
			}
		}
	}
	return 0;
}

int CMesa::ValCarta(int numero)
{
	if(numero > 10)
	{
		return 10;
	}
	else
	{
		return numero;
	}
}

int CMesa::SumPuntos(int jugador)
{
	int i, j;
	int puntos = 0;
	int desde, hasta;

   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	desde = m_pMesa->mazo.desde;
	hasta = m_pMesa->mazo.hasta;
	for(i = 0; i < CARTAS_MAZO; i++)
	{
		if((i + desde) > CARTAS_MAZO)
		{
			j = i + desde - CARTAS_MAZO;
		}
		else
		{
			j = i + desde;
		}
		if(j == hasta) break;
		if(m_pMesa->mazo.carta[j].poseedor == jugador)
		{
			puntos += ValCarta(m_pMesa->mazo.carta[j].numero);
		}
	}
   	m_pSincro->Signal(SEM_MESA);
	return puntos;
}

int CMesa::Cobrar(int jugador, int doble)
{
	char str[80];
	int cobrar;
	char nombre[40];

	sprintf(str, "CMesa::Cobrar(%i,%i)", jugador, doble);
	ShowMessage(str);
	if(jugador <= 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	cobrar = m_pMesa->jugador[jugador].apostado;
	// le saco lo apostado
	m_pMesa->jugador[jugador].apostado = 0;
	m_pMesa->banca.pozo += cobrar;
	if(doble)
	{
		// si debo cobrar doble tambien le saco del monedero
		m_pMesa->jugador[jugador].monedero -= cobrar;
		m_pMesa->banca.pozo += cobrar;
	}
	m_pMesa->jugador[jugador].estado = JST_PERDIO;
	strcpy(nombre, m_pMesa->jugador[jugador].nic);
   	m_pSincro->Signal(SEM_MESA);
	sprintf(str, "Cobrado %i a jugador %i", cobrar, jugador);
	ShowMessage(str);
	sprintf(str, "Cobrando a %s", nombre);
	if(Mensaje(str) != 0) return (-1);
	sleep(1);
	// le doy la noticia
	SendMessage(jugador, "Usted Perdio");
	return 0;
}

int CMesa::Pagar(int jugador, int doble)
{
	char str[80];
	int pagar;
	char nombre[40];

	sprintf(str, "CMesa::Pagar(%i,%i)", jugador, doble);
	ShowMessage(str);
	if(jugador <= 0) return (-1);
   	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	pagar = m_pMesa->jugador[jugador].apostado;
	// le devuelvo lo apostado
	m_pMesa->jugador[jugador].apostado = 0;
	m_pMesa->jugador[jugador].monedero += pagar;
	// saco de la banca el resto que le corresponda
	if(doble)
	{
		pagar *= 2;
	}
	m_pMesa->banca.pozo -= pagar;
	m_pMesa->jugador[jugador].monedero += pagar;
	m_pMesa->jugador[jugador].estado = JST_GANADO;
	strcpy(nombre, m_pMesa->jugador[jugador].nic);
   	m_pSincro->Signal(SEM_MESA);
	sprintf(str, "Pagado %i a jugador %i",pagar, jugador);
	ShowMessage(str);
	sprintf(str, "Pagando a %s", nombre);
	if(Mensaje(str) != 0) return (-1);
	sleep(1);
	// le doy la noticia
	SendMessage(jugador, "Usted Gano");
	return 0;
}

int CMesa::SendMessage(int jugador, const char* msg)
{
	if(jugador < 0) return (-1);
	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	m_pMesa->jugador[jugador].cmd = ACC_MENSAJE;
	strcpy(m_pMesa->jugador[jugador].msg, msg);
	m_pSincro->Signal(SEM_MESA);
	m_pSincro->Signal(jugador);
	// ahora espero a que vuelva
   	if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
	return 0;
}

int CMesa::Liquidacion()
{
    int i;
    int pstat;
	int pts = 0;
	int max_ptsplayers;
	char str[40];

	// termino el juego de la banca
	if(Mensaje("Juego de la BANCA")!= 0) return (-1);
	ShowMessage("A ver los juegos");
    for(i = 1; i <= MAX_JUGADORES; i++)
    {
		// mientras les reviso las cartas les hago ver la tapada de la banca
        if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
        pstat = m_pMesa->jugador[i].estado;
		if(pstat >= JST_PLANTADO)
		{
			m_pMesa->jugador[i].cmd = ACC_DESTAPAR;
			m_pMesa->jugador[i].dat = 1;
		}
        m_pSincro->Signal(SEM_MESA);
		// si le mande algo espero que vuelva
		if(pstat >= JST_PLANTADO)
		{
			m_pSincro->Signal(i);
			if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
		}
        if(pstat == JST_PLANTADO)
        {
            if((pts = SumPuntos(i)) > max_ptsplayers)
			{
				max_ptsplayers = pts;
			}
		}
	}
	ShowMessage("Resto de cartas de la banca");
	pts = SumPuntos(0);
	while(pts < max_ptsplayers && pts < 18)
	{
		DarCarta(0);
		Refresh();
		sleep(1);
		pts = SumPuntos(0);
	}
	sleep(1); // para darle un poco de suspenso
	if(pts > 21)
	{
		pts = 0;
		sprintf(str, "La banca se paso");
	}
	else
	{
		sprintf(str, "La banca plantada en %i", pts);
	}
	ShowMessage(str);
	if(Mensaje(str)!= 0) return (-1);
	sleep(5);
	ShowMessage("Revisando jugadores");
	// reviso a los jugadores que quedan
    for(i = 1; i <= MAX_JUGADORES; i++)
    {
        if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
        pstat = m_pMesa->jugador[i].estado;
        m_pSincro->Signal(SEM_MESA);
        if(pstat == JST_PLANTADO)
        {
			if(SumPuntos(i) > pts)
			{
				// si tiene mas que la banca gana porque si esta
				// plantado no puede tener mas de 21
				Pagar(i, 0);
			}
			else
			{
				Cobrar(i, 0);
			}
        }
    }
	// limpio el mazo y la mesa
   if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	memset(&m_pMesa->mazo, 0, sizeof(MAZO));
	m_pSincro->Signal(SEM_MESA);
    for(i = 1; i <= MAX_JUGADORES; i++)
	{
   		if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
		if(m_pMesa->jugador[i].estado > JST_JUGANDO)
		{
			m_pMesa->jugador[i].cmd = ACC_RESTART;
			m_pSincro->Signal(SEM_MESA);
			m_pSincro->Signal(i);
			// ahora espero a que vuelva
			if(m_pSincro->Wait(SEM_PARTIDA) != 0) return (-1);
		}
		else
		{
			m_pSincro->Signal(SEM_MESA);
		}
	}
	if(Refresh() != 0) return (-1);
    return 0;
}

int CMesa::Mensaje(const char *msg)
{
   if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	sprintf(m_pMesa->banca.msg, "%24.24s", msg);
	m_pSincro->Signal(SEM_MESA);
	if(Refresh() != 0) return (-1);
	return 0;
}

int CMesa::Toque()
{
	if(m_pSincro->Wait(SEM_MESA) != 0) return (-1);
	if(m_pMesa->banca.estado == PST_ESPERANDO)
	{
		m_pMesa->banca.estado = PST_INICIANDO;
		m_pSincro->Signal(SEM_PARTIDA);
	}
	m_pSincro->Signal(SEM_MESA);
	return 0;
}


