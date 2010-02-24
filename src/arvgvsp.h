#ifndef ARV_GVSP_H
#define ARV_GVSP_H

#include <arv.h>

G_BEGIN_DECLS

typedef enum {
	ARV_GVSP_PACKET_TYPE_START = 	0x0100,
	ARV_GVSP_PACKET_TYPE_STOP = 	0x0200,
	ARV_GVSP_PACKET_TYPE_DATA =	0x0300
} ArvGvspPacketType;

typedef struct {
	guint32 count;
	guint16 packet_type;
	guint16 sub_count;
} __attribute__((__packed__)) ArvGvspHeader;

typedef struct {
	guint32 data0;
	guint32 data1;
	guint32 data2;
	guint32 data3;
	guint32 width;
	guint32 height;
} __attribute__((__packed__)) ArvGvspStartData;

typedef struct {
	guint32 data0;
	guint32 data1;
} __attribute__((__packed__)) ArvGvspStopData;

typedef struct {
	ArvGvspHeader header;
	guint8 data[];
} ArvGvspPacket;

void 			arv_gvsp_packet_debug 			(const ArvGvspPacket *packet);

G_END_DECLS

#endif
