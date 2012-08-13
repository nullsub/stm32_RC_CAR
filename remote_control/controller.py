#!/usr/bin/env python
#Remote Controller for the rc-car 

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
import sys
import pygame

sock = 0
MAX_VALUE = (sys.maxint-1) 

MAX_SPEED = 25

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
	tmp = tmp*(float(150)/float(MAX_VALUE+1)) + float(150) #the car expects 150 to be left, and 300 right. so 225 is middle
	tmp += 0.5 #correct rounding
	return int(tmp)

class jstick(): 
	def __init__(self):
		pygame.joystick.init()
		pygame.display.init()
		joysticks = pygame.joystick.get_count()

		for i in range(joysticks):
			pygame.joystick.Joystick(i).init()
		return

	def update(self):
		try:
			pygame.event.pump()
			ev_list = pygame.event.get([pygame.JOYBUTTONDOWN, pygame.JOYBUTTONUP, pygame.JOYAXISMOTION])
		except:
			print "exception in joystick update"
			return
		for ev in ev_list:
			if ev.type == pygame.JOYBUTTONDOWN:
				print "button is ", ev.button
				if ev.button == 0:	
					value = 225-MAX_SPEED
					set_stat(ACCEL_INDEX, value, 0)
				if ev.button == 1:	
					value = 225+MAX_SPEED
					set_stat(ACCEL_INDEX, value, 0)
			elif ev.type == pygame.JOYBUTTONUP:
				if ev.button == 0:
					set_stat(ACCEL_INDEX, 225, 0)
				if ev.button == 1:
					set_stat(ACCEL_INDEX, 225, 0)
			elif ev.type == pygame.JOYAXISMOTION:
				value = convert_to_servo(ev.value*MAX_VALUE)
				value = (value/2)+150 #why this??
				if ev.axis == 0:
					set_stat(STEERING_INDEX, value, 0)
		return
	
class stat:
	def __init__(self, index, val, car_controlled):
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

stats = 0
steering  = 0
accel 	 = 0
lights  = 0
debug  = 0
temp  = 0
speed = 0
battery = 0

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

def get_stat(index, car_controlled):
	for stat in stats:
		if stat.get_index() == index and stat.get_car_controlled() == car_controlled:
			return stat.get_val()
	print "failed to get_stat"
	return False

def set_stat(index , val, car_controlled):
	for stat in stats:
		if stat.get_index() == index and stat.get_car_controlled() == car_controlled:
			stat.set_val(val)
			return True
	print "failed to set_stat"
	return False

class communication():
	def __init__(self):
		self.connected = False

	def connect(self, ip_address, port):
		global sock
		sock = socket.socket()
		try:
			sock.connect((ip_address, port))
		except:
	       		sock.close()
			sock = None
			print "could not connect"
			self.connected = False
			return False
		sock.settimeout(2)
		self.rec_thread = receive_thread()
		self.rec_thread.deamon = True
		self.rec_thread.start()
		self.connected = True
		self.joystick = jstick()
		self.next_update = threading.Timer(0.2, self.update)
		self.next_update.start()
		return True

	def disconnect(self):
		global sock
		if self.connected == False:
			return True
		self.connected = False
		self.next_update.cancel()
		self.rec_thread.stop()
		self.rec_thread.join()
		self.joystick = None
		sock.close()	
		sock = None
		return True

	def update(self):
		data = "" 
		self.joystick.update()
		for stat in stats:	
			if stat.get_car_controlled() == 0 and stat.get_modified():
				data += "{state}" .format(state = stat.get_index())
				data += " "
				data += "{val} ".format(val = stat.get_val())
		self.next_update = threading.Timer(0.15, self.update)
		self.next_update.start() 
		if data != "":
			self.send_packet(data, UPDATE)
		return

	def send_packet(self, data, mode):
		if self.connected == False:
			print "not connected"
			return False

		if mode != UPDATE and mode != REQUEST and mode != DEBUG :
			print "unknown mode ", mode
			return False

		if len(data) < 0 or len(data) > 255:
			print "Error length:" , len(data)
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
			except:
				print "couldnt receive packet type"
				continue
			if len(data) == 0:
				print "Debug: Len is 0"
			mode = data
			data = ''
			end = False
			while len(data) < length and end == False:
				try: 
					data += sock.recv(length-len(data)) #data
				except:
					print "couldnt receive the data"
					end = True
					continue
			if end != True:
				self.handle_package(mode, data)
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
				if set_stat(var_list[i], int(var_list[i+1]), 1) == False:
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
		else:
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
        	
		self.window.set_title("Remote Controller")

		#option field:
		self.options_box.lights = gtk.CheckButton("Lights")
		self.options_box.logging = gtk.CheckButton("Logging")
		self.options_box.debug = gtk.CheckButton("Debug")
		
		self.options_box.remote_ip = gtk.HBox()
		self.options_box.remote_ip.entry = gtk.Entry(15)
		self.options_box.remote_ip.entry.set_text("192.168.2.11")
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
		control_frame.add(control_box)

		control_box.sides = gtk.HBox()
		control_box.up_box = gtk.HBox()
		control_box.down_box = gtk.HBox()
		
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
					
		self.window.connect("key-press-event", self.click_event)
		self.window.connect("key-release-event", self.click_event)

		#Singals and Infos about the car
		stuff_box.battery = gtk.ProgressBar()
		stuff_box.battery.set_text("Battery")
		stuff_box.sig_strength = gtk.ProgressBar()
		stuff_box.sig_strength.set_text("Signal strength")
		stuff_box.speed = gtk.Label("N/A Km/h")
		stuff_box.temp = gtk.Label("N/A Degrees C.")

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

		# The final step is to display all widgets.
		self.window.show_all()	

		#do some initializations
		self.car = communication()
		init_stats()

	def options_handler(self, widget, data = None):
		if data == "debug" :
			val = 0
			if widget.get_active():
				val = 1
			self.do_action(DEBUG_INDEX, val)
		if data == "logging" :
			print "logging is checked"
		if data == "lights":
			val = 0
			if widget.get_active():
				val = 1
			self.do_action(LIGHTS_INDEX, val)
		if data == "connect_button":
			if widget.get_label() == "Disconnect":
				if self.car.disconnect():
					widget.set_label(" Connect ")
			else:
				if self.car.connect(self.options_box.remote_ip.entry.get_text(), 2000):
					widget.set_label("Disconnect")
					self.car.send_packet(data, REQUEST)	#get stats

	up_clicked = 0
	down_clicked = 0
	right_clicked = 0
	left_clicked = 0

	def released_handler(self, widget, data):
		if data == "forward":
			self.up_clicked = 0
			value = ((MAX_VALUE+1)/2) 
			self.do_action(ACCEL_INDEX, value)
		if data == "backward":
			self.down_clicked = 0
			value = ((MAX_VALUE+1)/2) 
			self.do_action(ACCEL_INDEX, value)
		if data == "left":
			value = ((MAX_VALUE+1)/2) 
			self.left_clicked = 0
			self.do_action(STEERING_INDEX, value)
		if data == "right":
			value = ((MAX_VALUE+1)/2) 
			self.right_clicked = 0
			self.do_action(STEERING_INDEX, value)

	def pressed_handler(self, widget, data):	
		if data == "forward":
			self.up_clicked = 1
			value = (MAX_VALUE) 
			self.do_action(ACCEL_INDEX, value)
		if data == "backward":
			self.down_clicked = 1
			value = 0
			self.do_action(ACCEL_INDEX, value)
		if data == "left":
			value = (MAX_VALUE) 
			self.left_clicked = 1
			self.do_action(STEERING_INDEX, value)
		if data == "right":
			value = 0 
			self.right_clicked = 1
			self.do_action(STEERING_INDEX, value)

	def click_event(self, widget, event):
		key = gtk.gdk.keyval_name(event.keyval)
		if event.type == gtk.gdk.KEY_PRESS:
        		if (key == "w" or key == "Up") and self.up_clicked == 0:
				key = "forward"
			if (key == "a" or key == "Right") and self.right_clicked == 0:
				key = "right"
			if (key == "s" or key == "Down") and self.down_clicked == 0:
				key = "backward"
			if (key == "d" or key == "Left") and self.left_clicked == 0:
				key = "left"
			self.pressed_handler(None, key)

		if event.type == gtk.gdk.KEY_RELEASE:	
        		if (key == "w" or key == "Up") and self.up_clicked == 1:
				key = "forward"
			if (key == "a" or key == "Right") and self.right_clicked == 1:
				key = "right"
			if (key == "s" or key == "Down") and self.down_clicked == 1:
				key = "backward"
			if (key == "d" or key == "Left") and self.left_clicked == 1:
				key = "left"
			self.released_handler(None, key)

	def do_action(self, index, value):
		if index == STEERING_INDEX or index == ACCEL_INDEX:
			value = convert_to_servo(value)
		set_stat(index, value, 0)
		return
	
# If the program is run directly or passed as an argument to the python
if __name__ == "__main__":
	app = main()
	app.main()

