import pygame
import time

from subprocess import Popen, PIPE
process = Popen(["/mnt/ssddata/altera_lite/15.1/quartus/bin/nios2-terminal","--instance","0"],stdin=PIPE)
pygame.init()
pygame.joystick.init()
joysticks = [pygame.joystick.Joystick(x) for x in range(pygame.joystick.get_count())]
joystick_count = pygame.joystick.get_count()

joystick = {}
name = {}
numaxes = {}
axis = {}
numbuttons = {}
buttons = {}
pressed = {}

for i in range( joystick_count ): #initialization
    joystick[i] = pygame.joystick.Joystick(i)
    joystick[i].init()
    name[i] = joystick[i].get_name()
    print name[i]
    numaxes[i] = joystick[i].get_numaxes() 
    print "Number of axes = " + str(numaxes[i])
    for cnt in range( numaxes[i] ):
        if(i > 0): #when it's on the second joystick it should put the values higer in the array
            axis[cnt + numaxes[i-1]] = joystick[i].get_axis(cnt)
        else:
            axis[cnt] = joystick[i].get_axis(cnt)

    numbuttons[i] = joystick[i].get_numbuttons()
    print "Number of buttons = " + str(numbuttons[i])
    for cnt in range (numbuttons[i]):
        if(i > 0):
            buttons[cnt + numbuttons[i-1]] = joystick[i].get_button(i)
            pressed[cnt+numbuttons[i-1]] = 0;
        else:
            buttons[cnt] = joystick[i].get_button(i)
            pressed[cnt] = 0;

    print "Player = " + str(i + 1) + "\n"

while 1:    
    for event in pygame.event.get(): # User did something

        if event.type == pygame.JOYBUTTONDOWN: #User pressed a button
            for i in range(numbuttons[0]): #It's on controller 1
                if(joystick[0].get_button(i) == 1):
                    print "1" + str(i) + "p"
                    pressed[i] = 1;

            for i in range(numbuttons[1]): #It's on controller 2
                if(joystick[1].get_button(i) == 1):
                    print "2" + str(i) + "p"
                    pressed[i + numbuttons[0]] = 1

        if event.type == pygame.JOYBUTTONUP: #User released a button
            for i in range(numbuttons[0]):
                if(joystick[0].get_button(i) == 0 and pressed[i] == 1):
                    print "1" + str(i) + "r"

            for i in range(numbuttons[1]):
                if(joystick[1].get_button(i) == 0 and pressed[i + numbuttons[0]] == 1):
                    print "2" + str(i) + "r"
