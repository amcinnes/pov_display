import controller
import time

while True:
	t = time.time()
	s = time.strftime('%H:%M:%S',time.localtime(t))
	controller.send(s)
	t = time.time()
	t = t-int(t)
	time.sleep(1-t)
