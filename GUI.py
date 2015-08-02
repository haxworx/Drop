#!/usr/bin/env python
import subprocess
from tkinter import *

class Application(Frame):
	def __init__(self, master):
		super(Application, self).__init__(master)
		self.grid()
		self.create_widgets()
		self.is_running = 0
	def start(self):
		self.listbox.insert(0, self.entry.get())	
		string = self.entry.get()
		self.p = subprocess.check_output(string)
		self.is_running = 1
		self.after(1000, update_result)
		
	def stop(self):
		self.listbox.insert(0,"hello")
	def update_result(self):
		if self.is_running:
			bytes = self.p.subprocess.check_output()	
			result = bytes.decode("utf-8")
			self.listbox.insert(0, result)

	def create_widgets(self):
		self.frame = Frame(self)
		self.frame.grid()
		self.label = Label(self.frame, text = "Command to Run").grid(row=0, column = 0)
		self.entry = Entry(self.frame)
		self.entry.grid(row=0, column = 1)
		self.button = Button(self.frame, text = "Run", command = self.start).grid(row=0, column=2)
		
		self.stop_button = Button(self.frame, text = "Stop", command = self.stop)
		self.stop_button.grid(row=0, column =3)
		
		self.listbox = Listbox(self.frame )
		self.listbox.grid(row=1, column = 0, columnspan=4 ,sticky=W+E+N+S)
def main():
	root = Tk()
	root.title("Bogotron Controller")
	root.geometry("400x300")
	app = Application(root)
	root.mainloop()

if __name__ == "__main__":
	main()
