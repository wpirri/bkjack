/* mesa.h */
#ifndef _MESA_H_
#define _MESA_H_

// cantidad de fichas que se le dan al jugador cuando entra
#define INIT_FICHAS     500
// para controlar la cantidad de jugadores que entran a la partida
#define SEM_INGRESO     0
// los semaforos de los jugadores tienen el mismo numero que el jugador
// jugadores = JUGADOR + SEM_INGRESO
// para el acceso a la memoria global
#define SEM_MESA        SEM_INGRESO + MAX_JUGADORES + 1		// 7
// semaforo de la banca
#define SEM_PARTIDA     SEM_MESA + 1						// 8
// solamente para saber cuantos semaforos crear
#define TOT_SEM         SEM_MESA + 2						// 9

class CMesa
{
public:
    CMesa(CSincro *sem);
    virtual ~CMesa();

    int Create();
    int Open(const char *nombre);

	int Iniciar();
	int Jugadores(int jugando = 0);
	int Next(int player);
	int Juego(int player = 0);

	int Estado(int st);
	int Estado(int jugador, int st);
	int Mesclar();
	int Carta(int jugador);
	int Refresh(int player = 0);
	int SeguirJugando();
	int Apostado(int jugador);
	int Restart();
	int SendMessage(int jugador, const char *msg);
	int DarCarta(int jugador);
	int ControlNatural();
	int Liquidacion();
	int Mensaje(const char *msg);
	int Toque();

    void Close();    

protected:
    int m_gIndex;
	int m_own;
	int m_juegoIniciado;
    MESA *m_pMesa;
	CSincro *m_pSincro;
	CBkDisplay *m_pDisplay;
	//int Estado();	// devielve el estado de la banca
	int RndCarta();
	int RndPalo();
	int EnMazo(int carta, int palo);
	int Turno();
	int Cartas(int jugador);			// cuantas tiene
	int Apuesta(int jugador);
	int Apuesta(int jugador, int apuesta);
	void GetApuesta();
	int Natural(int jugador);
	int ValCarta(int numero);
	int SumPuntos(int jugador);
	int Cobrar(int jugador, int doble);
	int Pagar(int jugador, int doble);

};
#endif /*_MESA_H_*/
