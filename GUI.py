#!/usr/bin/env python

# Clearly I don't use Python much!!!

from threading import Thread
import subprocess
from tkinter import *
from queue import Queue
import time
import os
import signal 

		
class Application(Frame):
	def __init__(self, master):
		self.myParent = master;
		self.pro = None
		super(Application, self).__init__(master)
		self.active = False;
		self.grid()
		self.create_widgets()

	def work(self, string):
		self.pro = subprocess.call(string, shell=True)

	def worker(self, string):
		self.active = False;
		print("pressed the bum button\n")
		t = Thread(target=self.work, args=(string,))
		t.start()
		
	
	def start(self):
		exe = self.entry.get()
		self.worker(exe)
		
	def stop(self):
		self.pro.terminate()

	def create_widgets(self):
		self.frame = Frame(self)
		self.frame.grid()
		self.label = Label(self.frame, text = "Command to Run").grid(row=0, column = 0)
		self.entry = Entry(self.frame)
		self.entry.grid(row=0, column = 1)
		self.button = Button(self.frame, text = "Run", command = self.start).grid(row=0, column=2)
		
		self.stop_button = Button(self.frame, text = "Stop", command = self.stop)
		self.stop_button.grid(row=0, column =3)
		
		#self.textbox = Text(self.frame )
		#self.textbox.grid(row=1, column = 0, columnspan=4 ,sticky=W+E+N+S)
def main():
	root = Tk()
	root.title("Bogotron Controller")
	root.geometry("400x300")
	app = Application(root)
	root.mainloop()

if __name__ == "__main__":
	main()
