#define RECORD_TELETEXT
#include <lib/dvb/edvb.h>
#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAVE_DVB_API_VERSION < 3
#include <dbox/info.h>
#endif
#include <algorithm>
#include <string.h>

#ifdef PROFILE
	#include <sys/time.h>
#endif

#include <lib/driver/eavswitch.h>
#include <lib/driver/rfmod.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/si.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/record.h>
#ifndef DISABLE_CI
	#include <lib/dvb/dvbci.h>
#endif
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbscan.h>
#include <lib/dvb/service.h>

#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/econfig.h>

eDVBController::~eDVBController()
{
}

eDVB *eDVB::instance;

eString eDVB::getVersion()
{
	return "eDVB lib 1.0, compiled " __DATE__;
}

void eDVB::event(const eDVBEvent &event)
{
	eventOccured(event);
	if (controller)
		controller->handleEvent(event);
}

void eDVB::tunedIn(eTransponder *trans, int err)
{
	event(eDVBEvent(eDVBEvent::eventTunedIn, err, trans));
}

eDVB::eDVB()
	:parentEIT(0), recorder(0)
	#ifndef DISABLE_CI
	, DVBCI(0), DVBCI2(0)
	#endif
	,state(eDVBState::stateIdle)
{
	settings=0;
	time_difference=0;

		// singleton
	if (instance)
		eFatal("eDVB already initialized!");
	instance=this;

#ifndef DISABLE_CI
	int numCI = eSystemInfo::getInstance()->hasCI();
	if ( numCI > 0 )
	{
		DVBCI=new eDVBCI();
		DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::start));
	}
	if ( numCI > 1 )
	{
		DVBCI2=new eDVBCI();
		DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::start));
	}
#endif
	if (eFrontend::open(eSystemInfo::getInstance()->getFEType())<0)
		eFatal("couldn't open frontend");

	settings = new eDVBSettings(*this);
		// tuned-in handling
	CONNECT(eFrontend::getInstance()->tunedIn, eDVB::tunedIn);

		// decoder init .... already done!
//	Decoder::Initialize();
	
		// set to idle state
	controller=0;
	setState(eDVBState(eDVBState::stateIdle));

		// intialize to service mode
	setMode(controllerService);
		// init AV switch
	switch (eSystemInfo::getInstance()->getHwType())
	{
	case eSystemInfo::dbox2Philips:
		new eAVSwitchPhilips;
		break;
	case eSystemInfo::dbox2Sagem:
		new eAVSwitchSagem;
		break;
	case eSystemInfo::dbox2Nokia:
	default:
		new eAVSwitchNokia;
		break;
	}

#ifdef ENABLE_RFMOD
	if ( eSystemInfo::getInstance()->hasRFMod() )
	{
		new eRFmod;
		eRFmod::getInstance()->init();
	}
#endif

	eDebug("eDVB::eDVB done.");
}

eDVB::~eDVB()
{
	delete settings; 

#ifndef DISABLE_FILE
	recEnd();
#endif // DISABLE_FILE

	eAVSwitch::getInstance()->setActive(0);
	delete eAVSwitch::getInstance();

#ifdef ENABLE_RFMOD
	if(eRFmod::getInstance())
		delete eRFmod::getInstance();
#endif

	Decoder::Close();

	eFrontend::close();
	instance=0;
}

#ifndef DISABLE_FILE
void eDVB::recMessage(int msg)
{
	switch (msg)
	{
	case eDVBRecorder::recWriteError:
		event(eDVBEvent(eDVBEvent::eventRecordWriteError));
		break;
	}
}
#endif // DISABLE_FILE

		// for external access only
PMT *eDVB::getPMT()
{
	return tPMT.ready()?tPMT.getCurrent():0;
}

EIT *eDVB::getEIT()
{
	if ( tEIT.ready() )
		return tEIT.getCurrent();
	else if ( parentEIT )
	{
		parentEIT->lock();
		return parentEIT;
	}
	else
		return 0;
}

SDT *eDVB::getSDT()
{
	return tSDT.ready()?tSDT.getCurrent():0;
}

static void unpack(__u32 l, int *t)
{
	for (int i=0; i<4; i++)
		*t++=(l>>((3-i)*8))&0xFF;
}

void eDVB::configureNetwork()
{
	__u32 sip=0, snetmask=0, sdns=0, sgateway=0;
	int ip[4], netmask[4], dns[4], gateway[4];
	int sdosetup=0, maxmtu=0, samba=1;

	eConfig::getInstance()->getKey("/elitedvb/network/ip", sip);
	eConfig::getInstance()->getKey("/elitedvb/network/netmask", snetmask);
	eConfig::getInstance()->getKey("/elitedvb/network/dns", sdns);
	eConfig::getInstance()->getKey("/elitedvb/network/gateway", sgateway);
	eConfig::getInstance()->getKey("/elitedvb/network/dosetup", sdosetup);
	eConfig::getInstance()->getKey("/elitedvb/network/maxmtu", maxmtu);
	eConfig::getInstance()->getKey("/elitedvb/network/samba", samba);

	unpack(sip, ip);
	unpack(snetmask, netmask);
	unpack(sdns, dns);
	unpack(sgateway, gateway);

	if (sdosetup)
	{
		FILE *f=fopen("/etc/resolv.conf", "wt");
		if (!f)
			eDebug("couldn't write resolv.conf");
		else
		{
			fprintf(f, "# Generated by enigma\nnameserver %d.%d.%d.%d\n", dns[0], dns[1], dns[2], dns[3]);
			fclose(f);
		}
		eString buffer;
		buffer.sprintf("/sbin/ifconfig eth0 %d.%d.%d.%d up netmask %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3], netmask[0], netmask[1], netmask[2], netmask[3]);
		if (system(buffer.c_str())>>8)
			eDebug("'%s' failed.", buffer.c_str());
		else
		{
			system("/sbin/route del default 2> /dev/null");
			buffer.sprintf("/sbin/route add default gw %d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
			if (system(buffer.c_str())>>8)
				eDebug("'%s' failed", buffer.c_str());
			if (maxmtu != 0)
			{
				buffer.sprintf("/sbin/ifconfig eth0 mtu %d", maxmtu);
				printf("[EDVB] configureNetwork: setting MAXMTU to %d\n", maxmtu);
				if (system(buffer.c_str())>>8)
					eDebug("'%s' failed", buffer.c_str());
			}
		}
		system("killall nmbd");
		signal(SIGCHLD, SIG_IGN);
		if (samba != 0)
		{
			if (fork() == 0)
			{
				for (unsigned int i=3; i < 90; ++i )
					close(i);
				execlp("nmbd", "nmbd", "-D", NULL);

				_exit(0);
			}
		}
		system("killall smbd");
		signal(SIGCHLD, SIG_IGN);
		if (samba != 0)
		{
			if (fork() == 0)
			{
				for (unsigned int i=3; i < 90; ++i )
					close(i);
				execlp("smbd", "smbd", "-D", NULL);
				_exit(0);
			}
		}
#ifndef DISABLE_NFS		
		//NFS
		system("umount -a");
		for(int e=0;e<4;)
		{
		    int automount=0;
		    eString cmd,sdir,ldir,opt;
		    
		    cmd.sprintf("/elitedvb/network/nfs%d/",e++);
		    eConfig::getInstance()->getKey((cmd+"automount").c_str(), automount);
		    if(automount)
		    {
			__u32 sip = 0;
			char *cvalue  = 0;
			int ivalue = 0, fstype = 0;
			int de[4],a;
			a = 0;
			
			eConfig::getInstance()->getKey((cmd+"fstype").c_str(), fstype);
			eConfig::getInstance()->getKey((cmd+"sdir").c_str(), cvalue);
			eConfig::getInstance()->getKey((cmd+"options").c_str(), ivalue);
			eConfig::getInstance()->getKey((cmd+"ip").c_str(), sip);
			unpack(sip, de);
	
			if(fstype){
			    sdir.sprintf("//%d.%d.%d.%d/%s",de[0],de[1],de[2],de[3],cvalue);
			    eConfig::getInstance()->getKey((cmd+"username").c_str(), cvalue);
			    opt.sprintf("username=%s",cvalue);
    			    eConfig::getInstance()->getKey((cmd+"password").c_str(), cvalue);
			    opt.sprintf("%s,password=%s,unc=%s",opt.c_str(),cvalue,sdir.c_str());
			    if(ivalue>0)
				opt=opt+",";
			}
			else
			    sdir.sprintf("%d.%d.%d.%d:%s",de[0],de[1],de[2],de[3],cvalue);
			
			eConfig::getInstance()->getKey((cmd+"ldir").c_str(), cvalue);
			ldir.sprintf("%s",cvalue);
			
			switch(ivalue)
			{
			    case  1:opt=opt+"ro";break;
			    case  2:opt=opt+"rw";break;
			    case  3:opt=opt+"ro,nolock";break;
			    case  4:opt=opt+"rw,nolock";break;
			    case  5:opt=opt+"ro,soft";break;
			    case  6:opt=opt+"rw,soft";break;	
			    case  7:opt=opt+"ro,soft,nolock";break;
			    case  8:opt=opt+"rw,soft,nolock";break;
			    case  9:opt=opt+"ro,udp,nolock";break;
			    case 10:opt=opt+"rw,udp,nolock";break;
			    case 11:opt=opt+"ro,soft,udp";break;
			    case 12:opt=opt+"rw,soft,udp";break;
			    case 13:opt=opt+"ro,soft,udp,nolock";break;
			    case 14:opt=opt+"rw,soft,udp,nolock";break;	
			    default:
			     break;
			}
			
			eConfig::getInstance()->getKey((cmd+"extraoptions").c_str(), cvalue);
			opt.sprintf("%s,%s",opt.c_str(),cvalue);
			       
			signal(SIGCHLD, SIG_IGN);
			if (fork() == 0)
			{
			    for (unsigned int i=3; i < 90; ++i )
				close(i);
			    if(fstype)
				execlp("busybox", "mount", "-t", "cifs", sdir.c_str(), ldir.c_str(), "-o", opt.c_str(), NULL);
			    else
				execlp("busybox", "mount", "-t", "nfs", sdir.c_str(), ldir.c_str(), "-o", opt.c_str(), NULL);
			    _exit(0);	
			}
		    }//automount
		}//NFS mount
#endif
	} //sdosetup
}

#ifndef DISABLE_FILE
void eDVB::recBegin(const char *filename, eServiceReferenceDVB service)
{
	if (recorder)
		recEnd();

	PMT *pmt=getPMT();
	PAT *pat=tPAT.ready()?tPAT.getCurrent():0;
	recorder=new eDVBRecorder(pmt, pat);
	if ( pat )
		pat->unlock();
	recorder->recRef=(eServiceReferenceDVB&)eServiceInterface::getInstance()->service;

	eServiceHandler *handler = eServiceInterface::getInstance()->getService();

	recorder->scrambled = handler->getFlags() & eServiceHandler::flagIsScrambled;

	recorder->open(filename);

	CONNECT(recorder->recMessage, eDVB::recMessage);

//	recorder->addNewPID(0); // PAT

//	if (Decoder::parms.pmtpid != -1)
//		recorder->addNewPID(Decoder::parms.pmtpid);

	if (!pmt)
	{
	        if (Decoder::parms.apid != -1)
			recorder->addNewPID(Decoder::parms.apid);
		if (Decoder::parms.vpid != -1)
			recorder->addNewPID(Decoder::parms.vpid);
		if (Decoder::parms.tpid != -1)
			recorder->addNewPID(Decoder::parms.tpid);
		if (Decoder::parms.pcrpid != -1)
			recorder->addNewPID(Decoder::parms.pcrpid);
	}
	else
	{
		recorder->addNewPID(pmt->PCR_PID);
#ifdef RECORD_ECM
		for (ePtrList<Descriptor>::iterator i(pmt->program_info.begin()); i != pmt->program_info.end(); ++i)
			if (i->Tag() == 9)
				recorder->addNewPID(((CADescriptor*)*i)->CA_PID);
#endif
		for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
		{
			int record=0;
			switch (i->stream_type)
			{
			case 1:	// video..
			case 2:
				record=1;
				break;
			case 3:	// audio..
			case 4:
				record=1;
				break;
			case 6:
				for (ePtrList<Descriptor>::iterator it(i->ES_info); it != i->ES_info.end(); ++it)
				{
					if (it->Tag() == DESCR_AC3)
						record=1;
#ifdef RECORD_TELETEXT
					if (it->Tag() == DESCR_TELETEXT)
						record=1;
#endif
				}
				break;
			}
			if (record)
				recorder->addNewPID(i->elementary_PID);
#ifdef RECORD_ECM
			for (ePtrList<Descriptor>::iterator it(i->ES_info); it != i->ES_info.end(); ++it)
				if (it->Tag() == 9)
					recorder->addNewPID(((CADescriptor*)*it)->CA_PID);
#endif
		}
		pmt->unlock();
	}

	// build SMI table.
	
	// build SIT:
	__u8 sit[4096];
	int pos=0;
	sit[pos++]=0x7F;              // TID
	sit[pos++]=0x80;              // section_syntax_indicator, length
	sit[pos++]=0;                 // length
	sit[pos++]=0; sit[pos++]=0;   // reserved
	sit[pos++]=1;                 // reserved, version number, current/next indicator
	sit[pos++]=0;                 // section number
	sit[pos++]=0;                 // last section number
	sit[pos++]=0;                 // loop length
	sit[pos++]=0;                 // "    "
	int loop_pos=pos;
		// TODO: build Partitial Transport Stream descriptor containing valid values about rate
	sit[loop_pos-2]|=(pos-loop_pos)>>8;
	sit[loop_pos-1]|=(pos-loop_pos)&0xFF;
	
	// create one single entry: our service...
	sit[pos++]=service.getServiceID().get()>>8;
	sit[pos++]=service.getServiceID().get()&0xFF;
	sit[pos++]=3<<4;              // running state
	sit[pos++]=0;
	loop_pos=pos;

	// create our special descriptor:
	sit[pos++]=0x80;              // private	
	sit[pos++]=0;
	int descr_pos=pos;
	memcpy(sit+pos, "ENIGMA", 6);
	pos+=6;

	if (Decoder::parms.vpid != -1)
	{
		sit[pos++]=eServiceDVB::cVPID;
		sit[pos++]=2;
		sit[pos++]=Decoder::parms.vpid>>8;
		sit[pos++]=Decoder::parms.vpid&0xFF;
	}

	if (Decoder::parms.apid != -1)
	{
		sit[pos++]=eServiceDVB::cAPID;
		sit[pos++]=3;
		sit[pos++]=Decoder::parms.apid>>8;
		sit[pos++]=Decoder::parms.apid&0xFF;
		sit[pos++]=Decoder::parms.audio_type;
	}
	
	if (Decoder::parms.tpid != -1)
	{
		sit[pos++]=eServiceDVB::cTPID;
		sit[pos++]=2;
		sit[pos++]=Decoder::parms.tpid>>8;
		sit[pos++]=Decoder::parms.tpid&0xFF;
	}

	if (Decoder::parms.pcrpid != -1)
	{
		sit[pos++]=eServiceDVB::cPCRPID;
		sit[pos++]=2;
		sit[pos++]=Decoder::parms.pcrpid>>8;
		sit[pos++]=Decoder::parms.pcrpid&0xFF;
	}

	sit[descr_pos-2]|=(pos-descr_pos)>>8;
	sit[descr_pos-1]|=(pos-descr_pos)&0xFF;

		// fix lengths
	sit[loop_pos-2]|=(pos-loop_pos)>>8;
	sit[loop_pos-1]|=(pos-loop_pos)&0xFF;
	
		// add CRC (calculate later)
	sit[pos++]=0;
	sit[pos++]=0;
	sit[pos++]=0;
	sit[pos++]=0;

		// fix length
	sit[1]|=(pos-3)>>8;
	sit[2]|=(pos-3)&0xFF;

	// generate CRC :)

	unsigned int cc=0;
	// write section.
	recorder->writeSection(sit, 0x1f, cc);

	recorder->validatePIDs();
}

void eDVB::recPause()
{
	recorder->stop();
}

void eDVB::recResume()
{
	recorder->start();
}

void eDVB::recEnd()
{
	if (!recorder)
		return;
	delete recorder;
	recorder=0;
}
#endif //DISABLE_FILE

void eDVB::setMode(int mode)
{
	if (controllertype == mode)
		return;
	controllertype = mode;
	if (controller)
		delete controller;
	switch (mode)
	{
	case controllerScan:
		controller = new eDVBScanController(*this);
		break;
	case controllerService:
		controller = new eDVBServiceController(*this);
#ifndef DISABLE_CI
		if ( DVBCI )
			DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getcaids));
		if ( DVBCI2 )
			DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getcaids));
#endif
	break;
	}
}

eDVBServiceController *eDVB::getServiceAPI()
{
	if (controllertype != controllerService)
		return 0;
	return (eDVBServiceController*)controller;
}

eDVBScanController *eDVB::getScanAPI()
{
	if (controllertype != controllerScan)
		return 0;
	return (eDVBScanController*)controller;
}

eAutoInitP0<eDVB> init_dvb(eAutoInitNumbers::dvb, "eDVB lib");
