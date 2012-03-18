/*
	$Id: scan_setup.cpp,v 1.18 2012/03/18 11:20:14 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	scan setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui/scan_setup.h"

#include <global.h>
#include <neutrino.h>

#include "gui/scan.h"
#include "gui/motorcontrol.h"

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/hintbox.h>

#include <driver/screen_max.h>

#include <system/debug.h>

#define scanSettings (CNeutrinoApp::getInstance()->ScanSettings())


CZapitClient::SatelliteList satList;

char zapit_lat[12];
char zapit_long[12];

CScanSetup::CScanSetup()
{
	width = w_max (500, 100);
	selected = -1;
	sat_list_size = 0;
	provider_list_size = 0;
}

CScanSetup::~CScanSetup()
{

}

int CScanSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init scan service\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey == "save_action") {
		scanSettings.gotoXXLatitude = strtod(zapit_lat, NULL);
		scanSettings.gotoXXLongitude = strtod(zapit_long, NULL);
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		g_Zapit->loadScanSetupSettings();
		res = showScanService();
		if (res == menu_return::RETURN_EXIT_ALL)
			return menu_return::RETURN_EXIT_ALL;
		return menu_return::RETURN_EXIT;
	}
	else if (actionKey == "show_scanmodes")
	{
		res = showScanModeMenue();
		if (res == menu_return::RETURN_EXIT_ALL)
			return menu_return::RETURN_EXIT_ALL;
		res = showScanService();
		if (res == menu_return::RETURN_EXIT_ALL)
			return menu_return::RETURN_EXIT_ALL;
		return menu_return::RETURN_EXIT;
	}

	res = showScanService();

	return res;
}

#define SATSETUP_SCANTP_FEC_COUNT 5
#if HAVE_DVB_API_VERSION < 3
const CMenuOptionChooser::keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
	{ 1, LOCALE_SCANTP_FEC_1_2 },
	{ 2, LOCALE_SCANTP_FEC_2_3 },
	{ 3, LOCALE_SCANTP_FEC_3_4 },
	{ 4, LOCALE_SCANTP_FEC_5_6 },
	{ 5, LOCALE_SCANTP_FEC_7_8 }
};
#else
const CMenuOptionChooser::keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
        { 1, LOCALE_SCANTP_FEC_1_2 },
        { 2, LOCALE_SCANTP_FEC_2_3 },
        { 3, LOCALE_SCANTP_FEC_3_4 },
        { 5, LOCALE_SCANTP_FEC_5_6 },
        { 7, LOCALE_SCANTP_FEC_7_8 }
};
#endif

#define CABLESETUP_SCANTP_MOD_COUNT 7
const CMenuOptionChooser::keyval CABLESETUP_SCANTP_MOD[CABLESETUP_SCANTP_MOD_COUNT] =
{
	{0, LOCALE_SCANTP_MOD_QPSK     } ,
	{1, LOCALE_SCANTP_MOD_QAM_16   } ,
	{2, LOCALE_SCANTP_MOD_QAM_32   } ,
	{3, LOCALE_SCANTP_MOD_QAM_64   } ,
	{4, LOCALE_SCANTP_MOD_QAM_128  } ,
	{5, LOCALE_SCANTP_MOD_QAM_256  } ,
	{6, LOCALE_SCANTP_MOD_QAM_AUTO }

};

#define SATSETUP_SCANTP_POL_COUNT 2
const CMenuOptionChooser::keyval SATSETUP_SCANTP_POL[SATSETUP_SCANTP_POL_COUNT] =
{
	{ 0, LOCALE_SCANTP_POL_H },
	{ 1, LOCALE_SCANTP_POL_V }
};

#define SATSETUP_DISEQC_OPTION_COUNT 6
const CMenuOptionChooser::keyval SATSETUP_DISEQC_OPTIONS[SATSETUP_DISEQC_OPTION_COUNT] =
{
	{ NO_DISEQC          , LOCALE_SATSETUP_NODISEQC    },
	{ MINI_DISEQC        , LOCALE_SATSETUP_MINIDISEQC  },
	{ DISEQC_1_0         , LOCALE_SATSETUP_DISEQC10    },
	{ DISEQC_1_1         , LOCALE_SATSETUP_DISEQC11    },
	{ DISEQC_1_2         , LOCALE_SATSETUP_DISEQC12    },
	{ SMATV_REMOTE_TUNING, LOCALE_SATSETUP_SMATVREMOTE }

};

#define SCANTS_BOUQUET_OPTION_COUNT 5
const CMenuOptionChooser::keyval SCANTS_BOUQUET_OPTIONS[SCANTS_BOUQUET_OPTION_COUNT] =
{
	{ CZapitClient::BM_DELETEBOUQUETS        , LOCALE_SCANTS_BOUQUET_ERASE     },
	{ CZapitClient::BM_CREATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_CREATE    },
	{ CZapitClient::BM_DONTTOUCHBOUQUETS     , LOCALE_SCANTS_BOUQUET_LEAVE     },
	{ CZapitClient::BM_UPDATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_UPDATE    },
	{ CZapitClient::BM_CREATESATELLITEBOUQUET, LOCALE_SCANTS_BOUQUET_SATELLITE }
};

#define SCANTS_ZAPIT_SCANTYPE_COUNT 4
const CMenuOptionChooser::keyval SCANTS_ZAPIT_SCANTYPE[SCANTS_ZAPIT_SCANTYPE_COUNT] =
{
	{  CZapitClient::ST_TVRADIO	, LOCALE_ZAPIT_SCANTYPE_TVRADIO     },
	{  CZapitClient::ST_TV		, LOCALE_ZAPIT_SCANTYPE_TV    },
	{  CZapitClient::ST_RADIO	, LOCALE_ZAPIT_SCANTYPE_RADIO     },
	{  CZapitClient::ST_ALL		, LOCALE_ZAPIT_SCANTYPE_ALL }
};

#define SECTIONSD_SCAN_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval SECTIONSD_SCAN_OPTIONS[SECTIONSD_SCAN_OPTIONS_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  },
	{ 2, LOCALE_OPTIONS_ON_WITHOUT_MESSAGES  }
};

#define SCANTS_SCAN_OPTION_COUNT	3
const CMenuOptionChooser::keyval SCANTS_SCAN_OPTIONS[SCANTS_SCAN_OPTION_COUNT] =
{
	{ CScanTs::SCAN_COMPLETE,	LOCALE_SCANTP_SCAN_ALL_SATS },
	{ CScanTs::SCAN_ONE_TP,		LOCALE_SCANTP_SCAN_ONE_TP },
	{ CScanTs::SCAN_ONE_SAT,	LOCALE_SCANTP_SCAN_ONE_SAT }
};


#define SCANTS_CABLESCAN_OPTION_COUNT	2
const CMenuOptionChooser::keyval SCANTS_CABLESCAN_OPTIONS[SCANTS_CABLESCAN_OPTION_COUNT] =
{
	{ CScanTs::SCAN_COMPLETE,	LOCALE_SCANTP_SCAN_COMPLETE },
	{ CScanTs::SCAN_ONE_TP,		LOCALE_SCANTP_SCAN_ONE_TP }
};

#define OPTIONS_SOUTH0_NORTH1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_SOUTH0_NORTH1_OPTIONS[OPTIONS_SOUTH0_NORTH1_OPTION_COUNT] =
{
	{ 0, LOCALE_SATSETUP_SOUTH },
	{ 1, LOCALE_SATSETUP_NORTH }
};
#define OPTIONS_EAST0_WEST1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_EAST0_WEST1_OPTIONS[OPTIONS_EAST0_WEST1_OPTION_COUNT] =
{
	{ 0, LOCALE_SATSETUP_EAST },
	{ 1, LOCALE_SATSETUP_WEST }
};

int CScanSetup::showScanService()
{
	dprintf(DEBUG_DEBUG, "init scansettings\n");
	initScanSettings();
	
	//menue init
	CMenuWidget* scansetup = new CMenuWidget(LOCALE_SERVICEMENU_HEAD, NEUTRINO_ICON_SETTINGS, width);
	scansetup->setPreselected(selected);

	//subhead
	scansetup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_SERVICEMENU_SCANTS));

	//prepare scantype green
	CMenuOptionChooser* ojScantype = new CMenuOptionChooser(LOCALE_ZAPIT_SCANTYPE, (int *)&scanSettings.scanType, SCANTS_ZAPIT_SCANTYPE, SCANTS_ZAPIT_SCANTYPE_COUNT, true, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);

	//prepare bouquet mode yellow
	CMenuOptionChooser* ojBouquets = new CMenuOptionChooser(LOCALE_SCANTS_BOUQUET, (int *)&scanSettings.bouquetMode, SCANTS_BOUQUET_OPTIONS, SCANTS_BOUQUET_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);

	// intros
	scansetup->addItem(GenericMenuSeparator);
	scansetup->addItem(GenericMenuBack);
	scansetup->addItem(GenericMenuSeparatorLine);

	//save button red
	scansetup->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "save_action", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	scansetup->addItem(GenericMenuSeparatorLine);

	//prepare sat-lnb-settings
	CMenuWidget* extSatSettings = NULL;
	CMenuWidget* extMotorSettings = NULL;
	CStringInput* toff_lat = NULL;
	CStringInput* toff_long = NULL;
	CSatDiseqcNotifier* satDiseqcNotifier = NULL;
	CScanSettingsSatManNotifier* scanSettingsSatManNotifier = NULL;

	//sat-lnb-settings
	if(g_info.delivery_system == DVB_S)
	{
 		g_Zapit->getScanSatelliteList(satList);

		//prepare diseqc
		CMenuOptionStringChooser* ojSat = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, scanSettings.satNameNoDiseqc, ((scanSettings.diseqcMode == DISEQC_1_2) || (scanSettings.diseqcMode == NO_DISEQC)));

		for (uint i=0; i < sat_list_size; i++)
		{
			ojSat->addOption(satList[i].satName);
			dprintf(DEBUG_DEBUG, "got scanprovider (sat): %s\n", satList[i].satName );
		}

		//prepare diseqc repeats
		CMenuOptionNumberChooser * ojDiseqcRepeats = new CMenuOptionNumberChooser(LOCALE_SATSETUP_DISEQCREPEAT, (int *)&scanSettings.diseqcRepeat, (scanSettings.diseqcMode != NO_DISEQC) && (scanSettings.diseqcMode != DISEQC_1_0), 0, 2);

		//extended sat settings
		extSatSettings = new CMenuWidget(LOCALE_SATSETUP_EXTENDED, NEUTRINO_ICON_SETTINGS);

		//intros ext sat settings
		extSatSettings->addItem(GenericMenuSeparator);
		extSatSettings->addItem(GenericMenuBack);
		extSatSettings->addItem(GenericMenuSeparatorLine);

		//prepare diseqc mode
		CMenuForwarder* ojExtSatSettings = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED, (scanSettings.diseqcMode != NO_DISEQC), NULL, extSatSettings, NULL, CRCInput::RC_1);

		//make sat list
		for( uint i=0; i < sat_list_size; i++)
		{
			CMenuOptionNumberChooser * oj = new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, scanSettings.diseqscOfSat(satList[i].satName), true, -1, sat_list_size - 1, 1, -1, LOCALE_OPTIONS_OFF, satList[i].satName);

			extSatSettings->addItem(oj);
		}

		//motor settings
		extMotorSettings = new CMenuWidget(LOCALE_SATSETUP_EXTENDED_MOTOR, NEUTRINO_ICON_SETTINGS);
		
		//intros motor settings
		extMotorSettings->addItem(GenericMenuSeparator);
		extMotorSettings->addItem(GenericMenuBack);
		extMotorSettings->addItem(GenericMenuSeparatorLine);
		
		//save motorsettings red
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_SAVESETTINGSNOW, true, NULL, this, "save_action", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		//motorspeed (how long to set wait timer for dish to travel to correct position) 
		extMotorSettings->addItem(new CMenuOptionNumberChooser(LOCALE_SATSETUP_MOTORSPEED, (int *)&scanSettings.motorRotationSpeed, true, 0, 64)) ;
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		//gotoxx settings
		extMotorSettings->addItem(new CMenuOptionChooser(LOCALE_SATSETUP_USEGOTOXX,  (int *)&scanSettings.useGotoXX, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

		sprintf(zapit_lat, "%02.6f", scanSettings.gotoXXLatitude);
		sprintf(zapit_long, "%02.6f", scanSettings.gotoXXLongitude);

		extMotorSettings->addItem(new CMenuOptionChooser(LOCALE_SATSETUP_LADIR,  (int *)&scanSettings.gotoXXLaDirection, OPTIONS_SOUTH0_NORTH1_OPTIONS, OPTIONS_SOUTH0_NORTH1_OPTION_COUNT, true));
		toff_lat = new CStringInput(LOCALE_SATSETUP_LAT, (char *) zapit_lat, 10, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789.");
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_LAT, true, zapit_lat, toff_lat));

		extMotorSettings->addItem(new CMenuOptionChooser(LOCALE_SATSETUP_LODIR,  (int *)&scanSettings.gotoXXLoDirection, OPTIONS_EAST0_WEST1_OPTIONS, OPTIONS_EAST0_WEST1_OPTION_COUNT, true));
		toff_long = new CStringInput(LOCALE_SATSETUP_LONG, (char *) zapit_long, 10, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789.");
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_LONG, true, zapit_long, toff_long));

		extMotorSettings->addItem(GenericMenuSeparatorLine);
		
		//manual motor control
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_MOTORCONTROL, true, NULL, new CMotorControl(), NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		//prepare motor control
		CMenuForwarder* ojExtMotorSettings = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, (scanSettings.diseqcMode == DISEQC_1_2), NULL, extMotorSettings, NULL, CRCInput::RC_2);

		//prepare/show sat list with options
		for( uint i=0; i < sat_list_size; i++)
		{
			CMenuOptionNumberChooser * oj = new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, scanSettings.motorPosOfSat(satList[i].satName), true, 0, 64/*sat_list_size*/, 0, 0, LOCALE_OPTIONS_OFF, satList[i].satName);

			extMotorSettings->addItem(oj);
		}

		//prepare sat list with diseqc options
		satDiseqcNotifier = new CSatDiseqcNotifier(ojSat, ojExtSatSettings, ojExtMotorSettings, ojDiseqcRepeats);
		CMenuOptionChooser* ojDiseqc = new CMenuOptionChooser(LOCALE_SATSETUP_DISEQC, (int *)&scanSettings.diseqcMode, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, true, satDiseqcNotifier);

		//show entries
		scansetup->addItem( ojScantype );
		scansetup->addItem( ojBouquets );
		scansetup->addItem(GenericMenuSeparatorLine);
		scansetup->addItem( ojDiseqc );
		scansetup->addItem( ojSat );
		scansetup->addItem( ojDiseqcRepeats );

		scansetup->addItem( ojExtSatSettings );
		scansetup->addItem( ojExtMotorSettings );
	}
	else
	{//cable

		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);
		
		//prepare/show providers
		CMenuOptionStringChooser* oj = new CMenuOptionStringChooser(LOCALE_CABLESETUP_PROVIDER, (char*)&scanSettings.satNameNoDiseqc, true);

		for( uint i=0; i< provider_list_size; i++)
		{
			oj->addOption(providerList[i].satName);
			dprintf(DEBUG_DEBUG, "got scanprovider (cable): %s\n", providerList[i].satName );
		}

		//show general entries
		scansetup->addItem( ojScantype );
		scansetup->addItem( ojBouquets );
		
		//show cable provider
		scansetup->addItem( oj);
	}

	//prepare scan mode (fast->on/off)
	CMenuOptionChooser* onoff_mode = ( new CMenuOptionChooser(LOCALE_SCANTP_SCANMODE, (int *)&scanSettings.scan_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	scansetup->addItem(GenericMenuSeparatorLine);

	if(scanSettings.TP_fec == 0) {
		scanSettings.TP_fec = 1;
	}

	//sub menue scanmode
	std::string scan_mode = getScanModeString(scanSettings.TP_scan);
	CMenuForwarder* fw_scanmode = new CMenuForwarder(LOCALE_SERVICEMENU_SCANMODES, true, scan_mode, this, "show_scanmodes", (g_info.delivery_system == DVB_S) ? CRCInput::RC_3 : CRCInput::RC_1);
	scansetup->addItem(fw_scanmode); 

	//show scan mode (fast->on/off)
	scansetup->addItem(onoff_mode);

	//prepare auto scan
	CSectionsdConfigNotifier sectionsdConfigNotifier;
	CMenuOptionChooser* onoffscanSectionsd = ( new CMenuOptionChooser(LOCALE_SECTIONSD_SCANMODE, (int *)&scanSettings.scanSectionsd, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true, &sectionsdConfigNotifier));

	//sat-lnb-settings
	if(g_info.delivery_system == DVB_S)
	{
		uint i;
		int satfound = -1;
		int firstentry = -1;

		scanSettingsSatManNotifier = new CScanSettingsSatManNotifier();
		scanSettings.TP_SatSelectMenu = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, scanSettings.TP_satname, ((scanSettings.diseqcMode != NO_DISEQC) && scanSettings.TP_scan), scanSettingsSatManNotifier);

		// add the sats which are configured (diseqc or motorpos) to the list of available sats */
		for (i = 0; i < sat_list_size; i++)
		{
			if ((((scanSettings.diseqcMode != DISEQC_1_2)) && (0 <= (*scanSettings.diseqscOfSat(satList[i].satName) ))) ||
			    (((scanSettings.diseqcMode == DISEQC_1_2)) && (0 <= (*scanSettings.motorPosOfSat(satList[i].satName)))))
			{
				if (firstentry == -1) firstentry = i;
				if (strcmp(scanSettings.TP_satname, satList[i].satName) == 0)
					satfound = i;
				scanSettings.TP_SatSelectMenu->addOption(satList[i].satName);
				dprintf(DEBUG_DEBUG, "satName = %s, diseqscOfSat(%d) = %d, motorPosOfSat(%d) = %d\n", satList[i].satName, i, *scanSettings.diseqscOfSat(satList[i].satName), i, *scanSettings.motorPosOfSat(satList[i].satName));
			}
		}
		// if scanSettings.TP_satname cannot be found in the list of available sats use 1st in list
		if ((satfound == -1) && (sat_list_size)) {
//			strcpy(scanSettings.TP_satname, satList[firstentry].satName);
			strcpy(scanSettings.TP_satname, scanSettings.satNameNoDiseqc);
		}
	} else {
		scanSettings.TP_SatSelectMenu = NULL;
	}

	//show auto scan
	scansetup->addItem(onoffscanSectionsd);
	scansetup->addItem(GenericMenuSeparatorLine);

	CScanTs* scanTs = new CScanTs();
	scansetup->addItem(new CMenuForwarder(LOCALE_SCANTS_STARTNOW, true, NULL, scanTs, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

	int res = scansetup->exec(NULL, "");
	scansetup->hide();
	selected = scansetup->getSelected();
	delete scansetup;

	delete extSatSettings;
	delete extMotorSettings;
	delete toff_lat;
	delete toff_long;
	delete satDiseqcNotifier;
	delete scanSettingsSatManNotifier;
	delete scanTs;

	return res;
}

//sub menue scanmode
int CScanSetup::showScanModeMenue()
{
	CMenuWidget* scanmode = new CMenuWidget(LOCALE_SERVICEMENU_SCANTS, NEUTRINO_ICON_SETTINGS, width);

	//prepare input signal rate
	CStringInput rate(LOCALE_SCANTP_RATE, (char *) scanSettings.TP_rate, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	//prepare fec select
	CMenuOptionChooser* fec = new CMenuOptionChooser(LOCALE_SCANTP_FEC, (int *)&scanSettings.TP_fec, SATSETUP_SCANTP_FEC, SATSETUP_SCANTP_FEC_COUNT, scanSettings.TP_scan == 1);

	//prepare frequency input, polarisation
	CStringInput* freq;
	CMenuOptionChooser* pol_mod;

	if(g_info.delivery_system == DVB_S) // sat
	{
		freq = new CStringInput(LOCALE_SCANTP_FREQ, (char *) scanSettings.TP_freq, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
		pol_mod = new CMenuOptionChooser(LOCALE_SCANTP_POL, (int *)&scanSettings.TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, scanSettings.TP_scan == 1);
	} 
	else // cable
	{
		freq = new CStringInput(LOCALE_SCANTP_FREQ, (char *) scanSettings.TP_freq, 9, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
		pol_mod = new CMenuOptionChooser(LOCALE_SCANTP_MOD, (int *)&scanSettings.TP_mod, CABLESETUP_SCANTP_MOD, CABLESETUP_SCANTP_MOD_COUNT, scanSettings.TP_scan == 1);
	}

		//prepare forwarders rate/freq
	CMenuForwarder *Rate = new CMenuForwarder(LOCALE_SCANTP_RATE, scanSettings.TP_scan == 1, scanSettings.TP_rate, &rate);
	CMenuForwarder *Freq = new CMenuForwarder(LOCALE_SCANTP_FREQ, scanSettings.TP_scan == 1, scanSettings.TP_freq, freq);
	
	CTP_scanNotifier *TP_scanNotifier;
	CMenuOptionChooser* scan;
	if(g_info.delivery_system == DVB_S) {
		TP_scanNotifier = new CTP_scanNotifier(fec, pol_mod, Freq, Rate, scanSettings.TP_SatSelectMenu);
		scan = ( new CMenuOptionChooser(LOCALE_SCANTP_SCAN, (int *)&scanSettings.TP_scan, SCANTS_SCAN_OPTIONS, SCANTS_SCAN_OPTION_COUNT, true/*(g_info.delivery_system == DVB_S)*/, TP_scanNotifier));
	} else {
		TP_scanNotifier = new CTP_scanNotifier(fec, pol_mod, Freq, Rate, 0);
		scan = ( new CMenuOptionChooser(LOCALE_SCANTP_SCAN, (int *)&scanSettings.TP_scan, SCANTS_CABLESCAN_OPTIONS, SCANTS_CABLESCAN_OPTION_COUNT, true/*(g_info.delivery_system == DVB_S)*/, TP_scanNotifier));
	}

	//intros scan mode
	scanmode->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_SERVICEMENU_SCANMODES));
	scanmode->addItem(GenericMenuSeparator);
	scanmode->addItem(GenericMenuBack);
	scanmode->addItem(GenericMenuSeparatorLine);
	scanmode->addItem(scan);
	if(g_info.delivery_system == DVB_S) {
		scanmode->addItem(scanSettings.TP_SatSelectMenu);
	}
	//show items for scanmode submenue
	scanmode->addItem(Freq);
	scanmode->addItem(pol_mod);
	scanmode->addItem(Rate);
	scanmode->addItem(fec);

	int res = scanmode->exec(NULL, "");
	scanmode->hide();
	delete scanmode;

	delete freq;
	delete TP_scanNotifier;

	return res;
}

void CScanSetup::initScanSettings()
{
	if(g_info.delivery_system == DVB_S) // sat
	{
		satList.clear();
		g_Zapit->getScanSatelliteList(satList);
	
		printf("[scan-setup] received %d sats\n", satList.size());
		t_satellite_position currentSatellitePosition = g_Zapit->getCurrentSatellitePosition();
	
		if (1/*scanSettings.diseqcMode == DISEQC_1_2*/)
		{
			for (uint i = 0; i < satList.size(); i++)
			{
				//printf("[neutrino] received %d: %s, %d\n", i, satList[i].satName, satList[i].satPosition);
				scanSettings.satPosition[i] = satList[i].satPosition;
				scanSettings.satMotorPos[i] = satList[i].motorPosition;
				strcpy(scanSettings.satName[i], satList[i].satName);
				//scanSettings.satDiseqc[i] = satList[i].satDiseqc;
				if (satList[i].satPosition == currentSatellitePosition)
					strcpy(scanSettings.satNameNoDiseqc, satList[i].satName);
			}
			for (uint i = satList.size(); i < MAX_SATELLITES; i++)
			{
				scanSettings.satName[i][0] = 0;
				scanSettings.satPosition[i] = 0;
				scanSettings.satDiseqc[i] = -1;
			}
		}
		
		sat_list_size = satList.size();
	}
	else // cable
	{
		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);

		printf("[scan-setup] received %d providers\n", providerList.size());
		provider_list_size = providerList.size();	
	}
	
}

typedef struct scan_mode_t
{
	const int scan_type;
	const neutrino_locale_t locale;
} scan_mode_struct_t;

const scan_mode_struct_t scan_mode[SCANTS_SCAN_OPTION_COUNT] =
{
	{CScanTs::SCAN_COMPLETE	, LOCALE_SCANTP_SCAN_ALL_SATS},
	{CScanTs::SCAN_ONE_TP	, LOCALE_SCANTP_SCAN_ONE_TP},
 	{CScanTs::SCAN_ONE_SAT	, LOCALE_SCANTP_SCAN_ONE_SAT},
};

std::string CScanSetup::getScanModeString(const int& scan_type)
{
	int st = scan_type;
	return g_Locale->getText(scan_mode[st].locale);

}

bool CSatDiseqcNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	if (*((int*) Data) == NO_DISEQC)
	{
		satMenu->setActive(true);
		extMenu->setActive(false);
		extMotorMenu->setActive(false);
		repeatMenu->setActive(false);
	}
	else
	if (*((int*) Data) == DISEQC_1_2)
	{
		satMenu->setActive(true);
		extMenu->setActive(true);
		extMotorMenu->setActive(true);
		repeatMenu->setActive(true);
	}
	else
	{
		satMenu->setActive(false);
		extMenu->setActive(true);
		extMotorMenu->setActive(false);
		repeatMenu->setActive((*((int*) Data) != DISEQC_1_0));
	}
	return true;
}

CTP_scanNotifier::CTP_scanNotifier(CMenuOptionChooser* i1, CMenuOptionChooser* i2, CMenuForwarder* i3, CMenuForwarder* i4, CMenuOptionStringChooser* i5)
{
	toDisable1[0]=i1;
	toDisable1[1]=i2;
	toDisable2[0]=i3;
	toDisable2[1]=i4;
	toDisable3[0]=i5;
}

bool CTP_scanNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	int tp_scan_mode = CNeutrinoApp::getInstance()->getScanSettings().TP_scan;
	bool set_true_false = tp_scan_mode;

	if ((*((int*) Data) == 0) || (*((int*) Data) == 2)) // all sats || one sat
		set_true_false = false;

	for (int i=0; i<2; i++)
	{
		if (toDisable1[i]) toDisable1[i]->setActive(set_true_false);
		if (toDisable2[i]) toDisable2[i]->setActive(set_true_false);
	}

	if (toDisable3[0]) {
		if (*((int*) Data) == 0) // all sat
			toDisable3[0]->setActive(false);
		else
			toDisable3[0]->setActive(true);
	}

	return true;
}

bool CScanSettingsSatManNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	(CNeutrinoApp::getInstance()->ScanSettings()).TP_diseqc =
		 *((CNeutrinoApp::getInstance()->ScanSettings()).diseqscOfSat((char*)Data));
	return true;
}
