import os
import pygame

# For headless mode
os.environ["SDL_VIDEODRIVER"] = "dummy"

pygame.init()
pygame.joystick.init()

joystick = pygame.joystick.Joystick(0)
joystick.init()

running = True
throttle_range = (-0.4, 0.4)

while running:
    pygame.event.pump()

    throttle = -joystick.get_axis(1)
    throttle = max(throttle_range[0], min(throttle_range[1], throttle))

    steering = joystick.get_axis(2)

    print(throttle, steering)
    if joystick.get_button(11): # start button
        running = False
