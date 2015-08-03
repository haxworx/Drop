#!/usr/bin/env python
# help from richo healey at psych0tik.net
# Clearly I don't use Python much!!!

from threading import Thread
import subprocess
from tkinter import *
from queue import Queue
import time
import os
import signal
import shlex

class Application(Frame):
        def __init__(self, master):
                self.myParent = master;
                super(Application, self).__init__(master)
                self.active = False;
                self.process = None
                self.grid()
                self.create_widgets()

        def work(self, cmd):
                if self.process:
                    self.process.terminate() 
                    self.active = False;
                    return

                self.process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=False)
	
                while True: #self.process.poll() is None:
                    output = self.process.stdout.readline()
                    if output == '':
                        break
                    output = output.decode("utf-8")
                    self.textbox.insert(INSERT, output)

        def worker(self, string):
                self.active = True;
                print("pressed the bum button\n")
                t = Thread(target=self.work, args=(string,))
                t.start()


        def start(self):
                cmd = self.entry.get()
                if self.process:
                    print("nooooooo")	
                else:
                    self.worker(cmd)

        def stop(self):
                #os.kill(self.process.pid, signal.SIGKILL)
                self.process.terminate()
                self.process = None
                self.active = False;

        def create_widgets(self):
                # This stuff gives me nightmares...nevermind!
                self.frame = Frame(self)
                self.frame.grid()
                #self.label = Label(self.frame, text = "Run").grid(row=0, column = 0)
                self.entry = Entry(self.frame)
                self.entry.grid(row=1, column = 0)
                self.button = Button(self.frame, text = "Run", command = self.start).grid(row=2, column=0)

                self.stop_button = Button(self.frame, text = "Stop", command = self.stop)
                self.stop_button.grid(row=3, column = 0)

                self.textbox = Text(self.frame, width=400)
                self.textbox.grid(row=5, column = 4,sticky=W+E+N+S)
def main():
        root = Tk()
        root.title("I've got the Bogotron blues...")
        root.geometry("500x400")
        app = Application(root)
        root.mainloop()

if __name__ == "__main__":
        main()
