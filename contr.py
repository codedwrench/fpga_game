import pygame
import time

from subprocess import Popen, PIPE
process = Popen(["C:\\altera_lite\\15.1\\quartus\\bin64\\nios-monitor-terminal.exe","1","0"],stdin=PIPE)
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
trippedaxes = {}

for i in range( joystick_count ): #initialization
    joystick[i] = pygame.joystick.Joystick(i)
    joystick[i].init()
    name[i] = joystick[i].get_name()
    print name[i]
    numaxes[i] = joystick[i].get_numaxes() 
    print "Number of axes = " + str(numaxes[i])
    for cnt in range( numaxes[i] ):
        if(i > 0): #when it's on the second joystick it should put the values higher in the array
            axis[cnt + numaxes[i-1]] = joystick[i].get_axis(cnt)
            trippedaxes[cnt + numaxes[i-1]] = 0;
        else:
            axis[cnt] = joystick[i].get_axis(cnt)
            trippedaxes[cnt] = 0;

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
                    process.stdin.write( "1" + str(i) + "p\n")
                    pressed[i] = 1;

            for i in range(numbuttons[1]): #It's on controller 2
                if(joystick[1].get_button(i) == 1):
                    process.stdin.write( "2" + str(i) + "p\n")
                    pressed[i + numbuttons[0]] = 1

        if event.type == pygame.JOYBUTTONUP: #User released a button
            for i in range(numbuttons[0]):
                if(joystick[0].get_button(i) == 0 and pressed[i] == 1):
                    process.stdin.write( "1" + str(i) + "r\n")

            for i in range(numbuttons[1]):
                if(joystick[1].get_button(i) == 0 and pressed[i + numbuttons[0]] == 1):
                    process.stdin.write( "2" + str(i) + "r\n")

        if event.type == pygame.JOYAXISMOTION: #An axis is handled differently
            for i in range(len(joystick)):   
                if(joystick[i].get_axis(0) > 0.9): #left and right
                    process.stdin.write(  str(i+1) + "rp\n")
                    if(i > 0):
                        trippedaxes[2] = 1
                    else:
                        trippedaxes[0] = 1;
                elif(joystick[i].get_axis(0) < -0.9):
                    process.stdin.write( str(i+1) + "lp\n")
                    if(i > 0):
                        trippedaxes[2] = 1
                    else:
                        trippedaxes[0] = 1;
                elif((trippedaxes[0] == 1 and i == 0) or (trippedaxes[2] == 1 and i >0) and (joystick[i].get_axis(0) > -0.1 and joystick[i].get_axis(0) < 0.1)):
                    trippedaxes[0] = 0;
                    trippedaxes[2] = 0;
                    process.stdin.write( str(i+1) + "lr\n")


                if(joystick[i].get_axis(1) > 0.9): #up and down
                    process.stdin.write(  str(i+1) + "gp\n")

                    if(i > 0):
                        trippedaxes[3] = 1
                    else:
                        trippedaxes[1] = 1;
                elif(joystick[i].get_axis(1) < -0.9):
                    process.stdin.write( str(i+1) + "hp\n")
                    if(i > 0):
                        trippedaxes[3] = 1
                    else:
                        trippedaxes[1] = 1;
                elif((trippedaxes[1] == 1 and i == 0) or (trippedaxes[3] == 1 and i >0) and (joystick[i].get_axis(1) > -0.1 and joystick[i].get_axis(1) < 0.1)):
                    trippedaxes[1] = 0;
                    trippedaxes[3] = 0;
                    process.stdin.write( str(i+1) + "hr\n")
