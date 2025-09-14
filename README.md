# Lost-and-Found-Locker-Box-Arduino-Code
This Arduino code creates a smart locker system controlled by an IR remote. Users enter a locker number and a PIN to operate a servo motor lock. The system provides comprehensive user feedback through an LCD for prompts, an RGB LED for visual status (blue, green, red), and a buzzer for distinct audible alerts on key presses and actions.

This Arduino code creates a complete smart locker system that uses an IR remote for user input.

Core Functionality: A user first enters a two-digit locker number, and if it's valid, they are prompted to enter a four-digit PIN. The code checks this combination against a stored database.

Hardware Control:

LCD Screen: Displays a welcome message ("Welcome!! Red Raider") and all prompts, such as "Enter Locker No" and "Enter PIN:".

Servo Motor: Acts as the lock, rotating to an "unlocked" position upon successful PIN entry and then relocking after a 5-second delay.

RGB LED: Provides visual status updates: blue for standby/input, green for success (unlocked), and red for errors (wrong PIN/invalid locker).

Buzzer: Gives audible feedback with different sounds for key presses, successful unlocking, and errors.

Special Feature: The code includes a custom manualTone function to generate sound. This is a clever solution to prevent conflicts between the buzzer and the IR remote library, which often compete for the same internal timers on the Arduino.

In short, it's a self-contained electronic lock project that effectively manages user interaction, feedback, and the physical locking mechanism.
