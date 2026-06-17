//BOX mode

#include <Arduino.h>

#define BOX_SIZE 3
#define GRID_SIZE 10

#define PRECISION 8
#define TAU 50

int SCOL[5] = {0, 1, 2, 3, 4};
int SROW[5] = {5, 6, 7, 8, 9};

int CS = 10;
int WR = 11;
int EN = 12;
int LED = 13;

int OUTCOL = A0;
int OUTROW = A1;

int colfix[32] = {
  16,17,18,19,20,21,22,23,
  24,25,26,27,28,29,30,31,
  0,1,2,3,4,5,6,7,
  8,9,10,11,12,13,14,15
};

int rowfix[32] = {
  0,1,2,3,4,5,6,7,
  8,9,10,11,12,13,14,15,
  31,30,29,28,27,26,25,24,
  23,22,21,20,19,18,17,16
};

int backgroundFrame[GRID_SIZE][GRID_SIZE];
int currentFrame[GRID_SIZE][GRID_SIZE];
bool changedBoxes[GRID_SIZE][GRID_SIZE];

int timeCounter = 0;
int startTime = 0;

int lastcol = -1;
int lastrow = -1;

void pixelSelect(int col, int row);
void captureCenters(int frame[][GRID_SIZE]);
void sendBox(int boxRow, int boxCol);
void sendAllBoxes();
void updateBackground();



// per-column gain (fix uneven brightness)
float colGain[32] = {
  1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,
  1.05,1,1,1,1,1,1,1,
  1.08,1,1,1,1,1,1,1
};

// optional offset (your old "-3, -4" style fix)
int colOffset[32] = {
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  -3,0,0,0,0,0,0,0,
  -4,0,0,0,0,0,0,0
};

// apply calibration
int calibrate(int val, int colIndex)
{
  val = (int)(val * colGain[colIndex]);
  val += colOffset[colIndex];
  return val;
}

void setup()
{
  startTime = millis();
  delay(1000);
 

  for (int i = 0; i < 5; i++) {
    pinMode(SCOL[i], OUTPUT);
    digitalWrite(SCOL[i], LOW);
  }

  for (int i = 0; i < 5; i++) {
    pinMode(SROW[i], OUTPUT);
    digitalWrite(SROW[i], LOW);
  }

  pinMode(CS, OUTPUT);
  digitalWrite(CS, LOW);

  pinMode(WR, OUTPUT);
  digitalWrite(WR, LOW);

  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  pinMode(OUTCOL, OUTPUT);
  digitalWrite(OUTCOL, HIGH);

  pinMode(OUTROW, INPUT);

  Serial.begin(115200);
  

  captureCenters(backgroundFrame);
  delay(1000);
  Serial.println("Initialized Box");
  sendAllBoxes();

  
}

void loop()
{
  captureCenters(currentFrame);

  bool motionDetected = false;

  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {

      int diff = abs(currentFrame[r][c] - backgroundFrame[r][c]);


      if (diff > PRECISION || changedBoxes[r][c]) {

        changedBoxes[r][c] = true;
        sendBox(r, c);
        motionDetected = true;

        digitalWrite(LED, HIGH);
        timeCounter++;

        Serial.println(timeCounter);

        if (timeCounter >= TAU) {
          updateBackground();
          timeCounter = 0;
        }

      }

      if(diff <= PRECISION) {
        changedBoxes[r][c] = false;
      }
    }
  }

  if (!motionDetected) {
    digitalWrite(LED, LOW);
  
    if (millis() - startTime >= 30000) {
      timeCounter = 0;
      startTime = millis();
    }
  }

  delay(50);
}


void captureCenters(int frame[][GRID_SIZE])
{
  int boxRow = 0;

  for (int y = BOX_SIZE/2; y <= 32 - BOX_SIZE; y += BOX_SIZE, boxRow++) {

    int boxCol = 0;

    for (int x = BOX_SIZE/2; x <= 32 - BOX_SIZE; x += BOX_SIZE, boxCol++) {

      pixelSelect(x, y);

      int raw = analogRead(OUTROW);

      int globalCol = boxCol * BOX_SIZE + BOX_SIZE/2;

      frame[boxRow][boxCol] = calibrate(raw, globalCol);
    }
  }
}


void sendBox(int boxRow, int boxCol)
{
  int centerX = boxCol * BOX_SIZE + 1;
  int centerY = boxRow * BOX_SIZE + 1;

  int startX = centerX - 1;
  int startY = centerY - 1;

  Serial.println("BOX");
  Serial.print(boxRow);
  Serial.print(" ");
  Serial.println(boxCol);

  for (int dy = 0; dy < BOX_SIZE; dy++) {

    for (int dx = 0; dx < BOX_SIZE; dx++) {

      int locX = startX + dx;
      int locY = startY + dy;

      pixelSelect(locX, locY);

      int raw = analogRead(OUTROW);

      raw = calibrate(raw, locX);

      Serial.print(raw);

      if (dx < BOX_SIZE - 1)
        Serial.print(" ");
    }

    Serial.println();
  }

  Serial.println("END");
}


void updateBackground()
{
  for (int r = 0; r < GRID_SIZE; r++) {
    for (int c = 0; c < GRID_SIZE; c++) {

      if (changedBoxes[r][c]) {
        backgroundFrame[r][c] = currentFrame[r][c];
      }
    }
    delayMicroseconds(1000);
  }
}

// ======================================================
void sendAllBoxes()
{
  for (int boxRow = 0; boxRow < GRID_SIZE; boxRow++) {
    for (int boxCol = 0; boxCol < GRID_SIZE; boxCol++) {
      sendBox(boxRow, boxCol);
    }
  }
}

// ======================================================
void pixelSelect(int col, int row)
{
  int realrow = rowfix[row];
  int realcol = colfix[col];

  digitalWrite(WR, LOW);
  delayMicroseconds(5000);

  if (lastcol != realcol) {
    for (int x = 0; x < 5; x++) {
      byte state = bitRead(realcol, 4 - x);
      digitalWrite(SCOL[4 - x], state);
    }
    lastcol = realcol;
    delayMicroseconds(5000);
  }

  if (lastrow != realrow) {
    for (int y = 0; y < 5; y++) {
      byte state1 = bitRead(realrow, 4 - y);
      digitalWrite(SROW[4 - y], state1);
    }
    lastrow = realrow;
    delayMicroseconds(1000);
  }

  digitalWrite(WR, HIGH);
}