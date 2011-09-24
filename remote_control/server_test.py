#!/usr/bin/env python
# Echo server program
import socket
import sys

HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 12345              # Arbitrary non-privileged port
UPDATE = 0
REQUEST = 1
DEBUG = 2

def send_packet(the_socket,data, mode):
	length = len(data)
	length = len(data)
	if length <= 0 or length > 255:
		print "data has length" ,length
		return False
	if mode != UPDATE and mode != COMMAND and mode != DEBUG :
		print "unknown mode ", mode
		return False
	the_socket.send(chr(length))
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
	send_packet(conn,"temp 23 battery 76 signal 52",UPDATE)
	
	sys.stdin.read(1)	
	send_packet(conn,"temp 21 battery 76 signal 12",UPDATE)
	
	sys.stdin.read(1)	
	send_packet(conn,"speed 32",UPDATE)

	sys.stdin.read(1)	
	send_packet(conn,"temp 24 battery 70 signal 45",UPDATE)
conn.close()


