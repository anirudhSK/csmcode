from collections import deque
from operator import itemgetter, attrgetter

class CustomQueue(object):
	def __init__(self):
		self.dq = deque([])
	
	def empty(self):
		return len(self.dq) == 0
	
	def clear(self):
		while len(self.dq) > 0:
			del self.dq[0]
	
	def size(self):
		return len(self.dq)
	
	def top(self):
		return self.dq[0]
	
	def push(self, item):
		self.dq.append(item)
		return
	
	def pop(self):
		return self.dq.popleft()
	
	def sort(self, fn, r):
		self.dq = deque(sorted(self.dq, key=fn, reverse=r))
	
	def __repr__(self):
		return 'PacketQueue -' + repr(self.dq)
	
	def __getitem__(self, index):
		return self.dq[index]
	
	def __delitem__(self, index):
		del self.dq[index]