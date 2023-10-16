default:
	ninja -C ../../build
	#ARV_DEBUG=gentl:5 gdb --quiet -ex 'set print thread-events off' -ex r -ex quit -args python3 test-01-load.py
	ARV_DEBUG=gentl:5 gdb --quiet -ex 'set print thread-events off' -ex r -ex quit -args python3 test-02-nodes.py
