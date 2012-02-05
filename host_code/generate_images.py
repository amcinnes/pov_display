import subprocess

def read_token(f):
	# Read next token
	token = b''
	while True:
		c = bytes([f.__next__()])
		if c==b'#':
			while c!=b'\r' and c!=b'\n':
				c = bytes([f.__next__()])
		elif c in b' \t\r\n':
			if token:
				break
		else:
			token += c
	return token

def read_image(img):
	f = img.__iter__()
	magic = read_token(f)
	assert(magic==b'P4')
	width = int(read_token(f))
	height = int(read_token(f))
	assert(height==16)
	data = [0]*width
	for i in range(height):
		for j in range(width):
			if j%8==0:
				c = ord(bytes([f.__next__()]))
			if (c>>(7-j%8))&1:
				data[j] |= 1<<i
	return data	

indices = [0]
array = []

for i in range(32,128):
	if i==92:
		s = '\\\\'
	else:
		s = chr(i)
	p = subprocess.Popen(['convert','-pointsize','15','-crop','x16+0+2','label:'+s,'pbm:-'],stdout=subprocess.PIPE)
	(out,err) = p.communicate()
	d = read_image(out)
	indices.append(indices[-1]+len(d))
	array+=d

print('PROGMEM const uint16_t font_index[] = {')
for i in indices:
	print('\t%d,'%i)
print('};')
print()
print('PROGMEM const uint16_t font_data[] = {')
for i in array:
	print('\t0x%04x,'%i)
print('};')
