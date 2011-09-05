#!/usr/bin/env python

#Remote Cotnroller for the STM rc-car 

import pygtk
pygtk.require('2.0')
import gtk
import socket
import threading
import array

UPDATE = '0'
COMMAND = '1'

actions = ["right","left", "forward","backward"]

class communication(threading.Thread):
	def connect(self, ip_address, port) :
		self.sock = socket.socket()
		try:
			self.sock.connect((ip_address, port))
		except socket.error, msg:
	       		self.sock.close()
       			self.sock = None
			print "could not connect"
			return False
		print "connected"
		self.rec_thread = receive_thread()
		self.rec_thread.deamon = True
		self.rec_thread.start()
		return True

	def disconnect(self):
		self.sock.close()
		self.rec_thread.stop()
		self.rec_thread.join()	
		return True
		
	def send_packet(self, data, mode):
		package.length = len(data)
		if length == 0 :
			print "data has length 0"
			return False
		if mode != UPDATE or mode != COMMAND :
			print "unknown mode"
			return False

		print "sending {length}bytes as descriptor... should be 4!" .format(length = len(htonl(length)))
		self.sock.send(htonl(length + 1))
		self.sock.send(mode)
		print "mode is {length} in size" .format(length = len(mode))
		
		print "sending {length}bytes of data" .format(length = lenght)
		self.sock.send(data)
		return True
	        
	# the receive thread. Its Blocking!
       	def run(self): 	
		while 1:
			if self._stop_receive.isSet():
				return
			try:
				data = self.sock.recv(4, MSG_WAITALL)
			except socket.error, msg:
				package.length = ntohl(data)
				print "couldnt receive"
			print "receiving {length}bytes of data" .format(length = package.length)
			package.data = self.sock.recv(package.length, MSG_WAITALL)
			#handle_package
		return
	
	def __init__(self):
       	 	super(communication, self).__init__()
       	 	self._stop_receive = threading.Event()
		
	def stop_reveive(self):
		self._stop_receive.set()


# Create an Arrow widget with the specified parameters
# and pack it into a button
def create_arrow_button(arrow_type, shadow_type, label_name):
	button = gtk.Button()
	arrow = gtk.Arrow(arrow_type, shadow_type)
	box =  gtk.HBox()
	label = gtk.Label(label_name)
	box.pack_start(arrow)
	box.pack_start(label)
	button.add(box)
	return button

class main:	
	def main(self):
		gtk.main()

	def delete_event(self, widget, event, data=None):
		return False
	
	def destroy(self, widget, data=None):
		gtk.main_quit()

	def __init__(self):
		# create a new window
		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		self.window.connect("delete_event", self.delete_event)
		self.window.connect("destroy", self.destroy)

		# Sets the border width of the window.
		self.window.set_border_width(15)

		main_box = gtk.VBox() 
		self.options_box = gtk.HBox()
		control_box = gtk.VBox()
		stuff_box = gtk.HBox()	
		
		#option field:
		self.options_box.lights = gtk.CheckButton("Lights")
		self.options_box.logging = gtk.CheckButton("Logging")
		self.options_box.debug = gtk.CheckButton("Debug")

		self.options_box.remote_ip = gtk.HBox()
		self.options_box.remote_ip.entry = gtk.Entry(15)
		self.options_box.remote_ip.entry.set_text("192.168.2.3")
		self.options_box.remote_ip.connect_button = gtk.Button("Connect    ")
		self.options_box.remote_ip.pack_start(self.options_box.remote_ip.entry, False, False, 13)
		self.options_box.remote_ip.pack_start(self.options_box.remote_ip.connect_button, False, False, 3)

		#connect option buttons
		self.options_box.lights.connect("toggled", self.options_handler, "lights")			
		self.options_box.logging.connect("toggled", self.options_handler, "logging")			
		self.options_box.debug.connect("toggled", self.options_handler, "debug")			
		self.options_box.remote_ip.connect_button.connect("clicked", self.options_handler, "connect_button")			

		#Control Frame:
		control_frame = gtk.Frame()
		control_frame.set_label("Controlling")
		control_frame.add(control_box);

		control_box.sides = gtk.HBox()
		control_box.up_box = gtk.HBox();
		control_box.down_box = gtk.HBox();
		
		control_box.up = create_arrow_button(gtk.ARROW_UP, gtk.SHADOW_IN, "Forwards")
		control_box.left = create_arrow_button(gtk.ARROW_LEFT, gtk.SHADOW_IN, "Left")		
		control_box.right = create_arrow_button(gtk.ARROW_RIGHT, gtk.SHADOW_IN, "Right")		
		control_box.down = create_arrow_button(gtk.ARROW_DOWN, gtk.SHADOW_IN, "Backwards")		

		control_box.sides.pack_start(control_box.left, True, False, 20)
		control_box.sides.pack_start(control_box.right, True, False, 20)
		control_box.up_box.pack_start(control_box.up, True, False, 13)
		control_box.down_box.pack_start(control_box.down, True, False, 13)

		#connect controlling buttons
		control_box.up.connect("clicked", self.controlling_handler, "forward")
		control_box.down.connect("clicked", self.controlling_handler, "backward")
		control_box.left.connect("clicked", self.controlling_handler, "left")
		control_box.right.connect("clicked", self.controlling_handler, "right")
					
		self.window.connect("key-press-event",self.click_event)

		#Singals and Infos about the car:
		stuff_box.battery_status = gtk.ProgressBar()
		stuff_box.battery_status.set_text("Battery")
		stuff_box.battery_status.pulse()
		stuff_box.signal_strength = gtk.ProgressBar()
		stuff_box.signal_strength.set_text("Signal strength")
		stuff_box.signal_strength.pulse()
		stuff_box.speed = gtk.Label("0.0 km/h")
		stuff_box.temp = gtk.Label("25 Decrees C.");

		#add the widgets to the boxes
		self.options_box.pack_start(self.options_box.lights, False, False, 3)
		self.options_box.pack_start(self.options_box.logging, False, False, 3)
		self.options_box.pack_start(self.options_box.debug, False, False, 3)
		self.options_box.pack_start(self.options_box.remote_ip, False, False, 3)

		control_box.pack_start(control_box.up_box, True , False, 3)
		control_box.pack_start(control_box.sides, True, False, 3)
		control_box.pack_start(control_box.down_box, True, False, 3)

		stuff_box.pack_start(stuff_box.battery_status, False, False, 3)
		stuff_box.pack_start(stuff_box.signal_strength, False, False, 3)
		stuff_box.pack_start(stuff_box.speed, False, False, 3)
		stuff_box.pack_start(stuff_box.temp, False, False, 3)

		#add the boxes to the main_box
		main_box.pack_start(self.options_box, True, False, 13)
		main_box.pack_start(control_frame, True, True, 13)
		main_box.pack_start(stuff_box, True, False, 13)
	
		#add the main_box to the window
		self.window.add(main_box)

		# The final step is to display all widget.
		self.window.show_all()	

		#do some initializations
		self.car = communication()
	
		self.action = array.array("i")     
		for i in range(0, len(actions)):
			self.action.insert(i, 0)	

	def options_handler(self, widget, data=None):
		if data == "debug" or data == "logging" or data == "lights":
			if (("OFF", "ON") [widget.get_active()]) == "ON":
				print "turning %s on" %data 
			else :
				print "turning %s off" %data
		
		if data == "connect_button":
			if widget.get_label() == "Connected":
				if self.car.disconnect() == True :
					widget.set_label("Disconnected")
			else:
				if self.car.connect(self.options_box.remote_ip.entry.get_text(), 2005) == True :
					widget.set_label("Connected")

	def controlling_handler(self, widget, data = None):	
		self.do_action(data,1)

	def click_event(self, widget, event):
		key = gtk.gdk.keyval_name(event.keyval)
        	if key == "w" or key == "Up":
			self.do_action("forward",1)
		if key == "a" or key == "Right":
			self.do_action("right",1)
		if key == "s" or key == "Down":
			self.do_action("backward",1)
		if key == "d" or key == "Left":
			self.do_action("left",1)

	def do_action(self, the_action, value):
		self.action[actions.index(the_action)] = value
		print actions
		print self.action 
		return

	def update(self):
		#self.car.send_package()		
		return
	
# If the program is run directly or passed as an argument to the python
if __name__ == "__main__":
	hello = main()
	hello.main()



