/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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


#ifndef __messagebox__
#define __messagebox__

#include <driver/fb_window.h>

#include "menue.h"

#include <string>
#include <vector>


class CMessageBoxNotifier
{
  public:
	virtual void onYes( ) = NULL;
	virtual void onNo( ) = NULL;
};



class CMessageBox
{
 private:

	CFBWindow *              window;

	int                      width, height;

	int                      fheight;
	int                      theight;
	
	std::string              caption;
	std::vector<std::string> text;
	std::string              iconfile;
	CMessageBoxNotifier*     notifier;

	int                      selected;
	int                      showbuttons;
	
	void paintHead();
	void paintButtons();
	
	void yes();
	void no();
	void cancel();

 public:
	enum result_
		{
			mbrYes,
			mbrNo,
			mbrCancel,
			mbrBack
		} result;
	
	enum buttons_
		{
			mbYes= 0x01,
			mbNo = 0x02,
			mbCancel = 0x04,
			mbAll = 0x07,
			mbBack = 0x08
		} buttons;
	
	// Text & Caption are always UTF-8 encoded
	CMessageBox(const std::string Caption, std::string Text, CMessageBoxNotifier* Notifier, const char * const Icon = NULL, const int Width = 500, const uint Default = mbrYes, const uint ShowButtons = mbAll);
	~CMessageBox(void);

	int exec(int timeout = -1);
	
};

// Text & Caption are always UTF-8 encoded
int ShowMsgUTF(const char * const Caption, std::string Text, const uint Default, const uint ShowButtons, const char * const Icon = NULL, const int Width = 450, const int timeout = -1);

#endif
