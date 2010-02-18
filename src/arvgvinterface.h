#ifndef ARV_GV_INTERFACE_H
#define ARV_GV_INTERFACE_H

#include <arv.h>
#include <arvinterface.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ARV_GV_CONTROL_PORT	3956

#define ARV_GVCP_DISCOVER_IP_OFFSET			0x24
#define ARV_GVCP_DISCOVER_SUPPLIER_NAME_OFFSET		0x48
#define ARV_GVCP_DISCOVER_MODEL_NAME_OFFSET		0x68
#define ARV_GVCP_DISCOVER_CAMERA_NAME_OFFSET		0xE8

typedef enum {
	ARV_GVCP_PACKET_TYPE_ANSWER = 		0x0000,
	ARV_GVCP_PACKET_TYPE_COMMAND = 		0x4201
} ArvGvcpPacketType;

typedef enum {
	ARV_GVCP_COMMAND_DISCOVER_CMD =		0x0002,
	ARV_GVCP_COMMAND_DISCOVER_ANS =		0x0003,
	ARV_GVCP_COMMAND_BYE_CMD = 		0x0004,
	ARV_GVCP_COMMAND_BYE_ANS = 		0x0005,
	ARV_GVCP_COMMAND_REGISTER_READ_CMD =	0x0080,
	ARV_GVCP_COMMAND_REGISTER_READ_ANS =	0x0081,
	ARV_GVCP_COMMAND_REGISTER_WRITE_CMD =	0x0082,
	ARV_GVCP_COMMAND_REGISTER_WRITE_ANS =	0x0083,
	ARV_GVCP_COMMAND_STRING_READ_CMD =	0x0084,
	ARV_GVCP_COMMAND_STRING_READ_ANS =	0x0085,
	ARV_GVCP_COMMAND_STRING_WRITE_CMD =	0x0086,
	ARV_GVCP_COMMAND_STRING_WRITE_ANS =	0x0087
} ArvGvcpCommand;

typedef struct {
	guint16 packet_type;
	guint16 command;
	guint16 size;
	guint16 count;
}  __attribute__((__packed__)) ArvGvHeader;

typedef struct {
	ArvGvHeader header;
	unsigned char data[];
} ArvGvControlPacket;

#define ARV_TYPE_GV_INTERFACE             (arv_gv_interface_get_type ())
#define ARV_GV_INTERFACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_GV_INTERFACE, ArvGvInterface))
#define ARV_GV_INTERFACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_GV_INTERFACE, ArvGvInterfaceClass))
#define ARV_IS_GV_INTERFACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_GV_INTERFACE))
#define ARV_IS_GV_INTERFACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_GV_INTERFACE))
#define ARV_GV_INTERFACE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_GV_INTERFACE, ArvGvInterfaceClass))

typedef struct _ArvGvInterfaceClass ArvGvInterfaceClass;

struct _ArvGvInterface {
	ArvInterface	interface;

	GSocket *socket;
	GSocketAddress *control_address;
	GSocketAddress *broadcast_address;

	GHashTable *devices;
};

struct _ArvGvInterfaceClass {
	ArvInterfaceClass parent_class;
};

GType arv_gv_interface_get_type (void);

ArvInterface * 		arv_gv_interface_get_instance 		(void);

ArvDevice * 		arv_gv_interface_get_device_by_address 	(ArvGvInterface *gv_interface, const char *address);

G_END_DECLS

#endif
