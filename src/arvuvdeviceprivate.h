#ifndef ARV_UV_DEVICE_PRIVATE
#define ARV_UV_DEVICE_PRIVATE

#include <arvuvdevice.h>

gboolean 	arv_uv_device_bulk_transfer 		(ArvUvDevice *uv_device, unsigned char endpoint, void *data,
							 size_t size, size_t *transferred_size, GError **error);

#endif
