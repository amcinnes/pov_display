import sys
sys.path = ['/home/angus/printer_control/pyusb']+sys.path
import usb.core
import usb.util
import time
import struct

# Low level functions

def reconnect():
	global dev
	dev = usb.core.find(idVendor=0x03eb,idProduct=0x204f)
	if not dev:
		print('Waiting for device',file=sys.stderr)
		while not dev:
			time.sleep(0.1)
			dev = usb.core.find(idVendor=0x03eb,idProduct=0x204f)
	#dev.set_configuration()
	print('Found device',file=sys.stderr)

def sendb(b):
	assert(dev.ctrl_transfer(0x41,b,0,0,'')==0)

def send(s):
	# A couple of 0xff's for the receiver to warm up
	# Then 0xa5 to start the message
	# Then 0x00 to end the message
	s = b'\xff\xff\xa5'+s.encode()+b'\x00'
	for c in s:
		sendb(c)

reconnect()
