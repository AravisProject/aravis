Title: Porting to Aravis 0.8

### Porting to Aravis 0.8

Aravis 0.8 has seen a major rewrite of how communication errors are handled.
Instead of relying on a status API, each function that can fail has an
additional error parameter now. This is the standard way of handling error in
the glib ecosytem. The nice side effect is now errors throw exceptions in
bindings where the language support them (rust, python, javascript).

A quick port from the older series to 0.8 series is just a matter of adding a
NULL parameter to most of the modified functions. But you are advised to take
this opportunity to correctly handle errors.

There is a page explaining Glib errors and how to manage them in the [Glib
documentation](https://developer.gnome.org/glib/stable/glib-Error-Reporting.html).

During the camera configuration, in C language it can be somehow cumbersome to
check for errors at each function call. A convenient way to deal with this issue
is the following construction:

```c
GError **error = NULL;

if (!error) arv_camera_... (..., &error);
if (!error) arv_camera_... (..., &error);
if (!error) arv_camera_... (..., &error);
if (!error) arv_camera_... (..., &error);

if (error) {
	handle error here;
	g_clear_error (&error);
}
```
