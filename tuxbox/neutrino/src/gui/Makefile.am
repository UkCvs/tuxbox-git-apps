SUBDIRS = widget bedit

INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/src \
	@CURL_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@MPEGTOOLS_CFLAGS@ \
	-I$(top_srcdir)/lib

if ENABLE_INTERNAL_TUXTXT
INCLUDES += \
	@TUXTXT_CFLAGS@
endif

noinst_LIBRARIES = libtimerlist.a libneutrino_gui.a libneutrino_gui2.a libneutrino_gui3.a

libneutrino_gui_a_SOURCES = \
	audio_select.cpp \
	audio_setup.cpp \
	bookmarkmanager.cpp \
	bouquetlist.cpp \
	channellist.cpp \
	color.cpp \
	driver_boot_setup.cpp \
	epg_menu.cpp \
	epgview.cpp \
	eventlist.cpp \
	favorites.cpp \
	filebrowser.cpp \
	imageinfo.cpp \
	infoviewer.cpp \
	keybind_setup.cpp \
	lcd_setup.cpp \
	miscsettings_menu.cpp \
	neutrino_menu.cpp \
	network_setup.cpp \
	nfs.cpp \
	osd_setup.cpp \
	osdlang_setup.cpp \
	parentallock_setup.cpp \
	personalize.cpp \
	pluginlist.cpp \
	plugins.cpp \
	record_setup.cpp \
	rc_lock.cpp \
	scan.cpp \
	scan_setup.cpp \
	screensetup.cpp \
	sleeptimer.cpp \
	software_update.cpp \
	streaminfo2.cpp \
	subchannel_select.cpp \
	themes.cpp \
	timeosd.cpp \
	update.cpp \
	user_menue_setup.cpp \
	video_setup.cpp \
	zapit_setup.cpp

libneutrino_gui3_a_SOURCES =  movieinfo.cpp

if ENABLE_GUI_MOUNT
libneutrino_gui_a_SOURCES += nfs.cpp
endif

if ENABLE_ESD
libneutrino_gui_a_SOURCES += esound.cpp esd_setup.cpp mediaplayer_setup.cpp
endif

if ENABLE_AUDIOPLAYER
libneutrino_gui_a_SOURCES += \
	audioplayer.cpp audioplayer_setup.cpp mediaplayer_setup.cpp
endif

if ENABLE_PICTUREVIEWER
libneutrino_gui_a_SOURCES += \
	pictureviewer.cpp pictureviewer_setup.cpp mediaplayer_setup.cpp
endif

if ENABLE_MOVIEPLAYER
libneutrino_gui_a_SOURCES += moviebrowser.cpp movieplayer_setup.cpp mediaplayer_setup.cpp movieplayer_menu.cpp
if ENABLE_MOVIEPLAYER2
libneutrino_gui_a_SOURCES += movieplayer2.cpp
else
libneutrino_gui_a_SOURCES += \
	movieplayer.cpp
endif
endif

if ENABLE_EPGPLUS
libneutrino_gui_a_SOURCES += \
	epgplus.cpp
endif

#TODO: k26 support for ENABLE_DRIVE_GUI, it's disabled with -enable-kernel26 yet
if ENABLE_DRIVE_GUI
libneutrino_gui_a_SOURCES += drive_setup.cpp
if ENABLE_SAMBASERVER
libneutrino_gui_a_SOURCES += sambaserver_setup.cpp
endif
endif

if !DISABLE_INTERNET_UPDATE
libneutrino_gui_a_SOURCES += proxyserver_setup.cpp
endif


#	experimental_menu.cpp
#	ch_mosaic.cpp

libneutrino_gui2_a_SOURCES = \
	motorcontrol.cpp

if BOXTYPE_DBOX2
libneutrino_gui2_a_SOURCES += \
	alphasetup.cpp 
endif

libtimerlist_a_SOURCES = \
	timerlist.cpp

if ENABLE_UPNP
INCLUDES += \
	@XMLTREE_CFLAGS@ \
	@UPNPCLIENT_CFLAGS@

libneutrino_gui_a_SOURCES += \
	upnpbrowser.cpp
endif
