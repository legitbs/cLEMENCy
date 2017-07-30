import sys, os
import socket
import subprocess
import select
import time
import signal

def signal_handler(signal, frame):
        #emulator will get it as we are passing stdin straight through so ignore
	return

def main():
	#setup a listen socket, once connected pass it along to the application
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.bind(('', 4000))
	print "Listening on port 4000"

	s.listen(1)
	(a, hostinfo) = s.accept()

	print "Received connection from %s:%d" % (hostinfo[0], hostinfo[1])
	print "Socket FD: %d" % (a.fileno())

	#kick off the emulator
	p = subprocess.Popen(["./clemency-emu-debug","-d","0","-f","%d" % a.fileno(), "perplexity.bin"])
	#p = subprocess.Popen(["./clemency-emu","-f","%d" % a.fileno(), "perplexity.bin"])
	#p = subprocess.Popen(["./clemency-emu-debug","-d","0","-f","%d" % a.fileno(), "../../2017/heapfun4u-clemency/heapfun4u.bin"])
	#p = subprocess.Popen(["./clemency-emu-debug","-d","0","-f","%d" % a.fileno(), "../../2017/test/perplexity.bin"])

	#make sure they can ctrl-c into the emulator and not kill the script
	signal.signal(signal.SIGINT, signal_handler)
	while(1):
		if p.poll() != None:
			break

		time.sleep(0.5)
main()
