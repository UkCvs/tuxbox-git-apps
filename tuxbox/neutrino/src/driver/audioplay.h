/*
	Neutrino-GUI  -   DBoxII-Project

	Homepage: http://www.dbox2.info/

	Kommentar:

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __AUDIO_PLAY__
#define __AUDIO_PLAY__

#include <pthread.h>
#include <driver/audiodec/basedec.h>
#include <driver/audiofile.h>
#include <driver/audiometadata.h>
#include <string>

class CAudioPlayer
{
private:
	time_t m_played_time;
	int  m_sc_buffered;
	FILE		*soundfd;
	pthread_t	thrPlay;
	FILE		*fp;
	CBaseDec::State state;
	static void* PlayThread(void*);
	void clearMetaData();
	unsigned int m_SecondsToSkip;

protected: 
	CAudioMetaData m_MetaData;
	bool SetDSP(int soundfd, int fmt, unsigned int dsp_speed, unsigned int channels);

public:
	static CAudioPlayer* getInstance();
	bool play(const CAudiofile*, bool highPrio=false);
	void stop();
	void pause();
	void init();
	void ff(unsigned int seconds=0);
	void rev(unsigned int seconds=0);
	CAudioMetaData getMetaData();
	bool hasMetaDataChanged();
	CAudioMetaData readMetaData(const char*, bool);
	time_t getTimePlayed(){return m_played_time;}
	time_t getTimeTotal(){return m_MetaData.total_time;}
	int getScBuffered(){return m_sc_buffered;}
	CBaseDec::State getState(){return state;}

	void sc_callback(void *arg);
	CAudioPlayer();
	~CAudioPlayer();


};


#endif

