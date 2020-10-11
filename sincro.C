/* *****************************************************************************
*
*	sincro.c
*
***************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/ipc.h>
//#include <sys/sem.h>

#include "sincro.h"

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
	/* union semun is defined by including <sys/sem.h> */
#else
	/* according to X/OPEN we have to define it ourselves */
	union semun
	{
		int val;                    /* value for SETVAL */
		struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
		unsigned short int *array;  /* array for GETALL, SETALL */
		struct seminfo *__buf;      /* buffer for IPC_INFO */
	};
#endif

#define SINC_WAIT	-1
#define SINC_SIGNAL	1

extern void ShowMessage(const char* msg);

CSincro::CSincro(int key)
{
	m_SemCount = 0;
	m_SemId = (-1);
	m_own = 0;
	m_key = key;
}

CSincro::~CSincro()
{
	Close();
}

int CSincro::Create(int count)
{
	Close();
	m_SemCount = count;
	if((m_SemId = semget(m_key, m_SemCount, IPC_CREAT|IPC_EXCL|0666)) == (-1))
	{
		ShowMessage("[CSincro::Create] No se pudo crear el semaforo");
		return (-1);
	}
	// para saber que soy la clase que creó el semaforo
	m_own = 1;
	// valor inicial de los semáforos
	for(int i = 0; i < count; i++)
	{
		Set(i, 0);
	}
	return 0;
}

int CSincro::Open(int count)
{
	Close();
	m_SemCount = count;
	if((m_SemId = semget(m_key, m_SemCount, 0660)) == (-1))
	{
		ShowMessage("[CSincro::Open] No se pudo abrir el semaforo");
		return (-1);
	}
	return 0;
}

int CSincro::Wait(int sem = 0)
{
	char str[40];
	sprintf(str, "[CSincro::Wait] (%i)", sem);
	ShowMessage(str);
	if(m_SemId != (-1) && sem < m_SemCount)
	{
		return semop(m_SemId, SemBuff(sem, SINC_WAIT), 1);
	}
	return (-1);
}

int CSincro::Signal(int sem = 0)
{
	char str[40];
	sprintf(str, "[CSincro::Signal] (%i)", sem);
	ShowMessage(str);
	if(m_SemId != (-1) && sem < m_SemCount)
	{
		return semop(m_SemId, SemBuff(sem, SINC_SIGNAL), 1);
	}
	return (-1);
}

void CSincro::Close()
{
	if(m_own)
	{
		semctl(m_SemId, 0, IPC_RMID, 0);
	}
	m_SemId = (-1);
	m_own = 0;
}

int CSincro::Set(int sem, int val)
{
	if(m_SemId != (-1) && sem < m_SemCount && m_own)
	{
		semun arg;
		arg.val = val;
		return semctl(m_SemId, sem, SETVAL, arg);
	}
	ShowMessage("[CSincro::Set] Error");
	return (-1);
}

sembuff* CSincro::SemBuff(int sem, int opt)
{
	m_SemBuff.sem_num = sem;
	m_SemBuff.sem_op = opt;
	m_SemBuff.sem_flg = 0;
	return &m_SemBuff;
}

