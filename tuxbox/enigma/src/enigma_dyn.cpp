#include <enigma_dyn.h>

#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#include <enigma.h>
#include <timer.h>
#include <enigma_main.h>
#include <enigma_plugins.h>
#include <enigma_standby.h>
#include <sselect.h>

#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/dvb/record.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/gdi/fb.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/epng.h>
#include <lib/gui/emessage.h>
#include <lib/system/http_dyn.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>

// #include <lib/dvr/dvrsocket.h>

#define TEMPLATE_DIR DATADIR+eString("/enigma/templates/")
#define CHARSETMETA "<META http-equiv=Content-Type content=\"text/html; charset=UTF-8\">\n"

#define DELETE(WHAT) result.strReplace(#WHAT, "")

static eString getRight(const eString& str, char c )
{
	unsigned int found = str.find(c);
	unsigned int beg = ( found != eString::npos ? found : 0 );
	unsigned int len = str.length();
	if ( found != eString::npos )
		beg++;
	return str.mid( beg, len-beg );
}

static eString getLeft( const eString& str, char c )
{
	unsigned int found = str.find(c);
	return found != eString::npos ? str.left(found):str;
}

static eString getVersionInfo(const char *info)
{
	FILE *f=fopen("/.version", "rt");
	if (!f)
		return "";
	eString result;
	while (1)
	{
		char buffer[128];
		if (!fgets(buffer, 128, f))
			break;
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;
		if ((!strncmp(buffer, info, strlen(info)) && (buffer[strlen(info)]=='=')))
		{
			int i = strlen(info)+1;
			result = eString(buffer).mid(i, strlen(buffer)-i);
			break;
		}
	}
	fclose(f);
	return result;
}

static int getHex(int c)
{
	c=toupper(c);
	c-='0';
	if (c<0)
		return -1;
	if (c > 9)
		c-='A'-'0'-10;
	if (c > 0xF)
		return -1;
	return c;
}

eString httpUnescape(const eString &string)
{
	eString ret;
	for (unsigned int i=0; i<string.length(); ++i)
	{
		int c=string[i];
		switch (c)
		{
		case '%':
		{
			int val='%';
			if ((i+1) < string.length())
				val=getHex(string[++i]);
			if ((i+1) < string.length())
			{
				val<<=4;
				val+=getHex(string[++i]);
			}
			ret+=val;
			break;
		}
		case '+':
			ret+=' ';
			break;
		default:
			ret+=c;
			break;
		}
	}
	return ret;
}

eString httpEscape(const eString &string)
{
	eString ret;
	for (unsigned int i=0; i<string.length(); ++i)
	{
		int c=string[i];
		int valid=0;
		if ((c >= 'a') && (c <= 'z'))
			valid=1;
		else if ((c >= 'A') && (c <= 'Z'))
			valid=1;
		else if (c == ':')
			valid=1;
		else if ((c >= '0') && (c <= '9'))
			valid=1;
		else
			valid=0;

		if (valid)
			ret+=c;
		else
			ret+=eString().sprintf("%%%x", c);
	}
	return ret;
}

std::map<eString,eString> getRequestOptions(eString opt)
{
	std::map<eString,eString> result;

	if (opt[0]=='?')
		opt=opt.mid(1);

	while (opt.length())
	{
		unsigned int e=opt.find("=");
		if (e==eString::npos)
			e=opt.length();
		unsigned int a=opt.find("&", e);
		if (a==eString::npos)
			a=opt.length();
		eString n=opt.left(e);

		unsigned int b=opt.find("&", e+1);
		if(b==eString::npos)
			b=(unsigned)-1;
		eString r=httpUnescape(opt.mid(e+1, b-e-1));
		result.insert(std::pair<eString, eString>(n, r));
		opt=opt.mid(a+1);
	}
	return result;
}

static eString doStatus(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString result;
	time_t atime;
	time(&atime);
	atime+=eDVB::getInstance()->time_difference;
	result="<html>\n"
		CHARSETMETA
		"<head>\n"
		"  <title>enigma status</title>\n"
		"  <link rel=stylesheet type=\"text/css\" href=\"/index.css\">\n"
		"</head>\n"
		"<body>\n"
		"<h1>Enigma status</h1>\n"
		"<table>\n"
		"<tr><td>current time:</td><td>" + eString(ctime(&atime)) + "</td></tr>\n"
		"</table>\n"
		"</body>\n"
		"</html>\n";
	return result;
}

static eString switchService(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eDebug("[ENIGMA_DYN] switchService...");
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	int service_id=-1, dvb_namespace=-1, original_network_id=-1, transport_stream_id=-1, service_type=-1;
	unsigned int optval=opt.find("=");
	if (optval!=eString::npos)
		opt=opt.mid(optval+1);
	if (opt.length())
		sscanf(opt.c_str(), "%x:%x:%x:%x:%x", &service_id, &dvb_namespace, &transport_stream_id, &original_network_id, &service_type);

	eString result;

	if ((service_id!=-1) && (original_network_id!=-1) && (transport_stream_id!=-1) && (service_type!=-1))
	{
		eServiceInterface *iface=eServiceInterface::getInstance();
		if(!iface)
			return "-1";
		eServiceReferenceDVB *ref=new eServiceReferenceDVB(eDVBNamespace(dvb_namespace), eTransportStreamID(transport_stream_id), eOriginalNetworkID(original_network_id), eServiceID(service_id), service_type);
#ifndef DISABLE_FILE
		if ( eDVB::getInstance()->recorder && !ref->path )
		{
			int canHandleTwoScrambledServices=0;
			eConfig::getInstance()->getKey("/ezap/ci/handleTwoServices",
				canHandleTwoScrambledServices);

			if ( !canHandleTwoScrambledServices && eDVB::getInstance()->recorder->scrambled )
			{
				delete ref;
				return "-1";
			}
			eServiceReferenceDVB &rec = eDVB::getInstance()->recorder->recRef;
			if ( ref->getTransportStreamID() != rec.getTransportStreamID() ||
					ref->getOriginalNetworkID() != rec.getOriginalNetworkID() ||
					ref->getDVBNamespace() != rec.getDVBNamespace() )
			{
				delete ref;
				return "-1";
			}
		}
#endif
		eZapMain::getInstance()->playService(*ref, eZapMain::psSetMode|eZapMain::psDontAdd);
		delete ref;
		result="0";
	} else
	{
		result+="-1";
	}
	return result;
}

static eString admin(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString command=opt["command"];
	if (command)
	{
		if ( command=="shutdown")
		{
			eZap::getInstance()->quit();
			return "<html>" CHARSETMETA "<head><title>Shutdown</title></head><body>Shutdown initiated.</body></html>\n";
		}
		else if ( command=="reboot")
		{
			eZap::getInstance()->quit(4);
			return "<html>" CHARSETMETA "<head><title>Reboot</title></head><body>Reboot initiated.</body></html>\n";
		}
		else if ( command=="restart")
		{
			eZap::getInstance()->quit(2);
			return "<html>" CHARSETMETA "<head><title>Restart Enigma</title></head><body>Restart of enigma is initiated.</body></html>\n";
		}
		else if ( command=="wakeup")
		{
			if ( eZapStandby::getInstance() )
			{
				eZapStandby::getInstance()->wakeUp(0);
				return "<html>" CHARSETMETA "<head><title>Wakeup</title></head><body>enigma is waking up.</body></html>\n";
			}
			return "<html>" CHARSETMETA "<head><title>Wakeup</title></head><body>enigma doesn't sleep :)</body></html>\n";
		}
		else if ( command=="standby" )
		{
			if ( eZapStandby::getInstance() )
				return "<html>" CHARSETMETA "<head><title>Standby</title></head><body>enigma is already sleeping</body></html>\n";
			eZapMain::getInstance()->gotoStandby();
			return "<html>" CHARSETMETA "<head><title>Standby</title></head><body>enigma is sleeping now</body></html>\n";
		}
	}
	return "<html>" CHARSETMETA "<head><title>Error</title></head><body>Unknown admin command.(valid commands are: shutdown, reboot, restart, standby, wakeup) </body></html>\n";
}

static eString audio(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString result;
	eString volume=opt["volume"];
	if (volume)
	{
		int vol=atoi(volume.c_str());
		eAVSwitch::getInstance()->changeVolume(1, vol);
		result+="Volume set.<br>\n";
	}
	eString mute=opt["mute"];
	if (mute)
	{
		eAVSwitch::getInstance()->toggleMute();
		result+="mute set<br>\n";
	}
	result+=eString().sprintf("volume: %d<br>\nmute: %d<br>\n", eAVSwitch::getInstance()->getVolume(), eAVSwitch::getInstance()->getMute());
	return result;
}

static eString getPMT(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	//"x-application/PMT";
	PMT *pmt=eDVB::getInstance()->getPMT();
	if (!pmt)
		return "result=ERROR\n";
	eString res="result=OK\n";
	res+="PMT"+eString().sprintf("(%04x)\n", pmt->pid);
	res+="program_number="+eString().sprintf("%04x\n", pmt->program_number);
	res+="PCR_PID="+eString().sprintf("%04x\n", pmt->PCR_PID);
	res+="program_info\n";
	for (ePtrList<Descriptor>::iterator d(pmt->program_info); d != pmt->program_info.end(); ++d)
		res+=d->toString();
	for (ePtrList<PMTEntry>::iterator s(pmt->streams); s != pmt->streams.end(); ++s)
	{
		res+="PMTEntry\n";
		res+="stream_type="+eString().sprintf("%02x\n", s->stream_type);
		res+="elementary_PID="+eString().sprintf("%04x\n", s->elementary_PID);
		res+="ES_info\n";
		for (ePtrList<Descriptor>::iterator d(s->ES_info); d != s->ES_info.end(); ++d)
			res+=d->toString();
	}
	pmt->unlock();
	return res;
}

static eString getEIT(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	//"x-application/PMT";
	EIT *eit=eDVB::getInstance()->getEIT();
	if (!eit)
		return "result=ERROR\n";
	eString res="result=OK\n";
	res+="EIT"+eString().sprintf("(%04x)\n", eit->service_id);
	res+="original_network_id="+eString().sprintf("%04x\n", eit->original_network_id);
	res+="transport_stream_id="+eString().sprintf("%04x\n", eit->transport_stream_id);
	res+="events\n";
	for (ePtrList<EITEvent>::iterator s(eit->events); s != eit->events.end(); ++s)
	{
		res+="EITEvent\n";
		res+="event_id="+eString().sprintf("%04x\n", s->event_id);
		res+="duration="+eString().sprintf("%04x\n", s->start_time);
		res+="duration="+eString().sprintf("%04x\n", s->duration);
		res+="running_status="+eString().sprintf("%d\n", s->running_status);
		res+="free_CA_mode="+eString().sprintf("%d\n", s->free_CA_mode);
		res+="descriptors\n";
		for (ePtrList<Descriptor>::iterator d(s->descriptor); d != s->descriptor.end(); ++d)
			res+=d->toString();
	}
	eit->unlock();
	return res;
}

static eString version(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain";
	eString result;
	result.sprintf("enigma");
//	result.sprintf("EliteDVB Version : %s\r\n, eZap Version : doof\r\n",eDVB::getInstance()->getVersion().c_str());
	return result;
}

static eString channels_getcurrent(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	content->local_header["Content-Type"]="text/plain; charset=utf-8";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "-1";

	eServiceDVB *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if(current)
		return current->service_name.c_str();
	else
		return "-1";
}

static eString getVolume()
{
	return eString().setNum((63-eAVSwitch::getInstance()->getVolume())*100/63, 10);
}

static eString setVolume(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString mute="0";
	eString volume;
	eString result;
	int mut=0, vol=0;

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	result+="<script language=\"javascript\">window.close();</script>";
	mute=opt["mute"];
	volume=opt["volume"];

	if(!mute) {
		mut=0;
	} else {
		eAVSwitch::getInstance()->toggleMute();
		result+="[mute OK]";
		return result;
	}

	if(volume) {
		vol=atoi(volume.c_str());
	} else {
		result+="[no params]";
		return result;
	}
	if(vol>10) vol=10;
	if(vol<0) vol=0;

	float temp=(float)vol;
	temp=temp*6.3;
	vol=(int)temp;

	eAVSwitch::getInstance()->changeVolume(1, 63-vol);
	result+="[volume OK]";

	return result;
}

eString read_file(eString filename)
{
#define BLOCKSIZE 8192
	int fd;
	fd=open(filename.c_str(), O_RDONLY);
	if(!fd)
		return eString("file: "+filename+" not found\n");

	int size=0;

	char tempbuf[BLOCKSIZE+200];
	eString result("");

	while((size=read(fd, tempbuf, BLOCKSIZE))>0)
	{
		tempbuf[size]=0;
		result+=eString(tempbuf);
	}
	return result;
}

eString ref2string(const eServiceReference &r)
{
	return httpEscape(r.toString());
}

eServiceReference string2ref(const eString &service)
{
	eString str=httpUnescape(service);
	return eServiceReference(str);
}

static eString getIP()
{
	eString tmp;
	int sd;
	struct ifreq ifr;
	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if(sd<0)
	{
		return "?.?.?.?-socket-error";
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family=AF_INET; // fixes problems with some linux vers.
	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
	if(ioctl(sd, SIOCGIFADDR, &ifr) < 0)
	{
		return "?.?.?.?-ioctl-error";
	}
	close(sd);

	tmp.sprintf("%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr) );
	return tmp;
}

eString filter_string(eString string)
{
	return string.
		removeChars('\x86').
		removeChars('\x87').
		removeChars('\xC2').
		removeChars('\x05');
}

#ifndef DISABLE_FILE
extern int freeRecordSpace(void);

static eString getDiskSpace(void)
{
	eString result = "";
	eString tmp;
	result = "<span class=\"white\">";
	result += "Remaining Disk Space: ";
	int fds = freeRecordSpace();
	if (fds != -1)
	{
		if (fds < 1024)
			tmp.sprintf("%d MB", fds);
		else
			tmp.sprintf("%d.%02d GB", fds/1024, (int)((fds%1024)/10.34));
		result += tmp;
		result += "/";
		int min = fds/33;
		if (min < 60)
			tmp.sprintf("~%d min", min);
		else
			tmp.sprintf("~%dh%02dmin", min/60, min%60);
		result += tmp;
	}
	else
		result += "unknown";

	result += "</span>";
	return result;
}
#endif

static eString getStats()
{
	eString result = "";
	eString apid, vpid;
	int bootcount = 0;

	result +="<span class=\"white\">";
#ifndef DISABLE_FILE
	result += getDiskSpace();
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";
#endif
	int sec = atoi(read_file("/proc/uptime").c_str());
	result += eString().sprintf("%d:%02dh up", sec/3600, (sec%3600)/60);
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	result += getIP();
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	eConfig::getInstance()->getKey("/elitedvb/system/bootCount", bootcount);
	result += eString().sprintf("booted %d times", bootcount);
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	vpid = (Decoder::current.vpid == -1) ? "none" : vpid.sprintf("0x%x", Decoder::current.vpid);
	result += "vpid: " + vpid;
	result += "&nbsp;<img src=\"squ.png\">&nbsp;";

	apid = (Decoder::current.apid == -1) ? "none" : apid.sprintf("0x%x", Decoder::current.apid);
	result += "<u><a href=\"/audio.m3u\">apid: " + apid + "</a></u>";

	result += "</span>";

	return result;
}

static eString getVolBar()
{
// returns the volumebar
	eString result;
	int volume=atoi(getVolume().c_str());

	result+="<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\">";
	result+="<tr>";

	for(int i=1;i<=(volume/10);i++)
	{
		result+="<td width=15 height=8><a class=\"volgreen\" href=\"javascript:setVol(";
		result+=eString().setNum(i, 10);
		result+=")\"><img src=\"trans.gif\" border=0></a></span></td>";
	}
	for(int i=(volume/10)+1;i<=(100/10);i++)
	{
		result+="<td width=15 height=8><a class=\"volnot\" href=\"javascript:setVol(";
		result+=eString().setNum(i, 10);
		result+=")\"><img src=\"trans.gif\" border=0></a></span></td>";
	}

	result+="<td>";

	if(eAVSwitch::getInstance()->getMute()==1) {
		result+="<a class=\"mute\" href=\"javascript:Mute()\">";
		result+="<img src=\"speak_off.gif\" border=0>";
	} else {
		result+="<a class=\"mute\" href=\"javascript:unMute()\">";
		result+="<img src=\"speak_on.gif\" border=0>";
	}
	result+="</a></td>";
	result+="</tr>";
	result+="</table>";
	return result;
}

class eWebNavigatorListDirectory: public Object
{
	eString &result;
	eString origpath;
	eString path;
	eServiceInterface &iface;
	int num;
public:
	eWebNavigatorListDirectory(eString &result, eString origpath, eString path, eServiceInterface &iface): result(result), origpath(origpath), path(path), iface(iface)
	{
		eDebug("path: %s", path.c_str());
		num=0;
	}
	void addEntry(const eServiceReference &e)
	{
#ifndef DISABLE_FILE
		if ( eDVB::getInstance()->recorder && !e.path && !e.flags )
		{
			eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
			eServiceReferenceDVB &rec = eDVB::getInstance()->recorder->recRef;
			if ( rec.getTransportStreamID() != ref.getTransportStreamID() ||
					 rec.getOriginalNetworkID() != ref.getOriginalNetworkID() ||
					 rec.getDVBNamespace() != ref.getDVBNamespace() )
					 return;
		}
#endif
		result+="<tr><td bgcolor=\"#";
		if (num & 1)
			result += "c0c0c0";
		else
			result += "d0d0d0";
		result+="\"><font color=\"#000000\">";
		if (!(e.flags & eServiceReference::isDirectory))
		{
			result += "<a href=\'javascript:openEPG(\"" + ref2string(e) + "\")\'>[EPG]</a>";
			result += "<a href=\"/?path=" + ref2string(e) + "\">[PLAY]</a> ";
		}
		else
			result+=eString("<a href=\"/")+ "?path=" + ref2string(e) + "\">";

		eService *service=iface.addRef(e);
		if (!service)
			result += "N/A";
		else
		{
			result += filter_string(service->service_name);
			iface.removeRef(e);
		}

		if (e.flags & eServiceReference::isDirectory)
			result += "</a>";

		result+="</font></td></tr>\n";
		num++;
	}
};

static eString getZapContent(eString mode, eString path)
{
	eString result("");
	eString tpath;

	int pos=0, lastpos=0, temp=0;

	if((path.find(";", 0))==(unsigned)-1)
		path=";"+path;

	while ((pos=path.find(";", lastpos))!=-1)
	{
		lastpos=pos+1;
		if ((temp=path.find(";", lastpos))!=-1)
		{
			tpath=path.mid(lastpos, temp-lastpos);
		}
		else
		{
			tpath=path.mid(lastpos, strlen(path.c_str())-lastpos);
		}

		eServiceReference current_service=string2ref(tpath);
		eServiceInterface *iface=eServiceInterface::getInstance();

		if (!(current_service.flags&eServiceReference::isDirectory))	// is playable
		{
			eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);
//			iface->play(current_service);
			result += "Done. Please select ZAP to refresh screen.";
		}
		else
		{
			eWebNavigatorListDirectory navlist(result, path, tpath, *iface);
			Signal1<void,const eServiceReference&> signal;
			signal.connect(slot(navlist, &eWebNavigatorListDirectory::addEntry));
				result+="<table width=\"100%%\">\n";
			iface->enterDirectory(current_service, signal);
				result+="</table>\n";
			eDebug("entered");
			iface->leaveDirectory(current_service);
			eDebug("exited");
		}
	}

	return result;
}

static eString aboutDreambox(void)
{
	eString result("");
	result += "<table border=0>";

	switch (eSystemInfo::getInstance()->getHwType())
	{
		case eSystemInfo::dbox2Nokia:
			result += "<tr><td>Model:</td><td>&nbsp;</td><td>d-Box 2</td></tr>";
			result += "<tr><td>Manufacturer:</td><td>&nbsp;</td><td>Nokia</td></tr>";
			result += "<tr><td>Processor:</td><td>&nbsp;</td><td>XPC823, 66MHz</td></tr>";
			break;
		case eSystemInfo::dbox2Philips:
			result += "<tr><td>Model:</td><td>&nbsp;</td><td>d-Box 2</td></tr>";
			result += "<tr><td>Manufacturer</td><td>&nbsp;</td><td>Philips</td></tr>";
			result += "<tr><td>Processor</td><td>&nbsp;</td><td>XPC823, 66MHz</td></tr>";
			break;
		case eSystemInfo::dbox2Sagem:
			result += "<tr><td>Model:</td><td>&nbsp;</td><td>d-Box 2</td></tr>";
			result += "<tr><td>Manufacturer:</td><td>&nbsp;</td><td>Sagem</td></tr>";
			result += "<tr><td>Processor</td><td>&nbsp;</td><td>XPC823, 66MHz</td></tr>";
			break;
		case eSystemInfo::DM5600:
			result += "<tr><td>Model:</td><td>&nbsp;</td><td>DM5600</td></tr>";
			result += "<tr><td>Manufacturer</td><td>&nbsp;</td><td>Dream-Multimedia-TV</td></tr>";
			result += "<tr><td>Processor:</td><td>&nbsp;</td><td>STBx25xx, 252MHz</td></tr>";
			break;
		case eSystemInfo::DM5620:
			result += "<tr><td>Model:</td><td>&nbsp;</td><td>DM5620</td></tr></td></tr>";
			result += "<tr><td>Manufacturer:</td><td>&nbsp;</td><td>Dream-Multimedia-TV</td></tr>";
			result += "<tr><td>Processor:</td><td>&nbsp;</td><td>STBx25xx, 252MHz</td></tr>";
			break;
		case eSystemInfo::DM7000:
			result += "<tr><td>Model:</td><td>&nbsp;</td><td>DM7000</td></tr>";
			result += "<tr><td>Manufacturer:</td><td>&nbsp;</td><td>Dream-Multimedia-TV</td></tr>";
			result += "<tr><td>Processor:</td><td>&nbsp;</td><td>STB04500, 252MHz</td></tr>";
			break;
	}

	switch (eSystemInfo::getInstance()->getFEType())
	{
		case eSystemInfo::feSatellite:
			result += "<tr><td>Frontend:</td><td>&nbsp;</td><td>Satellite</td></tr>";
			break;
		case eSystemInfo::feCable:
			result += "<tr><td>Frontend:</td><td>&nbsp;</td><td>Cable</td></tr>";
			break;
		case eSystemInfo::feTerrestrial:
			result += "<tr><td>Frontend:</td><td>&nbsp;</td><td>Terrestrial</td></tr>";
			break;
	}

	eString sharddisks;
#ifndef DISABLE_FILE
	for (int c='a'; c<'h'; c++)
	{
		char line[1024];
		int ok=1;
		FILE *f=fopen(eString().sprintf("/proc/ide/hd%c/media", c).c_str(), "r");
		if (!f)
			continue;
		if ((!fgets(line, 1024, f)) || strcmp(line, "disk\n"))
			ok=0;
		fclose(f);
		if (ok)
		{
			FILE *f=fopen(eString().sprintf("/proc/ide/hd%c/model", c).c_str(), "r");
			if (!f)
				continue;
			*line=0;
			fgets(line, 1024, f);
			fclose(f);
			if (!*line)
				continue;
			line[strlen(line)-1]=0;
			sharddisks+=line;
			f=fopen(eString().sprintf("/proc/ide/hd%c/capacity", c).c_str(), "r");
			if (!f)
				continue;
			int capacity=0;
			fscanf(f, "%d", &capacity);
			fclose(f);
			sharddisks+=" (";
			if (c&1)
				sharddisks+="master";
			else
				sharddisks+="slave";
			if (capacity)
				sharddisks+=eString().sprintf(", %d MB", capacity/2048);
			sharddisks+=")";
		}
	}

	result += "<tr><td>Harddisk:</td><td>&nbsp;</td><td>";
	if (sharddisks == "")
		sharddisks="none</td></tr>";
	result += sharddisks;
#endif //DISABLE_FILE

	result += "<tr><td>Firmware:</td><td>&nbsp;</td><td>";
	eString verid=getVersionInfo("version");
	if (!verid)
	{
		result += "unknown</td></tr>";
	}
	else
	{
		int type=atoi(verid.left(1).c_str());
		char *typea[3];
		typea[0]="release";
		typea[1]="beta";
		typea[2]="internal";
		eString ver=verid.mid(1, 3);
		eString date=verid.mid(4, 8);
//		eString time=verid.mid(12, 4);
		if (eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM5600
			|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM5620)
			result += eString(typea[type%3]) + eString(" ") + ver[0] + "." + ver[1] + "." + ver[2]+ ", " + date.mid(6, 2) + "." + date.mid(4, 2) + "." + date.left(4);
		else
			result += eString().sprintf("%s %c.%d. %s", typea[type%3], ver[0], atoi(eString().sprintf("%c%c", ver[1], ver[2]).c_str()	), (date.mid(6, 2) + "." + date.mid(4, 2) + "." + date.left(4)).c_str());
		result += "</td></tr>";
	}
	result += "</table>";

	return result;
}

static eString getCurService()
{
	eString result;

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "n/a";

	eService *current=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if(current)
		return current->service_name.c_str();
	else
		return "n/a";
}

struct getEntryString: public std::unary_function<ePlaylistEntry*, void>
{
	eString &result;
	eString begin;
	eString end;

	getEntryString(eString &result): result(result)
	{
	}

	void operator()(ePlaylistEntry* se)
	{
		result += "<tr>";
		begin = "";
		end = "";
		tm startTime = *localtime(&se->time_begin);
		time_t time_end = se->time_begin + se->duration;
		tm endTime = *localtime(&time_end);
		begin.sprintf("%02d.%02d.- %02d:%02d", startTime.tm_mday, startTime.tm_mon+1, startTime.tm_hour, startTime.tm_min);
		end.sprintf("%02d.%02d.- %02d:%02d", endTime.tm_mday, endTime.tm_mon+1, endTime.tm_hour, endTime.tm_min);
		if (se->type & ePlaylistEntry::stateFinished)
			result += "<td align=center><img src=\"on.gif\"></td>";
		else
		if (se->type & (ePlaylistEntry::errorNoSpaceLeft |
				ePlaylistEntry::errorUserAborted |
				ePlaylistEntry::errorZapFailed|
				ePlaylistEntry::errorOutdated))
			result += "<td align=center><img src=\"off.gif\"></td>";
		else
			result += "<td>&nbsp;</td>";

		result += "<td>" + begin + "</td>";
		result += "<td>" + end + "</td>";

		eString description = se->service.descr;
		eString channel = getLeft(description, '/');
		if (!channel)
			channel = "No channel available";
		result += "<td>" + channel + "</td>";
		description = getRight(description, '/');
		if (!description)
			description = "No description available";
		result += "<td>" + description + "</td>";
		result += "</tr>";
	}
};

static eString genTimerListBody(void)
{
	eString result = "";
	eString tbody = "";
	eString tmpFile = "";
	eTimerManager::getInstance()->forEachEntry(getEntryString(tbody));
	if (tbody == "")
		result = "No timer events available.<br>";
	else
	{
		result += "<table width=100% border=1 rules=all>";
		result += "<thead>";
		result += "<th align=\"left\">";
		result += "Status";
		result += "</th>";
		result += "<th align=\"left\">";
		result += "Start Time";
		result += "</th>";
		result += "<th align=\"left\">";
		result += "End Time";
		result += "</th>";
		result += "<th align=\"left\">";
		result += "Channel";
		result += "</th>";
		result += "<th align=\"left\">";
		result += "Description";
		result += "</th>";
		result += "</thead>";
		result += "<tbody>";
		result += tbody;
		result += "</tbody>";
		result += "</table>";
	}
	return result;
}

static eString showTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result = "";
	eString tmpFile = "";
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	result = genTimerListBody();
	tmpFile += read_file(TEMPLATE_DIR + "timerList.tmp");
	tmpFile.strReplace("#BODY#", result);
	return tmpFile;
}

static eString getEITC()
{
	eString result;

	EIT *eit=eDVB::getInstance()->getEIT();

	if(eit)
	{
		eString now_time, now_duration, now_text, now_longtext;
		eString next_time, next_duration, next_text, next_longtext;

		int p=0;

		for(ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
		{
			if(*event)
			{
				if(p==0)
				{
					if(event->start_time!=0) {
						now_time.sprintf("%s", ctime(&event->start_time));
						now_time=now_time.mid(10, 6);
					} else {
						now_time="";
					}
					now_duration.sprintf("%d", (int)(event->duration/60));
				}
				if(p==1)
				{
					if(event->start_time!=0) {
 						next_time.sprintf("%s", ctime(&event->start_time));
						next_time=next_time.mid(10,6);
						next_duration.sprintf("%d", (int)(event->duration/60));
					} else {
						now_time="";
					}
				}
				for(ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
				{
					if(descriptor->Tag()==DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_text=ss->event_name;
								break;
							case 1:
								next_text=ss->event_name;
								break;
						}
					}
					if(descriptor->Tag()==DESCR_EXTENDED_EVENT)
					{
						ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_longtext+=ss->item_description;
								break;
							case 1:
								next_longtext+=ss->item_description;
								break;
						}
					}
				}
				p++;
		 	}
		}

		if(now_time!="")
		{
			result=read_file(TEMPLATE_DIR+"eit.tmp");
			result.strReplace("#NOWT#", now_time);
			result.strReplace("#NOWD#", now_duration);
			result.strReplace("#NOWST#", now_text);
			result.strReplace("#NOWLT#", filter_string(now_longtext));
			result.strReplace("#NEXTT#", next_time);
			result.strReplace("#NEXTD#", next_duration);
			result.strReplace("#NEXTST#", next_text);
			result.strReplace("#NEXTLT#", filter_string(next_longtext));
		}
		else
		{
			result="eit undefined";
		}
		eit->unlock();
	}
	else
	{
		result="";
	}

	return result;
}

static eString getContent(eString mode, eString path)
{
	eString result = "";
	eString zap_result = "";
	if (mode == "zap")
	{
		zap_result += getZapContent(mode, path);
//		if (path)
//		{
//			path = eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTV).toString();
//			zap_result += getZapContent(mode, path);
//		}
		result += getEITC();
		result.strReplace("#SERVICENAME#", filter_string(getCurService()));

		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

		if(sapi && sapi->service)
		{
			result.strReplace("#EPG#", "<u><a href=\"javascript:openEPG()\" class=\"small\">EPG-Info</a></u>");
			result.strReplace("#SI#", "<u><a href=\"javascript:openSI()\" class=\"small\">Stream-Info</a></u>");
		}
		else
		{
			DELETE(#EPG#);
			DELETE(#SI#);
		}
		result += zap_result;
	}
	else
	if (mode == "links")
	{
		result = "Select one of the link categories on the left";
	}
	else
	if (mode == "linksOfficialSites")
	{
		result = read_file(TEMPLATE_DIR+"linksOfficialSites.tmp");
		if (result == "")
			result = "No links available";
	}
	else
	if (mode == "linksOtherSites")
	{
		result = read_file(TEMPLATE_DIR+"linksOtherSites.tmp");
		if (result == "")
			result = "No links available";
	}
	else
	if (mode == "linksForums")
	{
		result = read_file(TEMPLATE_DIR+"linksForums.tmp");
		if (result == "")
			result = "No links available";
	}
	else
	if (mode == "about")
	{
		result = "Enigma Web Control<br>Version 0.4";
	}
	else
	if (mode == "aboutDreambox")
	{
		result = aboutDreambox();
	}
	else
	if (mode == "aboutDMM")
	{
		result = read_file(TEMPLATE_DIR+"aboutDMM.tmp");
	}
	else
	if (mode == "menuShutdown")
	{
		eZap::getInstance()->quit();
		result = "Enigma is shutting down...";
	}
	else
	if (mode == "menu")
	{
		result = "Control your box using the commands on the left";
	}
	else
	if (mode == "menuReboot")
	{
		eZap::getInstance()->quit(4);
		result = "Enigma is rebooting...";
	}
	else
	if (mode == "menuRestart")
	{
		eZap::getInstance()->quit(2);
		result = "Enigma is restarting...";
	}
	else
	if (mode == "menuWakeup")
	{
		if ( eZapStandby::getInstance() )
		{
			eZapStandby::getInstance()->wakeUp(0);
			result = "Enigma is waking up...";
		}
		else
		{
			result = "Enigma is already awake";
		}
	}
	else
	if (mode == "menuStandby" )
	{
		if (eZapStandby::getInstance())
		{
			result = "Enigma is already sleeping";
		}
		else
		{
			eZapMain::getInstance()->gotoStandby();
			result = "Enigma is going to sleep...";
		}
	}
	else
	if (mode == "menuFBShot")
	{
		gPixmap *p=0;
		p=&gFBDC::getInstance()->getPixmap();

		if (!p)
		{
			result = "FBShot failed";
		}
		else
		{
			if (!savePNG("/tmp/fbshot.png", p))
			{
				result = "<img width=\"650\" src=\"/root/tmp/fbshot.png\" border=0>";
			}
		}
	}
	else
	if (mode == "menuScreenShot")
	{
		if (access("/dev/grabber", R_OK) == 0)
		{
			eString cmd("cat /dev/grabber > /tmp/screenshot.bmp");
			system(cmd.c_str());

			result = "<img width=\"650\" src=\"/root/tmp/screenshot.bmp\" border=0>";
		}
		else
		{
			result = "Module grabber.o is required but not installed";
		}
	}
	else
	if (mode == "menuTimerList")
	{
		result = genTimerListBody();
		result += "<br>";
		result += "<u><a href=\"javascript:cleanupTimerList()\">Delete completed/failed timer events</a></u>";
		result += "<br>";
		result += "<u><a href=\"javascript:clearTimerList()\">Delete all events</a></u>";
	}
	else
		result = mode + " is not available yet";

	return result;
}

static eString getEITC2()
{
	eString now_time = "&nbsp;", now_duration = "&nbsp;", now_text = "&nbsp;", now_longtext = "&nbsp;";
	eString next_time = "&nbsp;", next_duration = "&nbsp;", next_text = "&nbsp;", next_longtext = "&nbsp;";
	eString result;

	EIT *eit=eDVB::getInstance()->getEIT();

	if(eit)
	{
		int p=0;

		for (ePtrList<EITEvent>::iterator event(eit->events); event != eit->events.end(); ++event)
		{
			if (*event)
			{
				if (p == 0)
				{
					if (event->start_time != 0)
					{
						now_time.sprintf("%s", ctime(&event->start_time));
						now_time=now_time.mid(10, 6);
					}
					else
					{
						now_time="";
					}
					now_duration.sprintf("%d", (int)(event->duration/60));
				}
				if (p == 1)
				{
					if (event->start_time != 0)
					{
 						next_time.sprintf("%s", ctime(&event->start_time));
						next_time=next_time.mid(10,6);
						next_duration.sprintf("%d", (int)(event->duration/60));
					}
					else
					{
						now_time="";
					}
				}
				for (ePtrList<Descriptor>::iterator descriptor(event->descriptor); descriptor != event->descriptor.end(); ++descriptor)
				{
					if (descriptor->Tag() == DESCR_SHORT_EVENT)
					{
						ShortEventDescriptor *ss=(ShortEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_text=ss->event_name;
								break;
							case 1:
								next_text=ss->event_name;
								break;
						}
					}
					if (descriptor->Tag() == DESCR_EXTENDED_EVENT)
					{
						ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)*descriptor;
						switch(p)
						{
							case 0:
								now_longtext+=ss->item_description;
								break;
							case 1:
								next_longtext+=ss->item_description;
								break;
						}
					}
				}
				p++;
		 	}
		}
		eit->unlock();
	}

	now_text = now_text.left(30);
	next_text = next_text.left(30);
	result = read_file(TEMPLATE_DIR+"header.tmp");
	result.strReplace("#NOWT#", now_time);
	if (now_duration != "&nbsp;")
		now_duration += " min";
	result.strReplace("#NOWD#", now_duration);
	result.strReplace("#NOWST#", now_text);
	result.strReplace("#NEXTT#", next_time);
	if (next_duration != "&nbsp;")
		next_duration += " min";
	result.strReplace("#NEXTD#", next_duration);
	result.strReplace("#NEXTST#", next_text);
	result.strReplace("#VOLBAR#", getVolBar());
	eString curService = filter_string(getCurService());
	if (curService == "n/a")
		curService = "";
	result.strReplace("#SERVICENAME#", curService);
	result.strReplace("#STATS#", getStats());
	result.strReplace("#EMPTYCELL#", "&nbsp;");

	return result;
}

static eString getMode(eString mode)
{
	eString result;
	mode.upper();
	result="<span class=\"titel\">"+mode+"</span>";
	return result;
}

static eString audiom3u(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result;
	eString tmp;

	content->local_header["Content-Type"]="audio/mpegfile";
	result="http://"+getIP()+":31338/";
        tmp.sprintf("%02x\n", Decoder::current.apid);
 	result+=tmp;
	return result;
}

static eString getcurepg(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result("");
	eString tmp;
	eService* current;

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eServiceReference ref(opt);

	current=eDVB::getInstance()->settings->getTransponders()->searchService(ref?ref:sapi->service);
	if(!current)
		return eString("epg not ready yet");

	result+=eString("<html>" CHARSETMETA "<head><title>epgview</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/epgview.css\"></head><body bgcolor=#000000>");
	result+=eString("<span class=\"title\">");
	result+=filter_string(current->service_name);
	result+=eString("</span>");
	result+=eString("<br>\n");

	const timeMap* evt=ref ?
		eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref)
			:
		eEPGCache::getInstance()->getTimeMap(sapi->service);

	if(!evt)
		return eString("epg not ready yet");

	timeMap::const_iterator It;

	for(It=evt->begin(); It!= evt->end(); It++)
	{
		EITEvent event(*It->second);
		for(ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if(descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				tm* t = localtime(&event.start_time);
				result+=eString().sprintf("<!-- ID: %04x -->", event.event_id);
				tmp.sprintf("<span class=\"epg\">%02d.%02d - %02d:%02d ", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min);
				result+=tmp;
				result+=((ShortEventDescriptor*)descriptor)->event_name;
				result+="</span><br>\n";
			}
		}

	}
	result+="</body></html>";
	return result;
}

eString genRecordString(eServiceReference ref, EITEvent event)
{
	eString result = "";
	result += "javascript:record(\"ref=" + ref2string(ref);
	result += "&ID=" + eString().sprintf("%d", event.event_id);
	result += "&start=" + eString().sprintf("%d", event.start_time);
	result += "&duration=" + eString().sprintf("%d", event.duration);
	result += "\")";
	return result;
}

static eString getcurepg2(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result("");
	eString tmp;
	eService* current;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString serviceRef = opt["ref"];

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "sapi is not available.";

	eServiceReference ref = (serviceRef == "undefined") ? sapi->service : string2ref(serviceRef);

	eDebug("[ENIGMA_DYN] getcurepg2: opts = %s, serviceRef = %s", opts.c_str(), serviceRef.c_str());

	current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);

	if(!current)
		return eString("EPG is not yet ready.");

	const timeMap* evt = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)ref);

	if(!evt)
	{
		result = "EPG is not yet available.";
	}
	else
	{
		timeMap::const_iterator It;

		for(It=evt->begin(); It!= evt->end(); It++)
		{
			EITEvent event(*It->second);
			for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag() == DESCR_SHORT_EVENT)
				{
					tm* t = localtime(&event.start_time);
					result += eString().sprintf("<!-- ID: %04x -->", event.event_id);
					result += "<u><a href=\'";
					result += genRecordString(ref, event);
					result += "\'>Record</a></u>";
					result += "&nbsp;&nbsp;";
					result += eString().sprintf("<span class=\"epg\"><a  href=\"javascript:EPGDetails()\">%02d.%02d - %02d:%02d ", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min);
					result += tmp;
					result += ((ShortEventDescriptor*)descriptor)->event_name;
					result += "</span></a></u><br>\n";
				}
			}

		}
	}

	eString tmp2 = read_file(TEMPLATE_DIR+"EPG.tmp");
	tmp2.strReplace("#CHANNEL#", filter_string(current->service_name));
	tmp2.strReplace("#BODY#", result);
	return tmp2;
}

static eString record(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	return "<html>" CHARSETMETA "<head><title>Record Event</title></head><body>Coming soon ;-)</body></html>\n";
}

static eString EPGDetails(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	return "<html>" CHARSETMETA "<head><title>EPG Details</title></head><body>Coming soon ;-)</body></html>\n";
}

static eString getsi(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::stringstream result;
	eString name,
					provider,
					vpid,
					apid,
					pcrpid,
					tpid,
					vidform("n/a"),
					tsid,
					onid,
					sid,
					pmt;

	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return "not available";

	eServiceDVB *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
	if (service)
	{
		name=filter_string(service->service_name);
		provider=filter_string(service->service_provider);
	}
	vpid=eString().sprintf("%04xh (%dd)", Decoder::current.vpid, Decoder::current.vpid);
	apid=eString().sprintf("%04xh (%dd)", Decoder::current.apid, Decoder::current.apid);
	pcrpid=eString().sprintf("%04xh (%dd)", Decoder::current.pcrpid, Decoder::current.pcrpid);
	tpid=eString().sprintf("%04xh (%dd)", Decoder::current.tpid, Decoder::current.tpid);
	tsid=eString().sprintf("%04xh", sapi->service.getTransportStreamID().get());
	onid=eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get());
	sid=eString().sprintf("%04xh", sapi->service.getServiceID().get());
	pmt=eString().sprintf("%04xh", Decoder::current.pmtpid);

	FILE *bitstream=0;

	if (Decoder::current.vpid!=-1)
		bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres=0, yres=0, aspect=0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
		}
		fclose(bitstream);
		vidform.sprintf("%dx%d ", xres, yres);
		switch (aspect)
		{
		case 1:
			vidform+="square"; break;
		case 2:
			vidform+="4:3"; break;
		case 3:
			vidform+="16:9"; break;
		case 4:
			vidform+="20:9"; break;
		}
	}

	result << "<html>" CHARSETMETA "<head><title>streaminfo</title><link rel=\"stylesheet\" type=\"text/css\" href=\"/si.css\"></head><body bgcolor=#000000>"
						"<!-- " << sapi->service << "-->" << std::endl <<
						"<table cellspacing=0 cellpadding=0 border=0>"
						"<tr><td>name:</td><td>" << name << "</td></tr>"
						"<tr><td>provider:</td><td>" << provider << "</td></tr>"
						"<tr><td>vpid:</td><td>" << vpid << "</td></tr>"
						"<tr><td>apid:</td><td>" << apid << "</td></tr>"
						"<tr><td>pcrpid:</td><td>" << pcrpid << "</td></tr>"
						"<tr><td>tpid:</td><td>" << tpid << "</td></tr>"
						"<tr><td>tsid:</td><td>" << tsid << "</td></tr>"
						"<tr><td>onid:</td><td>" << onid << "</td></tr>"
						"<tr><td>sid:</td><td>" << sid << "</td></tr>"
						"<tr><td>pmt:</td><td>" << pmt << "</td></tr>"
						"<tr><td>vidformat:<td>" << vidform << "</td></tr>"
						"</table>"
						"</body></html>";

	return result.str();
}

static eString message(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (opt.length())
	{
		opt = httpUnescape(opt);
		eZapMain::getInstance()->postMessage(eZapMessage(1, "external message", opt, 10), 0);
		return eString("ok\n");
	} else
		return eString("error\n");
}

static eString start_plugin(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt);

/*	if (opts.find("path") == opts.end())
		return "E: no path set";*/

	if (opts.find("name") == opts.end())
		return "E: no plugin name given";

	eZapPlugins plugins(-1);
	eString path;
	if ( opts.find("path") != opts.end() )
	{
		path = opts["path"];
		if ( path.length() )
		{
			if ( path[path.length()-1] != '/' )
				path+='/';
		}
	}
	return plugins.execPluginByName( (path+opts["name"]).c_str() );
}

static eString stop_plugin(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( ePluginThread::getInstance() )
	{
		ePluginThread::getInstance()->kill(true);
		return "OK plugin is stopped";
	}
	else
		return "E: no plugin is running";
}

static eString xmessage(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt);

	if (opts.find("timeout") == opts.end())
		return "E: no timeout set";

	if (opts.find("caption") == opts.end())
		return "E: no caption set";

	if (opts.find("body") == opts.end())
		return "E: no body set";

	int type=-1;
	if (opts.find("type") != opts.end())
		type=atoi(opts["type"].c_str());

	int timeout=atoi(opts["timeout"].c_str());

	eZapMain::getInstance()->postMessage(eZapMessage(1, opts["caption"], opts["body"], timeout), type != -1);

	return eString("OK\n");
}

/*
static eString record_off(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	DVRSocket *dvr;
	dvr=new DVRSocket(eString("10.0.0.2"), 3000, NGRAB);
	dvr->stopRecording();
	delete dvr;
	return "ok";
}

static eString record_on(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	DVRSocket *dvr;
	dvr=new DVRSocket(eString("10.0.0.2"), 3000, NGRAB);;
	dvr->startRecording();
	return "ok";
}
*/

static eString reload_settings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if (!eDVB::getInstance())
		return "-no dvb\n";
	if (eDVB::getInstance()->settings)
	{
		eDVB::getInstance()->settings->loadServices();
		eDVB::getInstance()->settings->loadBouquets();
		eZap::getInstance()->getServiceSelector()->actualize();
		eServiceReference::loadLockedList( (eZapMain::getInstance()->getEplPath()+"/services.locked").c_str() );
		return "+ok\n";
	}
	return "-no settings to load\n";
}

#ifndef DISABLE_FILE
static eString load_recordings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->loadRecordings();
	return "+ok\n";
}

static eString save_recordings(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->saveRecordings();
	return "+ok\n";
}
#endif

static eString load_timerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eTimerManager::getInstance()->loadTimerList();
	return "+ok\n";
}

static eString save_timerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eTimerManager::getInstance()->saveTimerList();
	return "+ok\n";
}

static eString load_playlist(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->loadPlaylist();
	return "+ok\n";
}

static eString save_playlist(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->savePlaylist();
	return "+ok\n";
}

static eString load_userBouquets(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->loadUserBouquets();
	return "+ok\n";
}

static eString save_userBouquets(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eZapMain::getInstance()->saveUserBouquets();
	return "+ok\n";
}

#define NAVIGATOR_PATH "/cgi-bin/navigator"

class eNavigatorListDirectory: public Object
{
	eString &result;
	eString path;
	eServiceInterface &iface;
	int num;
public:
	eNavigatorListDirectory(eString &result, eString path, eServiceInterface &iface): result(result), path(path), iface(iface)
	{
		eDebug("path: %s", path.c_str());
		num=0;
	}
	void addEntry(const eServiceReference &e)
	{
		result+="<tr><td bgcolor=\"#";
		if (num & 1)
			result += "c0c0c0";
		else
			result += "d0d0d0";
		result+="\"><font color=\"#000000\">";
		if (!(e.flags & eServiceReference::isDirectory))
			result+="[PLAY] ";

		result+=eString("<a href=\"" NAVIGATOR_PATH ) + "?path=" + path + ref2string(e) +"\">" ;

		eService *service=iface.addRef(e);
		if (!service)
			result+="N/A";
		else
			result+=service->service_name;
		iface.removeRef(e);

		result+="</a></font></td></tr>\n";
		eDebug("ok");
		num++;
	}
};

static eString navigator(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	std::map<eString,eString> opts=getRequestOptions(opt);

	if (opts.find("path") == opts.end())
	{
		content->code=301;
		content->code_descr="Moved Permanently";
		content->local_header["Location"]=eString(NAVIGATOR_PATH) + "?path=" + ref2string(eServiceReference(eServiceReference::idStructure, eServiceReference::isDirectory, 0));
		return "redirecting..";
	}
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	eString spath=opts["path"];

	eServiceInterface *iface=eServiceInterface::getInstance();
	if (!iface)
		return "n/a\n";

	eString current;

	unsigned int pos;
	if ((pos=spath.rfind(';')) != eString::npos)
	{
		current=spath.mid(pos+1);
		spath=spath.left(pos);
	} else
	{
		current=spath;
		spath="";
	}

	eDebug("current service: %s", current.c_str());
	eServiceReference current_service(current);

	eString res;

	res="<html>\n"
		CHARSETMETA
		"<head><title>Enigma Navigator</title></head>\n"
		"<body bgcolor=\"#f0f0f0\">\n"
		"<font color=\"#000000\">\n";

	res+=eString("Current: ") + current + "<br>\n";
	res+="<hr>\n";
	res+=eString("path: ") + spath + "<br>\n";

	if (! (current_service.flags&eServiceReference::isDirectory))	// is playable
	{
		eZapMain::getInstance()->playService(current_service, eZapMain::psSetMode|eZapMain::psDontAdd);
//		iface->play(current_service);
		res+="ok, hear the music..";
	} else
	{
		eNavigatorListDirectory navlist(res, spath + ";" + current + ";", *iface);
		Signal1<void,const eServiceReference&> signal;
		signal.connect(slot(navlist, &eNavigatorListDirectory::addEntry));

		res+="<table width=\"100%%\">\n";
		iface->enterDirectory(current_service, signal);
		res+="</table>\n";
		eDebug("entered");
		iface->leaveDirectory(current_service);
		eDebug("leaved");
	}

	return res;
}

static eString getCurrentServiceRef(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( eServiceInterface::getInstance()->service )
		return eServiceInterface::getInstance()->service.toString();
	else
		return "E:no service running";
}

static eString web_root(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	std::map<eString,eString> opt=getRequestOptions(opts);
	content->local_header["Content-Type"]="text/html; charset=utf-8";

	eString mode = opt["mode"];
	eString spath = opt["path"];

	eDebug("[ENIGMA_DYN] web_root: mode = %s, spath = %s", mode.c_str(), spath.c_str());

	if (!spath)
		spath=eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTV).toString();
		//ref2string(eServiceReference(eServiceReference::idStructure, eServiceReference::isDirectory, 0));

	if (!mode)
		mode = "zap";

	result = read_file(TEMPLATE_DIR+"index.tmp");
	result.strReplace("#COP#", getContent(mode, spath));
	result.strReplace("#MODE#", getMode(mode));
	result.strReplace("#HEADER#", getEITC2());

	return result;
}

static eString screenshot(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	std::map<eString,eString> opt=getRequestOptions(opts);
	gPixmap *p=0;
#ifndef DISABLE_LCD
	if (opt["mode"]=="lcd")
		p=&gLCDDC::getInstance()->getPixmap();
	else
#endif
		p=&gFBDC::getInstance()->getPixmap();

	if (!p)
		return "no\n";

	if (!savePNG("/tmp/screenshot.png", p))
	{
		content->local_header["Location"]="/root/tmp/screenshot.png";
		content->code=307;
		return "ok\n";
	}

	return "not ok\n";
}

static eString listDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString answer;
	content->local_header["Content-Type"]="text/plain; charset=utf-8";
	answer.sprintf(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<directory path=\"%s\" dircount=\"\" filecount=\"\" linkcount=\"\">\n",
		opt.length()?opt.c_str():"?");
	DIR *d=0;
	if( opt.length() )
	{
		if ( opt[opt.length()-1] != '/' )
			opt+='/';
		d = opendir(opt.c_str());
	}
	if ( d )
	{
		char buffer[255];
		int dircount, filecount, linkcount;
		dircount=filecount=linkcount=0;
		while (struct dirent *e=readdir(d))
		{
			eString filename=opt;
			filename+=e->d_name;

			struct stat s;
			if (lstat(filename.c_str(), &s)<0)
				continue;
			if ( S_ISLNK(s.st_mode) )
			{
				int count = readlink(filename.c_str(), buffer, 255);
				eString dest(buffer,count);
				answer+=eString().sprintf("\t<object type=\"link\" name=\"%s\" dest=\"%s\"/>\n",e->d_name, dest.c_str() );
				++linkcount;
			}
			else if (S_ISDIR(s.st_mode))
			{
				answer+=eString().sprintf("\t<object type=\"directory\" name=\"%s\"/>\n",e->d_name );
				++dircount;
			}
			else if (S_ISREG(s.st_mode))
			{
				answer+=eString().sprintf("\t<object type=\"file\" name=\"%s\" size=\"%d\"/>\n",
					e->d_name,
					s.st_size );
				++filecount;
			}
		}
		unsigned int pos = answer.find("dircount=\"");
		answer.insert( pos+10, eString().sprintf("%d", dircount ) );
		pos = answer.find("filecount=\"");
		answer.insert( pos+11, eString().sprintf("%d", filecount ) );
		pos = answer.find("linkcount=\"");
		answer.insert( pos+11, eString().sprintf("%d", linkcount ) );
		closedir(d);
		answer+="</directory>\n";
		return answer;
	}
	else
		return eString().sprintf("E: couldn't read directory %s", opt.length()?opt.c_str():"?" );
}

static eString makeDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( opt.find("&&") == eString::npos )
	{
		if ( system(eString().sprintf("mkdir %s", opt.c_str()).c_str()) >> 8 )
			return eString().sprintf("E: create directory %s failed", opt.c_str() );
		return "OK";
	}
	return "E: invalid command";
}

static eString removeDirectory(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( opt.find("&&") == eString::npos )
	{
		if ( system(eString().sprintf("rmdir %s", opt.c_str()).c_str()) >> 8 )
			return eString().sprintf("E: remove directory %s failed", opt.c_str() );
		return "OK";
	}
	return "E: invalid command";
}

static eString removeFile(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( opt.find("&&") == eString::npos )
	{
		if ( system(eString().sprintf("rm %s", opt.c_str()).c_str()) >> 8 )
			return eString().sprintf("E: remove file %s failed", opt.c_str() );
		return "OK";
	}
	return "E: invalid command";
}

static eString moveFile(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( opt.find("&&") == eString::npos )
	{
		std::map<eString,eString> opts=getRequestOptions(opt);
		if ( opts.find("source") == opts.end() || !opts["source"].length() )
			return "E: option source missing or empty source given";
		if ( opts.find("dest") == opts.end() || !opts["dest"].length() )
			return "E: option dest missing or empty dest given";
		if ( system(eString().sprintf("mv %s %s", opts["source"].c_str(), opts["dest"].c_str() ).c_str()) >> 8 )
			return eString().sprintf("E: cannot move %s to %s", opts["source"].c_str(), opts["dest"].c_str() );
		return "OK";
	}
	return "E: invalid command";
}

static eString createSymlink(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if ( opt.find("&&") == eString::npos )
	{
		std::map<eString,eString> opts=getRequestOptions(opt);
		if ( opts.find("source") == opts.end() || !opts["source"].length() )
			return "E: option source missing or empty source given";
		if ( opts.find("dest") == opts.end() || !opts["dest"].length() )
			return "E: option dest missing or empty dest given";
		if ( system(eString().sprintf("ln -sf %s %s", opts["source"].c_str(), opts["dest"].c_str() ).c_str()) >> 8 )
			return eString().sprintf("E: cannot create symlink %s to %s", opts["source"].c_str(), opts["dest"].c_str() );
		return "OK";
	}
	return "E: invalid command";
}

static eString neutrino_suck_zapto(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	if(opt!="getpids")
		return eString("ok\n");
	else
		return eString().sprintf("%u\n%u\n", Decoder::current.vpid, Decoder::current.apid);
}

static eString neutrino_suck_getonidsid(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi || !sapi->service)
		return "200\n";

	int onidsid = (sapi->service.getOriginalNetworkID().get() << 8)
		| sapi->service.getServiceID().get();

	return eString().sprintf("%d\n", onidsid);
}

struct addToString
{
	eString &dest;
	eServiceReferenceDVB &current;
	addToString(eString &dest, eServiceReferenceDVB &current)
		:dest(dest), current(current)
	{
	}
	void operator()(const eServiceReference& s)
	{
		eServiceReferenceDVB &bla((eServiceReferenceDVB&)s);
		if ( current.getTransportStreamID() == bla.getTransportStreamID() &&
				 current.getOriginalNetworkID() == bla.getOriginalNetworkID() &&
				 current.getDVBNamespace() == bla.getDVBNamespace() )
		{
			dest+=s.toString();
			eServiceDVB *service = (eServiceDVB*) eServiceInterface::getInstance()->addRef(s);
			if ( service )
			{
				for(int i=0; i < (int)eServiceDVB::cacheMax; ++i)
				{
					int d=service->get((eServiceDVB::cacheID)i);
					if ( d != -1 )
						dest+=eString().sprintf(";%02d%04x", i, d);
				}
				eServiceInterface::getInstance()->removeRef(s);
			}
			dest+='\n';
		}
	}
};

static eString getTransponderServices(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	content->local_header["Content-Type"]="text/plain; charset=utf-8";
	eServiceReferenceDVB cur = (eServiceReferenceDVB&)eServiceInterface::getInstance()->service;
	if ( cur.type == eServiceReference::idDVB && !cur.path )
	{
		eString result;
		eTransponderList::getInstance()->forEachServiceReference( addToString(result,cur) );
		if ( result )
			return result;
		else
			return "E: no other services on the current transponder";
	}
	return "E: no DVB service is running.. or this is a playback";
}

struct appendonidsidnamestr: public std::unary_function<const eServiceDVB&, void>
{
	eString &str;
	appendonidsidnamestr(eString &s)
		:str(s)
	{
	}
	void operator()(eServiceDVB& s)
	{
		str += filter_string(eString().sprintf("%d %s\n",
			(s.original_network_id.get()<<8)|s.service_id.get(),
			s.service_name.c_str() ));
	}
};

static eString neutrino_suck_getchannellist(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString channelstring;

	eTransponderList::getInstance()->forEachService( appendonidsidnamestr( channelstring ) );

	return channelstring;
}

static eString cleanupTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result = "";
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	result+="<script language=\"javascript\">window.close();</script>";
	eTimerManager::getInstance()->cleanupEvents();
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString clearTimerList(eString request, eString dirpath, eString opt, eHTTPConnection *content)
{
	eString result = "";
	content->local_header["Content-Type"]="text/html; charset=utf-8";
	result+="<script language=\"javascript\">window.close();</script>";
	eTimerManager::getInstance()->clearEvents();
	eTimerManager::getInstance()->saveTimerList(); //not needed, but in case enigma crashes ;-)
	return result;
}

static eString addTimerEvent(eString request, eString dirpath, eString opts, eHTTPConnection *content)
{
	eString result;
	eService *current = NULL;

	content->local_header["Content-Type"]="text/html; charset=utf-8";
	std::map<eString,eString> opt=getRequestOptions(opts);
	eString serviceRef = opt["ref"];
	eString eventID = opt["ID"];
	int eventid = atoi(eventID.c_str());
	eString eventStartTime = opt["start"];
	eString eventDuration = opt["duration"];
	eString description = "No description available";

	eDebug("[ENIGMA_DYN] addTimerEvent: serviceRef = %s, ID = %s, start = %s, duration = %s\n", serviceRef.c_str(), eventID.c_str(), eventStartTime.c_str(), eventDuration.c_str());

	// search for the event... to get the description...
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		eServiceReference ref(string2ref(serviceRef));
		current = eDVB::getInstance()->settings->getTransponders()->searchService((eServiceReferenceDVB&)ref);
		if(current)
		{
			EITEvent *event = eEPGCache::getInstance()->lookupEvent((eServiceReferenceDVB&)ref, eventid );
			if(event)
			{
				for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
				{
					if (d->Tag() == DESCR_SHORT_EVENT)
					{
						// ok, we probably found the event...
						description = ((ShortEventDescriptor*)*d)->event_name;
						eDebug("[ENIGMA_DYN] addTimerEvent: found description = %s", description.c_str());
						break;
					}
				}
				delete event;
			}
		}
	}

	description = filter_string(current->service_name) + "/" + description;

	int timeroffset = 0;
	if ((eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset)) != 0)
		timeroffset = 0;

	int start = atoi(eventStartTime.c_str()) - (timeroffset * 60);
	int duration = atoi(eventDuration.c_str()) + (2 * timeroffset * 60);

	ePlaylistEntry entry(string2ref(serviceRef), start, duration, atoi(eventID.c_str()), ePlaylistEntry::stateWaiting | ePlaylistEntry::RecTimerEntry | ePlaylistEntry::recDVR);
	eDebug("[ENIGMA_DYN] description = %s\n", description.c_str());
	entry.service.descr = description;

	if (eTimerManager::getInstance()->addEventToTimerList(entry) == -1)
		result += "Timer event could not be added because time of the event overlaps with an already existing event.";
	else
		result += "Timer event was created successfully.";

	return result;
}

void ezapInitializeDyn(eHTTPDynPathResolver *dyn_resolver)
{
	dyn_resolver->addDyn("GET", "/", web_root);
	dyn_resolver->addDyn("GET", NAVIGATOR_PATH, navigator);

	dyn_resolver->addDyn("GET", "/cgi-bin/ls", listDirectory);
	dyn_resolver->addDyn("GET", "/cgi-bin/mkdir", makeDirectory, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/rmdir", removeDirectory, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/rm", removeFile, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/mv", moveFile, true);
	dyn_resolver->addDyn("GET", "/cgi-bin/ln", createSymlink, true );

	dyn_resolver->addDyn("GET", "/setVolume", setVolume);
	dyn_resolver->addDyn("GET", "/record", record);
	dyn_resolver->addDyn("GET", "/EPGDetails", EPGDetails);
	dyn_resolver->addDyn("GET", "/showTimerList", showTimerList);
	dyn_resolver->addDyn("GET", "/addTimerEvent", addTimerEvent);
	dyn_resolver->addDyn("GET", "/cleanupTimerList", cleanupTimerList);
	dyn_resolver->addDyn("GET", "/clearTimerList", clearTimerList);
	dyn_resolver->addDyn("GET", "/cgi-bin/status", doStatus);
	dyn_resolver->addDyn("GET", "/cgi-bin/switchService", switchService);
	dyn_resolver->addDyn("GET", "/cgi-bin/admin", admin);
	dyn_resolver->addDyn("GET", "/cgi-bin/audio", audio);
	dyn_resolver->addDyn("GET", "/cgi-bin/getPMT", getPMT);
	dyn_resolver->addDyn("GET", "/cgi-bin/getEIT", getEIT);
	dyn_resolver->addDyn("GET", "/cgi-bin/message", message);
	dyn_resolver->addDyn("GET", "/control/message", message);
	dyn_resolver->addDyn("GET", "/cgi-bin/xmessage", xmessage);

	dyn_resolver->addDyn("GET", "/audio.m3u", audiom3u);
	dyn_resolver->addDyn("GET", "/version", version);
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg", getcurepg);
	dyn_resolver->addDyn("GET", "/cgi-bin/getcurrentepg2", getcurepg2);
	dyn_resolver->addDyn("GET", "/cgi-bin/streaminfo", getsi);
	dyn_resolver->addDyn("GET", "/channels/getcurrent", channels_getcurrent);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadSettings", reload_settings);
#ifndef DISABLE_FILE
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadRecordings", load_recordings);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveRecordings", save_recordings);
#endif
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadPlaylist", load_playlist);
	dyn_resolver->addDyn("GET", "/cgi-bin/savePlaylist", save_playlist);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadUserBouquets", load_userBouquets);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveUserBouquets", save_userBouquets);
	dyn_resolver->addDyn("GET", "/cgi-bin/reloadTimerList", load_timerList);
	dyn_resolver->addDyn("GET", "/cgi-bin/saveTimerList", save_timerList);
	dyn_resolver->addDyn("GET", "/cgi-bin/startPlugin", start_plugin);
	dyn_resolver->addDyn("GET", "/cgi-bin/stopPlugin", stop_plugin);
	dyn_resolver->addDyn("GET", "/cgi-bin/screenshot", screenshot);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentService", getCurrentServiceRef);
	dyn_resolver->addDyn("GET", "/cgi-bin/currentTransponderServices", getTransponderServices);
	dyn_resolver->addDyn("GET", "/control/zapto", neutrino_suck_zapto);
	dyn_resolver->addDyn("GET", "/control/getonidsid", neutrino_suck_getonidsid );
	dyn_resolver->addDyn("GET", "/control/channellist", neutrino_suck_getchannellist );

/*
	dyn_resolver->addDyn("GET", "/record/on", record_on);
	dyn_resolver->addDyn("GET", "/record/off", record_off);
*/
/*	dyn_resolver->addDyn("GET", "/channels/numberchannels", channels_numberchannels);
	dyn_resolver->addDyn("GET", "/channels/gethtmlchannels", channels_gethtmlchannels);
	dyn_resolver->addDyn("GET", "/channels/getchannels", channels_getgetchannels);
	dyn_resolver->addDyn("GET", "/epg/now", epg_now);
	dyn_resolver->addDyn("GET", "/epg/next", epg_next); */
}
