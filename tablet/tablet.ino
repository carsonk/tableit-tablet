#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
// optional
#define LCD_RESET A4

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define BOXSIZE 40
#define MARGIN 10
#define PENRADIUS 3

#define MENU_ITEM_CONTAINER_HEIGHT 50
#define HEADER_SIZE (BOXSIZE + (MARGIN * 2))

int oldcolor, currentcolor;

struct MenuItem {
  char title[50];
  char price;
};

void draw_page() {
  int i = 0;
  int frame_start = (MARGIN * 2) + BOXSIZE + MARGIN;
  int frame_end = tft.height() - MARGIN;
  int y_pos = 0;
  
  tft.reset();

  tft.begin(0x9341);
  tft.setRotation(2);

  tft.fillScreen(WHITE);
  
  tft.fillRect(MARGIN, MARGIN, BOXSIZE, BOXSIZE, RED); // Page back.
  tft.fillRect(tft.width() - BOXSIZE - MARGIN, MARGIN, BOXSIZE, BOXSIZE, GREEN); // Page forward.

  tft.setTextSize(2);

  for(i = 0; i < 4; i++) {
    y_pos = i * (MENU_ITEM_CONTAINER_HEIGHT + MARGIN) + HEADER_SIZE;
    tft.fillRect(MARGIN, y_pos, tft.width() - MARGIN * 2, MENU_ITEM_CONTAINER_HEIGHT, BLACK);
    tft.setCursor(MARGIN * 2, y_pos + 10);
    tft.print("Potato");
    tft.setCursor(tft.width() - MARGIN - 40, y_pos + 10);
    tft.print("$4");
  }
}

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("MENU start."));
  
  Serial.println("Bloop.");

  draw_page();
 
  pinMode(13, OUTPUT);

  Serial.print("height: ");
  Serial.print(tft.height());
  Serial.print(" width: ");
  Serial.print(tft.width());
}

#define MINPRESSURE 10
#define MAXPRESSURE 1000

void loop()
{
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    /*
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);
    */
    
    // scale from 0->1023 to tft.width
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));

    currentcolor = 0xEEEE;
    
    if ((p.y-PENRADIUS) && ((p.y+PENRADIUS) < tft.height())) {
      tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
    }
  }
}
