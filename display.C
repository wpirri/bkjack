/* *****************************************************************************
*	display.C
*
***************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "bkjack.h"

//#include "status.h"
#include "display.h"

extern void ShowMessage(const char* msg);

CBkDisplay::CBkDisplay(int own, MESA *pMesa)
{
	int i;

	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();

	m_own = own;
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_RED);
    init_pair(4, COLOR_WHITE, COLOR_BLUE);
    init_pair(5, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);

	m_pWindow = newwin(LINES,COLS,0,0);
    wbkgd(m_pWindow, ' '|COLOR_PAIR(1)|A_BOLD);
    leaveok(m_pWindow, TRUE);
	box(m_pWindow, 0, 0);
	for(i = 1; i < (COLS - 2); i++)
	{
		mvwaddch(m_pWindow, LINES - 3, i, ACS_HLINE|COLOR_PAIR(3)|A_BOLD);
		mvwaddch(m_pWindow, LINES - 14, i, ACS_HLINE|COLOR_PAIR(6)|A_BOLD);
	}
	for(i = 2; i < 9; i++)
	{
		mvwaddch(m_pWindow, i, ColJugador(0) - 1, ACS_VLINE|COLOR_PAIR(5)|A_BOLD);
	}
	mvwprintw(m_pWindow, 1, ColJugador(0) - 9, "Banca ->");

	wrefresh(m_pWindow);
    keypad(m_pWindow, TRUE);
    leaveok(m_pWindow, TRUE);
	m_pMesa = pMesa;
	memset(m_pCartas, 0, (sizeof(CCarta*) * MAX_JUGADORES * MAX_CARTAS));
}

CBkDisplay::~CBkDisplay()
{
	delwin(m_pWindow);
	endwin();
}

int CBkDisplay::MessageBox(const char *title, const char *msg, int wait)
{
	WINDOW *wnd;

	int lines = 7;
	int	columns = MAX(strlen(msg), 27) + 4;

	int top = 1/*(LINES / 2) - (lines / 2)*/;
	int left = 1/*(COLS / 2) - (columns / 2)*/;

	wnd = newwin(lines, columns, top, left);
	leaveok(wnd, TRUE);
	wbkgd(wnd, COLOR_PAIR(4));
	mvwprintw(wnd, 1, 1, title);
	mvwprintw(wnd, 3, (columns - strlen(msg)) / 2, msg);
    box(wnd, 0, 0);
    wrefresh(wnd);
	if(wait)
	{
		sleep(wait);
	}
	else
	{
    	wgetch(wnd);
	}
    delwin(wnd);
	refresh();
	Update();
	return 0;
}

int CBkDisplay::QuestionBox(const char *title, const char *question)
{
	WINDOW *wnd;
	int ch;
	int result = 0;
	char str[10];
	char buffer[10];

	int lines = 7;
	int	columns = MAX(strlen(question), 27) + 4;

	int top = 2/*(LINES / 2) - (lines / 2)*/;
	int left = (COLS / 2) - (columns / 2);

	wnd = newwin(lines, columns, top, left);
	keypad(wnd, TRUE);
	leaveok(wnd, TRUE);
	wbkgd(wnd, COLOR_PAIR(4));
	mvwprintw(wnd, 1, 1, title);
	mvwprintw(wnd, 3, (columns - strlen(question)) / 2, question);
	mvwprintw(wnd, 5, columns/3, "?:");
    box(wnd, 0, 0);
    wrefresh(wnd);

	strcpy(buffer, "");
	while((ch = wgetch(wnd)) != ERR)
	{
		if(ch == 'y' || ch == 'Y' || ch == 's' || ch == 'S')
		{
			result = 1;
			break;
		}
		else if(ch == 'n' || ch == 'N' || ch == 0x1b)
		{
			result = 0;
			break;
		}
		else if(ch >= '0' && ch <= '9')
		{
			str[0] = ch;
			str[1] = 0;
			strcat(buffer, str);
			mvwprintw(wnd, 5, columns/2, buffer);
		}
		else if(ch == '\r') 
		{
			if(strlen(buffer))
			{
				if(strlen(buffer) < 5)
				{
					result = atoi(buffer);
					break;
				}
				else
				{
					strcpy(buffer, "");
					mvwprintw(wnd, 5, columns/2, "          ");
				}
			}
		}
		else if(ch == 0x107 || ch == 0x14a || ch == 0x17f)	// BkSpace, Del, '.'
		{
			strcpy(buffer, "");
			mvwprintw(wnd, 5, columns/2, "          ");
		}
	}
    delwin(wnd);
	refresh();
	Update();
	return result;
}

void CBkDisplay::Update()
{
    wclear(stdscr);
    touchwin(stdscr);
    touchwin(m_pWindow);
	wnoutrefresh(m_pWindow);
	doupdate();
	Cartas();
	Info();
    touchwin(m_pWindow);
	wnoutrefresh(m_pWindow);
	doupdate();
}

void CBkDisplay::Cartas()
{
	int i/*,j*/;
	int carta;
	int jugador;
	char str[80];
	int linea;
	int desde, hasta;

	memset(m_CountCartas, 0, sizeof(m_CountCartas));
	desde = m_pMesa->mazo.desde;
	hasta = m_pMesa->mazo.hasta;
	for(i = 0; i < CARTAS_MAZO; i++)
	{
		// analizo el vector de cartas en forma circular
		if((i + desde) > CARTAS_MAZO)
		{
			carta = i + desde - CARTAS_MAZO;
		}
		else
		{
			carta = i - desde;
		}
		if(carta == hasta) break;
		if((jugador = m_pMesa->mazo.carta[carta].poseedor) >= 0)
		{
			// me fijo si ya la tengo creada
			if(!m_pCartas[jugador][m_CountCartas[jugador]])
			{
				if(jugador == 0)
				{
					// la banca
					linea = m_CountCartas[jugador] + 1;
				}
				else
				{
					// un jugador
					linea = (LINES - 6 - (m_CountCartas[jugador]));
				}
				// sino creo la ventana
				if(!m_CountCartas[jugador] && jugador != m_own)
				{	// invisible
					m_pCartas[jugador][m_CountCartas[jugador]] =
						new CCarta(	m_pWindow, ColJugador(jugador), linea,
								m_pMesa->mazo.carta[carta].numero,
								m_pMesa->mazo.carta[carta].palo, 0);
				}
				else
				{	// visible
					m_pCartas[jugador][m_CountCartas[jugador]] =
						new CCarta(	m_pWindow, ColJugador(jugador), linea,
								m_pMesa->mazo.carta[carta].numero,
								m_pMesa->mazo.carta[carta].palo, 1);
				}
				sprintf(str, "Carta nueva <%2i>-<%i>",
								m_pMesa->mazo.carta[carta].numero,
								m_pMesa->mazo.carta[carta].palo);
				ShowMessage(str);
			}
			sprintf(str, "m_pCartas[%i][%i]->Update", jugador, m_CountCartas[jugador]);
			ShowMessage(str);
			m_pCartas[jugador][m_CountCartas[jugador]]->Update();
			ShowMessage("CountCartas++");
			m_CountCartas[jugador]++;
		}
	}
}

void CBkDisplay::Info()
{
	int i;
	long sumApuesta = 0l;
	char strstatus[20];

	// muestro en pantalla la info de cada jugador
	// algunos datos son visibles solamente para el propietario
	for(i = 1; i <= MAX_JUGADORES; i++)	
	{
		if(i == m_own)
		{
			// si es del propio jugador imprimo su nombre y el monedero
			mvwprintw(m_pWindow, LINES - 4, ColJugador(i), "<%s>", m_pMesa->jugador[i].nic);
			mvwprintw(m_pWindow, LINES - 2, ColJugador(i), "%5li", m_pMesa->jugador[i].monedero);
		}
		else
		{
			// si es de otro jugador imprimo solo su nombre
			mvwprintw(m_pWindow, LINES - 4, ColJugador(i), "%s", m_pMesa->jugador[i].nic);
			mvwprintw(m_pWindow, LINES - 2, ColJugador(i), "-???-");
		}
		// imprimo el monto apostado
		mvwprintw(m_pWindow, LINES - 15, ColJugador(i), "%5li", m_pMesa->jugador[i].apostado);
		// imprimo el estado
		switch(m_pMesa->jugador[i].estado)
		{
		case JST_VACIO:
			strcpy(strstatus, "   Libre  ");
			break;
		case JST_ESPERANDO:
			strcpy(strstatus, " Esperando");
			break;
		case JST_JUGANDO:
			strcpy(strstatus, "  Jugando ");
			break;
		case JST_PLANTADO:
			strcpy(strstatus, " Plantado ");
			break;
		case JST_PERDIO:
			strcpy(strstatus, "  Perdio  ");
			break;
		case JST_GANADO:
			strcpy(strstatus, "   Gano   ");
			break;
		case JST_ESPECTADOR:
			strcpy(strstatus, "Espectador");
			break;
		default:
			strcpy(strstatus, "          ");
			break;
		}
		mvwprintw(m_pWindow, LINES - 13, ColJugador(i) - 3, strstatus);
		// sumo el total apostado para informarlo
		sumApuesta += m_pMesa->jugador[i].apostado;
	}
	// muestro un resumen del estado de la mesa
	mvwprintw(m_pWindow, 2, COLS - 30, "Resolucion:        %ix%i", (int)COLS, (int)LINES);
	mvwprintw(m_pWindow, 4, COLS - 30, "Saldo de la banca: %6li", m_pMesa->banca.pozo);
	mvwprintw(m_pWindow, 5, COLS - 30, "Total apostado:    %6li", sumApuesta);
	mvwprintw(m_pWindow, 6, COLS - 30, "->%s", m_pMesa->banca.msg);
}

int CBkDisplay::ColJugador(int jugador)
{
	switch(jugador)
	{
	case 0: return (COLS / 2);	// para la banca

	case 1: return (COLS / 7 * 2);
	case 2: return (COLS / 7 * 5);
	case 3: return (COLS / 7 * 1);
	case 4: return (COLS / 7 * 6);
	case 5: return (COLS / 7 * 3);
	case 6: return (COLS / 7 * 4);
	}
	return 0;
}

void CBkDisplay::Restart()
{
	int i, j;
	// limpio el tablero
	for(i = 0; i <= MAX_JUGADORES; i++)
	{
		for(j = 0; j < MAX_CARTAS; j++)
		{
			if(m_pCartas[i][j])
			{
				delete m_pCartas[i][j];
				m_pCartas[i][j] = NULL;
			}
		}
	}
    wclear(stdscr);
    touchwin(stdscr);
    touchwin(m_pWindow);
	wnoutrefresh(m_pWindow);
	doupdate();
	Info();
}

void CBkDisplay::Mostrar(int todos = 0)
{
    int i = 1, j;
    // muestro todas las cartas menos la banca
	if(todos) i = 0;
    for(i; i <= MAX_JUGADORES; i++)
    {
        for(j = 0; j < MAX_CARTAS; j++)
        {
            if(m_pCartas[i][j])
            {
                if(!m_pCartas[i][j]->Visible())
				{
					m_pCartas[i][j]->Mostrar();
				}
            }
        }
    }
    wclear(stdscr);
    touchwin(stdscr);
    touchwin(m_pWindow);
    wnoutrefresh(m_pWindow);
    doupdate();
}

// para controlar que la resolucion de la terminal sea la correcta
// si es la correcta devuelve 1
int CBkDisplay::Check()
{
	if(LINES < 24 || COLS < 80)
	{
		MessageBox("Error", "Resolucion minima 80x25", 5);
		return 0;
	}
	else
	{
		return 1;
	}
}


