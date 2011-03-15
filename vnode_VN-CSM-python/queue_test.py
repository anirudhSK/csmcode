from collections import deque
from operator import itemgetter, attrgetter

def lookup(queue, pkt):
	if not queue or len(queue) == 0:
		return 0
	for i in range(len(queue)):
		item = queue[i]
		if item != pkt:
			print '%s != %s' % (item, pkt)
		else:
			print '%s == %s, removing...' % (item, pkt)
			del queue[i]
			return 1
	return 0


class Packet(object):
	def __init__(self, id, time):
		self.id = id
		self.time = time
	def __repr__(self):
		return "Packet" + repr((self.id, self.time))


# Create test deque
b = Packet('b', 5)
a = Packet('a', 2)
e = Packet('e', 1)
d = Packet('d', 7)
q = deque([b, a, e, d])
c = Packet('c', 3)
q.append(c)
print q

# Test VN-C_CSM lookup function (should not be called lookup...)
lookup(q, e)
print q

# Test priority sorting
q = deque(sorted(q, key=attrgetter('time'), reverse=False))
print q

# Test popping
while len(q):
	print 'popped ' + str(q.popleft())

print q



class PacketQueue(object):
	def __init__(self):
		self.dq = deque([])
	
	def empty(self):
		return len(self.dq) == 0
	
	def size(self):
		return len(self.dq)
	
	def top(self):
		return self.dq[0]
	
	def push(self, item):
		self.dq.append(item)
		return
	
	def pop(self):
		return self.dq.popleft()
	
	def sort(self, attr, r):
		self.dq = deque(sorted(self.dq, key=attrgetter(attr), reverse=r))
	
	def __repr__(self):
		return 'PacketQueue -' + repr(self.dq)
	
	def __getitem__(self, index):
		return self.dq[index]
	
	def __delitem__(self, index):
		del self.dq[index]
	

def lookup2(queue, pkt):
	if not queue or queue.size() == 0:
		return 0
	for i in range(queue.size()):
		item = queue[i]
		if item != pkt:
			print '%s != %s' % (item, pkt)
		else:
			print '%s == %s, removing...' % (item, pkt)
			del queue[i]
			return 1
	return 0


# Create test deque
b = Packet('b', 5)
a = Packet('a', 2)
e = Packet('e', 1)
d = Packet('d', 7)
c = Packet('c', 3)
q = PacketQueue()
q.push(b)
q.push(a)
q.push(e)
q.push(d)
q.push(c)
print q

# Test VN-C_CSM lookup function (should not be called lookup...)
lookup2(q, e)
print q

# Test priority sorting
q.sort('time', False)
print q

# Test popping
while not q.empty():
	print 'popped ' + str(q.pop())

print q