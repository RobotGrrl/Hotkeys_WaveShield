/*

 An example of Hotkeys for Arduino and the Wave Shield!
 
 Wave Shield: http://adafru.it/94
 WaveHC library: http://code.google.com/p/wavehc/downloads/list
 Hotkeys for Arduino: http://appsforarduino.com/hotkeys
 
 by RobotGrrl Feb 21, 2012
 http://robotgrrl.com/blog
 
 */


#include <WaveHC.h>
#include <WaveUtil.h>

#define DEBUG false

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader file;   // This object represent the WAV file for a pi digit or period
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

uint8_t dirLevel; // indent level for file/dir names    (for prettyprinting)
dir_t dirBuf;     // buffer for directory reads
int16_t rateval = 0;
int16_t lastrateval = 0; // for changing the rate

#define HYSTERESIS 3
#define error(msg) error_P(PSTR(msg)) // Define macro to put error messages in flash memory

void play(FatReader &dir); // Function definitions (we define them here, but the code is below)

/////////
// all of the other variables!

int numfiles = 0;
int loopcount = 0;
boolean in = true;
int led1 = 7;
int led2 = 6;
int hotkey = 0;
boolean singing = false;

//////////////////////////////////// SETUP
void setup() {

  Serial.begin(115200);

  /////////
  // starting up the wave shield!

  putstring_nl("\nHi!");  // say we woke up!

  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(FreeRam());

  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    error("Card init. failed!");  // Something went wrong, lets print out why
  }

  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);

  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {   // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                           // we found one, lets bail
  }
  if (part == 5) {                     // if we ended up not finding one  :(
    error("No valid FAT partition!");  // Something went wrong, lets print out why
  }

  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(), DEC);     // FAT16 or FAT32?

  // Try to open the root directory
  if (!root.openRoot(vol)) {
    error("Can't open root dir!");      // Something went wrong,
  }

  // Whew! We got past the tough parts.
  //putstring_nl("Files found (* = fragmented):");
  // Print out all of the files in all the directories.
  //root.ls(LS_R | LS_FLAG_FRAGMENTED);

  /////////
  // other init things!

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  root.rewind();
  count(root);

}

//////////////////////////////////// LOOP
void loop() {
  
  if(Serial.available() > 0) {
    char c = (char)Serial.read();
    hotkey = atoi(&c);
    //blinky(hotkey); // uncomment this if you want LED1 to blink the hotkey number!
    switch(hotkey) {
      case 1:
       action1();
       break; 
      case 2:
       action2();
       break;
      case 3:
       action3();
       break;
      case 4:
       action4();
       break;
      case 5:
       action5();
       break;
      case 6:
       singing = !singing;
       break;
    }
  }
  
  if(hotkey == 6 && singing) action6();
  
  breathy();
  delay(10);

}



/////////
// hotkey actions!

void action1() {

  int random1 = (int)random(0, numfiles);
  int random2 = (int)random(0, numfiles);
  while(random2 == random1) random2 = (int)random(0, numfiles); 

  Serial.print(random1);
  Serial.print("&");
  Serial.println(random2);

  playIndex(random1, false);
  playIndex(random2, false);
  playIndex(random1, false);

}

void action2() {

  int random1 = (int)random(0, numfiles);
  int random2 = (int)random(0, numfiles);
  while(random2 == random1) random2 = (int)random(0, numfiles); 

  Serial.print(random1);
  Serial.print("&");
  Serial.println(random2);

  rateval = 1023;
  playIndex(random1, true);
  
  rateval = 512;
  playIndex(random2, true);
  
  rateval = 200;
  playIndex(random1, true);

}

void action3() {
 
  rateval = 1023;
  int random1 = (int)random(0, numfiles);
  playIndex(random1, true);
  
  rateval = 200;
  playIndex(random1, true);
  
}

void action4() {

  rateval = 200;
  int random1 = (int)random(0, numfiles);
  playIndex(random1, true);
  
  rateval = 100;
  playIndex(random1, true);
  
}

void action5() {

  int random1 = (int)random(0, numfiles);

  for(int i=0; i<5; i++) {
    playIndex(random1, false);
  }

}

void action6() {

  int random1 = (int)random(0, numfiles);
  playIndex(random1, false);
  
}



/////////
// LED effects

void blinky(int hotkey) {
  for(int i=0; i<hotkey; i++) {
    digitalWrite(led1, HIGH);
    delay(50);
    digitalWrite(led1, LOW);
    delay(50);
  } 
}

void breathy() {
  
  analogWrite(led2, loopcount);
  
  if(in) {
    loopcount++; 
  } else {
    loopcount--; 
  }
  
  if(loopcount == 0) {
    in = true;
  } else if(loopcount == 255) {
    in = false;
  }
  
}



/////////
// wave methods

char filename[13];
void playIndex(int index, boolean rate) {

  // copy flash string for 'period' to filename
  strcpy_P(filename, PSTR("00.wav"));

  int tens = 0;
  int units = 0;
  char buff[2];

  if(index < 10) {
    filename[0] = '0';
    itoa(index, buff, 10);
    filename[1] = buff[0];
  } 
  else {
    tens = (int)(index/10);
    units = index-(tens*10);
    itoa(tens, buff, 10);
    filename[0] = buff[0];
    itoa(units, buff, 10);
    filename[1] = buff[0];
  }

  Serial.print("Playing: ");
  Serial.print(filename[0]);
  Serial.print(filename[1]);
  Serial.println(".wav");

  if(!rate) {
    playcomplete(filename);
  } else {
    playcomplete_rate(filename); 
  }
}


void count(FatReader &dir) {

  numfiles = 0;

  FatReader file;
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time

    // Skip it if not a .WAV file
    if (strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) continue;

    if (!file.open(vol, dirBuf)) {        // open the file in the directory
      error("file.open failed");          // something went wrong
    }

    if (!file.isDir()) {
      if (!wave.create(file)) {            // Figure out, is it a WAV proper?
        putstring(" Not a valid WAV");     // ok skip it
      } 
      else {
        numfiles++;  
        sdErrorCheck();                    // everything OK?
        // if (wave.errors)Serial.println(wave.errors);     // wave decoding errors
      }
    }
  }

  Serial.print(numfiles);
  Serial.println("files");

}



/////////////////////////////////// HELPERS
/*
 * print error message and halt
 */
void error_P(const char *str) {
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  while(1);
}

/*
 * print error message and halt if SD I/O error, great for debugging!
 */
void sdErrorCheck(void) {
  if (!card.errorCode()) return;
  PgmPrint("\r\nSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  PgmPrint(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

/*
 * Play a file and wait for it to complete
 */
void playcomplete(char *name) {
  playfile(name);
  while (wave.isplaying) {
    breathy();
    delay(10);
  }

  // see if an error occurred while playing
  sdErrorCheck();
}


/*
 * play file with sample rate changes
 */
void playcomplete_rate(char *name) { //FatReader &file) {
  uint32_t newsamplerate;

  /*
  if (!wave.create(file)) {
    putstring_nl(" Not a valid WAV"); 
    return;
  }
  // ok time to play!
  wave.play();
  */
  
  playfile(name);

  while (wave.isplaying) {
    if(millis()%100 == 0) {
    putstring("rate = ");
    Serial.println(rateval, DEC); 
    putstring("tickspersam = ");
    Serial.print(wave.dwSamplesPerSec, DEC);
    putstring(" -> ");
    newsamplerate = wave.dwSamplesPerSec;
    newsamplerate *= rateval;
    newsamplerate /= 512;   // we want to 'split' between sped up and slowed down.
    if (newsamplerate > 24000) {
      newsamplerate = 24000;  
    }
    if (newsamplerate < 1000) {
      newsamplerate = 1000;  
    }        
    wave.setSampleRate(newsamplerate);

    Serial.println(newsamplerate, DEC);
    lastrateval = rateval;
    }
    breathy();
    delay(10);
  }
  sdErrorCheck();
}

/*
 * Open and start playing a WAV file
 */
void playfile(char *name) {
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  if (!file.open(root, name)) {
    PgmPrint("Couldn't open file ");
    Serial.print(name); 
    return;
  }
  if (!wave.create(file)) {
    PgmPrintln("Not a valid WAV");
    return;
  }
  // ok time to play!
  wave.play();
}

