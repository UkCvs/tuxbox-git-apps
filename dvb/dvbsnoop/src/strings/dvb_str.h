/*
$Id: dvb_str.h,v 1.5.2.4 2003/11/26 20:38:12 coronas Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de

 -- dvb decoder helper functions



$Log: dvb_str.h,v $
Revision 1.5.2.4  2003/11/26 20:38:12  coronas
Compilefix rel-branch/Update from HEAD

Revision 1.14  2003/11/26 19:55:34  rasc
no message

Revision 1.13  2003/11/09 20:48:35  rasc
pes data packet (DSM-CC)

Revision 1.12  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.11  2003/10/25 19:11:50  rasc
no message

Revision 1.10  2003/10/19 21:05:53  rasc
- some datacarousell stuff started

Revision 1.9  2003/10/17 18:16:54  rasc
- started more work on newer ISO 13818  descriptors
- some reorg work started

Revision 1.8  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.7  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/



#ifndef __DVB_STR_H
#define __DVB_STR_H 1


char *dvbstrBouquetTable_ID (u_int i);
char *dvbstrStreamContent_Component_TYPE (u_int i);
char *dvbstrTableID (u_int id);
char *dvbstrMPEGDescriptorTAG (u_int tag);
char *dvbstrDVBDescriptorTAG (u_int tag);
char *dvbstrWEST_EAST_FLAG (u_int tag);
char *dvbstrPolarisation_FLAG (u_int tag);
char *dvbstrModulationCable_FLAG(u_int flag);
char *dvbstrModulationSAT_FLAG (u_int tag);
char *dvbstrFECinner_SCHEME (u_int tag);
char *dvbstrFECouter_SCHEME (u_int tag);
char *dvbstrLinkage_TYPE(u_int tag);
char *dvbstrHandover_TYPE(u_int tag);
char *dvbstrOrigin_TYPE(u_int tag);
char *dvbstrService_TYPE(u_int tag);
char *dvbstrStream_TYPE(u_int tag);
char *dvbstrAudio_TYPE(u_int tag);
char *dvbstrCASystem_ID(u_int id);
char *dvbstrDataBroadcast_ID(u_int flag);
char *dvbstrOriginalNetwork_ID (u_int i);
char *dvbstrNetworkIdent_ID(u_int id);
char *dvbstrBroadcast_ID(u_int id);
char *dvbstrTeletext_TYPE(u_int id);
char *dvbstrTerrBandwidth_SCHEME(u_int id);
char *dvbstrTerrConstellation_FLAG(u_int id);
char *dvbstrTerrHierarchy_FLAG(u_int id);
char *dvbstrTerrCodeRate_FLAG(u_int id);
char *dvbstrTerrGuardInterval_FLAG(u_int id);
char *dvbstrTerrTransmissionMode_FLAG(u_int id);
char *dvbstrAspectRatioInfo_FLAG(u_int id);
char *dvbstrHierarchy_TYPE(u_int id);
char *dvbstrRunningStatus_FLAG (u_int id);
char *dvbstrDataStreamVIDEOAlignment_TYPE (u_int id);
char *dvbstrDataStreamAUDIOAlignment_TYPE (u_int id);
char *dvbstrDataService_ID (u_int id);
char *dvbstrContent_Component_TYPE (u_int id);
char *dvbstrLogCellPresInfo_TYPE(u_int id);
char *dvbstrCellLinkageInfo_TYPE(u_int id);
char *dvbstrTextCharset_TYPE(u_int id);
char *dvbstrContentNibble_TYPE(u_int id);
char *dvbstrParentalRating_TYPE(u_int id);
char *dvbstrDelivSysCoding_TYPE(u_int id);
char *dvbstrShortSmoothingBufSize_TYPE(u_int id);
char *dvbstrShortSmoothingBufLeakRate_TYPE(u_int id);
char *dvbstrAC3Component_TYPE(u_int id);
char *dvbstrAncillaryData_ID(u_int id);
char *dvbstrAnnouncement_TYPE(u_int id);
char *dvbstrAnnouncementReference_TYPE(u_int id);


/* -- Transport Stream Stuff */

char *dvbstrTSpid_ID (u_int id);
char *dvbstrTS_AdaptionField_TYPE (u_int id);
char *dvbstrTS_ScramblingCtrl_TYPE (u_int id);

/* -- PES stuff */

char *dvbstrPESstream_ID(u_int id);
char *dvbstrPESscrambling_ctrl_TYPE(u_int id);
char *dvbstrPESTrickModeControl (u_int i);
char *dvbstrPESDataIdentifier (u_int i);




/* -- Div Stuff */

char *dvbstrCountryCode_ID (u_int i);
char *dvbstrPrivateDataSpecifier_ID (u_int i);




#endif



