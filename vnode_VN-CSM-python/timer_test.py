#!/bin/python
from recurring_timer import *

class MyClass(object):
	def __init__(self):
		self.joinTimer = RecurringTimer(2, self.do_something)
		self.joinTimer.start()
		self.count = 0
	
	def do_something(self):
		self.count += 1
		if self.count > 3:
			self.joinTimer.resched(0.5)
		print 'hello world %d' % self.count
		
mc = MyClass()