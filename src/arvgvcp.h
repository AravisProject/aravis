#ifndef ARV_GVCP_H
#define ARV_GVCP_H

#include <arv.h>

G_BEGIN_DECLS

#define ARV_GVCP_PORT	3956

#define ARV_GVCP_IP_ADDRESS				0x00000024
#define ARV_GVCP_SUPPLIER_NAME_ADDRESS			0x00000048
#define ARV_GVCP_SUPPLIER_NAME_SIZE			16
#define ARV_GVCP_MODEL_NAME_ADDRESS			0x00000068
#define ARV_GVCP_MODEL_NAME_SIZE			16
#define ARV_GVCP_CAMERA_NAME_ADDRESS			0x000000E8
#define ARV_GVCP_CAMERA_NAME_SIZE			16

#define ARV_GVCP_GENICAM_FILENAME_ADDRESS_1		0x00000200
#define ARV_GVCP_GENICAM_FILENAME_ADDRESS_2		0x00000400
#define ARV_GVCP_GENICAM_FILENAME_SIZE			512

#define ARV_GVCP_DATA_SIZE_MAX				512

typedef enum {
	ARV_GVCP_PACKET_TYPE_ANSWER = 		0x0000,
	ARV_GVCP_PACKET_TYPE_COMMAND = 		0x4201
} ArvGvcpPacketType;

typedef enum {
	ARV_GVCP_COMMAND_DISCOVER_CMD =		0x0002,
	ARV_GVCP_COMMAND_DISCOVER_ANS =		0x0003,
	ARV_GVCP_COMMAND_BYE_CMD = 		0x0004,
	ARV_GVCP_COMMAND_BYE_ANS = 		0x0005,
	ARV_GVCP_COMMAND_READ_REGISTER_CMD =	0x0080,
	ARV_GVCP_COMMAND_READ_REGISTER_ANS =	0x0081,
	ARV_GVCP_COMMAND_WRITE_REGISTER_CMD =	0x0082,
	ARV_GVCP_COMMAND_WRITE_REGISTER_ANS =	0x0083,
	ARV_GVCP_COMMAND_READ_CMD =		0x0084,
	ARV_GVCP_COMMAND_READ_ANS =		0x0085,
	ARV_GVCP_COMMAND_WRITE_CMD =		0x0086,
	ARV_GVCP_COMMAND_WRITE_ANS =		0x0087
} ArvGvcpCommand;

typedef struct {
	guint16 packet_type;
	guint16 command;
	guint16 size;
	guint16 count;
}  __attribute__((__packed__)) ArvGvcpHeader;

typedef struct {
	ArvGvcpHeader header;
	unsigned char data[];
} ArvGvcpPacket;

void 			arv_gvcp_packet_free 			(ArvGvcpPacket *packet);
ArvGvcpPacket * 	arv_gvcp_read_packet_new 		(guint32 address, guint32 size,
								 guint32 packet_count, size_t *packet_size);
ArvGvcpPacket * 	arv_gvcp_discover_packet_new 		(size_t *size);
char * 			arv_gvcp_packet_to_string 		(const ArvGvcpPacket *packet);
void 			arv_gvcp_packet_dump 			(const ArvGvcpPacket *packet);

G_END_DECLS

#endif
