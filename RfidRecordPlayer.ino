#include <DFRobot_DF1201S.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
const int rolePerMinute = 17;         // Adjustable range of 28BYJ-48 stepper is 0~17 rpm

unsigned long lastStepTime = 0;
int stepInterval;        // microseconds between steps
int stepCount = 0;
int stepDir = 5;

Stepper myStepper(stepsPerRevolution, 5, 7, 6, 8);

SoftwareSerial DF1201SSerial(2, 3);  //RX  TX

#define RST_PIN   9     // Configurable, see typical pin layout above
#define SS_PIN    10    // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

static bool cardActive = false;

DFRobot_DF1201S DF1201S;

static const char* currentSong;

const int hallDigitalPin = 4; //pin for sensor reading
int hallDigital;

struct Track {
  byte uid[7];
  const char* fileName;
};

Track tracks[] = {
  {{0x04, 0x30, 0x73, 0x12, 0x4B, 0x11, 0x91}, "/001_SOMT.mp3"},  // SOMT
  {{0x04, 0x54, 0x70, 0x92, 0x2F, 0x15, 0x91}, "/002_DTMF.mp3"},  // DTMF
  {{0x04, 0x54, 0x6E, 0x92, 0x2F, 0x15, 0x91}, "/003_BP.mp3"},  // BP
  {{0x04, 0x30, 0x72, 0x12, 0x4B, 0x11, 0x91}, "/004_FTS.mp3"},  // FTS
  {{0x04, 0x30, 0x71, 0x12, 0x4B, 0x11, 0x91}, "/005_1979.mp3"},  // NineteenSeventyNine
  {{0x04, 0x54, 0x6D, 0x92, 0x2F, 0x15, 0x91}, "/006_Car.mp3"},  // Carino
  {{0x04, 0x54, 0x71, 0x92, 0x2F, 0x15, 0x91}, "/007.mp3"},  // FiveAM
};

void setup() {
  stepInterval = 60L * 1000000L / stepsPerRevolution / rolePerMinute;

  Serial.begin(115200); //serial register
  DF1201SSerial.begin(115200); //serial register
  Serial.println("Beginning");
  while(!DF1201S.begin(DF1201SSerial)){
    Serial.println("Init failed, please check the wire connection!");
    delay(1000);
  }

  //Hall Effect Setup
  pinMode(hallDigitalPin, INPUT);

  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card (RFID reader)

  DF1201S.setVol(/*VOL = */12);

  /*Enter music mode*/
  DF1201S.switchFunction(DF1201S.MUSIC);
  /*Set playback mode to "repeat all"*/
  DF1201S.setPlayMode(DF1201S.SINGLE);
  Serial.println("Completed");
}

void loop() {
  Serial.println("Starting Loop");
  hallDigital = !digitalRead(hallDigitalPin);
  setSong();

  if (hallDigital == 1) {
    DF1201S.start();
    // Step one step at a time, non-blocking
    unsigned long now = micros();
    if (now - lastStepTime >= stepInterval) {
      lastStepTime = now;
      myStepper.step(stepDir);  // only 1 step per call
    }
  } else {
    DF1201S.pause();
  }
}

void setSong() {
  Serial.println("Setting Song");
  // PICC_IsNewCardPresent() checks if a card is being held near the reader
  // PICC_ReadCardSerial() tries to read the card's data
  // If either fails (no card present, or can't read it), we exit early and try again next loop
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    cardActive = false;
    return;
  }
  // sizeof(tracks) gives total bytes of the whole array.
  // sizeof(tracks[0]) gives bytes of one Track element.
  // Dividing them gives the element count — equivalent to tracks.length in Java.
  int numTracks = sizeof(tracks) / sizeof(tracks[0]);

  if(cardActive) return;

  cardActive = true;

  // memcmp (memory compare) compares two blocks of raw bytes.
  // args: (pointer to block A, pointer to block B, number of bytes to compare)
  // returns 0 if they are identical, non-zero if they differ —
  // similar to Arrays.equals() in Java or just == on a list in Python.
  // We access the struct's fields with dot notation, same as Java/Python.
  for (int i = 0; i < numTracks; i++) {
    if (memcmp(mfrc522.uid.uidByte, tracks[i].uid, 7) == 0 && 
    strcmp(currentSong, tracks[i].fileName) != 0) {
      currentSong = tracks[i].fileName;
      DF1201S.playSpecFile(tracks[i].fileName); // Access fileNum with dot notation
      DF1201S.pause();
      Serial.print("Playing: ");
      Serial.println(tracks[i].fileName);
      break;
    }
  }
  mfrc522.PICC_HaltA();
}

