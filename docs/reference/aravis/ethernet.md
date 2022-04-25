Title: Ethernet Devices

# Ethernet Device Performance

## Network Layout

If you face a lots of lost packets, the first thing to try is to directly connect
the device to your machine, using a wired connection. If it is possible, use a
dedicated port.

Most of the cameras are able to saturate their gigabit link, so any additional
traffic may lead to packet lost. If your machine ethernet port has a lower
bandwidth than the camera, make sure the stream you are requesting will not
exceed its bandwidth.

## Socket Buffer Size

Under heavy CPU load, the input socket buffer size may be increased in order to avoid
lost packets because of a full buffer. You can try the `--auto` parameter of
`arv-camera-test`.

In you application, this can be tweaked using `g_object_set()` on the stream
object. For example, this code, to be called before
[method@Aravis.Camera.start_acquisition], will enable the auto buffer size mode:

```c
g_object_set (stream,
	      "socket-buffer", ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
	      "socket-buffer-size", 0,
	      NULL);
```

## Receiving Thread Priority

It is possible to increase the receiving thread priority. You can experiment
with thread priority using the `--realtime` or `--high-priority` options of
`arv-camera-test`.

In your code, stream thread priority can be changed in the stream buffer
callback:

```c
static void
stream_cb (void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
        if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
		if (!arv_make_thread_realtime (10))
			printf ("Failed to make stream thread realtime\n");
	}
}
```
or:

```c
static void
stream_cb (void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
        if (type == ARV_STREAM_CALLBACK_TYPE_INIT) {
		if (!arv_make_thread_high_priority (-10))
			printf ("Failed to make stream thread high priority\n");
	}
}
```

## Stream Packet Size

One way to increase streaming performance and lower the CPU use is to increase
the stream packet size. [method@Aravis.Camera.gv_set_packet_size] and
[method@Aravis.Camera.gv_auto_packet_size] allow you to change this parameter.
By default, the network adapter of your machine will probably not let you
receive packet bigger than 1500 bytes, which is the default Maximum Transfer
Unit (MTU). It means if you want to use big packets, you also have to increase
the network adapter MTU to a greater walue (8192 bytes being a recommended
value). The exact procedure to change the MTU depends on the distribution you
are using. Please refer to the administrator manual of your distribution.

On Fedora, MTU can be changed using the network setting panel. Edit the wired
network settings. MTU parameter is in the Identity page.

On Debian / Ubuntu you can add a line "mtu 8192" to /etc/network/interfaces
under the correct device.

```
iface ethusb0 inet static
	address 192.168.45.1
	netmask 255.255.255.0
	broadcast 192.168.45.255
	mtu 8192
```

Please note if your device is not connected directly to the machine, you may
also have to tweak the active devices on your network.

## Packet Socket Support

Aravis can use packet sockets for the video receiving thread. But this mode
requires extended capabilities. If you want to allow your application to use
packet socket, you must set the `cap_net_raw` capability using `setcap`. For
example, the following command gives this capability to the Aravis viewer:

```
sudo setcap cap_net_raw+ep arv-viewer
```
