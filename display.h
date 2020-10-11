/* display.h */
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <curses.h>

//#include "carta.h"
//#include "mesa.h"

class CBkDisplay
{
public:
	CBkDisplay(int own, MESA *pMesa);
	virtual ~CBkDisplay();

	int Check();

	void Info();
	void Update();
	// mensajes en pantalla
	int MessageBox(const char *title, const char *msg, int wait);
	// permite contestar y/n --> y devuelve 1, n devuelve 0
	// y si es un numero lo devuelve
	int	QuestionBox(const char *title, const char *question);
	void Restart();
	void Mostrar(int todos = 0);

protected:
	WINDOW *m_pWindow;
	CCarta *m_pCartas[MAX_JUGADORES + 1][MAX_CARTAS];
	MESA *m_pMesa;
	int m_own;
	void Cartas();
	int ColJugador(int jugador);	// devuelve la columna donde pone las cartas
	int MAX(int x, int y){ return (x > y ? x : y);}
	int m_CountCartas[MAX_JUGADORES + 1];

};
#endif /*_DISPLAY_H_*/

