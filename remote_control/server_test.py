#!/usr/bin/env python
# Echo server program
import socket
import sys

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 12345              # Arbitrary non-privileged port
UPDATE = 0
COMMAND = 1
DEBUG = 2

def send_packet(the_socket,data, mode):
	length = len(data)
	if length == 0 :
		print "data has length 0"
		return False
	if mode != UPDATE and mode != COMMAND and mode != DEBUG :
		print "unknown mode", mode
		return False 
	sended_length = the_socket.send(chr(length))
	print "sending {length}bytes of data" .format(length = length)
	the_socket.send(chr(mode))
	the_socket.send(data)
	return True

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
conn, addr = s.accept()
print "connected by ",addr
while 1:
	sys.stdin.read(1)	
	send_packet(conn,"helloweeeelltlltlltltlltt", DEBUG)
conn.close()


