/* sincro.h */

#ifndef _SINCRO_H_
#define _SINCRO_H_

#include <sys/sem.h>

typedef sembuf sembuff;

class CSincro
{
public:
	CSincro(int key);
	virtual ~CSincro();

	int Create(int count);
	int Open(int count);

	int Wait(int sem = 0);
	int Signal(int sem = 0);
	int Set(int sem, int val);

	void Close();

protected:
	int m_SemCount;
	int m_SemId;
	int m_own;
	sembuff m_SemBuff;
	int m_key;

	sembuff* SemBuff(int sem, int opt);


};
#endif /*_SINCRO_H_*/


