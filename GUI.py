#!/usr/bin/env python

# Clearly I don't use Python much!!!

from threading import Thread
import subprocess
from tkinter import *
from queue import Queue
import time

class Application(Frame):
	def __init__(self, master):
		self.myParent = master;
		super(Application, self).__init__(master)
		self.active = False;
		self.grid()
		self.create_widgets()
	def test(self, string):
		print("pressed the bum button\n")
		p = subprocess.Popen(string, stdout=subprocess.PIPE)
		out, err = p.communicate()
		print(out.decode())
	#	self.textbox.insert(INSERT, out.decode()) this is breaking!!!!

	def worker(self, string):
		self.active = False;
		t = Thread(target=self.test, args=(string,))
		t.start()
	
	def start(self):
		exe = self.entry.get()
		self.worker(exe)
		
	def stop(self):
		self.t.kill()
		self.active = False;

	def create_widgets(self):
		self.frame = Frame(self)
		self.frame.grid()
		self.label = Label(self.frame, text = "Command to Run").grid(row=0, column = 0)
		self.entry = Entry(self.frame)
		self.entry.grid(row=0, column = 1)
		self.button = Button(self.frame, text = "Run", command = self.start).grid(row=0, column=2)
		
		self.stop_button = Button(self.frame, text = "Stop", command = self.stop)
		self.stop_button.grid(row=0, column =3)
		
		self.textbox = Text(self.frame )
		self.textbox.grid(row=1, column = 0, columnspan=4 ,sticky=W+E+N+S)
def main():
	root = Tk()
	root.title("Bogotron Controller")
	root.geometry("400x300")
	app = Application(root)
	root.mainloop()

if __name__ == "__main__":
	main()
