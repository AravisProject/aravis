#include <arvenums.h>

static unsigned int
_from_string (const char *string, const char **strings, unsigned int n_strings)
{
	int i;

	if (string == NULL)
		return 0;

	for (i = 0; i < n_strings; i++)
		if (g_strcmp0 (string, strings[i]) == 0)
			return i;

	return 0;
}

static const char *arv_acquisition_mode_strings[] = {
	"Continuous",
	"SingleFrame"
};

const char *
arv_acquisition_mode_to_string (ArvAcquisitionMode value)
{
	return arv_acquisition_mode_strings[CLAMP (value, 0, ARV_ACQUISITION_MODE_SINGLE_FRAME)];
}

ArvAcquisitionMode
arv_acquisition_mode_from_string (const char *string)
{
	return _from_string (string, arv_acquisition_mode_strings,
			     G_N_ELEMENTS (arv_acquisition_mode_strings));
}

static const char *arv_trigger_source_strings[] = {
	"Line0",
	"Line1",
	"Line2"
};

const char *
arv_trigger_source_to_string (ArvTriggerSource value)
{
	return arv_trigger_source_strings[CLAMP (value, 0, ARV_ACQUISITION_MODE_SINGLE_FRAME)];
}

ArvTriggerSource
arv_trigger_source_from_string (const char *string)
{
	return _from_string (string, arv_trigger_source_strings,
			     G_N_ELEMENTS (arv_trigger_source_strings));
}
