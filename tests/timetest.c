#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

static gint64
time_wait (gint64 usec )
{
	gint64 wt, st, tt;
	gint64 i;

	st = g_get_real_time ();
	wt = st + usec;
	for (i = 0, tt = g_get_real_time(); tt < wt; i++ )
		tt = g_get_real_time ();

  return tt-st;
}

static gint64
sleep_meas (gint64 usec) {
	gint64 tt;

	tt = g_get_real_time ();
	g_usleep (usec);
	tt = g_get_real_time () - tt;

	return tt;
}

#define MAX_TIME_US	200000
#define N_ITERS		100

int
main (int argc, char **argv)
{
	gint64 i, j;
	double val, wt, min, max, swt;

	for( i = 1; i < MAX_TIME_US; i = i*2 ) {
		max = wt = swt = 0.; min = MAX_TIME_US;
		for (j = 0; j < N_ITERS; j++ ) {
			val = sleep_meas (i);
			wt  += val;
			swt += val*val;
			if (val < min) min = val;
			if (val > max) max = val;
		}
		wt /= j;
		g_print ("SleepMeas: %6" G_GINT64_FORMAT " - Mean %7g Max %5g Min %5g rms %g\n",
			i, wt, max, min, sqrt(swt/j - wt*wt));
	}

	for( i = 1; i < MAX_TIME_US; i = i*2 ) {
		max = wt = swt = 0.; min = MAX_TIME_US;
		for (j = 0; j < N_ITERS; j++ ) {
			val = time_wait (i);
			wt  += val;
			swt += val*val;
			if( val < min ) min = val;
			if( val > max ) max = val;
		}
		wt /= j;
		g_print ("TimeWait:  %6" G_GINT64_FORMAT " - Mean %7g Max %5g Min %5g rms %g\n",
			i, wt, max, min, sqrt(swt/j - wt*wt));
	}

	return EXIT_SUCCESS;
}
