#include <enigma_main.h>

#include <errno.h>
#include <iomanip>
//#include <stdio.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <streaminfo.h>
#include <parentallock.h>
#include <enigma_mainmenu.h>
#include <enigma_event.h>
#include <enigma.h>
#include <enigma_vcr.h>
#include <enigma_lcd.h>
#include <enigma_plugins.h>
#include <helpwindow.h>
#include <timer.h>
#include <download.h>
#include <epgwindow.h>
#include <lib/base/i18n.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/econfig.h>
#include <lib/dvb/servicedvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/iso639.h>
#include <lib/dvb/servicemp3.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/gdi/font.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/actions.h>
#include <lib/gui/echeckbox.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvbservice.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/glcddc.h>
#include <enigma_epg.h>

		// bis waldi das in nen .h tut
#define MOVIEDIR "/hdd/movie"

struct enigmaMainActions
{
	eActionMap map;
	eAction showMainMenu, standby_press, standby_nomenu_press, standby_repeat, standby_release, 
		showInfobar, hideInfobar, showInfobarEPG, showServiceSelector,
		showSubservices, showAudio, pluginVTXT, pluginExt, showEPGList, showEPG, 
		nextSubService, prevSubService, nextService, prevService,
		playlistNextService, playlistPrevService, serviceListDown,
		serviceListUp, volumeUp, volumeDown, toggleMute,
		stop, pause, play, record, 
		startSkipForward, repeatSkipForward, stopSkipForward, 
		startSkipReverse, repeatSkipReverse, stopSkipReverse,
		showUserBouquets, showDVBBouquets, showRecMovies, showPlaylist,
		modeTV, modeRadio, modeFile,
		toggleDVRFunctions, toggleIndexmark, indexSeekNext, indexSeekPrev;
	enigmaMainActions(): 
		map("enigmaMain", _("enigma Zapp")),
		showMainMenu(map, "showMainMenu", _("show main menu"), eAction::prioDialog),
		standby_press(map, "standby_press", _("go to standby (press)"), eAction::prioDialog),
		standby_nomenu_press(map, "standby_nomenu_press", _("go to standby without menu (press)"), eAction::prioDialog),
		standby_repeat(map, "standby_repeat", _("go to standby (repeat)"), eAction::prioDialog),
		standby_release(map, "standby_release", _("go to standby (release)"), eAction::prioDialog),

		showInfobar(map, "showInfobar", _("show infobar"), eAction::prioDialog),
		hideInfobar(map, "hideInfobar", _("hide infobar"), eAction::prioDialog),
		showInfobarEPG(map, "showInfobarEPG", _("show infobar or EPG"), eAction::prioDialog),
		showServiceSelector(map, "showServiceSelector", _("show service selector"), eAction::prioDialog),
		showSubservices(map, "showSubservices", _("show subservices/NVOD"), eAction::prioDialog),
		showAudio(map, "showAudio", _("show audio selector"), eAction::prioDialog),
		pluginVTXT(map, "pluginVTXT", _("show Videotext"), eAction::prioDialog),
		pluginExt(map, "pluginExt", _("show extension Plugins"), eAction::prioDialog),
		showEPGList(map, "showEPGList", _("show epg schedule list"), eAction::prioDialog),
		showEPG(map, "showEPG", _("show extended info"), eAction::prioDialog),
		nextSubService(map, "nextSubService", _("zap to next subService"), eAction::prioDialog),
		prevSubService(map, "prevSubService", _("zap to prev subService"), eAction::prioDialog),
		nextService(map, "nextService", _("quickzap next"), eAction::prioDialog),
		prevService(map, "prevService", _("quickzap prev"), eAction::prioDialog),

		playlistNextService(map, "playlistNextService", _("playlist/history next"), eAction::prioDialog),
		playlistPrevService(map, "playlistPrevService", _("playlist/history prev"), eAction::prioDialog),
		
		serviceListDown(map, "serviceListDown", _("service list and down"), eAction::prioDialog),
		serviceListUp(map, "serviceListUp", _("service list and up"), eAction::prioDialog),

		volumeUp(map, "volumeUp", _("volume up"), eAction::prioDialog),
		volumeDown(map, "volumeDown", _("volume down"), eAction::prioDialog),
		toggleMute(map, "toggleMute", _("toggle mute flag"), eAction::prioDialog),

		stop(map, "stop", _("stop playback"), eAction::prioWidget),
		pause(map, "pause", _("pause playback"), eAction::prioWidget),
		play(map, "play", _("resume playback"), eAction::prioWidget),
		record(map, "record", _("record"), eAction::prioWidget),
		
		startSkipForward(map, "startSkipF", _("start skipping forward"), eAction::prioWidget),
		repeatSkipForward(map, "repeatSkipF", _("repeat skipping forward"), eAction::prioWidget),
		stopSkipForward(map, "stopSkipF", _("stop skipping forward"), eAction::prioWidget),

		startSkipReverse(map, "startSkipR", _("start skipping reverse"), eAction::prioWidget),
		repeatSkipReverse(map, "repeatSkipR", _("repeat skipping reverse"), eAction::prioWidget),
		stopSkipReverse(map, "stopSkipR", _("stop skipping reverse"), eAction::prioWidget),

		showUserBouquets(map, "showUserBouquets", _("open the serviceselector and show bouquets"), eAction::prioWidget),
		showDVBBouquets(map, "showDVBBouquets", _("open the serviceselector and show provider"), eAction::prioWidget),
		showRecMovies(map, "showRecMovies", _("open the serviceselector and show recorded movies"), eAction::prioWidget),
		showPlaylist(map, "showPlaylist", _("open the serviceselector and shows the playlist"), eAction::prioWidget),

		modeTV(map, "modeTV", _("switch to TV mode"), eAction::prioDialog),
		modeRadio(map, "modeRadio", _("switch to Radio mode"), eAction::prioDialog),
		modeFile(map, "modeFile", _("switch to File mode"), eAction::prioDialog),

		toggleDVRFunctions(map, "toggleDVRFunctions", _("toggle DVR panel"), eAction::prioDialog),
		toggleIndexmark(map, "toggleIndexmark", _("toggle index mark"), eAction::prioDialog),

		indexSeekNext(map, "indexSeekNext", _("seek to next index mark"), eAction::prioDialog),
		indexSeekPrev(map, "indexSeekPrev", _("seek to previous index mark"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaMainActions> i_enigmaMainActions(eAutoInitNumbers::actions, "enigma main actions");

struct enigmaGlobalActions
{
	eActionMap map;
	eAction volumeUp, volumeDown, toggleMute;
	enigmaGlobalActions(): 
		map("enigmaGlobal", "enigma global"),

		volumeUp(map, "volumeUp", _("volume up"), eAction::prioGlobal),
		volumeDown(map, "volumeDown", _("volume down"), eAction::prioGlobal),
		toggleMute(map, "toggleMute", _("toggle mute flag"), eAction::prioGlobal)
	{
		eWidget::addGlobalActionMap(&map);
	}
	~enigmaGlobalActions()
	{
		eWidget::removeGlobalActionMap(&map);
	}
};

eAutoInitP0<enigmaGlobalActions> i_enigmaGlobalActions(eAutoInitNumbers::actions, "enigma global actions");

struct enigmaStandbyActions
{
	eActionMap map;
	eAction wakeUp;
	enigmaStandbyActions(): 
		map("enigmaStandby", _("enigma standby")),
		wakeUp(map, "wakeUp", _("wake up enigma"), eAction::prioDialog)	{
	}
};

eAutoInitP0<enigmaStandbyActions> i_enigmaStandbyActions(eAutoInitNumbers::actions, "enigma standby actions");

eZapStandby* eZapStandby::instance=0;

Signal0<void> eZapStandby::enterStandby, eZapStandby::leaveStandby;

void eZapStandby::wakeUp(int norezap)
{
	if (norezap)
		rezap=0;
	close(0);  
}

int eZapStandby::eventHandler(const eWidgetEvent &event)
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_enigmaStandbyActions->wakeUp)
			close(0);
		else
			break;
		return 0;
	case eWidgetEvent::execBegin:
	{
		/*emit*/ enterStandby();
#ifndef DISABLE_LCD
		eDBoxLCD::getInstance()->switchLCD(0);
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdStandby->show();
#endif
		rezap = 0;
		if (handler)
		{
			ref = eServiceInterface::getInstance()->service;
			if (handler->getFlags() & eServiceHandler::flagIsSeekable)
				handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 0));
			else
			{
				rezap=1;
				eServiceInterface::getInstance()->stop();
			}
		}
		eAVSwitch::getInstance()->setInput(1);
		eAVSwitch::getInstance()->setTVPin8(0);
		eFrontend::getInstance()->savePower();
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
		if (sapi)
			sapi->transponder=0;
		::sync();
		system("/sbin/hdparm -y /dev/ide/host0/bus0/target0/lun0/disc");
		system("/sbin/hdparm -y /dev/ide/host0/bus0/target1/lun0/disc");

		if(eDVB::getInstance()->getmID() == 6) //  in standby
		{
			time_t c=time(0)+eDVB::getInstance()->time_difference;
			tm *t=localtime(&c);

			int num=9999;
			int stdby=1;
			if (t && eDVB::getInstance()->time_difference)
			{
				num = t->tm_hour*100+t->tm_min;
			}
					
			eDebug("write number to led-display");
			int fd=::open("/dev/dbox/fp0",O_RDWR);
			::ioctl(fd,11,&stdby);
			::ioctl(fd,4,&num);
			::close(fd);
		}

		break;
	}
	case eWidgetEvent::execDone:
	{
#ifndef DISABLE_LCD
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdStandby->hide();
		pLCD->lcdMain->show();
#endif
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
		if (rezap)
			eServiceInterface::getInstance()->play(ref);
		eAVSwitch::getInstance()->setInput(0);
		eStreamWatchdog::getInstance()->reloadSettings();
#ifndef DISABLE_LCD
		eDBoxLCD::getInstance()->switchLCD(1);
#endif
		if(eDVB::getInstance()->getmID() == 6) //  out of standby
		{
			int stdby=0;
			int fd=::open("/dev/dbox/fp0",O_RDWR);
			::ioctl(fd,11,&stdby);
			::close(fd);
		}
		/*emit*/ leaveStandby();
		break;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

eZapStandby::eZapStandby(): eWidget(0, 1)
{
	addActionMap(&i_enigmaStandbyActions->map);
	if (!instance)
		instance=this;
	else
		eFatal("more than ob eZapStandby instance is created!");
}

eString getISO639Description(char *iso)
{
	for (unsigned int i=0; i<sizeof(iso639)/sizeof(*iso639); ++i)
	{
		if (!strnicmp(iso639[i].iso639foreign, iso, 3))
			return iso639[i].description1;
		if (!strnicmp(iso639[i].iso639int, iso, 3))
			return iso639[i].description1;
	}
	return eString()+iso[0]+iso[1]+iso[2];
}

#ifndef DISABLE_FILE

eZapSeekIndices::eZapSeekIndices()
{
	length = -1;
}

void eZapSeekIndices::load(const eString &filename)
{
	this->filename=filename;
	FILE *f=fopen(filename.c_str(), "rt");
	if (!f)
		return;
	
	while (1)
	{
		int real, time;
		if (fscanf(f, "%d %d\n", &real, &time) != 2)
			break;
		add(real, time);
	}
	fclose(f);
	changed=0;
}

void eZapSeekIndices::save()
{
	FILE *f=fopen(filename.c_str(), "wt");
	if (!f)
		return;
		
	for (std::map<int,int>::const_iterator i(index.begin()); i != index.end(); ++i)
		fprintf(f, "%d %d", i->first, i->second);
	fclose(f);
	changed=0;
}

void eZapSeekIndices::add(int real, int time)
{
	index.insert(std::pair<int,int>(real,time));
}

void eZapSeekIndices::remove(int real)
{
	index.erase(real);
}

void eZapSeekIndices::clear()
{
	index.clear();
}

int eZapSeekIndices::getNext(int real, int dir)
{
	int diff=-1, r=-1;
	for (std::map<int,int>::const_iterator i(index.begin()); i != index.end(); ++i)
	{
		if ((dir > 0) && (i->first <= real))
			continue;
		if ((dir < 0) && (i->first >= real))
			break;
		int d=abs(i->first-real);
		if ((d < diff) || (diff == -1))
		{
			diff=d;
			r=i->first;
		} else
			break;
	}
	return r;
}

int eZapSeekIndices::getTime(int real)
{
	std::map<int,int>::const_iterator i=index.find(real);
	if (i != index.end())
		return i->second;
	return -1;
}

std::map<int,int> &eZapSeekIndices::getIndices()
{
	return index;
}

void eZapSeekIndices::setTotalLength(int l)
{
	length = l;
}

int  eZapSeekIndices::getTotalLength()
{
	return length;
}

eProgressWithIndices::eProgressWithIndices(eWidget *parent): eProgress(parent)
{
	indexmarkcolor = eSkin::getActive()->queryColor("indexmark");
}

void eProgressWithIndices::redrawWidget(gPainter *target, const eRect &area)
{
	eProgress::redrawWidget(target, area);
	if (!indices)
		return;
	int  len = indices->getTotalLength();
	if (len <= 0)
		return;
	int xlen = size.width();
	target->setForegroundColor(indexmarkcolor);
	for (int i=indices->getNext(-1, 1); i != -1; i=indices->getNext(i, 1))
	{
		int time = indices->getTime(i);
		int pos  = time * xlen / len;
		target->fill(eRect(pos - 2, 0, 4, size.height()));
	}
}

void eProgressWithIndices::setIndices(eZapSeekIndices *i)
{
	indices = i;
	invalidate();
}

#endif

int NVODStream::validate()
{
	text.str(eString());
	for (ePtrList<EITEvent>::const_iterator event(eit.events); event != eit.events.end(); ++event)		// always take the first one
	{
		tm *begin=event->start_time!=-1?localtime(&event->start_time):0;

		if (begin)
			text << std::setfill('0') << std::setw(2) << begin->tm_hour << ':' << std::setw(2) << begin->tm_min;

		time_t endtime=event->start_time+event->duration;
		tm *end=event->start_time!=-1?localtime(&endtime):0;

		if (end)
			text << _(" to ") << std::setw(2) << end->tm_hour << ':' << std::setw(2) << end->tm_min;

		time_t now=time(0)+eDVB::getInstance()->time_difference;

		if ( now > endtime )
			return 0;

		if ((event->start_time <= now) && (now < endtime))
		{
			int perc=(now-event->start_time)*100/event->duration;
			text << " (" << perc << "%, " << perc*3/100 << '.' << std::setw(2) << (perc*3)%100 << _(" Euro lost)");
		}
		return event->start_time;
	}
	return 0;
}

void NVODStream::EITready(int error)
{
	eDebug("NVOD eit ready: %d", error);

	if ( error )
		delete this;
	else if ( eit.ready && !error && !begTime && (begTime = validate()) )
	{
		listbox->append( this );
		((eListBox<NVODStream>*)listbox)->sort(); // <<< without explicit cast the compiler nervs ;)
		/*emit*/ ready();
	}
}

NVODStream::NVODStream(eListBox<NVODStream> *listbox, eDVBNamespace dvb_namespace, const NVODReferenceEntry *ref, int type)
	: eListBoxEntryTextStream((eListBox<eListBoxEntryTextStream>*)listbox), 
		service(dvb_namespace, eTransportStreamID(ref->transport_stream_id), eOriginalNetworkID(ref->original_network_id),
			eServiceID(ref->service_id), 5), eit(EIT::typeNowNext, ref->service_id, type)
{
	CONNECT(eit.tableReady, NVODStream::EITready);
	listbox->remove(this);
	begTime=0;
	eit.start();
}

const eString &NVODStream::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )
{
	if (begTime && (begTime = validate()) )
		return eListBoxEntryTextStream::redraw(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state);

	listbox->remove( this );
	eit.start();

	static eString ret;
	return ret = _("not valid!");
}

void NVODStream::selfDestroy()
{
	if (!begTime)
		delete this;
}

void eNVODSelector::selected(NVODStream* nv)
{
	if (nv)
		eServiceInterface::getInstance()->play(nv->service);

	close(0);
}

eNVODSelector::eNVODSelector()
	:eListBoxWindow<NVODStream>(_("NVOD"), 10, 440), count(0)
{
	move(ePoint(100, 100));
	list.setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	CONNECT(list.selected, eNVODSelector::selected);
}

void eNVODSelector::clear()
{
	count=0;
	list.clearList();
	/*emit*/ clearEntrys();
}

struct findNVOD: public std::unary_function<NVODStream&, void>
{
	NVODStream **stream;
	time_t nowTime;
	
	findNVOD( NVODStream** str )
		:stream( str ), nowTime( eDVB::getInstance()->time_difference + time(0) )
	{
	}

	bool operator()(NVODStream& str)
	{
		if ( *stream )
		{
			if ( abs( (*stream)->getBegTime() - nowTime ) > abs( str.getBegTime() - nowTime ) )
				*stream = &str;
		}
		else
			*stream = &str;
		return false;
	}
};

void eNVODSelector::readyCallBack( )
{
	if ( count )
	{
		count--;
		if ( !count )
		{
			NVODStream *select=0;
			list.forEachEntry( findNVOD( &select ) );
			if ( select )
				eServiceInterface::getInstance()->play( select->service );
		}
	}
}

void eNVODSelector::add(eDVBNamespace dvb_namespace, NVODReferenceEntry *ref)
{
	eServiceReference &s=eServiceInterface::getInstance()->service;
	if (s.type != eServiceReference::idDVB)
		return ;
	eServiceReferenceDVB &service=(eServiceReferenceDVB&)s;

	int type= ((service.getTransportStreamID()==eTransportStreamID(ref->transport_stream_id))
			&&	(service.getOriginalNetworkID()==eOriginalNetworkID(ref->original_network_id))) ? EIT::tsActual:EIT::tsOther;
	count++;
	NVODStream *nvod = new NVODStream(&list, dvb_namespace, ref, type);
	CONNECT( nvod->ready, eNVODSelector::readyCallBack );
	clearEntrys.connect( slot( *nvod, &NVODStream::selfDestroy) );
}

VideoStream::VideoStream(eListBox<VideoStream> *listbox, PMTEntry *stream)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)listbox), component_tag(-1), stream(stream)
{
	for (ePtrList<Descriptor>::iterator c(stream->ES_info); c != stream->ES_info.end(); ++c)
	{
		if (c->Tag()==DESCR_STREAM_ID)
			component_tag=((StreamIdentifierDescriptor*)*c)->component_tag;
	}
	if (!text)
		text.sprintf("PID %04x", stream->elementary_PID);
	if (component_tag!=-1)
	{
		eServiceHandler *service=eServiceInterface::getInstance()->getService();
		if (service)
		{
			EIT *eit=service->getEIT();
			if (eit)
			{
				parseEIT(eit);
			}
			else
			{
				CONNECT( eDVB::getInstance()->tEIT.tableReady, VideoStream::EITready );
			}
		}
	}
}

void VideoStream::parseEIT(EIT* eit)
{
	for (ePtrList<EITEvent>::iterator e(eit->events); e != eit->events.end(); ++e)
	{
		if ((e->running_status>=2)||(!e->running_status))		// currently running service
		{
			for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
			{
				if (d->Tag()==DESCR_COMPONENT)
				{
					if (((ComponentDescriptor*)*d)->component_tag == component_tag )
					{
						text=((ComponentDescriptor*)*d)->text;
					}
				}
			}
		}
	}
	eit->unlock();
}

void VideoStream::EITready(int error)
{
	if (!error)
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		parseEIT(eit);
	}
}

void eVideoSelector::selected(VideoStream *l)
{
	eServiceHandler *service=eServiceInterface::getInstance()->getService();

	if (l && service)
		service->setPID(l->stream);

	close(0);
}

eVideoSelector::eVideoSelector()
	:eListBoxWindow<VideoStream>(_("Video"), 10, 330)
{
	move(ePoint(100, 100));
	CONNECT(list.selected, eVideoSelector::selected);
}

void eVideoSelector::clear()
{
	list.clearList();
}

void eVideoSelector::add(PMTEntry *pmt)
{
	new VideoStream(&list, pmt);
}

AudioStream::AudioStream(eListBox<AudioStream> *listbox, PMTEntry *stream)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)listbox), isAC3(0), isDTS(0), component_tag(-1), stream(stream)
{
	for (ePtrList<Descriptor>::iterator c(stream->ES_info); c != stream->ES_info.end(); ++c)
	{
		if (c->Tag()==DESCR_AC3)
			isAC3=1;
		else if (c->Tag() == DESCR_REGISTRATION)
		{
			RegistrationDescriptor *reg=(RegistrationDescriptor*)*c;
			if (!memcmp(reg->format_identifier, "DTS", 3))
				isDTS=1;
		} else if (c->Tag()==DESCR_ISO639_LANGUAGE)
			text=getISO639Description(((ISO639LanguageDescriptor*)*c)->language_code);
		else if (c->Tag()==DESCR_STREAM_ID)
			component_tag=((StreamIdentifierDescriptor*)*c)->component_tag;
		else if (c->Tag()==DESCR_LESRADIOS)
		{
			text=eString().sprintf("%d.) ", (((LesRadiosDescriptor*)*c)->id));
			text+=((LesRadiosDescriptor*)*c)->name;
		}
	}
	if (!text)
		text.sprintf("PID %04x", stream->elementary_PID);
	if (isAC3)
		text+=" (AC3)";
	if (isDTS)
		text+=" (DTS)";
	if (component_tag!=-1)
	{
		eServiceHandler *service=eServiceInterface::getInstance()->getService();
		if (service)
		{
			EIT *eit=service->getEIT();
			if (eit)
			{
				parseEIT(eit);
			}
			else
			{
				CONNECT( eDVB::getInstance()->tEIT.tableReady, AudioStream::EITready );
			}
		}
	}
}

void AudioStream::parseEIT(EIT* eit)
{
	for (ePtrList<EITEvent>::iterator e(eit->events); e != eit->events.end(); ++e)
	{
		if ((e->running_status>=2)||(!e->running_status))		// currently running service
		{
			for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
			{
				if (d->Tag()==DESCR_COMPONENT)
				{
					if (((ComponentDescriptor*)*d)->component_tag == component_tag )
					{
						text=((ComponentDescriptor*)*d)->text;
						if (isAC3)
							text+=" (AC3)";
						if (isDTS)
							text+=" (DTS)";
					}
				}
			}
		}
	}
	eit->unlock();
}

void AudioStream::EITready(int error)
{
	if (!error)
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		parseEIT(eit);
	}
}

void eAudioSelector::selected(AudioStream *l)
{
	eServiceHandler *service=eServiceInterface::getInstance()->getService();
	if (l && service)
	{
		service->setPID(l->stream);
		//service->setDecoder();
	}

	close(0);
}

eAudioSelector::eAudioSelector()
	:eListBoxWindow<AudioStream>(_("Audio"), 10, 330)
{
	move(ePoint(100, 100));
	CONNECT(list.selected, eAudioSelector::selected);
}

void eAudioSelector::clear()
{
	list.clearList();
}

void eAudioSelector::add(PMTEntry *pmt)
{
	new AudioStream(&list, pmt);
}


SubService::SubService(eListBox<SubService> *listbox, eDVBNamespace dvb_namespace, const LinkageDescriptor *descr)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*) listbox),
		service(dvb_namespace, 
			eTransportStreamID(descr->transport_stream_id), 
			eOriginalNetworkID(descr->original_network_id),
			eServiceID(descr->service_id), 7)
{
	text=(const char*)descr->private_data;
	service.descr = text;
}

struct selectCurSubService: public std::unary_function<SubService&, void>
{
	eServiceReferenceDVB& cur;
	eListBox<SubService> &lb;
	selectCurSubService( eServiceReferenceDVB& cur, eListBox<SubService> &lb )
		:cur(cur), lb(lb)
	{
	}

	bool operator()(SubService& service)
	{
		if ( service.service == cur )
		{
			lb.setCurrent( &service );
			return true;
		}
		return false;
	}
};

eSubServiceSelector::eSubServiceSelector( bool showbuttons )
	:eListBoxWindow<SubService>(_("multiple Services"), 10, 530),
	quickzap(0)
{
	move(ePoint(80, 70));
	cresize( eSize( getClientSize().width(), getClientSize().height()+80 ) );

	if ( showbuttons )
	{
		bToggleQuickZap = new eButton( this );
		bToggleQuickZap->resize( eSize( getClientSize().width()-20, 30 ) );
		bToggleQuickZap->move( ePoint( 10, getClientSize().height()-70 ) );
		bToggleQuickZap->setText(_("enable quickzap"));
		bToggleQuickZap->setShortcut("green");
		bToggleQuickZap->setShortcutPixmap("green");
		bToggleQuickZap->show();

		bAddToUserBouquet = new eButton( this );
		bAddToUserBouquet->resize( eSize( getClientSize().width()-20, 30 ) );
		bAddToUserBouquet->move( ePoint( 10, getClientSize().height()-40 ) );
		bAddToUserBouquet->setText(_("add to bouquet"));
		bAddToUserBouquet->setShortcut("yellow");
		bAddToUserBouquet->setShortcutPixmap("yellow");
		bAddToUserBouquet->show();

		CONNECT(bAddToUserBouquet->selected, eSubServiceSelector::addPressed );
		CONNECT(bToggleQuickZap->selected, eSubServiceSelector::quickZapPressed );
	}
	CONNECT(list.selected, eSubServiceSelector::selected);
}

void eSubServiceSelector::addPressed()
{
	if ( list.getCount() )
	{
		hide();
		list.getCurrent()->service.setServiceType(1);
		/* emit */ addToUserBouquet( &list.getCurrent()->service, 0 );
		list.getCurrent()->service.setServiceType(7);
		show();
	}
}

bool eSubServiceSelector::quickzapmode()
{
	if ( eActionMapList::getInstance()->getCurrentStyles().find("classic")
		!= eActionMapList::getInstance()->getCurrentStyles().end() )
		return true;
	else if ( quickzap )
		return true;
	else
		return false;
}

void eSubServiceSelector::quickZapPressed()
{
	quickzap ^= 1;
	if ( quickzap )
	{
		bToggleQuickZap->setText(_("disable quickzap"));
	}
	else
	{
		bToggleQuickZap->setText(_("enable quickzap"));
	}
	close(-1);
}

void eSubServiceSelector::selected(SubService *ss)
{
	if (ss) // bitte nicht schlagen f�r das was jetzt kommt *duck*
		close(0);
	else
		close(-1);
}

void eSubServiceSelector::disableQuickZap()
{
	quickzap=0;
	bToggleQuickZap->setText(_("enable quickzap"));
}

void eSubServiceSelector::clear()
{
	list.clearList();
}

void eSubServiceSelector::selectCurrent()
{
	list.forEachEntry( selectCurSubService( (eServiceReferenceDVB&)eServiceInterface::getInstance()->service, list ) );
}

void eSubServiceSelector::add(eDVBNamespace dvb_namespace, const LinkageDescriptor *ref)
{
	list.beginAtomic();
	new SubService(&list, dvb_namespace, ref);
	list.endAtomic();
}

void eSubServiceSelector::willShow()
{
	selectCurrent();
	eWindow::willShow();
}

void eSubServiceSelector::next()
{
	selectCurrent();
	if ( !list.goNext() )
		list.moveSelection( eListBox<SubService>::dirFirst );

	SubService* ss = list.getCurrent();

	if (ss)
		eServiceInterface::getInstance()->play(ss->service);
}

void eSubServiceSelector::prev()
{
	selectCurrent();
	if ( !list.goPrev() )
		list.moveSelection( eListBox<SubService>::dirLast );

	SubService* ss = list.getCurrent();

	if (ss)
		eServiceInterface::getInstance()->play(ss->service);
}

void eServiceNumberWidget::selected(int *res)
{
	if (!res)
	{
		close(-1);
		return;
	}
	close(number->getNumber());
}

void eServiceNumberWidget::timeout()
{
	close(number->getNumber());
}

eServiceNumberWidget::eServiceNumberWidget(int initial)
										:eWindow(0)
{
	setText(_("Channel"));
	move(ePoint(200, 140));
	resize(eSize(280, 120));
	eLabel *label;
	label=new eLabel(this);
	label->setText(_("Channel:"));
	label->move(ePoint(50, 15));
	label->resize(eSize(110, eSkin::getActive()->queryValue("fontsize", 20)+4));
	
	number=new eNumber(this, 1, 1, 9999, 4, 0, 1, label);
	number->move(ePoint(160, 15));
	number->resize(eSize(60, eSkin::getActive()->queryValue("fontsize", 20)+4));
	number->setNumber(initial);
	CONNECT(number->selected, eServiceNumberWidget::selected);
	CONNECT(number->numberChanged, eServiceNumberWidget::numberChanged );
	
	timer=new eTimer(eApp);
	timer->start(2000);
	CONNECT(timer->timeout, eServiceNumberWidget::timeout);	
}

eServiceNumberWidget::~eServiceNumberWidget()
{
	if (timer)
		delete timer;
}

void eServiceNumberWidget::numberChanged()
{
	timer->start(2000);
}

eZapMain *eZapMain::instance;

bool eZapMain::CheckService( const eServiceReference& ref )
{
	if ( mode == modeFile )
		return ref.path.length();
	else if ( ref.type == eServiceReference::idDVB )
	{
		switch( mode )
		{
			case modeTV:
				return ref.data[0] == 1 || ref.data[0] == 4 || ref.data[0] == 6;
			case modeRadio:
				return ref.data[0] == 2;
		}
	}
	return false;
}

static bool ModeTypeEqual( const eServiceReference& ref1, const eServiceReference& ref2 )
{
	if ( (ref1.path.length()>0) == (ref2.path.length()>0) )
	{
		if ( ref1.path.length() )
			return true;   // booth are mp3 or rec ts
		else if ( ref1.type == eServiceReference::idDVB && ref2.type == eServiceReference::idDVB )
		{
			if ( ref1.data[0] == ref2.data[0] )
				return true;  // have self types..
			else if ( ref1.data[0] & 1 && ref2.data[0] & 1 )
				return true;  // nvod, linkage, tv, nvodref
			else if ( ref1.data[0] == 4 && ref2.data[0] & 1 )
				return true;  // nvod, linkage, tv, nvodref
			else if ( ref1.data[0] & 1 && ref2.data[0] == 4 )
				return true;  // nvod, linkage, tv, nvodref
			else if ( ref1.data[0] == 6 && ref2.data[0] & 1 )
				return true;  // mosaic
			else if ( ref1.data[0] & 1 && ref2.data[0] == 6 )
				return true;  //  mosaic
		}
	}
	return false;
}

void eZapMain::onRotorStart( int newPos )
{
	if (!pRotorMsg)
	{
		pRotorMsg = new eMessageBox( eString().sprintf(_("Please wait while the motor is turning to %d.%d\xB0%c ...."),abs(newPos)/10,abs(newPos)%10,newPos<0?'W':'E'), _("Message"), 0);
		pRotorMsg->show();
	}
}

void eZapMain::onRotorStop()
{
	if (pRotorMsg)
	{
		pRotorMsg->hide();
		delete pRotorMsg;
		pRotorMsg=0;
	}
}

void eZapMain::onRotorTimeout()
{
	if (pRotorMsg)
	{
		pRotorMsg->hide();
		delete pRotorMsg;
		pRotorMsg=0;
	}
/*	pRotorMsg = new eMessageBox( _("Rotor has timeouted... check your LNB Cable, or if you use rotor drive speed for running detection then decrease the �/sec value")
									, _("Message"),
									eMessageBox::btOK|eMessageBox::iconInfo);
	pRotorMsg->show();
	timeout.start(10000, true);*/
}

void eZapMain::eraseBackground(gPainter *painter, const eRect &where)
{
	(void)painter;
	(void)where;
}

#ifndef DISABLE_FILE
void eZapMain::saveRecordings( bool destroy )
{
	// save and destroy recordingslist
	recordings->save();
	if (destroy)
	{
		eServiceInterface::getInstance()->removeRef(recordingsref);
		eServicePlaylistHandler::getInstance()->removePlaylist(recordingsref);
		recordings=0;
		recordingsref=eServiceReference();
	}
}
#endif

void eZapMain::savePlaylist(bool destroy)
{
	// save and destroy playlist
	playlist->save();
	if (destroy)
	{
		eServiceInterface::getInstance()->removeRef(playlistref);
		eServicePlaylistHandler::getInstance()->removePlaylist(playlistref);
		playlist=0;
		playlistref=eServiceReference();
	}
}

void eZapMain::loadPlaylist( bool create )
{
	// create Playlist
	if (create)
	{
		eServicePlaylistHandler::getInstance()->addNum( 0 );
		playlistref=eServiceReference( eServicePlaylistHandler::ID,
			eServiceReference::isDirectory, 0, 0 );
		playlist=(ePlaylist*)eServiceInterface::getInstance()->addRef(playlistref);
		ASSERT(playlist);
		eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), playlistref);
	}
	playlist->load((eplPath+"/playlist.epl").c_str());
	if ( !create && eZap::getInstance()->getServiceSelector()->getPath().current() == playlistref )
		eZap::getInstance()->getServiceSelector()->actualize();
}

#ifndef DISABLE_FILE
void eZapMain::loadRecordings( bool create )
{
	if ( create )
	{
		// create recordingslist..
		eServicePlaylistHandler::getInstance()->addNum( 1 );
		recordingsref=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 1);
		recordings=(ePlaylist*)eServiceInterface::getInstance()->addRef(recordingsref);
		ASSERT(recordings);
		eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile), recordingsref);
		recordings->service_name=_("recorded movies");
	}
	recordings->load(MOVIEDIR "/recordings.epl");
	if ( !create && eZap::getInstance()->getServiceSelector()->getPath().current() == recordingsref )
		eZap::getInstance()->getServiceSelector()->actualize();
}
#endif

void eZapMain::saveUserBouquets()
{
	userTVBouquets->save();
	for (std::list<ePlaylistEntry>::iterator it(userTVBouquets->getList().begin()); it != userTVBouquets->getList().end(); it++ )
	{
		ePlaylist *p = (ePlaylist*) eServiceInterface::getInstance()->addRef( it->service );
		if ( p )
		{
			p->save();
			eServiceInterface::getInstance()->removeRef( it->service );
		}
	}

	userRadioBouquets->save();
	for (std::list<ePlaylistEntry>::iterator it(userRadioBouquets->getList().begin()); it != userRadioBouquets->getList().end(); it++ )
	{
		ePlaylist *p = (ePlaylist*) eServiceInterface::getInstance()->addRef( it->service );
		if ( p )
		{
			p->save();
			eServiceInterface::getInstance()->removeRef( it->service );
		}
	}
}

void eZapMain::destroyUserBouquets( bool save )
{
	// destroy all userBouquets
	if (save)
	{
		userTVBouquets->save();
		userRadioBouquets->save();
		saveUserBouquets();
	}
	
	for (std::list<ePlaylistEntry>::iterator it(userTVBouquets->getList().begin()); it != userTVBouquets->getList().end(); it++ )
	{
		eServicePlaylistHandler::getInstance()->removePlaylist(it->service);
		eServiceInterface::getInstance()->removeRef(it->service);
	}

	eServiceInterface::getInstance()->removeRef(userTVBouquetsRef);
	eServicePlaylistHandler::getInstance()->removePlaylist(userTVBouquetsRef);
	userTVBouquets=0;
	userTVBouquetsRef=eServiceReference();

	// save and destroy userRadioBouquetList
	for (std::list<ePlaylistEntry>::iterator it(userRadioBouquets->getList().begin()); it != userRadioBouquets->getList().end(); it++ )
	{
		eServicePlaylistHandler::getInstance()->removePlaylist(it->service);
		eServiceInterface::getInstance()->removeRef(it->service);
	}

	eServiceInterface::getInstance()->removeRef(userRadioBouquetsRef);
	eServicePlaylistHandler::getInstance()->removePlaylist(userRadioBouquetsRef);
	userRadioBouquets=0;
	userRadioBouquetsRef=eServiceReference();
}

void eZapMain::loadUserBouquets( bool destroy )
{
	if ( destroy )
		destroyUserBouquets();
	// create user bouquet tv list
	eServicePlaylistHandler::getInstance()->addNum( 2 );
	userTVBouquetsRef=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 2);
	userTVBouquets=(ePlaylist*)eServiceInterface::getInstance()->addRef(userTVBouquetsRef);
	ASSERT(userTVBouquets);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), userTVBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTV), userTVBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTvRadio), userTVBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeBouquets), userTVBouquetsRef);
	userTVBouquets->service_name=_("Bouquets (TV)");
	userTVBouquets->load((eplPath+"/userbouquets.tv.epl").c_str());

	// create user bouquet radio list
	eServicePlaylistHandler::getInstance()->addNum( 3 );
	userRadioBouquetsRef=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 3);
	userRadioBouquets=(ePlaylist*)eServiceInterface::getInstance()->addRef(userRadioBouquetsRef);
	ASSERT(userRadioBouquets);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), userRadioBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRadio), userRadioBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTvRadio), userRadioBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeBouquets), userRadioBouquetsRef);
	userRadioBouquets->service_name=_("Bouquets (Radio)");
	userRadioBouquets->load((eplPath+"/userbouquets.radio.epl").c_str());

	int i=0;
	for (int d = modeTV; d <= modeRadio; d++)
	{
		ePlaylist* parentList=0;
		switch(d)
		{
			case modeTV:
				parentList = userTVBouquets;
				break;
			case modeRadio:
				parentList = userRadioBouquets;
				break;
		}
		for ( std::list<ePlaylistEntry>::iterator it(parentList->getList().begin()); it != parentList->getList().end(); it++, i++)
		{
			eServicePlaylistHandler::getInstance()->addNum( it->service.data[1] );
			eServiceReference ref = eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, it->service.data[1] );
			ref.path = it->service.path;
			(ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
		}
	}

	if(!i)  // no favlist loaded...
	{
		// create default user bouquet lists (favourite lists)
		for (int i = modeTV; i <= modeRadio; i++)
		{
			eServiceReference ref = eServicePlaylistHandler::getInstance()->newPlaylist();
			ePlaylist* parentList=0;
			eString path;
			eString name;
			switch(i)
			{
				case modeTV:
					parentList = userTVBouquets;
					path = eplPath+'/'+eString().sprintf("userbouquet.%x.tv",ref.data[1]);
					name = _("Favourites (TV)");
				break;
				case modeRadio:
					parentList = userRadioBouquets;
					path = eplPath+'/'+eString().sprintf("userbouquet.%x.radio",ref.data[1]);
					name = _("Favourites (Radio)");
				break;
			}
			addUserBouquet( parentList, path, name, ref, true );
		}
	}
	if ( destroy )
		eZap::getInstance()->getServiceSelector()->actualize();
}

void eZapMain::reloadPaths(int reset)
{
	// read for all modes last servicePaths from registry
	for (int m=modeTV; m < modeEnd; m++)
	{
		char* str;
		// normale dvb bouquet pathes...
		if ((!reset) && !eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path0", m).c_str(), str)
				&& eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path2", m).c_str(), str) )
		{
			modeLast[m][0].setString(str);
			//			eDebug(str);
			free(str);
		}
		else  // no path in registry... create default..
		{
			// workaround wenn vorher ein inoffizielles image auf der box war, wo �ber
			// 3 roots rotiert wird ( history ).. kann irgendwann wieder raus..
			eConfig::getInstance()->delKey( eString().sprintf("/ezap/ui/modes/%d/path2", m ).c_str() );

			modeLast[m][0]=eServiceStructureHandler::getRoot(m+1);
			modeLast[m][0].down( eServiceReference() );
		}
		if ((!reset) && !eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path1", m).c_str(), str)
				&&eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path2", m).c_str(), str) )
		{
			modeLast[m][1].setString(str);
			//			eDebug(str);
			free(str);
		}
		else  // no path in registry... create default..
		{
			// workaround wenn vorher ein inoffizielles image auf der box war, wo �ber
			// 3 roots rotiert wird ( history )
			eConfig::getInstance()->delKey( eString().sprintf("/ezap/ui/modes/%d/path2", m ).c_str());

			modeLast[m][1]=(m==modeTV)?userTVBouquetsRef:(m==modeRadio)?userRadioBouquetsRef:playlistref;
			modeLast[m][1].down( eServiceReference() );
		}
	}

	// set serviceSelector style
	int style;
	if (reset || eConfig::getInstance()->getKey("/ezap/ui/serviceSelectorStyle", style ) )
		style=eServiceSelector::styleSingleColumn;  // default we use single Column Style

	eZap::getInstance()->getServiceSelector()->setStyle(style);
		
	if (reset)
	{
		int oldm=mode;
		mode=-1;
		setMode(oldm);
	}
}

eZapMain::eZapMain()
	:eWidget(0, 1)
	,mute( eZap::getInstance()->getDesktop( eZap::desktopFB ) )
	,volume( eZap::getInstance()->getDesktop( eZap::desktopFB ) )
	,VolumeBar( new eProgress(&volume) ), pMsg(0), pRotorMsg(0), message_notifier(eApp, 0), timeout(eApp)
	,clocktimer(eApp), messagetimeout(eApp), progresstimer(eApp)
	,volumeTimer(eApp), recStatusBlink(eApp), doubleklickTimer(eApp), delayedStandbyTimer(eApp)
	,currentSelectedUserBouquet(0),wasSleeping(0), state(0)
{
	if (!instance)
		instance=this;
		
// Mute Symbol
	gPixmap *pm = eSkin::getActive()->queryImage("mute_symbol");
	int x = eSkin::getActive()->queryValue("mute.pos.x", 0),
		  y = eSkin::getActive()->queryValue("mute.pos.y", 0);

	if (pm && x && y )
	{
		mute.setPixmap(pm);
		mute.move( ePoint(x, y) );
		mute.resize( eSize( pm->x, pm->y ) );
		mute.pixmap_position = ePoint(0,0);
		mute.hide();
		mute.setBlitFlags( BF_ALPHATEST );
	}

// Volume Pixmap
	pm = eSkin::getActive()->queryImage("volume_grafik");
	x = eSkin::getActive()->queryValue("volume.grafik.pos.x", 0),
	y = eSkin::getActive()->queryValue("volume.grafik.pos.y", 0);

	if (pm && x && y )
	{
		volume.setPixmap(pm);
		volume.move( ePoint(x, y) );
		volume.resize( eSize( pm->x, pm->y ) );
		volume.pixmap_position = ePoint(0,0);
		volume.hide();
		volume.setBlitFlags( BF_ALPHATEST );
	}

// Volume Slider
	x = eSkin::getActive()->queryValue("volume.slider.pos.x", 0),
	y = eSkin::getActive()->queryValue("volume.slider.pos.y", 0);

	if ( x && y )
		VolumeBar->move( ePoint(x, y) );

	x = eSkin::getActive()->queryValue("volume.slider.width", 0),
	y = eSkin::getActive()->queryValue("volume.slider.height", 0);

	if ( x && y )
		VolumeBar->resize( eSize( x, y ) );

	VolumeBar->setLeftColor( eSkin::getActive()->queryColor("volume_left") );
	VolumeBar->setRightColor( eSkin::getActive()->queryColor("volume_right") );
	VolumeBar->setBorder(0);

	dvrInfoBar=new eLabel(this);
	dvrInfoBar->setName("dvrInfoBar");
	dvrInfoBar->hide();

	DVRSpaceLeft=new eLabel(dvrInfoBar);
	DVRSpaceLeft->setName("TimeLeft");

	dvbInfoBar=new eWidget(this);
	dvbInfoBar->setName("dvbInfoBar");
	dvbInfoBar->hide();

	fileInfoBar=new eWidget(this);
	fileInfoBar->setName("fileInfoBar");
	fileInfoBar->hide();

	fileinfos=new eLabel(fileInfoBar);
	fileinfos->setName("fileinfos");

	dvrfunctions=-1;
	stateOSD=0;

	recstatus=new eLabel(this);
	recstatus->setName("recStatus");
	recstatus->hide();

#ifndef DISABLE_FILE	
	Progress=new eProgressWithIndices(this);
	Progress->setName("progress_bar");
	Progress->setIndices(&indices);
#else
	Progress=new eProgress(this);
	Progress->setName("progress-bar");
#endif	

	isVT=0;
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "ezap_main"))
		eFatal("skin load of \"ezap_main\" failed");

#ifndef DISABLE_LCD
	lcdmain.show();
#endif

	ASSIGN(ChannelNumber, eLabel, "ch_number");
	ASSIGN(ChannelName, eLabel, "ch_name");

	ASSIGN(EINow, eLabel, "e_now_title");
	ASSIGN(EINext, eLabel, "e_next_title");

	ASSIGN(EINowDuration, eLabel, "e_now_duration");
	ASSIGN(EINextDuration, eLabel, "e_next_duration");

	ASSIGN(EINowTime, eLabel, "e_now_time");
	ASSIGN(EINextTime, eLabel, "e_next_time");

	ASSIGN(Description, eLabel, "description");
//	ASSIGN(VolumeBar, eProgress, "volume_bar");
//	ASSIGN(Progress, eProgress, "progress_bar");

	ASSIGN(ButtonRedEn, eLabel, "button_red_enabled");
	ASSIGN(ButtonGreenEn, eLabel, "button_green_enabled");
	ASSIGN(ButtonYellowEn, eLabel, "button_yellow_enabled");
	ASSIGN(ButtonBlueEn, eLabel, "button_blue_enabled");
	ASSIGN(ButtonRedDis, eLabel, "button_red_disabled");
	ASSIGN(ButtonGreenDis, eLabel, "button_green_disabled");
	ASSIGN(ButtonYellowDis, eLabel, "button_yellow_disabled");
	ASSIGN(ButtonBlueDis, eLabel, "button_blue_disabled");

	ASSIGN(DolbyOn, eLabel, "osd_dolby_on");
	ASSIGN(CryptOn, eLabel, "osd_crypt_on");
	ASSIGN(WideOn, eLabel, "osd_format_on");
	ASSIGN(DolbyOff, eLabel, "osd_dolby_off");
	ASSIGN(CryptOff, eLabel, "osd_crypt_off");
	ASSIGN(WideOff, eLabel, "osd_format_off");
	DolbyOn->hide();
	CryptOn->hide();
	WideOn->hide();
	DolbyOff->show();
	CryptOff->show();
	WideOff->show();

	ButtonRedEn->hide();
	ButtonRedDis->show();
	ButtonGreenEn->hide();
	ButtonGreenDis->show();
	ButtonYellowEn->hide();
	ButtonYellowDis->show();
	ButtonBlueEn->show();
	ButtonBlueDis->hide();

	Clock=new eLabel(this);
	ASSIGN(Clock, eLabel, "time");

	cur_start=cur_duration=-1;
	cur_event_text="";
	cur_event_id=-1;

	CONNECT(eEPGCache::getInstance()->EPGAvail, eZapMain::setEPGButton);

	CONNECT(eServiceInterface::getInstance()->serviceEvent, eZapMain::handleServiceEvent);

	CONNECT(timeout.timeout, eZapMain::timeOut);

	CONNECT(clocktimer.timeout, eZapMain::clockUpdate);
	CONNECT(messagetimeout.timeout, eZapMain::nextMessage);

	CONNECT(progresstimer.timeout, eZapMain::updateProgress);

	CONNECT(eDVB::getInstance()->timeUpdated, eZapMain::clockUpdate);
	CONNECT(eAVSwitch::getInstance()->volumeChanged, eZapMain::updateVolume);

	CONNECT(message_notifier.recv_msg, eZapMain::gotMessage);

	CONNECT(volumeTimer.timeout, eZapMain::hideVolumeSlider );
#ifndef DISABLE_FILE
	CONNECT(recStatusBlink.timeout, eZapMain::blinkRecord);
#endif

	CONNECT(delayedStandbyTimer.timeout, eZapMain::delayedStandby);

	CONNECT( eFrontend::getInstance()->s_RotorRunning, eZapMain::onRotorStart );
	CONNECT( eFrontend::getInstance()->s_RotorStopped, eZapMain::onRotorStop );
	CONNECT( eFrontend::getInstance()->s_RotorTimeout, eZapMain::onRotorTimeout );

	CONNECT( eWidget::showHelp, eZapMain::showHelp );

	CONNECT( i_enigmaGlobalActions->volumeUp.handler, eZapMain::volumeUp);
	CONNECT( i_enigmaGlobalActions->volumeDown.handler, eZapMain::volumeDown);
	CONNECT( i_enigmaGlobalActions->toggleMute.handler, eZapMain::toggleMute);

	actual_eventDisplay=0;

//	clockUpdate();
	standbyTime.tv_sec=-1;

#ifndef DISABLE_FILE
	skipcounter=0;
	skipping=0;
#endif

	addActionMap(&i_enigmaMainActions->map);
	addActionMap(&i_numberActions->map);

	gotPMT();
	gotSDT();
	gotEIT();

	eplPath = CONFIGDIR "/enigma";

#ifndef DISABLE_FILE
	loadRecordings(true);
#endif
	loadPlaylist(true);
	loadUserBouquets( false );

	eServiceSelector *sel = eZap::getInstance()->getServiceSelector();
	CONNECT(sel->addServiceToPlaylist, eZapMain::doPlaylistAdd);
	CONNECT(sel->addServiceToUserBouquet, eZapMain::addServiceToUserBouquet);
	CONNECT(subservicesel.addToUserBouquet, eZapMain::addServiceToUserBouquet );
	CONNECT(sel->removeServiceFromUserBouquet, eZapMain::removeServiceFromUserBouquet );
	CONNECT(sel->showMenu, eZapMain::showServiceMenu);
	CONNECT_2_1(sel->setMode, eZapMain::setMode, 0);
	CONNECT(sel->rotateRoot, eZapMain::rotateRoot);
	CONNECT(sel->moveEntry, eZapMain::moveService);
	CONNECT(sel->showMultiEPG, eZapMain::showMultiEPG);
	CONNECT(sel->showEPGList, eZapMain::showEPGList);
	CONNECT(sel->getRoot, eZapMain::getRoot);
	CONNECT(sel->deletePressed, eZapMain::deleteService );
	CONNECT(sel->renameService, eZapMain::renameService );
	CONNECT(sel->renameBouquet, eZapMain::renameBouquet );
	CONNECT(sel->newMarkerPressed, eZapMain::createMarker );

	reloadPaths();

	eServiceReference::loadLockedList( (eplPath+"/services.locked").c_str() );
	eTransponderList::getInstance()->readTimeOffsetData( (eplPath+"/timeOffsetMap").c_str() );
//	CONNECT( eEPGCache::getInstance()->timeNotValid, eZapMain::ShowTimeCorrectionWindow );

	mode=-1;  // fake mode for first call of setMode

	// get last mode form registry ( TV Radio File )
	int curMode;
	if ( eConfig::getInstance()->getKey("/ezap/ui/lastmode", curMode ) )
		curMode = 0;  // defaut TV Mode

	if (curMode < 0)
		curMode=0;

	setMode(curMode);  // do it..

	if ( eConfig::getInstance()->getKey("/ezap/ui/playlistmode", playlistmode ) )
		playlistmode = 0;  // default TV Mode

	if ( playlist->current != playlist->getConstList().end() )
		playService( playlist->current->service, psDontAdd|psSeekPos);  // then play the last service
	else if ( modeLast[mode][0].current() )
		playService( modeLast[mode][0].current() ,0 );  // then play the last service

	startMessages();

	dvrInfoBar->zOrderRaise();
	dvbInfoBar->zOrderRaise();
	CONNECT( eStreamWatchdog::getInstance()->VCRActivityChanged, eZapMain::toggleScart );
}

eZapMain::~eZapMain()
{
#ifndef DISABLE_FILE
	if ( state & stateRecording )
	{
		if ( state & stateInTimerMode )
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorUserAborted );
		else
			recordDVR(0,1);
	}
#endif
	getPlaylistPosition();

#ifndef DISABLE_FILE
	saveRecordings(true);
#endif
	savePlaylist(true);

	destroyUserBouquets(true);

	// get current selected serviceselector path
	if ( mode != -1 ) // valid mode?
		getServiceSelectorPath(modeLast[mode][0]);

  // save last mode to registry
	eConfig::getInstance()->setKey("ezap/ui/lastmode", mode );

	// save for all modes the servicePath to registry
	for (mode=modeTV; mode < modeEnd; mode++ )
	{
		for (int i=0; i < 2; i++)
		{
			eString str = modeLast[mode][i].toString();
			eConfig::getInstance()->setKey( eString().sprintf("/ezap/ui/modes/%d/path%d", mode, i).c_str(), str.c_str() );
		}
	}

	if (instance == this)
		instance = 0;

#ifndef DISABLE_LCD
	eZapLCD *pLCD=eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdShutdown->show();
	gLCDDC::getInstance()->setUpdate(0);
	if ( eDVB::getInstance()->getmID() == 5 )
		eDBoxLCD::getInstance()->switchLCD(0);
#endif
	eConfig::getInstance()->setKey("/ezap/ui/serviceSelectorStyle", eZap::getInstance()->getServiceSelector()->getStyle() );
	eConfig::getInstance()->setKey("/ezap/ui/playlistmode", playlistmode);
	eServiceReference::saveLockedList( (eplPath+"/services.locked").c_str() );
	eTransponderList::getInstance()->writeTimeOffsetData( (eplPath+"/timeOffsetMap").c_str() );
}

void eZapMain::prepareDVRHelp()
{
	addActionToHelpList(&i_enigmaMainActions->startSkipReverse);
	addActionToHelpList(&i_enigmaMainActions->play);
	addActionToHelpList(&i_enigmaMainActions->pause);
	addActionToHelpList(&i_enigmaMainActions->startSkipForward);

	addActionToHelpList(&i_enigmaMainActions->stop);
	addActionToHelpList(&i_enigmaMainActions->record);

	addActionToHelpList(&i_enigmaMainActions->showEPG);
	addActionToHelpList(&i_enigmaMainActions->showInfobarEPG);

	addActionToHelpList(&i_enigmaMainActions->playlistNextService);
	addActionToHelpList(&i_enigmaMainActions->playlistPrevService);

/*	addActionToHelpList(&i_enigmaMainActions->standby_press);
	addActionToHelpList(&i_enigmaMainActions->showInfobar);
	addActionToHelpList(&i_enigmaMainActions->hideInfobar);

	addActionToHelpList(&i_enigmaMainActions->showServiceSelector);
	addActionToHelpList(&i_enigmaMainActions->serviceListDown);
	addActionToHelpList(&i_enigmaMainActions->serviceListUp);
	addActionToHelpList(&i_enigmaMainActions->toggleIndexmark);*/
}

void eZapMain::prepareNonDVRHelp()
{
	addActionToHelpList(&i_enigmaMainActions->showMainMenu);
	addActionToHelpList(&i_enigmaMainActions->showEPG);
	addActionToHelpList(&i_enigmaMainActions->pluginVTXT);
	addActionToHelpList(&i_enigmaMainActions->toggleDVRFunctions);
	addActionToHelpList(&i_enigmaMainActions->modeTV);
	addActionToHelpList(&i_enigmaMainActions->modeRadio);
#ifndef DISABLE_FILE
	addActionToHelpList(&i_enigmaMainActions->modeFile);
#endif
	addActionToHelpList(&i_enigmaMainActions->playlistNextService);
	addActionToHelpList(&i_enigmaMainActions->playlistPrevService);

	addActionToHelpList(&i_enigmaMainActions->showEPGList);
	addActionToHelpList(&i_enigmaMainActions->showSubservices);
	addActionToHelpList(&i_enigmaMainActions->showAudio);
	addActionToHelpList(&i_enigmaMainActions->pluginExt);
}

void eZapMain::set16_9Logo(int aspect)
{
	if (aspect)
	{
		WideOff->hide();
		WideOn->show();
	} else
	{
		WideOn->hide();
		WideOff->show();
	}
}

void eZapMain::setEPGButton(bool b)
{
	if (b)
	{
		isEPG=1;
		ButtonRedDis->hide();
		ButtonRedEn->show();
	}
	else
	{
		isEPG=0;
		ButtonRedEn->hide();
		ButtonRedDis->show();
	}
}

void eZapMain::setVTButton(bool b)
{
	if (b)
	{
		ButtonBlueDis->hide();
		ButtonBlueEn->show();
	}
	else
	{
		ButtonBlueEn->hide();
		ButtonBlueDis->show();
	}
}

void eZapMain::setAC3Logo(bool b)
{
	if (b)
	{
		DolbyOff->hide();
		DolbyOn->show();
	} else
	{
		DolbyOn->hide();
		DolbyOff->show();
	}
}

void eZapMain::setSmartcardLogo(bool b)
{
	if (b)
	{
		CryptOff->hide();
		CryptOn->show();
	} else
	{
		CryptOn->hide();
		CryptOff->show();
	}
}

void eZapMain::setEIT(EIT *eit)
{
	eDebug("eZapMain::setEIT");
	cur_event_text="";

	if (eit)
	{
		eString nowtext, nexttext, nowtime="", nexttime="", descr;
		int val=0;
		int p=0;

		for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
		{
			EITEvent *event=*i;
			eServiceReferenceDVB &ref=(eServiceReferenceDVB&)eServiceInterface::getInstance()->service;

			eDebug("event->running_status=%d, p=%d", event->running_status, p );
			if ((event->running_status>=2) || ((!p) && (!event->running_status)))
			{
				eDebug("set cur_event_id to %d", event->event_id);
				cur_event_id=event->event_id;
				cur_start=event->start_time;
				cur_duration=event->duration;
				clockUpdate();

				for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					if ( d->Tag()==DESCR_LINKAGE && ((LinkageDescriptor*)*d)->linkage_type==0xB0 )
						subservicesel.clear();

				int cnt=0;
				for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					if (d->Tag()==DESCR_LINKAGE)
					{
						LinkageDescriptor *ld=(LinkageDescriptor*)*d;
						if (ld->linkage_type==0xB0)
						{
							subservicesel.add(ref.getDVBNamespace(), ld);
							cnt++;
						}
					}

				if ( cnt )  // subservices added ?
				{
					flags|=ENIGMA_SUBSERVICES;
					subservicesel.selectCurrent();
				}
			}
			for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;

					switch (p)
					{
					case 0:
						nowtext=ss->event_name;
						cur_event_text=ss->event_name;
						val|=1;
						descr=ss->text;
						break;
					case 1:
						nexttext=ss->event_name;
						val|=2;
						break;
					}
				}
			}
			tm *t=event->start_time!=-1?localtime(&event->start_time):0;
			eString start="";
			if (t && event->duration)
				start.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
			eString duration;
			if (event->duration>0)
				duration.sprintf("%d min", event->duration/60);
			else
				duration="";
			switch (p)
			{
			case 0:
			{
				int show_current_remaining=1;
				eConfig::getInstance()->getKey("/ezap/osd/showCurrentRemaining", show_current_remaining);
				if (!show_current_remaining || !eDVB::getInstance()->time_difference )
					EINowDuration->setText(duration);
				nowtime=start;
				break;
			}
			case 1:
				EINextDuration->setText(duration);
				nexttime=start;
				break;
			}
			Description->setText(descr);
			p++;
		}
		if (val&1)
		{
			EINow->setText(nowtext);
			EINowTime->setText(nowtime);
		}

		if (val&2)
		{
			EINext->setText(nexttext);
			EINextTime->setText(nexttime);
		}
	} else
	{
		EINow->setText(_("no EPG available"));
		EINext->setText("");
		EINowDuration->setText("");
		EINextDuration->setText("");
		EINowTime->setText("");
		EINextTime->setText("");
	}
	if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES))
	{
		ButtonGreenEn->show();
		ButtonGreenDis->hide();
	}
	ePtrList<EITEvent> dummy;
	if (actual_eventDisplay)
		actual_eventDisplay->setList(eit?eit->events:dummy);
}

void eZapMain::updateServiceNum( const eServiceReference &_serviceref )
{
	eService *rservice = eServiceInterface::getInstance()->addRef( refservice );
	eService *service = eServiceInterface::getInstance()->addRef( _serviceref );
	int num=-1;

	if (rservice)
		num=eZap::getInstance()->getServiceSelector()->getServiceNum(_serviceref);
	if ((num == -1) && service)
		num=eZap::getInstance()->getServiceSelector()->getServiceNum(_serviceref);
	if ((num == -1) && service && service->dvb)
		num=service->dvb->service_number;

	if ( num != -1)
	{
		ChannelNumber->setText(eString().sprintf("%d", num));
		if(eDVB::getInstance()->getmID() == 6)
		{
			eDebug("write number to led-display");
			int fd=::open("/dev/dbox/fp0",O_RDWR);
			::ioctl(fd,4,&num);
			::close(fd);
		}
	}

	if ( service )
		eServiceInterface::getInstance()->removeRef( _serviceref );
	if ( rservice )
		eServiceInterface::getInstance()->removeRef( refservice );
}

void eZapMain::updateProgress()
{
#ifndef DISABLE_LCD
	eZapLCD *pLCD=eZapLCD::getInstance();
#endif

#ifndef DISABLE_FILE
	if (serviceFlags & eServiceHandler::flagSupportPosition)
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (!handler)
			return;
		int total=handler->getPosition(eServiceHandler::posQueryLength);
		int current=handler->getPosition(eServiceHandler::posQueryCurrent);

		if (total != indices.getTotalLength())
			indices.setTotalLength(total);

		if ((total > 0) && (current != -1))
		{
			Progress->setPerc(current*100/total);
			Progress->show();
#ifndef DISABLE_LCD
			pLCD->lcdMain->Progress->setPerc(current*100/total);
			pLCD->lcdMain->Progress->show();
#endif
			int min=total-current;
			int sec=min%60;
			min/=60;
			int sign=-1;
			ChannelNumber->setText(eString().sprintf("%s%d:%02d", (sign==-1)?"-":"", min, sec));
		} else
		{
#ifndef DISABLE_LCD
			pLCD->lcdMain->Progress->hide();
#endif
			Progress->hide();
			ChannelNumber->setText("");
		}
	}
	else
#endif
	{
		updateServiceNum( eServiceInterface::getInstance()->service );

		time_t c=time(0)+eDVB::getInstance()->time_difference;
		tm *t=localtime(&c);
		if (t && eDVB::getInstance()->time_difference)
		{
			if ((cur_start <= c) && (c < cur_start+cur_duration))
			{
				Progress->setPerc((c-cur_start)*100/cur_duration);
				int show_current_remaining=1;
				eConfig::getInstance()->getKey("/ezap/osd/showCurrentRemaining", show_current_remaining);
				if (show_current_remaining)
					EINowDuration->setText(eString().sprintf("+%d min", ((cur_start+cur_duration) - c) / 60));
				else
					EINowDuration->setText(eString().sprintf("%d min", cur_duration / 60));
				Progress->show();
#ifndef DISABLE_LCD
				pLCD->lcdMain->Progress->setPerc((c-cur_start)*100/cur_duration);
				pLCD->lcdMain->Progress->show();
#endif
			} else
			{
				EINowDuration->setText(eString().sprintf("%d min", cur_duration / 60));
				Progress->hide();
#ifndef DISABLE_LCD
				pLCD->lcdMain->Progress->hide();
#endif
			}
		} else
		{
			Progress->hide();
#ifndef DISABLE_LCD
			pLCD->lcdMain->Progress->hide();
#endif
		}
	}
}

void eZapMain::setPlaylistPosition()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	if (playlist->current != playlist->getConstList().end())
		if (playlist->current->current_position != -1)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekReal, playlist->current->current_position));
}

void eZapMain::getPlaylistPosition()
{
	int time=-1;
	if (serviceFlags & eServiceHandler::flagIsSeekable)
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (!handler)
			return;
		time=handler->getPosition(eServiceHandler::posQueryRealCurrent);

		if ( playlist->current != playlist->getConstList().end() && playlist->current->service == eServiceInterface::getInstance()->service )
		{
			playlist->current->current_position=time;
		}
	}
}


void eZapMain::handleNVODService(SDTEntry *sdtentry)
{
	nvodsel.clear();
	for (ePtrList<Descriptor>::iterator i(sdtentry->descriptors); i != sdtentry->descriptors.end(); ++i)
		if (i->Tag()==DESCR_NVOD_REF)
		{
//			eDebug("nvod ref descr");
			for (ePtrList<NVODReferenceEntry>::iterator e(((NVODReferenceDescriptor*)*i)->entries); e != ((NVODReferenceDescriptor*)*i)->entries.end(); ++e)
				nvodsel.add(((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getDVBNamespace(), *e);
		}

	eService *service=eServiceInterface::getInstance()->addRef(eServiceInterface::getInstance()->service);
	if (service)
		nvodsel.setText(service->service_name.c_str());
	eServiceInterface::getInstance()->removeRef(eServiceInterface::getInstance()->service);
}

void eZapMain::setNewServiceSelectorRoot( eServiceReference root, eServiceReference path )
{
	if ( root )
	{
start:
		int cnt=0;
		int changed=0;
		eServicePath p=modeLast[mode][0];
		eServiceReference ref = p.current();
		p.up();

// evtl schon das passende root gefunden
		while ( p.current() != root )
		{
			while (p.size() > 1 )
				p.up();

			if( p.current() == root )  // gefunden...
			{
				changed=1;
				break;
			}

			rotateRoot();
			p=modeLast[mode][0];

// START WORKAROUND.. EIGENTLICH DARF DAS NICHT PASSIEREN
			if ( ++cnt > 5 )		// irgendwas l�uft hier ganz gewaltig schief...
			{
				reloadPaths(1);
				// da stellen wir doch ganz gemein die roots wieder her
				// besser als in der endlos loop zu verbleiben....
				goto start;
		// also wenns dann immer noch nicht geht..
		// dann ist eh alles kaputt.. und es gibt ne endlos loop
			}
		}
		if ( path )
			p.down( path );
		p.down( changed?eServiceReference():ref );
		setServiceSelectorPath(p);
	}
}

void eZapMain::showServiceSelector(int dir, eServiceReference root, eServiceReference path, int newmode )
{
	hide();

	entered_playlistmode=0;
	eServiceSelector *e = eZap::getInstance()->getServiceSelector();

#ifndef DISABLE_LCD
	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
	e->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif

	getServiceSelectorPath(modeLast[mode][0]);

	eServicePath savedmode[modeEnd][2];
	for ( int m=modeTV; m < modeEnd; m++ )
		for ( int i=0; i < 2; i++ )
			savedmode[m][i]=modeLast[m][i];
	int oldmode=mode;

	if ( newmode != -1 && newmode != mode )
		setMode(newmode);

	// this only have a effect, wher root is != eServiceReference()
	setNewServiceSelectorRoot( root, path );

	e->selectService(eServiceInterface::getInstance()->service);
	const eServiceReference *service = e->choose(dir); // reset path only when NOT showing specific list

#ifndef DISABLE_LCD
	pLCD->lcdMain->show();
	pLCD->lcdMenu->hide();
#endif

	int extZap=0;
	eConfig::getInstance()->getKey("/elitedvb/extra/extzapping", extZap);
	eServiceReference ref = eServiceInterface::getInstance()->service;
	if ( !service ||    // cancel pressed
			( !extZap && ref &&
				!(service->path.length() && mode == modeFile) &&
				!CheckService(*service) ) )
	{
		if ( !entered_playlistmode )
		{
			for ( int m=modeTV; m < modeEnd; m++ )
				for ( int i=0; i < 2; i++ )
					modeLast[m][i]=savedmode[m][i];

			if ( mode != oldmode ) // restore mode..
				setMode(oldmode);
			else // restore old path
				setServiceSelectorPath(modeLast[mode][0]);
		}
		else // push playlistref to top of current path
		{
			eServicePath p = eZap::getInstance()->getServiceSelector()->getPath();
			if ( p.current() != playlistref )
			{
				p.down( playlistref );
				p.down( eZap::getInstance()->getServiceSelector()->getSelected() );
				setServiceSelectorPath(p);
			}
		}
		return;
	}

	if (*service == eServiceInterface::getInstance()->service)
		return;

	if ( handleState() )
	{
		getServiceSelectorPath(modeLast[mode][0]);

		if (eZap::getInstance()->getServiceSelector()->getPath().current() != playlistref)
		{
			playlistmode=0;
			playService(*service, 0);
		}
		else
			playService(*service, playlistmode?psDontAdd:psDontAdd|psSeekPos);
	}
}

void eZapMain::nextService(int add)
{
	eServicePath p = eZap::getInstance()->getServiceSelector()->getPath();
	(void)add;

	if ( p.size() && p.current() == playlistref )
		playlistNextService();
	else
	{
		const eServiceReference *service=eZap::getInstance()->getServiceSelector()->next();
		if (!service)
			return;
		else
			getServiceSelectorPath( modeLast[mode][0] );

		if (service->flags & eServiceReference::isDirectory)
			return;

		playService(*service, 0 );
	}
}

void eZapMain::prevService()
{
	eServicePath p = eZap::getInstance()->getServiceSelector()->getPath();

	if ( p.size() && p.current() == playlistref )
		playlistPrevService();
	else
	{
		const eServiceReference *service=eZap::getInstance()->getServiceSelector()->prev();

		if (!service)
			return;
		else
			getServiceSelectorPath( modeLast[mode][0] );

		if (service->flags & eServiceReference::isDirectory)
			return;

		playService(*service, 0 );
	}
}

void eZapMain::playlistPrevService()
{
	int extZap=0;
	eConfig::getInstance()->getKey("/elitedvb/extra/extzapping", extZap);
	eServiceReference ref = eServiceInterface::getInstance()->service;
	if ( playlist->current != playlist->getConstList().end() && ref == playlist->current->service )
		getPlaylistPosition();
	while ( playlist->current != playlist->getConstList().begin())
	{
		playlist->current--;
		if ( playlist->current->service != ref && (extZap || ModeTypeEqual(ref, playlist->current->service) ) )
		{
			playService((eServiceReference&)(*playlist->current), playlistmode?psDontAdd:psDontAdd|psSeekPos);
			return;
		}
	}
}

void eZapMain::playlistNextService()
{
	int extZap=0;
	eConfig::getInstance()->getKey("/elitedvb/extra/extzapping", extZap);
	eServiceReference ref = eServiceInterface::getInstance()->service;
	if ( playlist->current != playlist->getConstList().end() && ref == playlist->current->service )
		getPlaylistPosition();
	while (playlist->current != playlist->getConstList().end())
	{
		playlist->current++;
		if (playlist->current == playlist->getConstList().end())
		{
			playlist->current--;
			return;
		}
		if ( playlist->current->service != ref && (extZap || ModeTypeEqual(ref,playlist->current->service)) )
		{
			playService((eServiceReference&)(*playlist->current), playlistmode?psDontAdd:psDontAdd|psSeekPos);
			return;
		}
	}
}

void eZapMain::volumeUp()
{
	eAVSwitch::getInstance()->changeVolume(0, -2);
	if ((!eZap::getInstance()->focus) || (eZap::getInstance()->focus == this))
	{
		volume.show();
		volumeTimer.start(2000, true);
	}
}

void eZapMain::volumeDown()
{
	eAVSwitch::getInstance()->changeVolume(0, +2);
	if ((!eZap::getInstance()->focus) || (eZap::getInstance()->focus == this))
	{
		volume.show();
		volumeTimer.start(2000, true);
	}
}

void eZapMain::hideVolumeSlider()
{
	volume.hide();
}

void eZapMain::toggleMute()
{
	eAVSwitch::getInstance()->toggleMute();
}

void eZapMain::showMainMenu()
{
	hide();

	eMainMenu mm;

#ifndef DISABLE_LCD
	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
	mm.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif

	mm.show();

	if (mm.exec() == 1 && handleState())
		eZap::getInstance()->quit();

	mm.hide();

#ifndef DISABLE_LCD
	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
#endif
}

void eZapMain::toggleTimerMode()
{
	if ( state & stateInTimerMode)
		state &= ~stateInTimerMode;
	else
		state |= stateInTimerMode;
}

void eZapMain::standbyPress(int n)
{
	standby_nomenu = n;
	gettimeofday(&standbyTime,0);
	standbyTime+=1000;
}

void eZapMain::standbyRepeat()
{
	timeval now;
	gettimeofday(&now,0);	
	if ( standbyTime < now )
		standbyRelease();
}

void eZapMain::standbyRelease()
{
	if ( standbyTime.tv_sec == -1) // we come from standby ?
		return;

	timeval now;
	gettimeofday(&now,0);
	bool b = standbyTime < now;
	if (b)
	{
		hide();
#ifndef DISABLE_LCD
		eSleepTimerContextMenu m( LCDTitle, LCDElement );
#else
		eSleepTimerContextMenu m;
#endif
		int ret;
		if (standby_nomenu)
			ret = 1; // always shutdown if shutdown_nomenu-key is pressed
		else
		{
			m.show();
			ret = m.exec();
			m.hide();
		}
		switch (ret)
		{
			case 2:
				goto standby;
			case 3:
			{
				eSleepTimer t;
				t.show();
				EITEvent *evt = (EITEvent*) t.exec();
				int i = t.getCheckboxState();
				if (evt != (EITEvent*)-1)
				{
					eServiceReference ref;
					ref.descr = ( i==2 ? _("ShutdownTimer") : _("SleepTimer") );
					eTimerManager::getInstance()->addEventToTimerList( &t,
						&ref, evt,
						ePlaylistEntry::stateWaiting|
						ePlaylistEntry::doFinishOnly|
						(i==2?ePlaylistEntry::doShutdown:0)|
						(i==3?ePlaylistEntry::doGoSleep:0)
					);
					delete evt;
				}
				t.hide();
				break;
			}
			case 4: // reboot
					eZap::getInstance()->quit(1);
					break;
			case 1: // shutdown
/*				if (handleState())*/
					eZap::getInstance()->quit();
			default:
			case 0:
				break;
		}
	}
	else
	{
standby:
		eZapStandby standby;
		hide();
		standby.show();
		state |= stateSleeping;
		standbyTime.tv_sec=-1;
		standby.exec();   // this blocks all main actions...
/*
	  ...... sleeeeeeeep
*/
		standby.hide();   // here we are after wakeup
		state &= ~stateSleeping;
	}
}

void eZapMain::showInfobar()
{
	if ( !isVisible() && eApp->looplevel() == 1 &&
			(
				!eZap::getInstance()->focus ||
				eZap::getInstance()->focus == this
			)
		 )
		show();

	if (eServiceInterface::getInstance()->service.type == eServiceReference::idDVB)
		timeout.start(6000, 1);
}

void eZapMain::hideInfobar()
{
	eServiceReference &ref = eServiceInterface::getInstance()->service;
	if ( ref.type == eServiceReference::idDVB
#ifndef DISABLE_FILE
		||
			(ref.type == eServiceReference::idUser &&
			ref.data[0] == eMP3Decoder::codecMPG )
#endif
		 )
	{
		timeout.stop();
		hide();
	}
}

#ifndef DISABLE_FILE
void eZapMain::play()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;

	if (skipping)
	{
		skipping=0;
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
	} else if (handler->getState() == eServiceHandler::statePause)
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
	else if ( handler->getState() != eServiceHandler::statePlaying )
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekAbsolute, 0));
	updateProgress();
}

void eZapMain::stop()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 0));
	handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekAbsolute, 0));
	updateProgress();
}

void eZapMain::pause()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	if (handler->getState() == eServiceHandler::statePause)
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
	else
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 0));
}

void eZapMain::record()
{
	if ( state & stateRecording )
	{
		if ( state & stateInTimerMode )
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorUserAborted );
		else
			recordDVR(0,1);
	}
	else
		recordDVR(1, 1);
}

int freeRecordSpace()
{
	struct statfs s;
	int free;
	if (statfs(MOVIEDIR, &s)<0)
		free=-1;
	else
		free=s.f_bfree/1000*s.f_bsize/1000;
	return free;
}

int eZapMain::recordDVR(int onoff, int user, const char *timer_descr )
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return -1;

	if (onoff) //  start recording
	{
		if ( freeRecordSpace() == -1 )
		{
			handleServiceEvent(eServiceEvent(eServiceEvent::evtRecordFailed));
			return -3;
		}
		if (state & stateRecording)
			recordDVR(0, 0); // try to stop recording.. should not happen
		if (state & stateRecording)
			return -2; // already recording

		eServiceReference &ref_= eServiceInterface::getInstance()->service;

		if ( (ref_.type != eServiceReference::idDVB) ||
				(ref_.data[0] < 0) || ref_.path.size())
		{
			if (user)
			{
				eMessageBox box(_("Sorry, you cannot record this service."), _("record"), eMessageBox::iconWarning|eMessageBox::btOK );
				box.show();
				box.exec();
				box.hide();
			}
			return -1; // cannot record this service
		}

		eServiceReferenceDVB &ref=(eServiceReferenceDVB&)ref_;
		// build filename
		eString servicename, descr = _("no description available");

		eService *service=0;

		if ( ref.getServiceType() > 4 && !timer_descr )  // nvod or linkage
			service = eServiceInterface::getInstance()->addRef(refservice);
		else
			service = eServiceInterface::getInstance()->addRef(ref);

		if (service)
		{
			servicename = service->service_name;
			static char strfilter[4] = { 0xC2, 0x87, 0x86, 0x00 };
			// filter short name brakets...
			for (eString::iterator it(servicename.begin()); it != servicename.end();)
				strchr( strfilter, *it ) ? it = servicename.erase(it) : it++;

			if ( ref.getServiceType() > 4 && !timer_descr )  // nvod or linkage
				eServiceInterface::getInstance()->removeRef(refservice);
			else
				eServiceInterface::getInstance()->removeRef(ref);
		}

		if ( timer_descr )
		{
//			eDebug("timer_descr = %s", timer_descr );
			descr = timer_descr;
		}
		else if (cur_event_text.length())
		{
//			eDebug("no timer_descr");
//			eDebug("use cur_event_text %s", cur_event_text.c_str() );
			descr = cur_event_text;
		}
//		else
//			eDebug("no timer_descr");

		eString filename;
		if ( servicename )
		{
//			eDebug("we have servicename... sname + \" - \" + descr(%s)",descr.c_str());
			filename = servicename + " - " + descr;
		}
		else
		{
//			eDebug("filename = descr = %s", descr.c_str() );
			filename = descr;
		}

		eString cname="";

		for (unsigned int i=0; i<filename.length(); ++i)
		{
			if (strchr("abcdefghijklkmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_- ()", filename[i]) || (filename[i] & 0x80)) 	// allow UTF-8
				cname+=filename[i];
			else
				cname+='_';
		}

		int suffix=0;
		struct stat s;
		do
		{
			filename=MOVIEDIR "/";
			filename+=cname;
			if (suffix)
				filename+=eString().sprintf(" [%d]", suffix);
			suffix++;
			filename+=".ts";
		} while (!stat(filename.c_str(), &s));

		if (handler->serviceCommand(
			eServiceCommand(
				eServiceCommand::cmdRecordOpen,
				reinterpret_cast<int>(
						strcpy(new char[strlen(filename.c_str())+1], filename.c_str())
					)
				)
			))
		{
			eDebug("couldn't record... :/");
			return -3; // couldn't record... warum auch immer.
		} else
		{
			eServiceReference ref2=ref;

  // no faked service types in recordings.epl
			if ( ref2.data[0] & 1 )  // tv or faked service type(nvod,linkage)
				ref2.data[0] = 1;  // this saved as tv service...

			ref2.path=filename;
			ref2.descr=descr;
			ePlaylistEntry en(ref2);
			en.type=ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile;
			recordings->getList().push_back(en); // add to playlist
			recordings->save(MOVIEDIR "/recordings.epl");
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
			state |= (stateRecording|recDVR);
			recstatus->show();
/*
// DETECT HARDDISC or NFS mount
			FILE *f=fopen("/proc/mounts", "r");
			char line[1024];
			int ok=0;
			if (!f)
				return -3;
			while (fgets(line, 1024, f))
			{
				if ( (strstr(line, "/dev/ide/host") || strstr(line, "nfs rw")) &&
					strstr(line, "/hdd") )
				{
					ok=1;
					break;
				}
			}
			fclose(f);
			if ( ok )*/
			{
				DVRSpaceLeft->show();
				recStatusBlink.start(500, 1);
			}
/*			else
				return -3;*/
		}
		return 0;
	}
	else   // stop recording
	{
		if ( !(state & stateRecording) )
			return -1; // not recording

		state &= ~(stateRecording|recDVR);

		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordClose));
		DVRSpaceLeft->hide();
		recStatusBlink.stop();
		recstatus->hide();

		int profimode=0;
		eConfig::getInstance()->getKey("/elitedvb/extra/profimode", profimode);

		if (user)
		{
			if (!profimode)
			{
				eMessageBox mb(_("Show recorded movies?"), _("recording finished"),  eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
				mb.show();
				int ret = mb.exec();
				mb.hide();
				if ( ret == eMessageBox::btYes )
					showServiceSelector( eServiceSelector::dirLast, eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile), recordingsref, modeFile );
			}
		}
		return 0;
	}
}

void eZapMain::startSkip(int dir)
{
	(void)dir;
	skipcounter=0;
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekBegin));
	}
	showInfobar();
	timeout.stop();
}

void eZapMain::repeatSkip(int dir)
{
	if (!skipping)
	{
		skipcounter++;
		int time=5000;
		if (skipcounter > 10)
			time=20000;
		else if (skipcounter > 30)
			time=120000;
		else if (skipcounter > 60)
			time=600000;
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSkip, (dir == skipForward)?time:-time));
		updateProgress();
	}
}

void eZapMain::stopSkip(int dir)
{
	(void)dir;
#if 0
	if (!skipcounter)
	{
		if (dir == skipForward)
			skipping++;
		else
			skipping--;

		if (skipping > 3)
			skipping=3;
		else if (skipping < -3)
			skipping=-3;

		int speed;
		if (!skipping)
			speed=1;
		else if (skipping < 0)
			speed=-(1<<(-skipping));
		else
			speed=1<<skipping;

		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, speed));
	}
#endif
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekEnd));
	}

	if (isVisible())
		timeout.start(2000, 1);
}

#endif //DISABLE_FILE

void eZapMain::handleStandby(int i)
{
	if ( i )
	{
		if ( state & stateSleeping )  // we are already sleeping
			return;
		wasSleeping = i;
	}

	if (state & stateSleeping)
	{
		wasSleeping=1;
		eZapStandby::getInstance()->wakeUp(1);
		// this breakes the eZapStandby mainloop...
		// and enigma wakes up
	}
	else switch(wasSleeping) // before record we was in sleep mode...
	{
		case 1: // delayed standby
			if ( delayedStandbyTimer.isActive() )
				delayedStandbyTimer.stop();
			else // delayed Standby after 10min
				delayedStandbyTimer.start( 1000 * 60 * 10, true );
			break;
		case 2: // complete Shutdown
			// we do hardly shutdown the box..
			// any pending timers are ignored
			message_notifier.send(eZapMain::messageShutdown);
			break;
		case 3: // immediate go to standby
			delayedStandby();
			break;
		default:
			break;
	}
}

void eZapMain::delayedStandby()
{
	wasSleeping=0;
	// use message_notifier to goto sleep...
	// we will not block the mainloop...
	message_notifier.send(eZapMain::messageGoSleep);
}

ePlaylist *eZapMain::addUserBouquet( ePlaylist *list, const eString &path, const eString &name, eServiceReference& ref, bool save )
{
	ePlaylist *pl = (ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
	pl->service_name = name;
	pl->load( path.c_str() );
	pl->save();
	eServiceInterface::getInstance()->removeRef(ref);
	ref.path=path;
	pl = (ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
	pl->load( path.c_str() );
	list->getList().push_back( ref );
	list->getList().back().type = ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile;
	list->save();
	if (save)
		pl->save();
	return pl;
}

extern bool checkPin( int pin, const char * text );

void eZapMain::renameService( eServiceSelector *sel )
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();
	ePlaylist *p=(ePlaylist*)eServiceInterface::getInstance()->addRef(path);
	if ( !p )
		return;

	std::list<ePlaylistEntry>::iterator it=std::find(p->getList().begin(), p->getList().end(), ref);
	if (it == p->getList().end())
	{
		eServiceInterface::getInstance()->removeRef(path);
		return;
	}

	if ( it->service.type == eServiceReference::idDVB )
	{
		TextEditWindow wnd(_("Enter new name:"));
		wnd.setText(_("Rename entry"));
		wnd.show();
		if ( it->service.descr.length() )
			wnd.setEditText(it->service.descr);
		else if ( it->type & ePlaylistEntry::boundFile )
		{
			int i = it->service.path.rfind('/');
			++i;
			wnd.setEditText(it->service.path.mid( i, it->service.path.length()-i ));
		}
		else
		{
			eService* s = eServiceInterface::getInstance()->addRef(it->service);
			if ( s )
			{
				wnd.setEditText(s->service_name);
				eServiceInterface::getInstance()->removeRef( it->service );
			}
		}
		wnd.setMaxChars(100);
		int ret = wnd.exec();
		wnd.hide();
		if ( !ret )
		{
			if ( it->service.descr != wnd.getEditText() )
			{
				it->service.descr = wnd.getEditText();
				sel->invalidateCurrent(it->service);
				p->save();
			}
		}
	}
	eServiceInterface::getInstance()->removeRef(path);
}

void eZapMain::deleteService( eServiceSelector *sel )
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();

	printf("path: '%s', ref: '%s', pathtype: %d, reftype: %d\n",path.toString().c_str(),ref.toString().c_str(),path.type,ref.type);

/*  i think is not so good to delete normal providers
		if (ref.data[0]==-3) // Provider
		{
			eDVB::getInstance()->settings->removeDVBBouquet(ref.data[2]);
			eDVB::getInstance()->settings->saveBouquets();
			sel->actualize();
			break;
		}*/
	ePlaylist *pl=0;
	if ( path.type == eServicePlaylistHandler::ID )
		pl =(ePlaylist*)eServicePlaylistHandler::getInstance()->addRef(path);
	if (!pl)
	{
		eMessageBox box(_("Sorry, you cannot delete this service."), _("delete service"), eMessageBox::iconWarning|eMessageBox::btOK );
		box.show();
		box.exec();
		box.hide();
		return;
	}
	bool removeEntry=true;

	std::list<ePlaylistEntry>::iterator it=std::find(pl->getList().begin(), pl->getList().end(), ref);
	if (it == pl->getList().end())
		return;

	// remove parent playlist ref...
	eServiceInterface::getInstance()->removeRef(path);

	int profimode=0;
	eConfig::getInstance()->getKey("/elitedvb/extra/profimode", profimode);

	// recorded stream selected ( in recordings.epl )
	if ( it->service.type == eServiceReference::idDVB )
	{
		if (!profimode)
		{
			if ( (it->type & (ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile))==(ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile) )
			{
				eMessageBox box(_("This is a recorded stream!\nReally delete?"), _("Delete recorded stream"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
				box.show();
				int r=box.exec();
				box.hide();
				if (r != eMessageBox::btYes)
					removeEntry=false;
			}
		}
		// delete running service ?
		if (removeEntry && eServiceInterface::getInstance()->service == ref)
			eServiceInterface::getInstance()->stop();
	} // bouquet (playlist) selected
	else if ( it->service.type == eServicePlaylistHandler::ID )
	{
		if (!profimode)
		{
			if ( (it->type & (ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile))==(ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile) )
			{
				eMessageBox box(
					_("This is a complete Bouquet!\nReally delete?"),
					_("Delete bouquet"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
				box.show();
				int r=box.exec();
				box.hide();
				if (r != eMessageBox::btYes)
					removeEntry=false;
			}
		}
	}
	if (removeEntry) // remove service.. and linked files..
	{
		eServicePlaylistHandler::getInstance()->removePlaylist( it->service );

		if (ref.type == eServicePlaylistHandler::ID)
			eServiceInterface::getInstance()->removeRef(ref);

		// move selection to prev entry... when exist...
		std::list<ePlaylistEntry>::iterator i = it;
		i++;
		if ( i == pl->getList().end() )
		{
			i=it;  
			i--;
			sel->removeCurrent(false);
		}
		else
			sel->removeCurrent(true);
		sel->updateNumbers();
		// remove the entry in playlist
		pl->deleteService(it);
		// save playlist
		pl->save();
		// enter new into the current path
	}
}

void eZapMain::renameBouquet( eServiceSelector *sel)
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();
	ePlaylist *p=(ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
	TextEditWindow wnd(_("Enter new name for the bouquet:"));
	wnd.setText(_("Rename bouquet"));
	wnd.show();
	wnd.setEditText(p->service_name);
	wnd.setMaxChars(100);
	int ret = wnd.exec();
	wnd.hide();
	if ( !ret )
	{
		// name changed?
		if ( p->service_name != wnd.getEditText() )
		{
			p->service_name=wnd.getEditText();
/*			if (ref.data[0]==-3) // dont rename provider
			{
				eDVB::getInstance()->settings->renameDVBBouquet(ref.data[2],p->service_name);
				eDVB::getInstance()->settings->saveBouquets();
			}*/
			p->save();
			sel->invalidateCurrent();
		}
	}
	eServiceInterface::getInstance()->removeRef(ref);
}

void eZapMain::createEmptyBouquet(int mode)
{
	TextEditWindow wnd(_("Enter name for the new bouquet:"));
	wnd.setText(_("Add new bouquet"));
	wnd.show();
	wnd.setEditText(eString(""));
	wnd.setMaxChars(100);
	int ret = wnd.exec();
	wnd.hide();
	if ( !ret )
	{
		eServiceReference newList=
			eServicePlaylistHandler::getInstance()->newPlaylist();
		switch ( mode )
		{
			case modeTV:
				addUserBouquet( userTVBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.tv",newList.data[1]), wnd.getEditText(), newList, true );
				break;
			case modeRadio:
				addUserBouquet( userRadioBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.radio",newList.data[1]), wnd.getEditText(), newList, true );
				break;
		}
	}
}

void eZapMain::createMarker(eServiceSelector *sel)
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();
	ePlaylist *pl=0;
	if ( path.type == eServicePlaylistHandler::ID )
		pl = (ePlaylist*)eServicePlaylistHandler::getInstance()->addRef( path );
	if ( pl )
	{
		std::set<int> markerNums;
		std::list<ePlaylistEntry>::iterator it=pl->getList().end();
		for (std::list<ePlaylistEntry>::iterator i(pl->getList().begin()); i != pl->getList().end(); ++i )
		{
			if ( i->service.flags & eServiceReference::isMarker )
				markerNums.insert( i->service.data[0] );
			if ( i->service == ref )
				it = i;
		}
		eServiceReference mark( eServiceReference::idDVB, eServiceReference::isMarker, markerNums.size() ? (*markerNums.end())+1 : 1 );
		mark.descr=_("unnamed");
		TextEditWindow wnd(_("Enter marker name:"));
		wnd.setText(_("create marker"));
		sel->hide();
		wnd.show();
		wnd.setMaxChars(50);
		int ret = wnd.exec();
		wnd.hide();
		if ( !ret )
		{
			if ( mark.descr != wnd.getEditText() )
				mark.descr = wnd.getEditText();
			if ( it != pl->getList().end() )
				pl->getList().insert( it, mark );
			else
				pl->getList().push_back( mark );
			sel->actualize();
			sel->selectService(mark);
		}
		sel->show();
		eServicePlaylistHandler::getInstance()->removeRef( path );
	}
}

void eZapMain::copyProviderToBouquets(eServiceSelector *sel)
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();

	// create new user bouquet
	currentSelectedUserBouquetRef = eServicePlaylistHandler::getInstance()->newPlaylist();

	// get name of the source bouquet
	eString name;

	const eService *pservice=eServiceInterface::getInstance()->addRef(ref);
	if (pservice)
	{
		if ( pservice->service_name.length() )
			name = pservice->service_name;
		else
			name = _("unnamed bouquet");
		eServiceInterface::getInstance()->removeRef(ref);
	}

	switch ( mode )
	{
		case modeTV:
			currentSelectedUserBouquet = addUserBouquet( userTVBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.tv",currentSelectedUserBouquetRef.data[1]), name, currentSelectedUserBouquetRef, false );
			break;
		case modeRadio:
			currentSelectedUserBouquet = addUserBouquet( userRadioBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.radio",currentSelectedUserBouquetRef.data[1]), name, currentSelectedUserBouquetRef, false );
			break;
	}

	Signal1<void,const eServiceReference&> signal;
	CONNECT(signal, eZapMain::addServiceToCurUserBouquet);

	eServicePath safe = sel->getPath();
	sel->enterDirectory(ref);
	sel->forEachServiceRef(signal, true);
	sel->setPath( safe, ref );

	currentSelectedUserBouquet->save();
	currentSelectedUserBouquet=0;
	currentSelectedUserBouquetRef = eServiceReference();
}

void eZapMain::showServiceMenu(eServiceSelector *sel)
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();
#ifndef DISABLE_LCD
	eServiceContextMenu m(ref, path, LCDTitle, LCDElement);
#else
	eServiceContextMenu m(ref, path);
#endif
	if ( !m.getCount() )
		return;
	m.show();
	int res=m.exec();
	m.hide();
	switch (res)
	{
	case 0: // cancel
		break;
	case 1: // delete service
		deleteService(sel);
		break;
	case 2: // enable/disable movemode
		toggleMoveMode(sel);
		break;
	case 3: // add service to playlist
		if (ref.flags & eServiceReference::mustDescent)
			playService(ref, playlistmode?psAdd:0);  // M3U File
		else
			doPlaylistAdd(ref);  // single service
		break;
	case 4: // add service to bouquet
		addServiceToUserBouquet(&ref);
		break;
	case 5: // toggle edit User Bouquet Mode
		toggleEditMode(sel);
		break;
	case 6: // add new bouquet
	{
		eServicePath path=sel->getPath();
		path.up();
		createEmptyBouquet(mode);
		if ( mode == modeTV && path.current() == userTVBouquetsRef )
			sel->actualize();
		else if ( mode == modeRadio && path.current() == userRadioBouquetsRef )
			sel->actualize();
		break;
	}
	case 7: // rename user bouquet
		renameBouquet(sel);
		break;
	case 8: // duplicate normal bouquet as user bouquet
		copyProviderToBouquets(sel);
		break;
	case 9: // rename service
		renameService(sel);
		break;
	case 10: // lock service ( parental locking )
	{
		if ( handleState() )
		{
			int pLockActive = eConfig::getInstance()->pLockActive();
			ref.lock();
			if ( pLockActive && ref == eServiceInterface::getInstance()->service )
				eServiceInterface::getInstance()->stop();
			sel->invalidateCurrent();
			break;
		}
	}
	case 11:  // unlock service ( parental locking )
		if ( eConfig::getInstance()->pLockActive() )
		{
			if ( !checkPin( eConfig::getInstance()->getParentalPin(), _("parental")) )
				break;
		}
		ref.unlock();
		sel->invalidateCurrent();
		break;
	case 12:  // show / hide locked service ( parental locking )
	{
		if ( handleState() )
		{
			if ( eConfig::getInstance()->pLockActive() )
			{
				if ( !checkPin( eConfig::getInstance()->getParentalPin(), _("parental")) )
					break;
			}
			eConfig::getInstance()->locked^=1;
			sel->actualize();
			if ( eServiceInterface::getInstance()->service.isLocked() && eConfig::getInstance()->locked )
				eServiceInterface::getInstance()->stop();
		}
		break;
	}
	case 13:
		createMarker(sel);
	default:
		break;
	}
}

void eZapMain::toggleMoveMode( eServiceSelector* sel )
{
	if ( sel->toggleMoveMode() )
	{
		currentSelectedUserBouquetRef=sel->getPath().current();
		currentSelectedUserBouquet=(ePlaylist*)eServiceInterface::getInstance()->addRef( currentSelectedUserBouquetRef );
	}
	else
	{
		currentSelectedUserBouquet->save();
		currentSelectedUserBouquet=0;
		eServiceInterface::getInstance()->removeRef( currentSelectedUserBouquetRef );
		currentSelectedUserBouquetRef=eServiceReference();
	}
}

int eZapMain::toggleEditMode( eServiceSelector *sel, int defmode )
{
	if ( sel->toggleEditMode() ) // toggleMode.. and get new state !!!
	{
		std::list<ePlaylistEntry> &lst =
			defmode == modeTV ?
					userTVBouquets->getList() :
			defmode == modeRadio ?
					userRadioBouquets->getList() :
			mode == modeRadio ?
				userRadioBouquets->getList() :
			userTVBouquets->getList();
		UserBouquetSelector s( lst );
		s.show();
		s.exec();
		s.hide();
		if (s.curSel)
		{
			currentSelectedUserBouquetRef = s.curSel;
			currentSelectedUserBouquet = (ePlaylist*)eServiceInterface::getInstance()->addRef( s.curSel );
			for (std::list<ePlaylistEntry>::const_iterator i(currentSelectedUserBouquet->getConstList().begin()); i != currentSelectedUserBouquet->getConstList().end(); ++i)
				eListBoxEntryService::hilitedEntrys.insert(*i);
			return 0;
		}
		else  // no user bouquet is selected...
		{
			sel->toggleEditMode();  // disable edit mode
			return -1;
		}
	}
	else
	{
		eListBoxEntryService::hilitedEntrys.clear();
		currentSelectedUserBouquet->save();
		currentSelectedUserBouquet=0;
		eServiceInterface::getInstance()->removeRef( currentSelectedUserBouquetRef );
		currentSelectedUserBouquetRef=eServiceReference();
	}
	return 0;
}

void eZapMain::addServiceToCurUserBouquet(const eServiceReference& service)
{
	currentSelectedUserBouquet->getList().push_back(service);
}

void eZapMain::playService(const eServiceReference &service, int flags)
{
	int first=0;

	if (flags&psSetMode)
	{
		if ( service.type == eServiceReference::idDVB && !service.path )
		{
			if ( service.data[0] == 2 )
				setMode( modeRadio );
			else
				setMode( modeTV );
		}
		else
			setMode( modeFile );
	}

	if (flags&psDontAdd)
	{
//		eDebug("psDontAdd");

		if ( (playlist->current != playlist->getConstList().end()) && (playlist->current->service != service) )
		{
//			eDebug("new service is played... getPlaylistPosition");
			getPlaylistPosition();
		}

//		eDebug("play");
		eServiceInterface::getInstance()->play(service);

		if ( flags&psSeekPos )
		{
			std::list<ePlaylistEntry>::iterator i =
				std::find( playlist->getList().begin(),
					playlist->getList().end(), service);

			if (i != playlist->getList().end())
			{
//			eDebug("we have stored PlaylistPosition.. get this... and set..");
				playlist->current=i;
				setPlaylistPosition();
			}
			else
				addService(service);
		}
		return;
	}
		// we assume that no "isDirectory" is set - we won't open the service selector again.
	if (!(flags&psAdd))
	{
		if (!playlistmode)		// dem user liebgewonnene playlists nicht einfach killen
		{
//			eDebug("not playlistmode.. shrink playlist");
			while (playlist->getConstList().size() > 25)
				playlist->getList().pop_front();
		}
/*		else
			eDebug("playlistmode.. do not shrink playlist");*/

		// a M3U File is selected...
		if ((!playlistmode) && (service.flags & eServiceReference::mustDescent))
		{
			playlist->getList().clear();
			first=1;
			playlistmode=1;
		}
		else
		{
			playlist->current=playlist->getList().end();
			if (playlist->current == playlist->getList().begin())
				first=1;
		}
	}

	addService(service);

	if (!(flags&psAdd))
	{
		if (first)
			playlist->current = playlist->getList().begin();
		if (playlist->current != playlist->getConstList().end())
			eServiceInterface::getInstance()->play((eServiceReference&)(*playlist->current));
		else
		{
			Description->setText(_("This directory doesn't contain anything playable!"));
			postMessage(eZapMessage(0, _("Play"), _("This directory doesn't contain anything playable!")), 1);
		}
	}

	if (eZap::getInstance()->getServiceSelector()->getPath().current() == playlistref)
		eZap::getInstance()->getServiceSelector()->actualize();
}

void eZapMain::rotateRoot()
{
	eServicePath tmp;
	getServiceSelectorPath(tmp); // save current path
	modeLast[mode][0]=modeLast[mode][1];
	modeLast[mode][1]=tmp;
	setServiceSelectorPath(modeLast[mode][0]); // set new path
}

void eZapMain::addService(const eServiceReference &service)
{
	if (service.flags & eServiceReference::mustDescent) // recursive add services..
	{
//		eDebug("recursive add");
		entered_playlistmode=1;
		Signal1<void,const eServiceReference&> signal;
		CONNECT( signal, eZapMain::addService);
		eServiceInterface::getInstance()->enterDirectory(service, signal);
		eServiceInterface::getInstance()->leaveDirectory(service);
	} else
	{
//		eDebug("addService");
		int last=0;
		if (playlist->current != playlist->getConstList().end()
			&& *playlist->current == service)
		{
			++playlist->current;
			last=1;
		}
		playlist->getList().remove(service);
		playlist->getList().push_back(ePlaylistEntry(service));
		if ((playlist->current == playlist->getConstList().end()) || last)
			--playlist->current;
	}
}

void eZapMain::doPlaylistAdd(const eServiceReference &service)
{
//	eDebug("playlistmode=%d", playlistmode);
	entered_playlistmode=1;
	if (!playlistmode)
	{
		playlistmode=1;
		playlist->getList().clear();
		playlist->current=playlist->getList().begin();
		playService(service, 0);
	}
	else
	{
//		eDebug("call playService(service,psAdd)");
		playService(service, psAdd);
	}
}

void eZapMain::addServiceToUserBouquet(eServiceReference *service, int dontask)
{
	if (!service)
		return;

	if (!dontask)
	{
		if ( (mode > modeRadio) || (mode < 0) )
			return;
		UserBouquetSelector s( mode == modeTV?userTVBouquets->getList():mode==modeRadio?userRadioBouquets->getList():userFileBouquets->getList() );
		s.show();
		s.exec();
		s.hide();

		if ( s.curSel != eServiceReference() )
		{
			currentSelectedUserBouquetRef = s.curSel;
			currentSelectedUserBouquet = (ePlaylist*)eServiceInterface::getInstance()->addRef( currentSelectedUserBouquetRef );
		}
		else
		{
			currentSelectedUserBouquet=0;
			currentSelectedUserBouquetRef=eServiceReference();
		}
	}

	if (currentSelectedUserBouquet)
	{
		std::list<ePlaylistEntry> &list =
			currentSelectedUserBouquet->getList();

		if ( std::find( list.begin(), list.end(), ePlaylistEntry(*service) ) == list.end() )
			list.push_back(*service);

		if (!dontask)
		{
			currentSelectedUserBouquet->save();
			currentSelectedUserBouquet=0;
			eServiceInterface::getInstance()->removeRef( currentSelectedUserBouquetRef );
			currentSelectedUserBouquetRef = eServiceReference();
		}
	}
}

void eZapMain::removeServiceFromUserBouquet(eServiceSelector *sel )
{
	const eServiceReference &service=sel->getSelected();
	if ((mode > modeRadio) || (mode < 0))
		return;

	if (currentSelectedUserBouquet)
		currentSelectedUserBouquet->getList().remove(service);
}

void eZapMain::showSubserviceMenu()
{
	if (!(flags & (ENIGMA_NVOD|ENIGMA_SUBSERVICES|ENIGMA_VIDEO)))
		return;

	if (isVisible())
	{
		timeout.stop();
		hide();
	}

#ifndef DISABLE_LCD
	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
#endif

	if (flags&ENIGMA_NVOD)
	{
#ifndef DISABLE_LCD
		nvodsel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
		nvodsel.show();
		nvodsel.exec();
		nvodsel.hide();
	}
	else if (flags&ENIGMA_SUBSERVICES)
	{
#ifndef DISABLE_LCD
		subservicesel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
		subservicesel.show();
		int ret = subservicesel.exec();
		subservicesel.hide();
		eServiceReferenceDVB *ref=0;
		if ( !ret && ( ref = subservicesel.getSelected() ) )
			eServiceInterface::getInstance()->play(*ref);
	}
	else if (flags&ENIGMA_VIDEO)
	{
#ifndef DISABLE_LCD
		videosel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
		videosel.show();
		videosel.exec();
		videosel.hide();
	}
#ifndef DISABLE_LCD
	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
#endif
}

void eZapMain::showAudioMenu()
{
	if (flags&ENIGMA_AUDIO)
	{
#ifndef DISABLE_LCD
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
#endif
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
#ifndef DISABLE_LCD
		audiosel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
		audiosel.show();
		audiosel.exec();
		audiosel.hide();
#ifndef DISABLE_LCD
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
#endif
	}
}

extern eString getInfo(const char *file, const char *info);

void eZapMain::runVTXT()
{
	eDebug("runVTXT");
//	if (isVT)
	{
		eZapPlugins plugins(2);
		if ( plugins.execPluginByName("tuxtxt.cfg") != "OK" )
			plugins.execPluginByName("_tuxtxt.cfg");
	}
}

void eZapMain::runPluginExt()
{
	hide();
	eZapPlugins plugins(2);
	plugins.exec();
}

void eZapMain::showEPGList(eServiceReferenceDVB service)
{
	if (service.type != eServiceReference::idDVB)
		return;
	const timeMap* e = eEPGCache::getInstance()->getTimeMap((eServiceReferenceDVB&)service);
	if (e && !e->empty())
	{
		eEPGSelector wnd(service);
#ifndef DISABLE_LCD
		eZapLCD* pLCD = eZapLCD::getInstance();
		bool bMain = pLCD->lcdMain->isVisible();
		bool bMenu = pLCD->lcdMenu->isVisible();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		wnd.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		wnd.show();
		wnd.exec();
		wnd.hide();
#ifndef DISABLE_LCD
		if (!bMenu)
			pLCD->lcdMenu->hide();
		if ( bMain )
			pLCD->lcdMain->show();
#endif
	}
}

void eZapMain::showEPG()
{
	if ( doubleklickTimerConnection.connected() )
		doubleklickTimerConnection.disconnect();

	if ( eServiceInterface::getInstance()->service.type != eServiceReference::idDVB)
		return;

	int stype = ((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getServiceType();

	eServiceReferenceDVB& ref = ( stype > 4 ) ? refservice : (eServiceReferenceDVB&)eServiceInterface::getInstance()->service;
	const eService *service=0;

	service = eServiceInterface::getInstance()->addRef( ref );

	if (!service && !(ref.flags & eServiceReference::isDirectory) )
		return;

	if (isVisible())
	{
		timeout.stop();
		hide();
	}

	ePtrList<EITEvent> events;
	if (isEPG)
	{
		const timeMap* pMap = eEPGCache::getInstance()->getTimeMap( ref );
		if (pMap)  // EPG vorhanden
		{
			timeMap::const_iterator It = pMap->begin();

			while (It != pMap->end())
			{
				events.push_back( new EITEvent(*It->second) );
				It++;
			}
			actual_eventDisplay=new eEventDisplay( service->service_name.c_str(), ref, &events );
		}
	}
	else
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		actual_eventDisplay=new eEventDisplay( service->service_name.c_str(), ref, eit?&eit->events:&events);
		if (eit)
			eit->unlock(); // HIER liegt der hund begraben.
	}
#ifndef DISABLE_LCD
	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
#endif
	if (actual_eventDisplay)
	{
#ifndef DISABLE_LCD
		actual_eventDisplay->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
		actual_eventDisplay->show();
		actual_eventDisplay->exec();
		actual_eventDisplay->hide();
	}
	events.setAutoDelete(true);
#ifndef DISABLE_LCD
	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
#endif
	if (actual_eventDisplay)
	{
		delete actual_eventDisplay;
		actual_eventDisplay=0;
	}

	eServiceInterface::getInstance()->removeRef( ref );
}

void eZapMain::showHelp( ePtrList<eAction>* actionHelpList, int helpID )
{
	if ( (actionHelpList && actionHelpList->size()) || (helpID) )
	{
		eHelpWindow helpwin(*actionHelpList, helpID);

		helpwin.show();
		helpwin.exec();
		helpwin.hide();
	}
}

bool eZapMain::handleState(int justask)
{
	eString text, caption;
	bool b=false;

	int profimode=0;
	eConfig::getInstance()->getKey("/elitedvb/extra/profimode", profimode);

#ifndef DISABLE_FILE
	if ( state & stateRecording )
	{
		if ( state & stateInTimerMode )
		{
			if (state & recDVR )
				text=_("A timer based digital recording is currently in progress!\n"
							"This stops the timer, and digital recording!");
			else
				return true; // we wouldn't destroy the VCR Recording *g*
/*				text=_("Currently an timer based VCR recording is in progress!\n"
							"This stops the timer, and the VCR recording!");*/
		}
		else
		{
			if (state & recDVR )
				text=_("A digital recording is currently in progress!\n"
							"This stops the digital recording!");
			else
				return true; // we wouldn't destroy the VCR Recording *g*
/*				text=_("Currently an VCR recording is in progress!\n"
							"This stops the VCR recording!");*/
		}
	}
	else
#endif //DISABLE_FILE
	if ( state & stateInTimerMode )
	{
		text=_("A timer event is currently in progress!\n"
					"This stops the timer event!");
	}
	else		// not timer event or recording in progress
		return true;

	if (!profimode)
	{
		eMessageBox box(text, _("Really do this?"), eMessageBox::iconQuestion|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
		box.show();
		b = (box.exec() == eMessageBox::btYes);
		box.hide();
	}
	else
		b=true;

	if (b && !justask)
	{
		if (state & stateInTimerMode)
		{
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorUserAborted );
		}
#ifndef DISABLE_FILE
		else if (state & stateRecording)
		{
			if (state & recDVR)
				recordDVR(0,0);
			else
				; // here send LIRC VCR Stop
		}
#endif
	}
	return b;
}

#ifndef DISABLE_FILE
void eZapMain::blinkRecord()
{
	if (state & stateRecording)
	{
		if (isVisible())
		{
			if (recstatus->isVisible())
				recstatus->hide();
			else
				recstatus->show();

			static int cnt=0;
			static int swp=0;
			int fds=freeRecordSpace();
			if (fds)
			{
				if (!(cnt++ % 7))
					swp^=1;
				if (swp)
				{
					if (fds<1024)
						DVRSpaceLeft->setText(eString().sprintf("%dMB\nfree", fds));
					else
						DVRSpaceLeft->setText(eString().sprintf("%d.%02d GB\nfree", fds/1024, (int)((fds%1024)/10.34) ));
				}
				else
				{
					int min = fds/33;

					if (min<60)
						DVRSpaceLeft->setText(eString().sprintf("~%d min\nfree", min ));
					else
						DVRSpaceLeft->setText(eString().sprintf("~%dh%02dmin\nfree", min/60, min%60 ));
				}
			}
		}
		recStatusBlink.start(500, 1);
	}
}
#endif // DISABLE_FILE

int eZapMain::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
	{
		int num=0;
		stopMessages();

		// any user action stops pending delayedStandbyTimer
		if ( delayedStandbyTimer.isActive() )
		{
			delayedStandbyTimer.stop();
			wasSleeping=0;
		}

		if (event.action == &i_enigmaMainActions->showMainMenu)
		{
			int oldmode=mode;
			showMainMenu();
			if (mode != oldmode)
				showServiceSelector(-1);
		}
		else if (event.action == &i_enigmaMainActions->standby_press)
		{
			if ( handleState() )
				standbyPress(0);
		}
		else if (event.action == &i_enigmaMainActions->standby_nomenu_press)
		{
			if ( handleState() )
				standbyPress(1);
		}
		else if (event.action == &i_enigmaMainActions->standby_repeat)
			standbyRepeat();
		else if (event.action == &i_enigmaMainActions->standby_release)
			standbyRelease();
		else if ( !isVisible() && event.action == &i_enigmaMainActions->showInfobar)
		{
			stateOSD=1;
			showInfobar();
		}
		else if (event.action == &i_enigmaMainActions->hideInfobar)
		{
			stateOSD=0;
			hideInfobar();
		}
		else if ( isVisible() && event.action == &i_enigmaMainActions->showInfobarEPG)
			showEPG();
		else if (event.action == &i_enigmaMainActions->showServiceSelector)
			showServiceSelector(-1);
		else if (event.action == &i_enigmaMainActions->showSubservices )
		{
			if ( handleState() )
				showSubserviceMenu();
		}
		else if (event.action == &i_enigmaMainActions->showAudio)
			showAudioMenu();
		else if (event.action == &i_enigmaMainActions->pluginVTXT)
			runVTXT();
		else if (event.action == &i_enigmaMainActions->pluginExt)
			runPluginExt();
		else if (event.action == &i_enigmaMainActions->showEPGList && isEPG)
		{
			showEPGList((eServiceReferenceDVB&)eServiceInterface::getInstance()->service);
		}
		else if ( subservicesel.quickzapmode() && event.action == &i_enigmaMainActions->nextSubService )
		{
			if ( flags&ENIGMA_SUBSERVICES && handleState() )
				subservicesel.next();
		}
		else if (event.action == &i_enigmaMainActions->nextService)
		{
			if ( handleState() )
				nextService();
		}
		else if ( subservicesel.quickzapmode() && event.action == &i_enigmaMainActions->prevSubService )
		{
			if ( flags&ENIGMA_SUBSERVICES && handleState() )
				subservicesel.prev();
		}
		else if (event.action == &i_enigmaMainActions->prevService)
		{
			if ( handleState() )
				prevService();
		}
		else if (event.action == &i_enigmaMainActions->playlistNextService)
		{
			if ( handleState() )
				playlistNextService();
		}
		else if (event.action == &i_enigmaMainActions->playlistPrevService)
		{
			if ( handleState() )
				playlistPrevService();
		}
		else if (event.action == &i_enigmaMainActions->serviceListDown)
			showServiceSelector(eServiceSelector::dirDown);
		else if (event.action == &i_enigmaMainActions->serviceListUp)
			showServiceSelector(eServiceSelector::dirUp);
		else if (event.action == &i_enigmaMainActions->volumeUp)
			volumeUp();
		else if (event.action == &i_enigmaMainActions->volumeDown)
			volumeDown();
		else if (event.action == &i_enigmaMainActions->toggleMute)
			toggleMute();
#ifndef DISABLE_FILE
		else if (dvrfunctions && event.action == &i_enigmaMainActions->play)
			play();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stop)
		{
			if ( state & stateRecording )
			{
				if ( handleState(1) )
					record();
			}
			else
				stop();
		}
		else if (dvrfunctions && event.action == &i_enigmaMainActions->pause)
			pause();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->record )
		{
			if ( state & stateRecording && !(state & stateInTimerMode) )
			{
#ifndef DISABLE_LCD
				eRecordContextMenu menu(LCDTitle, LCDElement);
#else
				eRecordContextMenu menu;
#endif
				hide();
				menu.show();
				int ret = menu.exec();
				menu.hide();
				switch ( ret )
				{
					case 1: // stop record now
						record();
					break;

					case 2: // set Record Duration...
					{
						eTimerInput e;
						e.show();
						EITEvent *evt = (EITEvent*) e.exec();

						if (evt != (EITEvent*)-1)
						{
							int i = e.getCheckboxState();
							eTimerManager::getInstance()->addEventToTimerList( &e, &eServiceInterface::getInstance()->service, evt,
									ePlaylistEntry::stateWaiting|
									ePlaylistEntry::RecTimerEntry|
									ePlaylistEntry::recDVR|
									ePlaylistEntry::doFinishOnly|
									(i==2?ePlaylistEntry::doShutdown:0)|
									(i==3?ePlaylistEntry::doGoSleep:0)
									);
							delete evt;
						}

						e.hide();
					}
					break;

					case 3: // set Record Stop Time...
					{
						eRecTimeInput e;
						e.show();
						EITEvent *evt = (EITEvent*) e.exec();
						if (evt != (EITEvent*)-1)
						{
							int i = e.getCheckboxState();
							eTimerManager::getInstance()->addEventToTimerList( &e, &eServiceInterface::getInstance()->service, evt,
								ePlaylistEntry::stateWaiting|
								ePlaylistEntry::RecTimerEntry|
								ePlaylistEntry::recDVR|
								ePlaylistEntry::doFinishOnly|
								(i==2?ePlaylistEntry::doShutdown:0)|
								(i==3?ePlaylistEntry::doGoSleep:0)
 							);
							delete evt;
						}
						e.hide();
					}
					break;

					case 0:
					default:
						;
				}
			}
			else if ( handleState(1) )
			{
				record();
			}
		}
		else if (dvrfunctions && event.action == &i_enigmaMainActions->startSkipForward)
			startSkip(skipForward);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->repeatSkipForward)
			repeatSkip(skipForward);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stopSkipForward)
			stopSkip(skipForward);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->startSkipReverse)
			startSkip(skipReverse);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->repeatSkipReverse)
			repeatSkip(skipReverse);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stopSkipReverse)
			stopSkip(skipReverse);
#endif // DISABLE_FILE
		else if (event.action == &i_enigmaMainActions->showEPG)
			showEPG_Streaminfo();
		else if (event.action == &i_numberActions->key0)
		{
			if ( !playlistmode && playlist->getConstList().size() > 1 )
			{
				std::list<ePlaylistEntry>::iterator prev,last;
				last = playlist->getList().end();
				prev = --last;
				prev--;
				eServiceReference ref = eServiceInterface::getInstance()->service;
				if ( !ref || last->service != ref )
					break;
		// do not zap with the 0 to a service with another type
				int extZap=0;
				eConfig::getInstance()->getKey("/elitedvb/extra/extzapping", extZap);
				if ( extZap || ModeTypeEqual( prev->service ,last->service ) )
				{
					std::iter_swap( prev, last );
					playlist->current=last;
					playService( playlist->current->service, psDontAdd|psSeekPos );
				}
			}
		}
		else if (event.action == &i_numberActions->key1)
			num=1;
		else if (event.action == &i_numberActions->key2)
			num=2;
		else if (event.action == &i_numberActions->key3)
			num=3;
		else if (event.action == &i_numberActions->key4)
			num=4;
		else if (event.action == &i_numberActions->key5)
			num=5;
		else if (event.action == &i_numberActions->key6)
			num=6;
		else if (event.action == &i_numberActions->key7)
			num=7;
		else if (event.action == &i_numberActions->key8)
			num=8;
		else if (event.action == &i_numberActions->key9)
			num=9;
		else if (mode != modeFile && event.action == &i_enigmaMainActions->showUserBouquets)
			showServiceSelector( -1, mode==modeTV?userTVBouquetsRef:userRadioBouquetsRef );
		else if (mode != modeFile && event.action == &i_enigmaMainActions->showDVBBouquets)
		{
			eServiceReference root = eServiceStructureHandler::getRoot(mode==modeTV?eServiceStructureHandler::modeTV:eServiceStructureHandler::modeRadio);
			if ( mode == modeTV )
				showServiceSelector( -1, root, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1), 0xFFFFFFFF) );
			else
				showServiceSelector( -1, root, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2, 0xFFFFFFFF ) );
		}
#ifndef DISABLE_FILE
		else if (event.action == &i_enigmaMainActions->showRecMovies)
			showServiceSelector( eServiceSelector::dirLast, eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile), recordingsref, modeFile );
		else if (event.action == &i_enigmaMainActions->showPlaylist && mode == modeFile)
			showServiceSelector( -1, playlistref );
#endif
		else if (event.action == &i_enigmaMainActions->modeRadio)
		{
			if ( handleState() )
			{
				if ( mode != modeRadio )
					setMode(modeRadio, 1);
				showServiceSelector(-1);
			}
		}
		else if (event.action == &i_enigmaMainActions->modeTV)
		{
			if ( handleState() )
			{
				if ( mode != modeTV )
					setMode(modeTV, 2);
				showServiceSelector(-1);
			}
		}
		else if (event.action == &i_enigmaMainActions->modeFile)
		{
			if ( handleState() )
			{
				if ( mode != modeFile )
					setMode(modeFile, 2);
				showServiceSelector(-1);
			}
		}
#ifndef DISABLE_FILE
		else if (event.action == &i_enigmaMainActions->toggleDVRFunctions)
		{
			showServiceInfobar(!dvrfunctions);
			showInfobar();
		}
		else if (event.action == &i_enigmaMainActions->toggleIndexmark)
			toggleIndexmark();
		else if (event.action == &i_enigmaMainActions->indexSeekNext)
			indexSeek(1);
		else if (event.action == &i_enigmaMainActions->indexSeekPrev)
			indexSeek(-1);
#endif //DISABLE_FILE
		else
		{
			startMessages();
			break;
		}
		startMessages();
		if ( mode == modeFile )
			num=-1;
		else if ( num && handleState())
		{
			hide();
			eServiceNumberWidget s(num);
			s.show();
			num = s.exec();
			if (num != -1)
			{
				if (num < 200)	// < 200 is in favourite list
				{
					ePlaylist *p=0;
					switch(mode)
					{
						case modeTV:
							if ( userTVBouquets->getList().size() )
								p=userTVBouquets;
						break;
						case modeRadio:
							if ( userRadioBouquets->getList().size() )
								p=userRadioBouquets;
						break;
					}
					if ( p )
					{
						ePlaylist *pl = (ePlaylist*)eServiceInterface::getInstance()->addRef( p->getList().front().service );
						std::list<ePlaylistEntry>::iterator it(pl->getList().begin());

						for (; it != pl->getList().end(); it++ )
							if ( it->service.flags & eServiceReference::isMarker )
								continue;
							else if (!--num)
								break;

						if (!num)
							playService( it->service, 0);

						eServiceInterface::getInstance()->removeRef( p->getList().front().service );
					}
				}
				else
				{
					eServiceReferenceDVB s=eDVB::getInstance()->settings->getTransponders()->searchServiceByNumber(num);
					if (s)
						playService(s, 0);
				}
			}
			num = 0;
  	}
		return 1;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void eZapMain::handleServiceEvent(const eServiceEvent &event)
{
	switch (event.type)
	{
	case eServiceEvent::evtStateChanged:
		break;
	case eServiceEvent::evtFlagsChanged:
	{
		serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
		setSmartcardLogo( serviceFlags & eServiceHandler::flagIsScrambled );
		if (serviceFlags & eServiceHandler::flagSupportPosition)
		{
			progresstimer.start(1000);
		} else
		{
			progresstimer.stop();
		}
		updateProgress();
		showServiceInfobar(serviceFlags & eServiceHandler::flagIsSeekable);
		break;
	}
	case eServiceEvent::evtAspectChanged:
	{
		int aspect = eServiceInterface::getInstance()->getService()->getAspectRatio();
		set16_9Logo(aspect);
		break;
	}
	case eServiceEvent::evtStart:
	{
		int err = eServiceInterface::getInstance()->getService()->getErrorInfo();
		serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
		startService(eServiceInterface::getInstance()->service, err);
		break;
	}
	case eServiceEvent::evtStop:
		leaveService();
		break;
	case eServiceEvent::evtGotEIT:
	{
		eDebug("enigma_main::handleServiceEvent.. evtGotEIT");
		gotEIT();
		break;
	}
	case eServiceEvent::evtGotSDT:
		gotSDT();
		break;
	case eServiceEvent::evtGotPMT:
		gotPMT();
		break;
#ifndef DISABLE_FILE
	case eServiceEvent::evtRecordFailed:
		if ( state&stateInTimerMode )
			message_notifier.send(eZapMain::messageNoRecordSpaceLeft);
		else
			recordDVR(0,1);  // stop Recording
		break;
	case eServiceEvent::evtEnd:
	{
		int serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
		if(! (serviceFlags & eServiceHandler::flagIsTrack)  )
			break;

		if (playlist->current != playlist->getConstList().end())
		{
			playlist->current->current_position=-1;
			++playlist->current;
		}
		if (playlist->current != playlist->getConstList().end() )
		{
			playlist->current->current_position=-1;		// start from beginning.
			eServiceInterface::getInstance()->play((eServiceReference&)(*playlist->current));
		}
		else if (!playlistmode)
			nextService(1);
		else
			eDebug("ende im gelaende.");
		break;
	}
	case eServiceEvent::evtStatus:
	{
		eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
		if (!sapi)
			return;
		eDebug("evtStatus");
		showServiceInfobar(0);
		fileinfos->setText(sapi->getInfo(0));
		break;
	}
	case eServiceEvent::evtInfoUpdated:
	{
		eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
		if (!sapi)
			return;
		showServiceInfobar(0);
		eString str = sapi->getInfo(2);
		fileinfos->setText(sapi->getInfo(1)+(str.length()>2?"\n"+str:""));
		break;
	}
#endif // DISABLE_FILE
	}
}

void eZapMain::showEPG_Streaminfo()
{
	if ( doubleklickTimer.isActive() )
	{
		doubleklickTimer.stop();
		doubleklickTimerConnection.disconnect();
		if ( isVisible() )
			hide();
		eStreaminfo si(0, eServiceInterface::getInstance()->service);
#ifndef DISABLE_LCD
		si.setLCD(LCDTitle, LCDElement);
#endif
		si.show();
		si.exec();
		si.hide();
	}
	else
	{
		doubleklickTimer.start(300,true);
		doubleklickTimerConnection = CONNECT( doubleklickTimer.timeout, eZapMain::showEPG );
	}
}

void eZapMain::startService(const eServiceReference &_serviceref, int err)
{
#ifndef DISABLE_FILE
	skipcounter=0;
#endif
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();

	if (!sapi)
		return;

	eService *service=eServiceInterface::getInstance()->addRef(_serviceref);

	if (_serviceref.type == eServiceReference::idDVB )
	{
		isVT = Decoder::parms.tpid != -1;

		const eServiceReferenceDVB &serviceref=(const eServiceReferenceDVB&)_serviceref;

//		setVTButton(isVT);

		// es wird nur dann versucht einen service als referenz-service zu uebernehmen, wenns den ueberhaupt
		// gibt.

		switch (serviceref.getServiceType())
		{
			case 1: // TV
			case 2: // radio
			case 4: // nvod ref
				refservice=serviceref;
				break;
		}

		eService *rservice=0;

		if ( refservice != serviceref  // linkage or nvod
			&& !( refservice.flags & eServiceReference::flagDirectory )
			&& !serviceref.path.length() )
		{
			rservice=eServiceInterface::getInstance()->addRef(refservice);

			if (refservice.getServiceType()==4) // nvod ref service
				flags|=ENIGMA_NVOD;
		}

		switch ( serviceref.getServiceType() )
		{
			case 4:
				flags|=ENIGMA_NVOD;
			default:
				subservicesel.disableQuickZap();
			case 7:
				break;
		}

		eString name="";

		if (rservice)
		{
			if ( refservice.descr.length() )
				name = refservice.descr;
			else
				name = rservice->service_name;

			eServiceInterface::getInstance()->removeRef( refservice );
		}
		else if (serviceref.descr.length())
			name = serviceref.descr;
		else if (service)
			name=service->service_name;

		if (!name.length())
			name="unknown service";

		if (serviceref.getServiceType() == 7)  // linkage service..
			name+=" - " + serviceref.descr;

#ifndef DISABLE_LCD
		eZapLCD::getInstance()->lcdMain->setServiceName(name);
#endif

		if ( !serviceref.path.length() &&   // no recorded movie
				eFrontend::getInstance()->Type() == eFrontend::feSatellite )
		{
			int opos=0;
			if (rservice)
				opos = refservice.data[4]>>16;
			else
				opos = serviceref.data[4]>>16;
			name+=eString().sprintf(" (%d.%d\xB0%c)", abs(opos / 10), abs(opos % 10), opos>0?'E':'W');
//			name+=eString().sprintf("(%04x)",((eServiceReferenceDVB&)_serviceref).getServiceID().get() );
		}
		ChannelName->setText(name);

		int hideerror=0;
		if(err!=0)
		    eConfig::getInstance()->getKey("/elitedvb/extra/hideerror", hideerror);

		switch (err)
		{
		case 0:
			Description->setText("");
			postMessage(eZapMessage(0), 1);
			break;
		case -EAGAIN:
			Description->setText(_("One moment please..."));
			postMessage(eZapMessage(0, _("switch"), _("One moment please...")), 1);
			break;
		case -ENOENT:
			if (hideerror) break;
			Description->setText(_("Service could not be found !"));
			postMessage(eZapMessage(0, _("switch"), _("Service could not be found !")), 1);
			break;
		case -ENOCASYS:
		{
			if (hideerror) break;

			int serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
			if( serviceFlags & eServiceHandler::flagIsScrambled )
			{
				Description->setText(_("This service could not be descrambled"));
				postMessage(eZapMessage(0, _("switch"), _("This service could not be descrambled"), 2), 1);
				eDebug("This service could not be descrambled");
			}
			break;
		}
		case -ENOSTREAM:
			if (hideerror) break;
			Description->setText(_("This service doesn't currently send a signal"));
			postMessage(eZapMessage(0, _("switch"), _("This service doesn't currently send a signal"), 2), 1);
			eDebug("This service doesn't currently send a signal");
			break;
		case -ENOSYS:
			if (hideerror) break;
			Description->setText(_("This content could not be displayed"));
			eDebug("This content could not be displayed");
			postMessage(eZapMessage(0, _("switch"), _("This content could not be displayed"), 2), 1);
			break;
		case -ENVOD:
			Description->setText(_("NVOD Service - please select a starttime"));
			eDebug("NVOD Service - please select a starttime");
			postMessage(eZapMessage(0, _("switch"), _("NVOD Service - please select a starttime"), 5), 1);
			break;
		default:
			if (hideerror) break;
			Description->setText(_("Unknown error!!"));
			eDebug("Unknown error!!");
			postMessage(eZapMessage(0, _("switch"), _("Unknown error!!")), 1);
			break;
		}
		updateServiceNum( _serviceref );

		if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES|ENIGMA_VIDEO))
		{
			ButtonGreenDis->hide();
			ButtonGreenEn->show();
		}
		else
		{
			ButtonGreenEn->hide();
			ButtonGreenDis->show();
		}

		if (flags&ENIGMA_AUDIO)
		{
			ButtonYellowDis->hide();
			ButtonYellowEn->show();
		}
		else
		{
			ButtonYellowEn->hide();
			ButtonYellowDis->show();
		}
	}
#ifndef DISABLE_FILE
	else
	{
		eString name;
		postMessage(eZapMessage(0), 1);
		if (service)
			name=service->service_name;
		else
			name="bla :(";

		ChannelName->setText(name);
#ifndef DISABLE_LCD
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->setServiceName(name);
#endif
		if (service && service->id3)
		{
			std::map<eString,eString> &tags = service->id3->getID3Tags();
			eString artist="unknown artist", album="unknown album", title="", num="";
			if (tags.count("TALB"))
				album=tags.find("TALB")->second;
			if (tags.count("TIT2"))
				title=tags.find("TIT2")->second;
			if (tags.count("TPE1"))
				artist=tags.find("TPE1")->second;
			if (tags.count("TRCK"))
				num=tags.find("TRCK")->second;
			eString text = artist + ": " + album + '\n';
			if (num)
				text +='[' + num + ']';
			text+=title;
			fileinfos->setText(text);
		}
	}
#endif // DISABLE_FILE
	showInfobar();

	eServiceInterface::getInstance()->removeRef(_serviceref);

// Quick und Dirty ... damit die aktuelle Volume sofort angezeigt wird.
	eAVSwitch::getInstance()->sendVolumeChanged();

	if ( eServiceInterface::getInstance()->service.type == eServiceReference::idDVB )
		timeout.start((sapi->getState() == eServiceHandler::statePlaying)?5000:2000, 1);
}

void eZapMain::gotEIT()
{
	eDebug("eZapMain::gotEIT");
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
	{
		eDebug("no sapi");
		return;
	}

	EIT *eit=sapi->getEIT();
	eDebug("eit = %p", eit);
	int old_event_id=cur_event_id;
	eDebug("old_event_id = %d...call setEIT", cur_event_id);
	setEIT(eit);
	eDebug("cur_event_id = %d", cur_event_id);
	if (eit)
	{
		int state=0;
		if (old_event_id != cur_event_id)
		{
			eConfig::getInstance()->getKey("/ezap/osd/showOSDOnEITUpdate", state);

			if (state)
			{
				showInfobar();
				if (eServiceInterface::getInstance()->service.type == eServiceReference::idDVB)
					timeout.start((sapi->getState() == eServiceHandler::statePlaying)?10000:2000, 1);
			}
		}
		eDebug("unlock eit");
		eit->unlock();
	}
	else
		eDebug("no eit");
}

void eZapMain::gotSDT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;

	SDT *sdt=sapi->getSDT();
	if (!sdt)
		return;

	switch (((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getServiceType())
	{
	case 0x4:	// nvod reference
	{
		for (ePtrList<SDTEntry>::iterator i(sdt->entries); i != sdt->entries.end(); ++i)
		{
			if (eServiceID(i->service_id)==((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getServiceID())
			{
				handleNVODService(*i);
			}
		}
		break;
	}
	}
	sdt->unlock();
}

void eZapMain::gotPMT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;

	PMT *pmt=sapi->getPMT();
	if (!pmt)
		return;

	bool isAc3 = false;
	int numaudio=0;
	int numvideo=0;
	audiosel.clear();
	videosel.clear();
	for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;
		int isaudio=0, isvideo=0;
		if (pe->stream_type==1)
			isvideo=1;
		else if (pe->stream_type==2)
			isvideo=1;
		else if (pe->stream_type==3)
			isaudio=1;
		else if (pe->stream_type==4)
			isaudio=1;
		else if (pe->stream_type==6)
		{
			for (ePtrList<Descriptor>::iterator d(pe->ES_info); d != pe->ES_info.end(); ++d)
			{
				if (d->Tag()==DESCR_AC3)
				{
					isaudio++;
					isAc3=true;
				} else if (d->Tag() == DESCR_REGISTRATION)
				{
					RegistrationDescriptor *reg=(RegistrationDescriptor*)*d;
					if (!memcmp(reg->format_identifier, "DTS", 3))
					{
						isaudio++;
						isAc3=true;
					}
				}
			}
		}
		if (isaudio)
		{
			audiosel.add(pe);
			numaudio++;
		}
		if (isvideo)
		{
			videosel.add(pe);
			numvideo++;
		}
	}
	if (numaudio>1)
		flags|=ENIGMA_AUDIO;
	else
		flags&=~ENIGMA_AUDIO;

	if (numvideo>1)
		flags|=ENIGMA_VIDEO;
	else
		flags&=~ENIGMA_VIDEO;

	setAC3Logo(isAc3);

	pmt->unlock();
}

void eZapMain::timeOut()
{
	int state=1;
	eConfig::getInstance()->getKey("/ezap/osd/anableAutohideOSDOn", state);
	if (pRotorMsg && pRotorMsg->isVisible() )
	{
		pRotorMsg->hide();
		delete pRotorMsg;
		pRotorMsg=0;
	}
	else if ((eZap::getInstance()->focus==this) && ((state == 1) || (stateOSD == 0)))
			hide();
}

void eZapMain::leaveService()
{
	ButtonGreenDis->show();
	ButtonGreenEn->hide();
	ButtonYellowDis->show();
	ButtonYellowEn->hide();

	flags&=~(ENIGMA_NVOD|ENIGMA_SUBSERVICES|ENIGMA_AUDIO|ENIGMA_VIDEO);

	ChannelName->setText("");
//	ChannelNumber->setText("");
	Description->setText("");

	EINow->setText("");
	EINowDuration->setText("");
	EINowTime->setText("");
	EINext->setText("");
	EINextDuration->setText("");
	EINextTime->setText("");

	Progress->hide();
#ifndef DISABLE_FILE
	indices.clear();
#endif
}

void eZapMain::clockUpdate()
{
	time_t c=time(0)+eDVB::getInstance()->time_difference;
	tm *t=localtime(&c);
#ifndef DISABLE_LCD
	eZapLCD *pLCD=eZapLCD::getInstance();
#endif
	if (t && eDVB::getInstance()->time_difference)
	{
		eString s;
		s.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
		clocktimer.start((70-t->tm_sec)*1000);
		Clock->setText(s);

		if(eDVB::getInstance()->getmID() == 6  // DM5K6...
			&& eZapStandby::getInstance() ) //  in standby
		{
			int num = t->tm_hour*100+t->tm_min;
			eDebug("write time to led-display");
			int fd=::open("/dev/dbox/fp0",O_RDWR);
			::ioctl(fd,4,&num);
			::close(fd);
		}
#ifndef DISABLE_LCD
		else
		{
			pLCD->lcdMain->Clock->setText(s);
			pLCD->lcdStandby->Clock->setText(s);
		}
#endif
	} else
	{
		Clock->setText("--:--");
		clocktimer.start(60000);
		if(eDVB::getInstance()->getmID() == 6  // DM5K6...
			&& eZapStandby::getInstance() ) //  in standby
		{
			int num=9999;
			eDebug("write number to led-display");
			int fd=::open("/dev/dbox/fp0",O_RDWR);
			::ioctl(fd,4,&num);
			::close(fd);
		}
#ifndef DISABLE_LCD
		else
		{
			pLCD->lcdMain->Clock->setText("--:--");
			pLCD->lcdStandby->Clock->setText("--:--");
		}
#endif
	}
	updateProgress();
}

void eZapMain::updateVolume(int mute_state, int vol)
{
	//int show=(!eZap::getInstance()->focus) || (eZap::getInstance()->focus == this);

	if (mute_state)
	{
		volume.hide();
		mute.show();
	}
	else
	{
		VolumeBar->setPerc((63-vol)*100/63);
		mute.hide();
	}
}

void eZapMain::postMessage(const eZapMessage &message, int clear)
{
	eLocker l(messagelock);

	int c=0;
	if (clear)
	{
		for (std::list<eZapMessage>::iterator i(messages.begin()); i != messages.end(); )
		{
			if (message.isSameType(*i))
			{
				if (i == messages.begin())
				{
					c=1;
					++i;
				} else
					i = messages.erase(i);
			} else
				++i;
		}
	}
	if (!message.isEmpty())
		messages.push_back(message);
	message_notifier.send(c);
}

void eZapMain::gotMessage(const int &c)
{
	if ( c == eZapMain::messageGoSleep )
	{
		if (!eZapStandby::getInstance())
		{
			eDebug("goto Standby (sleep)");
			standbyPress(0);
			standbyRelease();
		}
		else
			eDebug("goto Standby... but already sleeping");
		return;
	}
	else if ( c == eZapMain::messageShutdown )
	{
		eZap::getInstance()->quit();
		return;
	}
#ifndef DISABLE_FILE
	else if ( c == eZapMain::messageNoRecordSpaceLeft )
	{
		eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorNoSpaceLeft );
		return;
	}
#endif
	if ((!c) && pMsg) // noch eine gueltige message vorhanden
	{
		return;
	}
	if ((!isVisible()) && eZap::getInstance()->focus)
	{
		pauseMessages();
		message_notifier.send(c);
		return;
	}
	pauseMessages();
	while (!messages.empty())
	{
		nextMessage();
		if (pMsg)
			break;
	}
	startMessages();
}

void eZapMain::nextMessage()
{
	eZapMessage msg;
	messagelock.lock();

	messagetimeout.stop();

	if (pMsg)
	{
#if 0
		if (pMsg->in_loop)
			pMsg->close();
#endif
		pMsg->hide();
		delete pMsg;
		pMsg=0;
		messages.pop_front();
	}

	std::list<eZapMessage>::iterator i(messages.begin());
 	if (i != messages.end())
 	{
		msg=messages.front();
		messagelock.unlock();
		int showonly=msg.getTimeout()>=0;
		if (!showonly)
			hide();
		pMsg = new eMessageBox(msg.getBody(), msg.getCaption(), showonly?0:eMessageBox::btOK);
		pMsg->show();
		if (!showonly)
		{
			pMsg->exec();
			pMsg->hide();
			delete pMsg;
			pMsg=0;
			messages.pop_front();
		} else if (msg.getTimeout())
			messagetimeout.start(msg.getTimeout()*1000, 1);
	} else
		messagelock.unlock();
}

void eZapMain::stopMessages()
{
	pauseMessages();
	if (pMsg)
	{
		delete pMsg;
		pMsg=0;
		messages.pop_front();
	}
}

void eZapMain::startMessages()
{
	message_notifier.start();
}

void eZapMain::pauseMessages()
{
	message_notifier.stop();
}

void eZapMain::setMode(int newmode, int user)
{
	if ( handleState() )
	{
#ifndef DISABLE_FILE
		if ( newmode == modeFile )
			playlist->service_name=_("Playlist");
		else
			playlist->service_name=_("History");

		if ( newmode == modeFile && mode != newmode )
			eEPGCache::getInstance()->pauseEPG();
		else if ( mode == modeFile && mode != newmode && newmode != -1 )
			eEPGCache::getInstance()->restartEPG();
#endif
		eDebug("setting mode to %d", newmode);

		// save oldMode
		if (mode != -1)
			getServiceSelectorPath(modeLast[mode][0]);

		if (mode == newmode)
			user=0;

		if ( newmode != -1 )
			mode=newmode;

		if (user)
		{
//			eDebug("playservice");
			playService(modeLast[mode][0].current(), psDontAdd|psSeekPos);
		}

		if (mode != -1)
		{
//			eDebug("setServiceSelectorPath");
			setServiceSelectorPath(modeLast[mode][0]);
		}
		eZap::getInstance()->getServiceSelector()->setKeyDescriptions();
	}
}

void eZapMain::setServiceSelectorPath(eServicePath path)
{
	eServiceReference ref=path.current();
	path.up();
	eServicePath p = path;
//	eDebug("Setting currentService to %s", ref.toString().c_str() );
//	eDebug("setting path to %s", p.toString().c_str());
	eZap::getInstance()->getServiceSelector()->setPath(path, ref);
}

void eZapMain::getServiceSelectorPath(eServicePath &path)
{
//	eDebug("selected = %s",eZap::getInstance()->getServiceSelector()->getSelected().toString().c_str() );
	path=eZap::getInstance()->getServiceSelector()->getPath();
	path.down(eZap::getInstance()->getServiceSelector()->getSelected());
//	eDebug("stored path for mode %d: %s", mode, eServicePath(path).toString().c_str());
}

eServicePath eZapMain::getRoot(int list)
{
	eServicePath b; // =eServiceStructureHandler::getRoot(mode+1);
	switch (mode)
	{
	case modeTV:
		switch (list)
		{
		case listAll:
			b.down(eServiceReference(eServiceReference::idDVB,
				eServiceReference::flagDirectory|eServiceReference::shouldSort,
				-2, (1<<4)|(1<<1), 0xFFFFFFFF ));
			break;
		case listSatellites:
			b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, (1<<4)|(1<<1) ));
			break;
		case listProvider:
			b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1), 0xFFFFFFFF ));
			break;
		case listBouquets:
			b.down(userTVBouquetsRef);
			break;
		}
		break;
	case modeRadio:
		switch (list)
		{
		case listAll:
			b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2, 0xFFFFFFFF ));
			break;
		case listSatellites:
			b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, 1<<2 ));
			break;
		case listProvider:
			b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2, 0xFFFFFFFF ));
			break;
		case listBouquets:
			b.down(userRadioBouquetsRef);
			break;
		}
		break;
#ifndef DISABLE_FILE
	case modeFile:
		switch (list)
		{
		case listAll:
			b.down(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile));
			break;
		case listSatellites:
			b.down(recordingsref);
			break;
		case listProvider:
			b.down(playlistref);
			break;
		case listBouquets:
			b.down(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile));
			break;
		}
		break;
#endif
	default:
		return eServicePath();
	}
	return b;
}

void eZapMain::showServiceInfobar(int show)
{
	dvrfunctions=show;
	clearHelpList();

	if (dvrfunctions)
	{
		dvbInfoBar->hide();
		fileInfoBar->hide();
		dvrInfoBar->show();
		prepareDVRHelp();
	} else
	{
		dvrInfoBar->hide();
		switch( eServiceInterface::getInstance()->getService()->getID() )
		{
			case 0x1000: // MP3
				dvbInfoBar->hide();
				fileInfoBar->show();
				break;
			case eServiceReference::idDVB: // DVB or ts playback
				fileInfoBar->hide();
				dvbInfoBar->show();
				break;
		}
		prepareNonDVRHelp();
	}
}

void eZapMain::moveService(const eServiceReference &path, const eServiceReference &ref, const eServiceReference &afterref)
{
	std::list<ePlaylistEntry>::iterator it=std::find(currentSelectedUserBouquet->getList().begin(), currentSelectedUserBouquet->getList().end(), ref),
																			after;

	if (afterref)
		after=std::find(currentSelectedUserBouquet->getList().begin(), currentSelectedUserBouquet->getList().end(), afterref);
	else
		after=currentSelectedUserBouquet->getList().end();

	currentSelectedUserBouquet->moveService(it, after);
}

void eZapMain::showMultiEPG()
{
	eZapEPG epg;

	epg.move(ePoint(50, 50));
	epg.resize(eSize(620, 470));

	int direction = 1;
	epg.show();
	do
	{
		epg.buildPage(direction);
		direction = epg.exec();
	}
	while ( direction != -1 );
	epg.hide();
}

#ifndef DISABLE_FILE
void eZapMain::toggleIndexmark()
{
	if (!(serviceFlags & eServiceHandler::flagIsSeekable))
		return;

	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;

	int real=handler->getPosition(eServiceHandler::posQueryRealCurrent), 
			time=handler->getPosition(eServiceHandler::posQueryCurrent);

	int nearest=indices.getNext(real, 0);
	if ((nearest == -1) || abs(indices.getTime(nearest)-time) > 5)
		indices.add(real, time);
	else
		indices.remove(nearest);
	
	Progress->invalidate();
}

void eZapMain::indexSeek(int dir)
{
	if (!(serviceFlags & eServiceHandler::flagIsSeekable))
		return;

	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;

	int real=handler->getPosition(eServiceHandler::posQueryRealCurrent),
			time=handler->getPosition(eServiceHandler::posQueryCurrent);

	int nearest=indices.getNext(real, dir);
	if ((nearest != -1) && (dir == -1)) // when seeking backward, check if 
	{
		int nearestt=indices.getTime(nearest);
		if ((time - nearestt) < 5)  // when less than 5 seconds, seek to prev
			nearest = indices.getNext(nearest, -1);
	}
	if (nearest == -1)
	{
		if (dir == -1)
			nearest = 0; // seek to start of file
		else
			return;     // TODO: seek to end of file, without playing next
	}
	handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekReal, nearest));
}

#endif // DISABLE_FILE

void eZapMain::toggleScart( int state )
{
	enigmaVCR *instance = enigmaVCR::getInstance();
	if ( state )
	{
		if ( !instance )
		{
			enigmaVCR mb("If you can read this, your scartswitch doesn't work", "VCR");
			mb.show();
#ifndef DISABLE_LCD
			eZapLCD *pLCD=eZapLCD::getInstance();
			eWidget *oldlcd=0;
			if ( pLCD->lcdMenu->isVisible() )
				oldlcd = pLCD->lcdMenu;
			else if ( pLCD->lcdMain->isVisible() )
				oldlcd = pLCD->lcdMain;
			else if ( pLCD->lcdStandby->isVisible() )
				oldlcd = pLCD->lcdStandby;
			if ( oldlcd )
				oldlcd->hide();
			pLCD->lcdScart->show();
#endif
			mb.exec();
#ifndef DISABLE_LCD
			pLCD->lcdScart->hide();
			if ( oldlcd )
				oldlcd->show();
#endif
			mb.hide();
		}
	}
	else if ( instance )
		instance->switchBack();
}

void eZapMain::ShowTimeCorrectionWindow( tsref ref )
{
	eTimeCorrectionEditWindow wnd( ref );
	wnd.show();
	wnd.exec();
	wnd.hide();
}

#ifndef DISABLE_NETWORK
void eZapMain::startNGrabRecord()
{
	state |= (stateRecording|recDVR);	
	ENgrab::getNew()->sendstart();
}

void eZapMain::stopNGrabRecord()
{
	state &= ~(stateRecording|recDVR);
	ENgrab::getNew()->sendstop();
}
#endif

eServiceContextMenu::eServiceContextMenu(const eServiceReference &ref, const eServiceReference &path, eWidget *lcdTitle, eWidget *lcdElement)
: eListBoxWindow<eListBoxEntryText>(_("Service Menu"), 14), ref(ref)
{
#ifndef DISABLE_LCD
	setLCD(lcdTitle,lcdElement);
#endif
	move(ePoint(150, 80));

	eListBoxEntry *prev=0;

	if ( !(ref.flags & eServiceReference::isDirectory)
		&& ( ref.type == 0x1000 // mp3
		|| ( ref.flags & eServiceReference::mustDescent )
		|| ( ref.type == eServiceReference::idDVB && ref.path ) ) )
			prev = new eListBoxEntryText(&list, _("add service to playlist"), (void*)3);

	eListBoxEntryText *sel=0;
	/* i think it is not so good to rename normal providers
	if ( ref.data[0]==-3 ) // rename Provider
		new eListBoxEntryText(&list, _("rename"), (void*)7);*/

	// create new bouquet
	if ( eZapMain::getInstance()->getMode() != eZapMain::modeFile )
		prev = new eListBoxEntryText(&list, _("create new bouquet"), (void*)6);

	if (path.type == eServicePlaylistHandler::ID)
	{
		// not in file mode
		if ( eZapMain::getInstance()->getMode() != eZapMain::modeFile )
		{
			// copy complete provider to bouquets
			if ( ref.flags & eServiceReference::flagDirectory )
				prev = new eListBoxEntryText(&list, _("duplicate bouquet"), (void*)8);
			else // add dvb service to specific bouquet
			{
				prev = new eListBoxEntryText(&list, _("add to specific bouquet"), (void*)4);
				if ( path.type == eServicePlaylistHandler::ID )
					prev = new eListBoxEntryText(&list, _("add marker"), (void*)13);
			}
			prev = new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
		}

		// rename bouquet
		if ( (ref.type == eServicePlaylistHandler::ID) )
			prev = new eListBoxEntryText(&list, _("rename"), (void*)7);

		// rename dvb service
		if ( ref.type == eServiceReference::idDVB )
			prev = new eListBoxEntryText(&list, _("rename"), (void*)9);

		// all what contain in a playlists is deleteable
		prev = new eListBoxEntryText(&list, _("delete"), (void*)1);

		prev = new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );

		// move mode in playlists
		if ( eZap::getInstance()->getServiceSelector()->movemode )
			prev = sel = new eListBoxEntryText(&list, _("disable move mode"), (void*)2);
		else
			prev = new eListBoxEntryText(&list, _("enable move mode"), (void*)2);
	}
	else  // not in a playlist
	{
		// not in file mode
		if ( eZapMain::getInstance()->getMode() != eZapMain::modeFile )
		{
			// add current service to favourite
			if ( !(ref.flags & eServiceReference::flagDirectory) )
			{
				prev = new eListBoxEntryText(&list, _("add to specific bouquet"), (void*)4);
				prev = new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
			}
			// copy provider to bouquet list
			else if (ref.data[0] == -2 || ref.data[0] == -3 )
			{
				prev = new eListBoxEntryText(&list, _("copy to bouquet list"), (void*)8);
				prev = new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
			}
		}
	}

	// not in File mode
	if ( eZapMain::getInstance()->getMode() != eZapMain::modeFile )
	{
		// edit Mode ( simple add services to any bouquet(playlist)
		if ( eZap::getInstance()->getServiceSelector()->editMode )
			prev = sel = new eListBoxEntryText(&list, _("disable edit mode"), (void*)5);
		else
			prev = new eListBoxEntryText(&list, _("enable edit mode"), (void*)5);
	}
	// options for activated parental locking
	if ( eConfig::getInstance()->getParentalPin() )
	{
		if ( prev->isSelectable() )
			new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
		if ( ref.isLocked() )
			new eListBoxEntryText(&list, _("unlock"), (void*)11);
		else
			new eListBoxEntryText(&list, _("lock"), (void*)10);

		if ( eConfig::getInstance()->pLockActive() )
			new eListBoxEntryText(&list, _("disable parental lock"), (void*)12 );
		else
			new eListBoxEntryText(&list, _("enable parental lock"), (void*)12 );
	}

	if ( sel )
		list.setCurrent( sel );

	CONNECT(list.selected, eServiceContextMenu::entrySelected);
}

void eServiceContextMenu::entrySelected(eListBoxEntryText *test)
{
	if (!test)
		close(0);
	else
		close((int)test->getKey());
}

eSleepTimerContextMenu::eSleepTimerContextMenu( eWidget* lcdTitle, eWidget *lcdElement )
	: eListBoxWindow<eListBoxEntryText>(_("Shutdown/Standby Menu"), 7)
{
#ifndef DISABLE_LCD
	setLCD(lcdTitle, lcdElement);
#endif
	move(ePoint(150, 200));
	if ( eDVB::getInstance()->getmID() != 6 )
	{
		new eListBoxEntryText(&list, _("shutdown now"), (void*)1);
		new eListBoxEntryText(&list, _("restart"), (void*)4);
	}
	else	
		new eListBoxEntryText(&list, _("reboot now"), (void*)1);		//use this type to reboot (with complete unmount)
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	new eListBoxEntryText(&list, _("goto standby"), (void*)2);
	new eListBoxEntryText(&list, _("set sleeptimer"), (void*)3);
	CONNECT(list.selected, eSleepTimerContextMenu::entrySelected);
	setHelpID(50);
}

void eSleepTimerContextMenu::entrySelected( eListBoxEntryText *sel )
{
	if (!sel)
		close(0);
	else
		close((int)sel->getKey());
}

eShutdownStandbySelWindow::eShutdownStandbySelWindow(eWidget *parent, int len, int min, int max, int maxdigits, int *init, int isactive, eWidget* descr, int grabfocus, const char* deco )
{
	num = new eNumber( parent, len, min, max, maxdigits, init, isactive, descr, grabfocus, deco );
	Shutdown = new eCheckbox(this);
	Shutdown->setName("shutdown");
	Standby = new eCheckbox(this);
	Standby->setName("standby");
	set = new eButton(this);
	set->setName("set");
	CONNECT( num->selected, eShutdownStandbySelWindow::fieldSelected );
	CONNECT( Shutdown->checked, eShutdownStandbySelWindow::ShutdownChanged );
	CONNECT( Standby->checked, eShutdownStandbySelWindow::StandbyChanged );
}

void eShutdownStandbySelWindow::StandbyChanged( int checked )
{
	if ( checked )
		Shutdown->setCheck( 0 );
}

void eShutdownStandbySelWindow::ShutdownChanged( int checked )
{
	if ( checked )
		Standby->setCheck( 0 );
}

int eShutdownStandbySelWindow::getCheckboxState()
{
	return Standby->isChecked()?3:Shutdown->isChecked()?2:0;
}

eSleepTimer::eSleepTimer()
:eShutdownStandbySelWindow( this, 1, 1, 240, 3, 0, 0 )
{
	eLabel *l = new eLabel(this);
	l->setName("l_duration");
	num->setDescr(l);
	num->setName("duration");
	num->setNumber(10);
	if (eSkin::getActive()->build(this, "sleeptimer"))
		eFatal("skin load of \"sleeptimer\" failed");
	CONNECT( set->selected, eSleepTimer::setPressed );
	if ( eDVB::getInstance()->getmID() == 6 )
	{
		Shutdown->hide();
		Standby->hide();
		Standby->setCheck(1);
	}
	setHelpID(51);
}

void eSleepTimer::setPressed()
{
	EITEvent *evt = new EITEvent();
	evt->start_time = time(0)+eDVB::getInstance()->time_difference;
	evt->duration = num->getNumber()*60;
	evt->event_id = -1;
	evt->free_CA_mode = -1;
	evt->running_status = -1;
	close((int)evt);
}

#ifndef DISABLE_FILE
eTimerInput::eTimerInput()
:eShutdownStandbySelWindow( this, 1, 1, 240, 3, 0, 0 )
{
	eLabel *l = new eLabel(this);
	l->setName("lrec_duration");
	num->setDescr(l);
	num->setName("rec_duration");
	num->setNumber(10);
	if (eSkin::getActive()->build(this, "recording_duration"))
		eFatal("skin load of \"recording_duration\" failed");
	CONNECT( set->selected, eTimerInput::setPressed );
}

void eTimerInput::setPressed()
{
	EITEvent *evt = new EITEvent();
	evt->start_time = time(0)+eDVB::getInstance()->time_difference;
	evt->duration = num->getNumber()*60;
	evt->event_id = -1;
	evt->free_CA_mode = -1;
	evt->running_status = -1;
	close((int)evt);
}

eRecordContextMenu::eRecordContextMenu( eWidget *LCDTitle, eWidget *LCDElement )
	: eListBoxWindow<eListBoxEntryText>(_("Record Menu"), 7)
{
#ifndef DISABLE_LCD
	setLCD(LCDTitle, LCDElement);
#endif
	move(ePoint(150, 200));
	new eListBoxEntryText(&list, _("stop record now"), (void*)1);
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	new eListBoxEntryText(&list, _("set record duration"), (void*)2);
	new eListBoxEntryText(&list, _("set record stop time"), (void*)3);
	CONNECT(list.selected, eRecordContextMenu::entrySelected);
}

void eRecordContextMenu::entrySelected( eListBoxEntryText *sel )
{
	if (!sel)
		close(0);
	else
		close((int)sel->getKey());
}

eRecTimeInput::eRecTimeInput()
:eShutdownStandbySelWindow( this, 2, 0, 59, 2, 0, 0 )
{
	eLabel *l = new eLabel(this);
	l->setName("lrec_end_time");
	num->setDescr(l);
	num->setName("rec_end_time");
	num->setFlags( eNumber::flagFillWithZeros|eNumber::flagTime );

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	struct tm *t = localtime( &now );
	num->setNumber(0, t->tm_hour);
	num->setNumber(1, t->tm_min);

	if (eSkin::getActive()->build(this, "recording_end_time"))
		eFatal("skin load of \"recording_end_time\" failed");

	CONNECT( set->selected, eRecTimeInput::setPressed );
}

extern time_t normalize( struct tm & t );

void eRecTimeInput::setPressed()
{
	int hour = num->getNumber(0);
	int min = num->getNumber(1);

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	struct tm *t = localtime( &now );

	if ( hour*60+min < t->tm_hour*60+t->tm_min )
	{
		t->tm_mday++;
		t->tm_hour = hour;
		t->tm_min = min;
		normalize(*t);
	}
	else
	{
		t->tm_hour = hour;
		t->tm_min = min;
	}

	time_t tmp = mktime( t );

	EITEvent *evt = new EITEvent();
	evt->start_time = time(0)+eDVB::getInstance()->time_difference;
	evt->duration = tmp - evt->start_time;
	evt->event_id = -1;
	evt->free_CA_mode = -1;
	evt->running_status = -1;
	close((int)evt);
}

#endif //DISABLE_FILE

TextEditWindow::TextEditWindow( const char *InputFieldDescr, const char* useableChars )
	:eWindow(0)
{
	input = new eTextInputField(this);
	input->setName("inputfield");
	input->setMaxChars(100);
	input->setHelpText(_("press ok to start edit mode"));
	if (useableChars)
		input->setUseableChars( useableChars );

	descr = new eLabel(this);
	descr->setName("descr");
	descr->setText(InputFieldDescr);

	image = new eLabel(this);
	image->setName("image");

	save = new eButton(this);
	save->setName("save");
	CONNECT( save->selected, TextEditWindow::accept );

	eStatusBar *n = new eStatusBar(this);
	n->setName("statusbar");

	if (eSkin::getActive()->build(this, "TextEditWindow"))
		eWarning("TextEditWindo widget build failed!");
}

UserBouquetSelector::UserBouquetSelector( std::list<ePlaylistEntry>&list )
	:eListBoxWindow<eListBoxEntryText>(_("Bouquets"), 9, 400),
	SourceList(list)
{
	move(ePoint(100,80));

	for (std::list<ePlaylistEntry>::iterator it( SourceList.begin() ); it != SourceList.end(); it++)
	{
		ePlaylist *pl = (ePlaylist*)eServiceInterface::getInstance()->addRef( it->service );
		new eListBoxEntryText( &this->list, pl->service_name, &it->service );
		eServiceInterface::getInstance()->removeRef( it->service );
	}
	CONNECT( this->list.selected, UserBouquetSelector::selected );
}

void UserBouquetSelector::selected( eListBoxEntryText *sel )
{
	if (sel && sel->getKey())
		curSel=*((eServiceReference*)sel->getKey());

	close(0);
}

eTimeCorrectionEditWindow::eTimeCorrectionEditWindow( tsref tp )
	:updateTimer(eApp), transponder(tp)
{
	setText(_("Time Correction"));
	move( ePoint(100,100) );
	cresize( eSize(440,270 ) );

	eLabel *l = new eLabel(this);
	l->move( ePoint(10,10) );
	l->resize( eSize(200,30));
	l->setText(_("Transponder Time:"));

	lCurTransponderTime = new eLabel(this);
	lCurTransponderTime->move( ePoint(210,10) );
	lCurTransponderTime->resize( eSize(150,30) );

	l = new eLabel(this);
	l->move( ePoint(10,50) );
	l->resize( eSize(200,30));
	l->setText(_("Transponder Date:"));

	lCurTransponderDate = new eLabel(this);
	lCurTransponderDate->move( ePoint(210,50) );
	lCurTransponderDate->resize( eSize(150,30) );

	l = new eLabel(this);
	l->setText(_("New Time:"));
	l->setFlags( eLabel::flagVCenter );
	l->move( ePoint(10,90) );
	l->resize( eSize(150,35) );

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm tmp = *localtime( &now );

	nTime = new eNumber(this, 2, 0, 59, 2, 0, 0, l);
	nTime->resize(eSize(75,35));
	nTime->move(ePoint(210,90));
	nTime->setFlags( eNumber::flagTime|eNumber::flagFillWithZeros );
	nTime->loadDeco();
	nTime->setHelpText(_("enter correct time here"));
	nTime->setNumber(0, tmp.tm_hour );
	nTime->setNumber(1, tmp.tm_min );
	CONNECT( nTime->selected, eTimeCorrectionEditWindow::fieldSelected );

	l = new eLabel(this);
	l->setText(_("New Date:"));
	l->setFlags( eLabel::flagVCenter );
	l->move( ePoint(10,140) );
	l->resize( eSize(150,35) );

	cday = new eComboBox(this);
	cday->move(ePoint(210,140));
	cday->resize(eSize(60,35));
	cday->loadDeco();
	cday->setHelpText(_("press ok to select another day"));

	cmonth = new eComboBox( this );
	cmonth->move(ePoint(280,140));
	cmonth->resize(eSize(60,35));
	cmonth->loadDeco();
	cmonth->setHelpText(_("press ok to select another month"));
	for ( int i = 0; i < 12; i++ )
		new eListBoxEntryText( *cmonth, eString().sprintf("%02d",i+1), (void*)i );
	CONNECT( cmonth->selchanged, eTimeCorrectionEditWindow::monthChanged );

	cyear = new eComboBox(this);
	cyear->move(ePoint(350,140));
	cyear->resize(eSize(80,35));
	cyear->loadDeco();
	cyear->setHelpText(_("press ok to select another year"));
	for ( int i = -1; i < 4; i++ )
		new eListBoxEntryText( *cyear, eString().sprintf("%d",tmp.tm_year+(1900+i)), (void*)(tmp.tm_year+i) );

	cyear->setCurrent( (void*) tmp.tm_year );
	cmonth->setCurrent( (void*) tmp.tm_mon, true );
	cday->setCurrent( (void*) tmp.tm_mday );
	CONNECT( cyear->selchanged, eTimeCorrectionEditWindow::yearChanged );

	bSet=new eButton(this);
	bSet->setText(_("set"));
	bSet->setShortcut("green");
	bSet->setShortcutPixmap("green");

	bSet->move(ePoint(10, clientrect.height()-80));
	bSet->resize(eSize(220,40));
	bSet->setHelpText(_("set new time and close window"));
	bSet->loadDeco();
	CONNECT(bSet->selected, eTimeCorrectionEditWindow::savePressed);

	sbar = new eStatusBar(this);
	sbar->move( ePoint(0, clientrect.height()-30) );
	sbar->resize( eSize( clientrect.width(), 30) );
	sbar->loadDeco();
	CONNECT( updateTimer.timeout, eTimeCorrectionEditWindow::updateTPTimeDate );
}

void eTimeCorrectionEditWindow::savePressed()
{
	time_t nowTP = time(0)+eDVB::getInstance()->time_difference;
	tm TPTime = *localtime( &nowTP );
	TPTime.tm_hour = nTime->getNumber(0);
	TPTime.tm_min = nTime->getNumber(1);
	TPTime.tm_mday = (int)cday->getCurrent()->getKey();
	TPTime.tm_wday = -1;
	TPTime.tm_yday = -1;
	TPTime.tm_mon = (int)cmonth->getCurrent()->getKey();
	TPTime.tm_year = (int)cyear->getCurrent()->getKey();
	time_t newTPTime = mktime(&TPTime);
	int diff = newTPTime - nowTP;
	eDebug("diff = %d", diff );
	eDVB::getInstance()->time_difference+=diff;
	eTransponderList::getInstance()->TimeOffsetMap[transponder]=
		diff;
	eDVB::getInstance()->timeUpdated();
	close(0);
}

void eTimeCorrectionEditWindow::updateTPTimeDate()
{
	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm ltime = *localtime( &now );
	lCurTransponderTime->setText(eString().sprintf("%02d:%02d:%02d", ltime.tm_hour, ltime.tm_min, ltime.tm_sec));
	lCurTransponderDate->setText(eString().sprintf("%02d.%02d.%04d", ltime.tm_mday, ltime.tm_mon+1, 1900+ltime.tm_year));
}

int eTimeCorrectionEditWindow::eventHandler( const eWidgetEvent &event )
{
	switch (event.type)
	{
		case eWidgetEvent::execBegin:
			updateTimer.start(1000);
			setFocus(bSet);
			break;
		case eWidgetEvent::execDone:
		{
			updateTimer.stop();
			std::map<tsref,int> &map =
				eTransponderList::getInstance()->TimeOffsetMap;
			// test if correction for this transponder in map
			if ( map.find(transponder) == map.end() )
			{
			// not in map... we set 0 as correction
				map[transponder]=0;
				eDebug("SET 0");
			}
			break;
		}
		default:
			return eWindow::eventHandler( event );
	}
	return 1;
}

void eTimeCorrectionEditWindow::yearChanged( eListBoxEntryText* )
{
	cmonth->setCurrent( (int) cmonth->getCurrent()->getKey(), true );
	setFocus(cyear);
}

void eTimeCorrectionEditWindow::monthChanged( eListBoxEntryText *e )
{
	if ( e )
	{
		const unsigned char monthdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		int month = (int)e->getKey();
		cday->clear();
		int days = monthdays[month];
		if ( month == 1 && __isleap( (int)cyear->getCurrent()->getKey()) )
			days++;
		for ( int i = 1; i <= days ; i++ )
			new eListBoxEntryText( *cday, eString().sprintf("%02d", i), (void*)i);
		cday->setCurrent( (void*) 1 );
	}
}
