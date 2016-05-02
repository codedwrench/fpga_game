# Released by rdb under the Unlicense (unlicense.org)
# Based on information from:
# https://www.kernel.org/doc/Documentation/input/joystick-api.txt

import os, struct, array
from fcntl import ioctl
from subprocess import Popen, PIPE

# Iterate over the joystick devices.
print('Available devices:')

for fn in os.listdir('/dev/input'):
    if fn.startswith('js'):
        print('  /dev/input/%s' % (fn))

process = Popen(["/mnt/ssddata/altera_lite/15.1/quartus/bin/nios2-terminal","--instance","0"],stdin=PIPE)
# We'll store the states here.
axis_states = {}
button_states = {}

# These constants were borrowed from linux/input.h
axis_names = {
    0x00 : 'x',
    0x01 : 'y',
}

button_names = {
    0x130 : 'a',
    0x122 : 'a',
    0x131 : 'b',
    0x121 : 'b',
    0x132 : 'c',
    0x124 : 'c',
    0x133 : 'x',
    0x123 : 'x',
    0x134 : 'y',
    0x120 : 'y',
    0x135 : 'z',
    0x125 : 'z',
    0x139 : 's',
    0x129 : 's',
    0x138 : 'f',
    0x128 : 'f',
    0x136 : 'd',
    0x126 : 'd',
    0x137 : 'e',
    0x127 : 'e',
}

axis_map = []
button_map = []

# Open the joystick device.
fn = '/dev/input/js0'
print('Opening %s...' % fn)
jsdev = open(fn, 'rb')

# Get the device name.
#buf = bytearray(63)
buf = array.array('c', ['\0'] * 64)
ioctl(jsdev, 0x80006a13 + (0x10000 * len(buf)), buf) # JSIOCGNAME(len)
js_name = buf.tostring()
print('Device name: %s' % js_name)

# Get number of axes and buttons.
buf = array.array('B', [0])
ioctl(jsdev, 0x80016a11, buf) # JSIOCGAXES
num_axes = buf[0]

buf = array.array('B', [0])
ioctl(jsdev, 0x80016a12, buf) # JSIOCGBUTTONS
num_buttons = buf[0]

# Get the axis map.
buf = array.array('B', [0] * 0x40)
ioctl(jsdev, 0x80406a32, buf) # JSIOCGAXMAP

for axis in buf[:num_axes]:
    axis_name = axis_names.get(axis, 'unknown(0x%02x)' % axis)
    axis_map.append(axis_name)
    axis_states[axis_name] = 0.0

# Get the button map.
buf = array.array('H', [0] * 200)
ioctl(jsdev, 0x80406a34, buf) # JSIOCGBTNMAP

for btn in buf[:num_buttons]:
    btn_name = button_names.get(btn, 'unknown(0x%03x)' % btn)
    button_map.append(btn_name)
    button_states[btn_name] = 0

print '%d axes found: %s' % (num_axes, ', '.join(axis_map))
print '%d buttons found: %s' % (num_buttons, ', '.join(button_map))

# Main event loop
while True:
    evbuf = jsdev.read(8)
    if evbuf:
        time, value, type, number = struct.unpack('IhBB', evbuf)

        if type & 0x80:
            print "(initial)",

        elif type & 0x01:
            button = button_map[number]
            if button:
                button_states[button] = value
                if value:
                    #print "1%sp" % (button)
                    process.stdin.write("1%sp" % (button))
                else:
                    #print "1%sr" % (button)
                    process.stdin.write("1%sr" % (button))

        elif type & 0x02:
                axis = axis_map[number]
                if axis:
                    fvalue = value / 32767.0
                    axis_states[axis] = fvalue
                    if(fvalue > 0 and axis == 'x'):
                        #print "1rp" 
                        process.stdin.write("1rp")
                    elif(fvalue < 0 and axis == 'x'):
                        #print "1lp"
                        process.stdin.write("1lp")
                    elif(fvalue > 0 and axis == 'y'):
                        #print "1gp"
                        process.stdin.write("1gp")
                    elif(fvalue < 0 and axis == 'y'):
                        #print "1hp"
                        process.stdin.write("1hp")
                    elif(fvalue == 0 and axis == 'x'):
                        #print "1lr"
                        process.stdin.write("1lr")
                    elif(fvalue == 0 and axis == 'y'):
                        #print "1gr"
                        process.stdin.write("1gr")


