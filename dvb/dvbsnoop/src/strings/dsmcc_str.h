/*
$Id: dsmcc_str.h,v 1.3.2.3 2003/11/26 20:38:12 coronas Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- dsmcc decoder helper functions




$Log: dsmcc_str.h,v $
Revision 1.3.2.3  2003/11/26 20:38:12  coronas
Compilefix rel-branch/Update from HEAD

Revision 1.6  2003/11/26 19:55:34  rasc
no message

Revision 1.5  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.4  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.3  2003/10/26 21:36:20  rasc
private DSM-CC descriptor Tags started,
INT-Section completed..

Revision 1.2  2003/10/25 19:11:50  rasc
no message

Revision 1.1  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162



*/


#ifndef __DSMCC_H
#define __DSMCC_H 1



char *dsmccStrDSMCC_DataCarousel_DescriptorTAG (u_int i);
char *dsmccStrDSMCC_INT_UNT_DescriptorTAG (u_int i);

char *dsmccStrMHPOrg (u_int id);
char *dsmccStrAction_Type (u_int id);
char *dsmccStrProcessing_order (u_int id);
char *dsmccStrPayload_scrambling_control (u_int id);
char *dsmccStrAddress_scrambling_control (u_int id);
char *dsmccStrLinkage0CTable_TYPE (u_int i);
char *dsmccStrMultiProtEncapsMACAddrRangeField (u_int i);
char *dsmccStrPlatform_ID (u_int id);
char *dsmccStrCarouselType_ID (u_int id);
char *dsmccStrHigherProtocol_ID (u_int id);


#endif




