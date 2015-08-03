#!/usr/bin/env python3

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

                self.stop_button.configure(state='normal')

                while True: #self.process.poll() is None:
                    output = self.process.stdout.readline()
                    output = output.decode("utf-8")
                    if output == '':
                        break
                    self.textbox.configure(state='normal')
                    self.textbox.insert(INSERT, output)
                    self.textbox.configure(state='disabled')
                    self.textbox.see(END)
                self.stop_button.configure(state='disabled')	
                self.process = None

        def worker(self, string):
                self.active = True;
                print("pressed the bum button\n")
                t = Thread(target=self.work, args=(string,))
                t.start()


        def start(self, *args):
                cmd = self.entry.get()
                if self.process:
                    print("nooooooo")
                else:
                    self.entry.delete(0, END)
                    self.worker(cmd)

        def stop(self):
                #os.kill(self.process.pid, signal.SIGKILL)
                if self.process:
                        self.process.terminate()
                        self.process = None
                        self.active = False;
                        self.stop_button.configure(state='disabled')

        def create_widgets(self):
                # This stuff gives me nightmares...nevermind!
                self.frame = Frame(self)
                self.frame.pack(expand=1, fill='both')
                self.myParent.bind('<Return>', self.start)
                #self.label = Label(self.frame, text = "Run").grid(row=0, column = 0)
                self.entry = Entry(self.frame, width=90)
                self.entry.grid(row=0, column = 0, sticky=W)
                #self.button = Button(self.frame, text = "Run", command = self.start).grid(row=0, sticky=W)

                self.stop_button = Button(self.frame, text = "Terminate", command = self.stop)
                self.stop_button.grid(row=1, column=0, sticky=W)
                self.stop_button.configure(state='disabled')
                self.textbox = Text(self.frame, width=80,  height=24)
                self.textbox.grid(row=4, column = 0,sticky=W+E+N+S)
                self.entry.focus_set()
def main():
        root = Tk()
        root.title("I've got the Bogotron blues...")
        app = Application(root)
        root.resizable(0, 0)
        root.mainloop()

if __name__ == "__main__":
        main()
