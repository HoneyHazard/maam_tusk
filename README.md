How to get those tusks glowing with new code: 

Step 1. Install FastLED. There is a FastLED directory in the root of the repository/zip file. Copy it to your Arduino's user libs folder. 

- On Linux it is /home/[username]/Arduino/libraries
- On OSX it is /Users/[username]/Documents/Arduino/libraries
- On Windows it is ...Documents\Arduino\libraries

Step 2: Configure the basics for right behavior

- Open maam_tusk_2017/maam_tusk_2017.ino in Arduino IDE
- Make sure "#define TESTING" is commented out at the top of the file
- Under "// MaaM Configuration" make sure correct clock and data pins are set
- You may need to comment out "NEEDS_GREEN_BLUE_GRADIENT_SWAP" at the top of the file as well, but only if your color palette looks wrong. If you are seeing blue -> purple -> pink -> white -> cyan -> blue, don't mess with it

Step 3: Knock on wood, compile and upload

Step 4: Tweak to achieve the behavior

- Change "#define FRAMES_PER_SECOND  120" on line 26 to change how fast everything flows
- Change "GRAD_LENGTH" on line 20 to have longer vs shorter gradients (and, conserquently, how dense they get pacled)

Step 5: Get hypnotized by pretty lights! Throw away all lists and have fun!

