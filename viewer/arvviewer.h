#include <arvviewertypes.h>

G_BEGIN_DECLS

ArvViewer * 		arv_viewer_new 			(void);
void			arv_viewer_set_options		(ArvViewer *viewer,
							 gboolean auto_socket_buffer,
							 gboolean packet_resend,
							 guint packet_timeout,
							 guint frame_retention);

G_END_DECLS
