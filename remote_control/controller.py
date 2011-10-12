#!/usr/bin/env python
#Remote Controller for the STM rc-car 

#Remote Cotrolling protocol:
#A package consists of:
#[length] + [mode] + [plaintext ascii command/message]
#where length and mode are one byte. Length may be 0!
#for the standart UPDATE_MODE package, the message text consinst of:
#[index_for_var_a][a_space][value_for_var_a][a_space].....up to a length of 255bytes!

import pygtk
pygtk.require('2.0')
import gtk
import glib
import socket
import threading
import thread
import array

global sock

UPDATE = '1'
REQUEST = '2'
DEBUG = '0'

my_states_index = [ "0" ,   "1"   , "2"  ,  "3"   ,]
my_states = ["steering", "accel","lights", "debug",]
my_state_vals = [ 50   ,     50  ,    0  ,    0   ,]

car_stats_index = ["4", "5"   ,   "6"   ,   "7"  ,]
car_stats = ["temp", "speed", "battery", "signal",]
car_stats_vals = [0,      0    ,     0   ,   0   ,]

car_stats_lock = thread.allocate_lock()

def add_nulls(num, cnt):
	cnt = cnt - len(str(num))
	nulls = '0' * cnt
	return '%s%s' % (nulls, num)

def set_car_stats(index, val):
	global car_stats_vals
	global car_stats
	global car_stats_lock

	car_stats_lock.acquire()
	try:
		car_stats_vals[car_stats_index.index(index)] = val
	except ValueError:
		car_stats_lock.release()
		return False
	car_stats_lock.release()
	return True	

def get_car_stats(name):
	global car_stats_vals
	global car_stats
	global car_stats_lock

	car_stats_lock.acquire()
	try:
		ret = car_stats_vals[car_stats.index(name)]
	except ValueError:
		car_stats_lock.release()
		return None
	car_stats_lock.release()
	return ret	

class communication():
	def __init__(self):
		self.connected = False

	def connect(self, ip_address, port) :
		global sock
		sock = socket.socket()
		try:
			sock.connect((ip_address, port))
		except socket.error, msg:
	       		sock.close()
			sock = None
			print "could not connect"
			return False
		sock.settimeout(3)
		self.rec_thread = receive_thread()
		self.rec_thread.deamon = True
		self.rec_thread.start()
		self.connected = True
		return True

	def disconnect(self):
		global sock
		if self.connected == False:
			return True
		self.connected = False
		self.rec_thread.stop()
		
		sock.close()	
		self.rec_thread.join()
		sock = None
		return True

	def send_packet(self, data, mode):
		global sock
		if self.connected == False:
			print "not connected"
			return False
<<<<<<< HEAD
		length  = add_nulls(len(data),3)
		if len(data) < 0 or len(data) > 255:
			print "Error length:" ,length
=======
		length = len(data)
		if length < 0 or length > 255:
			print "data has length" ,length
>>>>>>> parent of 7dc9646... added some debugging commands.
			return False
		if mode != UPDATE and mode != REQUEST and mode != DEBUG :
			print "unknown mode ", mode
			return False
		sock.send(length)
		sock.send(mode)
		sock.send(data)
		return True

class receive_thread(threading.Thread):
       	def run(self): 	
		while 1:
			if self._stop_receive.isSet():
				print "ending receive" 
				return
			try:	
				data = sock.recv(3) 	#length
			except:
				continue
			if len(data) == 0:
				continue
			length = int(data)
			try:	
				data = sock.recv(1)	#mode
			except	:
				print "couldnt receive packet type"
				continue
			mode = data
			data = ''
			end = False
			while len(data) < length and end == False:
				try: 
					data += sock.recv(length-len(data)) #data
				except :
					print "couldnt receive the data"
					end = True
					continue
			if end != True:
				self.handle_package(mode,data)
		return
	
	def handle_package(self, mode, data):	
		if mode == DEBUG:
			print data
			return True
		elif mode == UPDATE:
			var_list = data.split()
			length = len(var_list) 	
			i = 0
			while i < length-1:
				if set_car_stats(var_list[i],int(var_list[i+1]))== False:
					print "unknown var in Package update"
				i += 2

			glib.idle_add(stuff_box.temp.set_label,
				"{temp} Degrees C" .format(temp = get_car_stats("temp")))
			glib.idle_add(stuff_box.speed.set_label,
				"{speed} Km/h" .format(speed = get_car_stats("speed")))
			glib.idle_add(stuff_box.battery.set_fraction, 
				float(float(get_car_stats("battery"))/float(100)))	
			glib.idle_add(stuff_box.sig_strength.set_fraction, 
				float(float(get_car_stats("signal"))/float(100)))	
		elif mode == REQUEST:
			print "request" 
		else :
			print "unknown mode"
			return False
		return True

	def __init__(self):
       	 	super(receive_thread, self).__init__()
       	 	self._stop_receive = threading.Event()
		return
		
	def stop(self):
		self._stop_receive.set()

# Create a button with a arrow image and a name
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
		gtk.gdk.threads_init()	
		gtk.main()

	def delete_event(self, widget, event, data=None):
		self.car.disconnect()
		return False
	
	def destroy(self, widget, data=None):
		self.car.disconnect()
		gtk.main_quit()

	def __init__(self):
		global stuff_box
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
		self.options_box.remote_ip.entry.set_text("192.168.2.11");
		self.options_box.remote_ip.connect_button = gtk.Button(" Connect  ")
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
		control_box.up.connect("released", self.released_handler, "forward")
		control_box.down.connect("released", self.released_handler, "backward")
		control_box.left.connect("released", self.released_handler, "left")
		control_box.right.connect("released", self.released_handler, "right")	

		control_box.up.connect("pressed", self.pressed_handler, "forward")
		control_box.down.connect("pressed", self.pressed_handler, "backward")
		control_box.left.connect("pressed", self.pressed_handler, "left")
		control_box.right.connect("pressed", self.pressed_handler, "right")
					
		self.window.connect("key-press-event",self.click_event)
		self.window.connect("key-release-event",self.click_event)

		#Singals and Infos about the car:
		stuff_box.battery = gtk.ProgressBar()
		stuff_box.battery.set_text("Battery")
		stuff_box.sig_strength = gtk.ProgressBar()
		stuff_box.sig_strength.set_text("Signal strength")
		stuff_box.speed = gtk.Label("N/A Km/h")
		stuff_box.temp = gtk.Label("N/A Degrees C.");

		#add the widgets to the boxes
		self.options_box.pack_start(self.options_box.lights, False, False, 3)
		self.options_box.pack_start(self.options_box.logging, False, False, 3)
		self.options_box.pack_start(self.options_box.debug, False, False, 3)
		self.options_box.pack_start(self.options_box.remote_ip, False, False, 3)

		control_box.pack_start(control_box.up_box, True , False, 3)
		control_box.pack_start(control_box.sides, True, False, 3)
		control_box.pack_start(control_box.down_box, True, False, 3)

		stuff_box.pack_start(stuff_box.battery, False, False, 3)
		stuff_box.pack_start(stuff_box.sig_strength, False, False, 3)
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
	
	def options_handler(self, widget, data=None):
		if data == "debug" :
			val = 0
			if widget.get_active():
				val = 1
			self.do_action(data,val)
		if data == "logging" :
			print "logging is checked"
		if data == "lights":
			val = 0
			if widget.get_active():
				val = 1
			self.do_action(data,val)
		if data == "connect_button":
			if widget.get_label() == "Disconnect":
				if self.car.disconnect() == True:
					widget.set_label(" Connect ")
			else:
				if self.car.connect(self.options_box.remote_ip.entry.get_text(), 2005) == True :
					widget.set_label("Disconnect")

	up_clicked = 0
	down_clicked = 0
	right_clicked = 0
	left_clicked = 0

	def released_handler(self, widget, data):	
		if data == "forward":
			up_clicked = 0
			self.do_action(data,0)
		if data == "backward":
			down_clicked = 0
			self.do_action(data,0)
		if data == "left":
			left_clicked = 0
			self.do_action(data,0)
		if data == "right":
			right_clicked = 0
			self.do_action(data,0)

	def pressed_handler(self, widget, data):	
		if data == "forward":
			up_clicked = 1
			self.do_action(data,1)
		if data == "backward":
			down_clicked = 1
			self.do_action(data,1)
		if data == "left":
			left_clicked = 1
			self.do_action(data,1)
		if data == "right":
			right_clicked = 1
			self.do_action(data,1)

	def click_event(self, widget, event):
		key = gtk.gdk.keyval_name(event.keyval)
		if event.type == gtk.gdk.KEY_PRESS:
        		if (key == "w" or key == "Up") and self.up_clicked == 0:
				self.do_action("forward",1)
				self.up_clicked = 1
			if (key == "a" or key == "Right") and self.right_clicked == 0:
				self.do_action("right",1)
				self.right_clicked = 1
			if (key == "s" or key == "Down") and self.down_clicked == 0:
				self.do_action("backward",1)
				self.down_clicked = 1
			if (key == "d" or key == "Left") and self.left_clicked == 0:
				self.do_action("left",1)
				self.left_clicked = 1

		if event.type == gtk.gdk.KEY_RELEASE:	
        		if (key == "w" or key == "Up") and self.up_clicked == 1:
				self.do_action("forward",0)
				self.up_clicked = 0
			if (key == "a" or key == "Right") and self.right_clicked == 1:
				self.do_action("right",0)
				self.right_clicked = 0
			if (key == "s" or key == "Down") and self.down_clicked == 1:
				self.do_action("backward",0)
				self.down_clicked = 0
			if (key == "d" or key == "Left") and self.left_clicked == 1:
				self.do_action("left",0)
				self.left_clicked = 0

	def do_action(self, the_state, value):
		global my_state_vals
		global my_states
		if value == 0:
			print "released"
		else:
			print "pressed"
		if the_state == "forward":
			if value == 0: 
				value = 128 # stop
			else :
				value = 255	#fullspeed
			the_state = "accel"
		if the_state == "backward":
			if value == 0:
				value = 128
			else:
				 value = 0  #backward
			the_state = "accel"
			
		if the_state == "right":
			if value == 0:
				value = 128
			else :
				value = 255 #full right
			the_state = "steering"
		if the_state == "left":
			if value == 0:
				value = 128
			else :
				value = 0 #full left
			the_state = "steering"

		if the_state == "steering":
			value = ((value*(10/256))+10) #the car expects 10 to be left, and 20 right. so 15 is middle
			value += 0.5 #correct rounding
			value = int(value)
		my_state_vals[my_states.index(the_state)] = value
		self.update()
		return

	def update(self):
		global my_states
		global my_state_vals
		data = ""
		i = 0
		length = len(my_state_vals)
		while i < length:
			data += "{state}" .format(state = my_states_index[i])
			data += " "
			data += "{val} ".format(val = my_state_vals[i])
			i += 1
		print data
		self.car.send_packet(data, UPDATE)		
		self.car.send_packet(data, DEBUG)		
		return
	
# If the program is run directly or passed as an argument to the python
if __name__ == "__main__":
	app = main()
	app.main()

