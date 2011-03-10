#!/bin/python
from threading import Timer

class MyClass:
	def __init__(self):
		self.t = Timer(2.0, self.hello)
		self.t.start()
	
	def hello(self):
		print 'hello, world!'
		self.t = Timer(2.0, self.hello)
		self.t.start()

mc = MyClass()
