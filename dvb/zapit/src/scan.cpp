/*
 * $Id: scan.cpp,v 1.111 2003/05/09 13:10:49 digi_casi Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <fcntl.h>
#include <unistd.h>

/* libevent */
#include <eventserver.h>

#include <zapit/bouquets.h>
#include <zapit/client/zapitclient.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/nit.h>
#include <zapit/scan.h>
#include <zapit/sdt.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>

#define SERVICES_TMP "/tmp/services.tmp"

short scan_runs;
short curr_sat;
static int status = 0;
uint processed_transponders;
uint32_t actual_freq;
uint actual_polarisation;

CBouquetManager* scanBouquetManager;

extern tallchans allchans;   //  defined in zapit.cpp
extern int found_transponders;
extern int found_channels;
extern std::map <t_channel_id, uint8_t> service_types;
extern uint32_t found_tv_chans;
extern uint32_t found_radio_chans;
extern uint32_t found_data_chans;

/* zapit.cpp */
extern CFrontend *frontend;
extern xmlDocPtr scanInputParser;
extern std::map <uint8_t, std::string> scanProviders;
extern std::map <string, int32_t> satellitePositions;
extern CZapitClient::bouquetMode bouquetMode;
extern CEventServer *eventServer;

void cp(char * from, char * to)
{
	char cmd[256] = "cp -f ";
	strcat(cmd, from);
	strcat(cmd, " ");
	strcat(cmd, to);
	system(cmd);
}

void copy_to_satellite(FILE * fd, FILE * fd1, char * providerName)
{
	//copies services from previous services.xml file from start up to the sat that is being scanned...
	char buffer[256] = "";
	
	//look for sat to be scanned... or end of file
	fgets(buffer, 255, fd);
	while(!feof(fd1) && !((strstr(buffer, "sat name") && strstr(buffer, providerName)) || strstr(buffer, "</zapit>")))
	{
		fputs(buffer, fd);
		fgets(buffer, 255, fd1);
	}
	
	// if not end of file
	if (!feof(fd1) && !strstr(buffer, "</zapit>"))
		// skip to end of satellite
		while (!feof(fd1) && !strstr(buffer, "</sat>"))
			fgets(buffer, 255, fd1);
}

void copy_to_end(FILE * fd, FILE * fd1, char * providerName)
{
	//copies the services from previous services.xml file from the end of sat being scanned to the end of the file...
	char buffer[256] ="";
	
	fgets(buffer, 255, fd1);
	while(!feof(fd1) && !strstr(buffer, "</zapit>"))
	{
		fputs(buffer, fd);
		fgets(buffer, 255, fd1);
	}
	fclose(fd1);
	unlink(SERVICES_TMP);
}

char *getFrontendName(void)
{
	if (!frontend)
		return NULL;

	switch (frontend->getInfo()->type) {
	case FE_QPSK:   /* satellite frontend */
		return "sat";
	case FE_QAM:    /* cable frontend */
		return "cable";
	case FE_OFDM:   /* terrestrial frontend */
		return "terrestrial";
	default:        /* unsupported frontend */
		return NULL;
	}
}

void stop_scan(const bool success)
{
	/* notify client about end of scan */
	scan_runs = 0;
	eventServer->sendEvent(success ? CZapitClient::EVT_SCAN_COMPLETE : CZapitClient::EVT_SCAN_FAILED, CEventServer::INITID_ZAPIT);
	if (scanBouquetManager)
	{
		for (vector<CBouquet*>::iterator it = scanBouquetManager->Bouquets.begin(); it != scanBouquetManager->Bouquets.end(); it++)
		{
			for (vector<CZapitChannel*>::iterator jt = (*it)->tvChannels.begin(); jt != (*it)->tvChannels.end(); jt++)
				delete (*jt);
			for (vector<CZapitChannel*>::iterator jt = (*it)->radioChannels.begin(); jt != (*it)->radioChannels.end(); jt++)
				delete (*jt);
		}
		scanBouquetManager->clearAll();
		delete scanBouquetManager;
	}
}

int bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(uint32_t TsidOnid, struct dvb_frontend_parameters *feparams, uint8_t polarity, uint8_t DiSEqC)
{
	if (scantransponders.find(TsidOnid) == scantransponders.end())
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);

		scantransponders.insert
		(
			std::pair <unsigned int, transpondermap>
			(
				TsidOnid,
				transpondermap
				(
					(TsidOnid >> 16),
					TsidOnid,
					*feparams,
					polarity,
					DiSEqC
				)
			)
		);

		return 0;
	}


	return 1;
}

/* build transponder for cable-users with sat-feed*/
int build_bf_transponder(struct dvb_frontend_parameters *feparams)
{
	if (frontend->setParameters(feparams, 0, 0) < 0)
		return -1;

	return bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(get_sdt_TsidOnid(), feparams, 0, 0);
}


int get_nits(struct dvb_frontend_parameters *feparams, uint8_t polarization, uint8_t DiSEqC)
{
	if (frontend->setParameters(feparams, polarization, DiSEqC) < 0)
		return -1;

	if ((status = parse_nit(DiSEqC)) <= -2) /* nit unavailable */
		status = bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(get_sdt_TsidOnid(), feparams, polarization, DiSEqC);

	return status;
}

int get_sdts(void)
{
	stiterator tI;

	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++) {
			/* msg to neutrino */
			processed_transponders++;
			eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, CEventServer::INITID_ZAPIT, &processed_transponders, sizeof(processed_transponders));

		if (frontend->setParameters(&tI->second.feparams, tI->second.polarization, tI->second.DiSEqC) < 0)
			continue;

		INFO("parsing SDT (tsid:onid %04x:%04x)", tI->second.transport_stream_id, tI->second.original_network_id);

			actual_freq = tI->second.feparams.frequency;
 			eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCY,CEventServer::INITID_ZAPIT, &actual_freq,sizeof(actual_freq));
 			actual_polarisation = (uint)tI->second.polarization;
 			eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCYP,CEventServer::INITID_ZAPIT,&actual_polarisation,sizeof(actual_polarisation));
		parse_sdt(tI->second.transport_stream_id, tI->second.original_network_id, tI->second.DiSEqC);
	}

	return 0;
}

void write_xml_header(FILE * fd, const char *filename)
{
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");
}

void write_xml_footer(FILE *fd)
{
	fprintf(fd, "</zapit>\n");
	fclose(fd);
}

void write_bouquets(void)
{
	if (bouquetMode == CZapitClient::BM_DELETEBOUQUETS) 
	{
		INFO("removing existing bouquets");
		unlink(BOUQUETS_XML);
	}

	else if ((bouquetMode == CZapitClient::BM_DONTTOUCHBOUQUETS))
		INFO("leaving bouquets untouched");

	else
		scanBouquetManager->saveBouquets();
}

void write_transponder(FILE *fd, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id)
{
	stiterator tI = scantransponders.find((transport_stream_id << 16) | original_network_id);

	switch (frontend->getInfo()->type) {
	case FE_QAM: /* cable */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hu\" symbol_rate=\"%u\" fec_inner=\"%hu\" modulation=\"%hu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.qam.symbol_rate,
			tI->second.feparams.u.qam.fec_inner,
			tI->second.feparams.u.qam.modulation);
		break;

	case FE_QPSK: /* satellite */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hu\" symbol_rate=\"%u\" fec_inner=\"%hu\" polarization=\"%hu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.qpsk.symbol_rate,
			tI->second.feparams.u.qpsk.fec_inner,
			tI->second.polarization);
		break;

	case FE_OFDM: /* terrestrial */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hu\" bandwidth=\"%hu\" code_rate_HP=\"%hu\" code_rate_LP=\"%hu\" constellation=\"%hu\" transmission_mode=\"%hu\" guard_interval=\"%hu\" hierarchy_information=\"%hu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.ofdm.bandwidth,
			tI->second.feparams.u.ofdm.code_rate_HP,
			tI->second.feparams.u.ofdm.code_rate_LP,
			tI->second.feparams.u.ofdm.constellation,
			tI->second.feparams.u.ofdm.transmission_mode,
			tI->second.feparams.u.ofdm.guard_interval,
			tI->second.feparams.u.ofdm.hierarchy_information);
		break;

	default:
		return;
	}

	for (tallchans::const_iterator cI = allchans.begin(); cI != allchans.end(); cI++)
		if ((cI->second.getTransportStreamId() == transport_stream_id) && (cI->second.getOriginalNetworkId() == original_network_id)) {
			if (cI->second.getName().length() == 0)
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%04x\" service_type=\"%02x\"/>\n",
					cI->second.getServiceId(),
					cI->second.getServiceId(),
					cI->second.getServiceType());
			else
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%02x\"/>\n",
					cI->second.getServiceId(),
					convert_UTF8_To_UTF8_XML(cI->second.getName()).c_str(),
					cI->second.getServiceType());
		}

	fprintf(fd, "\t\t</transponder>\n");

	return;
}

int write_provider(FILE *fd, const char *type, const char *provider_name, const uint8_t DiSEqC)
{
	int status = -1;
	
	if (!scantransponders.empty())
	{
		/* cable tag */
		if (!strcmp(type, "cable"))
		{
			fprintf(fd, "\t<%s name=\"%s\">\n", type, provider_name);
		}

		/* satellite tag */
		else
		{
			fprintf(fd, "\t<%s name=\"%s\" diseqc=\"%hd\">\n", type, provider_name, DiSEqC);
		}

		/* channels */
		for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
		{
			write_transponder(fd, tI->second.transport_stream_id, tI->second.original_network_id);
		}

		/* end tag */
		fprintf(fd, "\t</%s>\n", type);
		status = 0; // this indicates that services have been found and that bouquets should be written...
	}

	/* clear results for next provider */
	allchans.clear();                  // different provider may have the same onid/sid pair // FIXME
	scantransponders.clear();

	return status;
}

int scan_transponder(xmlNodePtr transponder, bool satfeed, uint8_t diseqc_pos)
{
	uint8_t polarization = 0;
	dvb_frontend_parameters feparams;
		
	feparams.frequency = xmlGetNumericAttribute(transponder, "frequency", 0);
	feparams.inversion = INVERSION_AUTO;

	/* cable */
	if (frontend->getInfo()->type == FE_QAM)
	{
		feparams.u.qam.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.u.qam.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "fec_inner", 0);
		feparams.u.qam.modulation = (fe_modulation_t) xmlGetNumericAttribute(transponder, "modulation", 0);
	}

	/* satellite */
	else if (frontend->getInfo()->type == FE_QPSK)
	{
		feparams.u.qpsk.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.u.qpsk.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "fec_inner", 0);
		polarization = xmlGetNumericAttribute(transponder, "polarization", 0);
	}

	/* terrestrial */
	else if (frontend->getInfo()->type == FE_OFDM)
	{
		feparams.u.ofdm.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(transponder, "bandwidth", 0);
		feparams.u.ofdm.code_rate_HP = FEC_AUTO;
		feparams.u.ofdm.code_rate_LP = FEC_AUTO;
		feparams.u.ofdm.constellation = QAM_AUTO;
		feparams.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
		feparams.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
		feparams.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
	}

	if ((frontend->getInfo()->type == FE_QAM) && satfeed) 
	{
		/* build special transponder for cable with satfeed */
		status = build_bf_transponder(&feparams);
	}
	else 
	{
		/* read network information table */
		status = get_nits(&feparams, polarization, diseqc_pos);
	}
	
	return 0;
}

void scan_provider(xmlNodePtr search, char * providerName, bool satfeed, uint8_t diseqc_pos)
{
	xmlNodePtr transponder = NULL;

	/* send sat name to client */
	eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, providerName, strlen(providerName) + 1);
	transponder = search->xmlChildrenNode;

	/* read all transponders */
	while ((transponder = xmlGetNextOccurence(transponder, "transponder")) != NULL)
	{
		scan_transponder(transponder, satfeed, diseqc_pos);

		/* next transponder */
		transponder = transponder->xmlNextNode;
	}

	/* 
	 * parse:
	 * service description table,
	 * program association table,
	 * bouquet association table,
	 * network information table
	 */
	status = get_sdts();

	/*
	 * channels from PAT do not have service_type set.
	 * some channels set the service_type in the BAT or the NIT.
	 * should the NIT be parsed on every transponder?
	 */
	std::map <t_channel_id, uint8_t>::iterator stI;
	for (stI = service_types.begin(); stI != service_types.end(); stI++)
	{
		tallchans_iterator scI = allchans.find(stI->first);

		if (scI != allchans.end())
		{
			if (scI->second.getServiceType() != stI->second)
			{
				INFO("setting service_type of channel_id " PRINTF_CHANNEL_ID_TYPE " from %02x to %02x",
					stI->first,
					scI->second.getServiceType(),
					stI->second);
						
				scI->second.setServiceType(stI->second);
			}
		}
	}
}

void *start_scanthread(void *)
{
	FILE *fd = NULL;
	FILE *fd1 = NULL;
	char providerName[32] = "";
	char *type = NULL;
	uint8_t diseqc_pos = 0;
	bool satfeed = false;
	int scan_status = -1;

	scanBouquetManager = new CBouquetManager();
	processed_transponders = 0;
 	found_tv_chans = 0;
 	found_radio_chans = 0;
 	found_data_chans = 0;
 	string prov =" ";


	curr_sat = 0;

        if ((type = getFrontendName()) == NULL)
	{
		WARN("unable to scan without a supported frontend");
		stop_scan(false);
		pthread_exit(0);
	}

	/* get first child */
	xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;
	xmlNodePtr transponder = NULL;

	std::map <uint8_t, std::string>::iterator spI;

	/* read all sat or cable sections */
	while ((search = xmlGetNextOccurence(search, type)) != NULL)
	{
		/* get name of current satellite oder cable provider */
		strcpy(providerName, xmlGetAttribute(search, "name"));

		/* look whether provider is wanted */
		for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
		{
			if (!strcmp(spI->second.c_str(), providerName))
				break;
		}

		/* provider is not wanted - jump to the next one */
		if (spI != scanProviders.end())
		{
			/* Special mode for cable-users with sat-feed */
			if (frontend->getInfo()->type == FE_QAM)
				if (!strcmp(type, "cable") && xmlGetAttribute(search, "satfeed"))
					if (!strcmp(xmlGetAttribute(search, "satfeed"), "true"))
						satfeed = true;

			/* increase sat counter */
			curr_sat++;

			/* copy services.xml to /tmp directory */
			cp(SERVICES_XML, SERVICES_TMP);
		
			if ((fd = fopen(SERVICES_XML, "w")))
			{
				if ((fd1 = fopen(SERVICES_TMP, "r")))
					copy_to_satellite(fd, fd1, providerName);
				else
					write_xml_header(fd, SERVICES_XML);
					
				/* satellite receivers might need diseqc */
				if (frontend->getInfo()->type == FE_QPSK)
					diseqc_pos = spI->first;
				
				/* position satellite dish if provider is on a different satellite */
				if ((frontend->getDiseqcType() == DISEQC_1_2) && (frontend->getCurrentSatellitePosition() != satellitePositions[prov]))
				{
					printf("[scan] start_scanthread: moving satellite dish from satellite position %d to %d\n", frontend->getCurrentSatellitePosition(), satellitePositions[prov]);
					frontend->positionMotor(diseqc_pos);
				}
						
				scan_provider(search, providerName, satfeed, diseqc_pos);
					
				/* write services */
				scan_status = write_provider(fd, type, providerName, diseqc_pos);
			
				if (fd1)		
					copy_to_end(fd, fd1, providerName);
					
				write_xml_footer(fd);
			}
			else
			{
				ERROR(SERVICES_XML);
				stop_scan(false);
				pthread_exit(0);
			}
		}

		/* go to next satellite */
		search = search->xmlNextNode;
	}

	/* clean up - should this be done before every xmlNextNode ? */
	delete transponder;
	delete search;

	/* write bouquets if services were found */
	if (scan_status != -1)
		write_bouquets();

	/* report status */
	INFO("found %d transponders and %d channels", found_transponders, found_channels);

	/* load new services */
	CZapitClient myZapitClient;
	myZapitClient.reinitChannels();

	stop_scan(true);
	pthread_exit(0);
}


