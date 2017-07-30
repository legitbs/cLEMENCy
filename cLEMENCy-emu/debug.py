import sys, os
import socket
import select

#connect then allow us to transfer data
s = socket.create_connection(["127.0.0.1", 2000])

while(1):
	try:
		(r,w,e) = select.select([s.fileno(), 0], [], [])
		if len(r) == 0:
			break
	except KeyboardInterrupt:
		s.send("\x03")
		continue
	except Exception as ex:
		print ex
		break

	#read from one and send to the other
	try:
		if r[0] == s.fileno():
			Data = s.recv(4096)
			if len(Data) == 0:
				break;
			sys.stdout.write(Data)
			sys.stdout.flush()
		else:
			Data = sys.stdin.readline()
			s.send(Data)
	except KeyboardInterrupt:
		s.send("\x03")
	except Exception as ex:
		print ex
		break
