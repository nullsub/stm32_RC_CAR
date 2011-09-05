#!/usr/bin/env python

#Remote Cotnroller for the STM rc-car 

import pygtk
pygtk.require('2.0')
import gtk
import gtk.gdk

import socket

class remote:
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
		self.sock.send("hello")
		self.receive()
		return True

	def disconnect(self):
		self.sock.close()
		return True
	
	def receive():
		data = self.sock.recv(8, MSG_WAITALL)
		return



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
	remote = remote()	

	def delete_event(self, widget, event, data=None):
		# If you return FALSE in the "delete_event" signal handler,
		# GTK will emit the "destroy" signal. Returning TRUE means
		# you don't want the window to be destroyed.
		# This is useful for popping up 'are you sure you want to quit?'
		# type dialogs.
		print "delete event occurred"
		# Change FALSE to TRUE and the main window will not be destroyed
		# with a "delete_event".
		return False
	
	def destroy(self, widget, data=None):
		print "destroy signal occurred"
		gtk.main_quit()

	def __init__(self):
		# create a new window
		self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		# When the window is given the "delete_event" signal (this is given
		# by the window manager, usually by the "close" option, or on the
		# titlebar), we ask it to call the delete_event () function
		# as defined above. The data passed to the callback
		# function is NULL and is ignored in the callback function.
		self.window.connect("delete_event", self.delete_event)

		# Here we connect the "destroy" event to a signal handler.  
		# This event occurs when we call gtk_widget_destroy() on the window,
		# or if we return FALSE in the "delete_event" callback.
		self.window.connect("destroy", self.destroy)

		# Sets the border width of the window.
		self.window.set_border_width(15)

		main_box = gtk.VBox() 
		self.options_box = gtk.HBox()
		control_box = gtk.VBox()
		stuff_box = gtk.HBox()	

		self.options_box.lights = gtk.CheckButton("Lights")# Creates a new button with the label "Hello World".
		self.options_box.logging = gtk.CheckButton("Logging")# Creates a new button with the label "Hello World".
		self.options_box.debug = gtk.CheckButton("Debug")# Creates a new button with the label "Hello World".

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
		control_box.up.connect("clicked", self.controlling_handler, "forwards")
		control_box.down.connect("clicked", self.controlling_handler, "backwards")
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

	def main(self):
		# All PyGTK applications must have a gtk.main(). Control ends here
		# and waits for an event to occur (like a key press or mouse event).
		gtk.main()
	def options_handler(self, widget, data=None):
		if data == "debug" or data == "logging" or data == "lights":
			if (("OFF", "ON") [widget.get_active()]) == "ON":
				print "turning %s on" %data 
			else :
				print "turning %s off" %data
		
		if data == "connect_button":
			if widget.get_label() == "Connected":
				if self.remote.disconnect() == True :
					widget.set_label("Disconnected")
			else:
				if self.remote.connect(self.options_box.remote_ip.entry.get_text(), 2005) == True :
					widget.set_label("Connected")

	def controlling_handler(self, widget, data = None):	
		if data == "forwards" or data == "backwards":
			print ("forwards or backwards")
		if data == "right" or data == "left":
			print("right or left")

	def click_event(self, widget, event):
		key = gtk.gdk.keyval_name(event.keyval)
        	if key == "w" or key == "Up":
            		print "going forward"
		if key == "a" or key == "Right":
            		print "going right"
		if key == "s" or key == "Down":
            		print "going backward"
		if key == "d" or key == "Left":
            		print "going left"

# If the program is run directly or passed as an argument to the python
# interpreter then create a HelloWorld instance and show it
if __name__ == "__main__":
	hello = main()
	hello.main()



