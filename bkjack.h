#define MAX_JUGADORES   6
#define CARTAS_MAZO 48
#define MAX_CARTAS 8

    /* defino los strings estaticos para simplificar el la asignacion de memoria compartida */
    typedef struct _CARTA
    {
        int     palo;
        int     numero;
//        int     valor;
        int     poseedor;
//        int     estado;         // 0= invisible, 1= Visible
    } CARTA;

    typedef struct _MAZO
    {
        int     desde;
        int     hasta;
        CARTA   carta[CARTAS_MAZO];
    } MAZO;

    typedef struct _JUGADOR
    {
        int     estado;         // 0= Sin jugador, 1= Esperando, 2= Jugando, 3= Plantado, 4= Perdio - JST_xxxxx
        char    nic[40];
        long    monedero;
        long    apostado;
        int     cmd;
        char    msg[80];
        int     dat;
    } JUGADOR;

    typedef struct _BANCA
    {
        char        estado;         //
//        long        mesa;           // monto total apostado
		long		pozo;			// para ver como va la banca $$$
		char		msg[80];
    } BANCA;

    typedef struct _MESA
    {
        BANCA       banca;
        MAZO        mazo;
        JUGADOR     jugador[MAX_JUGADORES+1];
    } MESA;

#include "status.h"
#include "sincro.h"
#include "carta.h"
#include "display.h"
#include "mesa.h"

