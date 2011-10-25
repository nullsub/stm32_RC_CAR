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
import sys
import pygame

global sock
MAX_VALUE = (sys.maxint-1) 

UPDATE = '1'
REQUEST = '2'
DEBUG = '0'

STEERING_INDEX = '0'
ACCEL_INDEX = '1'
LIGHTS_INDEX = '2'
DEBUG_INDEX = '3'
TEMP_INDEX = '4'
SPEED_INDEX = '5'
BATTERY_INDEX = '6'



def convert_to_servo(value):
	tmp = float(value)
	tmp = ((tmp*(float(150)/float(MAX_VALUE+1)))+float(150)) #the car expects 150 to be left, and 300 right. so 225 is middle
	tmp += 0.5 #correct rounding
	return (int(tmp))
car_stats_lock = thread.allocate_lock()

class jstick(): 
	def __init__(self):
		pygame.joystick.init()
		pygame.display.init()
		self.nbJoy = pygame.joystick.get_count()

		for i in range(self.nbJoy):
			pygame.joystick.Joystick(i).init()
		return

	def update(self):
	def run(self):
		pygame.event.clear()

		while not self._stop.isSet():
			pygame.event.pump()
			ev_list = pygame.event.get([pygame.JOYBUTTONDOWN, pygame.JOYHATMOTION, pygame.JOYAXISMOTION, pygame.KEYDOWN, pygame.JOYAXISMOTION])
			for ev in ev_list:
				if ev.type == pygame.JOYHATMOTION:
					print "joyhatmotion"
				elif ev.type == pygame.JOYBUTTONDOWN:
					print "joybutton"
				elif ev.type == pygame.JOYAXISMOTION:
					if ev.axis == 0:
						the_state = "steering"
					elif ev.axis == 1: 
						the_state = "accel"
					else:
						the_state = "bug"
					print "value = ", value
					print the_state
				#	my_state_vals[my_states.index(the_state)] = value
				value = convert_to_servo(ev.value*MAX_VALUE)
				value = (value/2)+150 #why this??
		return
	
class stat:
	def __init__(self,index, val, car_controlled):
		self.lock = thread.allocate_lock()
		self.index = index
		self.val = val	
		self.car_controlled = car_controlled
		self.modified = True

	def set_val(self, val):
		self.lock.acquire()
		self.val = val
		self.modified = True
		self.lock.release()
	
	def get_car_controlled(self):
		return self.car_controlled	
	
	def get_index(self):
		return self.index
	
	def get_modified(self):
		self.lock.acquire()
		ret = self.modified
		self.modified = False
		self.lock.release()
		return ret	
	
	def get_val(self):
		self.lock.acquire()
		ret = self.val 
		self.lock.release()
		return ret

global steering 
global accel 	
global lights 
global debug 
global temp 
global speed
global battery

global stats

def init_stats():
	global stats

	global steering 
	global accel 	
	global lights 
	global debug 
	global temp 
	global speed
	global battery
		
	steering = stat(STEERING_INDEX, convert_to_servo((MAX_VALUE+1)/2), 0)
	accel 	=  stat(ACCEL_INDEX, convert_to_servo((MAX_VALUE+1)/2), 0)
	lights = stat(LIGHTS_INDEX, 0, 0)
	debug = stat(DEBUG_INDEX, 0, 0)
	temp = stat(TEMP_INDEX, 0, 1)
	speed = stat(SPEED_INDEX, 0, 1)
	battery = stat(BATTERY_INDEX, 0, 1)
	
	stats = [steering, accel, lights, debug, temp, speed, battery,]

def get_stat(index, val, car_controlled):
	global stats
	for stat in stats:
		if stat.get_index() == index and stat.get_car_controlled() == car_controlled:
			return stat.get_val()
	print "failed to get_stat"
	return False

def set_stat(index , val, car_controlled):
	global stats
	for stat in stats:
		if stat.get_index() == index and stat.get_car_controlled() == car_controlled:
			stat.set_val(val)
			return True
	def __del__(self):
		self._stop.set()
		pygame.quit()

	print "failed to set_stat"
	return False

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
		self.joystick = jstick()
		self.next_update = threading.Timer(0.2,self.update)
		self.next_update.start()
		return True

	def disconnect(self):
		global sock
		if self.connected == False:
			return True
		self.connected = False
		self.next_update.cancel()
		self.rec_thread.stop()
		
		sock.close()	
		self.rec_thread.join()
		self.joystick = None
		sock = None
		return True

	def update(self):
		global stats
		data = "" 
		self.joystick.update()
		for stat in stats:	
			if stat.get_car_controlled() == 0 and stat.get_modified():
				data += "{state}" .format(state = stat.get_index())
				data += " "
				data += "{val} ".format(val = stat.get_val())
		self.next_update = threading.Timer(0.15, self.update)
		self.next_update.start() 
		if data == "":
			return
		self.send_packet(data, UPDATE)
		return

	def send_packet(self, data, mode):
		global sock
		if self.connected == False:
			print "not connected"
			return False

		if mode != UPDATE and mode != REQUEST and mode != DEBUG :
			print "unknown mode ", mode
			return False

		if len(data) < 0 or len(data) > 255:
			print "Error length:" ,length
			return

		#add leading nulls
		nulls = '0' * (3 - len(str(len(data))))
		length = '%s%s' % (nulls, len(data))
		sock.send(length)
		sock.send(mode)
		sock.send(data)
		return True

class receive_thread(threading.Thread):
       	def run(self): 	
		data = ''
		while 1:
			if self._stop_receive.isSet():
				print "ending receive" 
				return
			try:	
				data += sock.recv(3-len(data)) 	#length
			except:
				continue
			if len(data) < 3:
				continue
			length = int(data)
			try:	
				data = sock.recv(1)	#mode
			except	:
				print "couldnt receive packet type"
				continue
			if len(data) == 0:
				print "bug in app"
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
			data = ''
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
				if set_stat(var_list[i],int(var_list[i+1]),1)== False:
					print "unknown var in Package update"
				i += 2

			glib.idle_add(stuff_box.temp.set_label,
				"{temp} Degrees C" .format(temp = temp.get_val()))
			glib.idle_add(stuff_box.speed.set_label,
				"{speed} Km/h" .format(speed = speed.get_val()))
			glib.idle_add(stuff_box.battery.set_fraction, 
				float(float(battery.get_val())/float(100)))	
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
		#	print "logging is checked"
			self.request_stats()
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
		if value == 0:	# released
			if the_state == "forward":
				value = ((MAX_VALUE+1)/2) # stop
				the_state = "accel"
			if the_state == "backward":
				value = ((MAX_VALUE+1)/2) 
				the_state = "accel"
			if the_state == "right":
				value = ((MAX_VALUE+1)/2) 
				the_state = "steering"
			if the_state == "left":
				value = ((MAX_VALUE+1)/2) 
				the_state = "steering"

		else:		# pressed
			if the_state == "forward":
				value = MAX_VALUE
				the_state = "accel"
			if the_state == "backward":
				value = 0
				the_state = "accel"
			if the_state == "right":
				value = MAX_VALUE
				the_state = "steering"
			if the_state == "left":
				value = 0
				the_state = "steering"
		if the_state == "steering" or the_state == "accel":
		my_state_vals[my_states.index(the_state)] = value
			value = convert_to_servo(value)
		set_stat(index, value, 0)
		return
	
	def request_stats(self):
		data = ''		
		self.car.send_packet(data, REQUEST)		
		return

# If the program is run directly or passed as an argument to the python
if __name__ == "__main__":
	app = main()
	app.main()

