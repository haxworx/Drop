#!/usr/bin/env python

from tkinter import *

class Application(Frame):
	def __init__(self, master):
		super(Application, self).__init__(master)
		self.grid()
		self.create_widgets()
	def start(self):
		self.label.delete()
	def stop(self):
		self.label.delete()
	def create_widgets(self):
		self.frame = Frame(self)
		self.frame.pack(side = TOP)
		self.label = Label(self.frame, text = "Command to Run").pack(side = LEFT)
		self.script = Entry(self.frame)
		self.script.pack(side = LEFT)
		self.button = Button(self.frame, text = "Run", command = self.start).pack(side = LEFT)
		self.stop_button = Button(self.frame, text = "Stop", command = self.stop)
		self.stop_button.pack(side = LEFT)
		self.second_frame = Frame(self)
		self.second_frame.pack()
		
		self.text = Text(self.second_frame)
		self.text.pack(side = BOTTOM)
def main():
	root = Tk()
	root.title("Bogotron Controller")
	root.geometry("400x300")
	app = Application(root)
	root.mainloop()

if __name__ == "__main__":
	main()
