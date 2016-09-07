#include <arvviewertypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_VIEWER             (arv_viewer_get_type ())
#define ARV_IS_VIEWER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_VIEWER))

GType 			arv_viewer_get_type 		(void);

ArvViewer * 		arv_viewer_new 			(void);
void			arv_viewer_set_options		(ArvViewer *viewer,
							 gboolean auto_socket_buffer,
							 gboolean packet_resend,
							 guint packet_timeout,
							 guint frame_retention);

G_END_DECLS
