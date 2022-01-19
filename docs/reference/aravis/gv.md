<part id="aravis-gv">
	<title>Ethernet protocol (GV)</title>

	<chapter id="aravis-gv-introduction">
		<title>Introduction</title>

		<para>
			This part contains a description of the protocol implemented by Aravis for the control of the gigabit ethenet cameras. This protocol is a standard used by many video cameras manufacturers for their products. It is a closed standard which should not be confused by what is used on 'IP cameras'. In the case of 'IP cameras' the video stream is a compressed stream (jpeg, h264...) send over HTTP or RTSP, whereas in Aravis, the video stream is a raw stream, mainly targeted to industrial applications (though the protocol supports non raw payload, it is not yet implemented in Aravis).
		</para>

		<para>
			The protocol implemented by Aravis library is based on UDP packets transmited over an ethernet link. Two different types of packets are used: GVCP packets and GVSP packets. GVCP packets are used for the control of the devices, and GVSP for the transmission of the data stream.
		</para>

		<para>
			This documentation is the result of a reverse engineering, which means it is far from complete. A more complete description of the ethernet protocol can be found in the GVSP and GVCP dissectors found in Wireshark sources.
		</para>	
	</chapter>

	<chapter id="aravis-gvcp">
		<title>GVCP packets</title>
		<para>
			Devices are waiting for GVCP packets listening to the GVCP port (3956). When the client sends a command GVCP packet to the device, the device sends back an acknowledge packets in return. Each command/acknowledge pair is identified by a 16 bit value, which allows to check if an acknowledge packet corresponds to the previsoulsy sent command. 0 is an error value for this identifier. If an acknowledge is not received after a given timeout period, another command packet is sent, until a maximum number of retries is reached.
		</para>

		<para>
			The content of a GVCP packet is composed by a 64 bit headers, followed or not by a data byte array, depending on the GVCP packet type. Multibyte values are in big endian encoding.
		</para>

		<sect2>
			<title>Discovery</title>

			<para>
				A device discovery mechanism is implemented using a discovery GVCP packet, broadcasted by the client to the GVCP port. Each available device responds to this discovery packet with an acknowledge packet, containing a description of the device (device name, model name, manufacturer name, MAC Address...).
			</para>

		<sect2>
		</sect2>
			<title>Read and write register</title>

			<para>
				These packets are used for the read and write of 32 bit registers, accessed using 32 bit adresses. For the read command, a list of addresses is sent ot the client, which returns a list of 32 bit values. For the write command, a list of address/value pairs is sent to the client, which returns a list of 32 bit values.
			</para>
			<para>
				Address and data are encoded in the packet as big endian values.
			</para>
		</sect2>
		<sect2>
			<title>Read and write memory</title>
			<para>
				Write memory packet data area consists in a 32 bit address, followed by the data to write. Client returns an acknowledge packet with the target address.
			</para>
			<para>
				For read memory command, an address/size pair is sent to the device, which returns the content of the given memory area.
			</para>
		</sect2>

	</chapter>

	<chapter id="aravis-gvsp">
		<title>GVSP packets</title>

		<para>
			GVSP packets are sent by the device for the video stream transmission. Depending on the device, it is possible to have more than one stream simultaneously, each directed to a different client port. Stream is splitted in frames. For each frame, identified by a frame id, three different packets are used: a leader packet, one or more data packets, and a trailer packet. Each packet in a frame is identified by a packet id, starting from 0.
		</para>

		<para>
			In the video stream reception code, client must take care to check if all frame packets are present, and ask for packet resend for missing ones, using a GVCP packet resend command.
		</para>
	</chapter>
</part>
