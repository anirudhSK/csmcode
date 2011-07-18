from threading import Timer

class RecurringTimer(object):
	def __init__(self, interval, f):
		self.f = f
		self.interval = interval
		self.t = Timer(self.interval, self.expire)
		self.t.start()
	
	def start(self):
		self.t.start()
	
	def expire(self):
		self.f()
		self.t.cancel() # maybe unnecessary
		self.t = Timer(self.interval, self.expire)
		self.t.start()
	
	def resched(self, interval):
		self.interval = interval