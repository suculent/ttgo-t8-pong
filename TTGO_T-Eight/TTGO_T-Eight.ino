// Simple example in memory of ATARI® PONG™
// Designed for TTGO T-Eight with Navigation Button and 1.3" oled display


/***
   Install the following libraries through Arduino Library Manager
   - Mini Grafx by Daniel Eichhorn
   - SSD display lkb by Daniel Eichhorn
 ***/
 
#include <Wire.h>
#include "SH1106Wire.h"
#include "OLEDDisplayUi.h"

// TTGO-Eight
#define SDA 21
#define SCL 22

// Initialize the OLED display using Wire library
SH1106Wire display(0x3c, SDA, SCL);
// SH1106Wire display(0x3c, D3, D5);

OLEDDisplayUi ui( &display );

bool in_game = false;
int currentFrame = 0;

float handicap = 0.4; // kid (dement is 0.2); 
// float handicap = 1.0; // configurable, how worse is the computer than dumb ideal

String buttonState; // may leak, just a quick hack

void setupGame();

int computer_score = 0;
int human_score = 0;

void cscoreOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {  
  if (!in_game) return;
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0, 0, "CPU " + String(computer_score));
}

void hscoreOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  if (!in_game) return;
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, "YOU " + String(human_score));
}

void drawFrameIntro(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  in_game = false;
  display->setFont(ArialMT_Plain_16);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 22 + y, "MONG");
}

void drawFrameGame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  in_game = true; // enables real-time rendering
}

void drawFrameWinner(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  in_game = false;
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 22 + y, "CONGRATULATIONS!");
}

void drawFrameLoser(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  in_game = false;
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 22 + y, "TRY AGAIN.");
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrameIntro, drawFrameGame, drawFrameWinner, drawFrameLoser };

// how many frames are there?
int frameCount = 4;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { cscoreOverlay, hscoreOverlay };
int overlaysCount = 2;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pinMode(37, INPUT);
  pinMode(38, INPUT);
  pinMode(39, INPUT); // LEFT

	// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);
  ui.disableAutoTransition();
  ui.disableAllIndicators();
	// Customize the active and inactive symbol
  // ui.setActiveSymbol(activeSymbol);
  // ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  // ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  // ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_DOWN);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();

  setupGame();

}

typedef enum {
  H_NONE = 0,
  H_LEFT = 1,
  H_RIGHT = 2,
  H_PRESS = 4
} human_action;

int screen_width = display.getWidth();
int screen_height = display.getHeight();

// human player coordinates
const uint8_t h_padding = 5;

// initial position is in the middle
uint8_t h_position = screen_height / 2; 

// computer player coordinates
const uint8_t c_padding = 5;

// initial position is in the middle
uint8_t c_position = screen_height / 2;

const uint8_t player_width = 3; // px
const uint8_t player_height = 10; // px

float humanSpeedFactor = 2.0; // UI adjustment (keep handicap in mind!)
float humanSpeedForce = 1.0; // should increment on use (1/log)

bool buttonPressed = false;

void humanMoveUp()  {
  if (h_position > 63 - player_height/2) return;
  h_position += (humanSpeedFactor * humanSpeedForce);
}

void humanMoveDown()  {
  if (h_position < 0 + player_height/2) return;
  h_position -= (humanSpeedFactor * humanSpeedForce);
}

bool evaluateControls() { 
       
    bool l = digitalRead(39); // LEFT
    bool b = digitalRead(38); // BUTTON
    bool r = digitalRead(37); // RIGHT 

#ifdef DEBUG
    Serial.print("39L: "); Serial.print(l); Serial.print(" 38B: "); Serial.print(b); Serial.print(" 37R: "); Serial.println(r);
#endif
    
    const char* format = "L: %i, B: %i, R: %i\n";
    char dstring[34] = {0};
    sprintf(dstring, format, l, b, r);  

    if (l && b && r) {
      buttonState = "";
    }      

    if (b == 0) {
      buttonState = "PRESS";      
      return true;
    }
  
    if (l == 0) {
      buttonState = "LEFT";
      humanMoveDown(); // or i don't know but this works, may need refactoring
    } else if (r == 0) {
      buttonState = "RIGHT";
      humanMoveUp(); // or i don't know but this works, may need refactoring
    }

    return false;
}


void renderHumanPlayer() {    
  display.fillRect(128 - h_padding, h_position - player_height/2, player_width, player_height);
}

void renderComputerPlayer() {  
  display.fillRect(0 + c_padding, c_position - player_height/2, player_width, player_height);  
}

uint8_t target_xpos = screen_width/2;
uint8_t target_ypos = screen_height/2;
uint8_t target_size = 2;

void ai() {
  int step = humanSpeedFactor * humanSpeedForce;  // handicap * (
  if (target_ypos > c_position) {    
    c_position += step;
  }
  if (target_ypos < c_position) {
    c_position -= step;
  }  
}

// primitive version, 45° reflections only... may get updated with float later
typedef struct {
  int x;
  int y;
} Vector;

Vector movement;

void setupGame() {
  movement.x = 1;
  movement.y = 1; // should be random from -1 to 1
}

bool vertically_inside(int max, int min, int what) {
  //Serial.print("what: "); Serial.print(what); Serial.print(" min: "); Serial.print(min); Serial.print(" max: "); Serial.println(max); 
  return (what > min && what < max) ? true : false;
}

// bounce if on boundary, does not update position, only movement struct
void bounce() {
  
  // human hit
  int hx = screen_width - target_size - h_padding;
  int hmax = h_position + player_height / 2;  
  int hmin = h_position - player_height / 2;

  if (target_xpos == hx) {
    if (vertically_inside(hmax, hmin, target_ypos)) {
      movement.x = -1;
      Serial.println("Human hit on left boundary.");
      return; // do not let it pass to lost cases
    } else {
      Serial.println("Human miss.");
    }
  }  

  // cpu hit
  int cx = c_padding + target_size;
  int cmax = c_position + player_height / 2;
  int cmin = c_position - player_height / 2;  

  if (target_xpos == cx) {
    if (vertically_inside(cmax, cmin, target_ypos)) {
      movement.x = 1;
      Serial.println("Computer hit on left boundary.");
      return;  // do not let it pass to lost cases
    } else {
      Serial.println("Computer miss.");
    }
  }

  // human lost
  int x_max = screen_width - target_size / 2;
  if (target_xpos == x_max) {
    movement.x = -1;
    computer_score++;
  }

  // cpu lost
  int x_min = target_size / 2;
  if (target_xpos == x_min) {
    movement.x = 1;
    human_score++;
  }

  // just bounce on the other axis...
  int y_max = screen_height - target_size / 2;
  if (target_ypos == y_max) {
    movement.y = -1;
  }
  
  int y_min = target_size / 2 + 10; // status bar included
  if (target_ypos == y_min) {
    movement.y = 1;
  } 
}

// uses prepared movement struct to update position
void moveTarget() {
  target_xpos += movement.x;
  target_ypos += movement.y;
}

void updateTargetPosition() {
  // todo: add collision with player...
  bounce(); // uses target xpos and pos
  moveTarget(); // uses movement additionaly
}

void renderTarget() {
  updateTargetPosition();
  display.fillCircle(target_xpos - target_size/2, target_ypos - target_size/2, target_size);
}

void resetScores() {  
  computer_score = 0;
  human_score = 0;
}
void loop() {

  ai();

  if (evaluateControls()) {
    
    // user pressed button for menu
    Serial.println("Pressed...");            
    if (currentFrame > frameCount) {      
      currentFrame = 0;      
    } else {
      currentFrame++;
    }    

    // FrameCallback frames[] = { drawFrameIntro, drawFrameGame, drawFrameWinner, drawFrameLoser };

    if (currentFrame == 0) {
      resetScores(); 
    } // drawFrameIntro

    if (currentFrame == 1) {
      // game...
    }

    if (currentFrame > 1) {
      if (computer_score < human_score) {
        currentFrame = 2; // winner        
      } else {
        // loser
        currentFrame = 3;
      }      
    }
    
    ui.transitionToFrame(currentFrame);
    
    delay(500); // debounce
    
  } else {
    
    if (in_game) {      
      renderTarget();
      renderHumanPlayer();
      renderComputerPlayer();      
      display.display();
    }
  }
  
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    // you wish
    delay(remainingTimeBudget);
  }
}
