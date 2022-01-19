Title: Ethernet Devices

# Ethernet Device Performance

## Stream Packet Size

One way to increase streaming performance is to increase the stream packet size.
arv_camera_gv_set_packet_size() and arv_camera_gv_auto_packet_size() allow you
to change this parameter. By default, the network adapter of your machine will
probably not let you receive packet bigger than 1500 bytes, which is the default
Maximum Transfer Unit (MTU). It means if you want to use big packets, you also
have to increase the network adapter MTU to a greater walue (8192 bytes being a
recommended value). The exact procedure to change the MTU depends on the
distribution you are using. Please refer to the administrator manual of your
distribution.

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
example, the following command gives this capability to the Aravis viewer (You
should do this on the installed arv-viewer executable, has the one in the source
tree is just a wrapper shell script):

```
sudo setcap cap_net_raw+ep arv-viewer
```
