#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"
#include <Wire.h>


SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

bool low, medium, high, obstacleHigh, obstacleLow, obstacleWall, obstacleDropOff, ft1, ft2, ft3, ft4, ft5, ft6, ft7, ft8, ft9, left, right, both, canreceiveevents;
int randNumber;

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

#define DEBOUNCE 5  // button debouncer

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
byte buttons[] = {14, 15, 16, 17, 18, 19};
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'pressed' (the current state
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

// this handy function will return the number of bytes currently free in RAM, great for debugging!   
int freeRam(void)
{
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

void setup() {
  canreceiveevents = false;
  byte i;
  randomSeed(analogRead(0));
  
  // set up serial port
  Serial.begin(9600);
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!
  
  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck();
    while(1);                            // then 'halt' - do nothing!
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
 
// Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();      // Something went wrong, lets print out why
    while(1);                            // then 'halt' - do nothing!
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(),DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while(1);                             // then 'halt' - do nothing!
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Ready!");
  canreceiveevents = true;
}

//    Every time the loop is executed, it will check what values
//      have been set to true and then plays the corresponding
//      audio files.


void loop() {
  
  //  "canreceiveevents" is set to false here so that while the
  //    main portion of loop is execuitng, it will not be
  //    interrupted if a new signal is received from the
  //    control board.
  canreceiveevents = false;

  //  These three sets of "if-else" statements are used to
  //    check what boolean values have been set to true,
  //    and then plays the appropriate audio file.
  
  if(left) {
    playcomplete("LEFT.wav");
    left = false;
  }
  else if (right) {
    playcomplete("RIGHT.wav");
    right = false;
  }
  else if (both) {
    playcomplete("BOTH.wav");
    both = false;
  }
  
  if(obstacleHigh) {
    playcomplete("HIGH.wav");
    obstacleHigh = false;
  }
  else if(obstacleLow) {
    playcomplete("LOW.wav");
    obstacleLow = false;
  }
  else if(obstacleWall) {
    playcomplete("WALL.wav");
    obstacleWall = false;
  }
  else if(obstacleDropOff) {
    playcomplete("DROPOFF.wav");
    obstacleDropOff = false;
  }

  if(ft1) {
     playcomplete("1FT.wav");
     ft1 = false;
  }
  else if (ft2) {
     playcomplete("2FT.wav");
     ft2 = false;
  }
  else if (ft3) {
     playcomplete("3FT.wav");
     ft3 = false;
  }
  else if (ft4) {
     playcomplete("4FT.wav");
     ft4 = false;
  }
  else if (ft5) {
     playcomplete("5FT.wav");
     ft5 = false;
  }
  else if (ft6) {
     playcomplete("6FT.wav");
     ft6 = false;
  }
  else if (ft7) {
     playcomplete("7FT.wav");
     ft7 = false;
  }
  else if (ft8) {
     playcomplete("8FT.wav");
     ft8 = false;
  }
  else if (ft9) {
     playcomplete("FT9.wav");
     ft9 = false;
  }

  //  "canreceiveevents" is now set to true, as this is the
  //    window where signals from the control board are
  //    accepted.
  
  canreceiveevents = true;
  Serial.println("END OF LOOP");
  delay(100);
}

//    receiveEvent is executed whenever a signal from the
//      control board is sent to the audio board.  This
//      means that it will interrupt the main loop.
//      By only allowing the boolean values to be updated
//      if "canreceiveevents" is true, this stops these
//      values from being changed in the middle of the
//      main loop. 

void receiveEvent(int inputLength) {

  //  Reads in three characters from the control board
  //    which will be used to determine what audio cues
  //    will be played in the main loop.
  
  char type = Wire.read(); // receive byte as a character
  char distance = Wire.read();
  char direc = Wire.read();

  //  Only allows boolean values to be changed if the
  //    program is not in the middle of the main loop.
  
  if(canreceiveevents) {
  Serial.println("    RECEIVING EVENT");
    switch(type) {
      case('H'):
        obstacleHigh = true;
       break;
    case('L'):
        obstacleLow = true;
       break;
    case('B'):
        obstacleWall = true;
       break;
    case('D'):
        obstacleDropOff = true;
       break;
    default:
        obstacleHigh = false;
        obstacleLow = false;
        obstacleWall = false;
        obstacleDropOff = false;
      break;
    }
    switch(distance)  {
      case('1'):
        ft1 = true;
        break;
    case('2'):
        ft2 = true;
        break;
    case('3'):
        ft3 = true;
        break;
    case('4'):
        ft4 = true;
        break;
    case('5'):
        ft5 = true;
        break;
    case('6'):
        ft6 = true;
        break;
    case('7'):
        ft7 = true;
        break;
    case('8'):
        ft8 = true;
        break;
    case('9'):
        ft9 = true;
        break;
    default:
        ft1 = false;
        ft2 = false;
        ft3 = false;
        ft4 = false;
        ft5 = false;
        ft6 = false;
        ft7 = false;
        ft8 = false;
        ft9 = false;
        break;
    }

    switch(direc) {
      case('L'):
          left = true;
          break;
      case('R'):
          right = true;
          break;
      case('B'):
          both = true;
          break;
      default:
          left = false;
          right = false;
          both = false;
          break;
    }
  }
  else {
    Serial.println("    CANT RECEIVE EVENT");
  }
}

//  When called with a file name, plays the audio
//    cue without being interrupted.

void playcomplete(char *name) {
  Serial.println(name);
  playfile(name);
  //    WHILE WAV IS PLAYING DO NOTHING
  while (wave.isplaying) {
  }
}

void playfile(char *name) {
  //    CHECK FOR FILE
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name);
    Serial.println(""); 
    return;
  }
  //  VALIDATE FILE IS OF TYPE WAV
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); 
    return;
  }
  //    BEGIN PLAYBACK
  wave.play();
}
