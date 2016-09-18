#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

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

#define ITEMS_PER_PAGE 4

int oldcolor, currentcolor;

struct MenuItem {
  char title[10];
  int price;
};

MenuItem menu_list[20];
char current_page = 2;

#define PREVIOUS_BOX_X MARGIN
#define PREVIOUS_BOX_Y MARGIN

#define NEXT_BOX_X (tft.width() - BOXSIZE - MARGIN)
#define NEXT_BOX_Y MARGIN

IPAddress ip(192, 168, 0, 177);
EthernetClient client;
StaticJsonBuffer<200> jsonBuffer;

char server[] = "www.google.com";

void draw_page() {
  int i = 0;
  int frame_start = (MARGIN * 2) + BOXSIZE + MARGIN;
  int frame_end = tft.height() - MARGIN;
  int y_pos = 0;
  MenuItem current_item;

  tft.fillScreen(WHITE);
  
  tft.fillRect(PREVIOUS_BOX_X, PREVIOUS_BOX_Y, BOXSIZE, BOXSIZE, RED); // Page back.
  tft.fillRect(NEXT_BOX_X, NEXT_BOX_Y, BOXSIZE, BOXSIZE, GREEN); // Page forward.

  tft.setTextSize(2);

  for(i = 0; i < ITEMS_PER_PAGE; i++) {
    current_item = menu_list[i + (ITEMS_PER_PAGE * (current_page - 1))];
    y_pos = i * (MENU_ITEM_CONTAINER_HEIGHT + MARGIN) + HEADER_SIZE;
    tft.fillRect(MARGIN, y_pos, tft.width() - MARGIN * 2, MENU_ITEM_CONTAINER_HEIGHT, BLACK);
    tft.setCursor(MARGIN * 2, y_pos + 10);
    tft.print(current_item.title);
    tft.setCursor(tft.width() - MARGIN - 40, y_pos + 10);
    tft.print("$");
    tft.print(current_item.price);
  }
}

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("MENU start."));
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

  tft.reset();

  tft.begin(0x9341);
  tft.setRotation(2);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  delay(1000);
  Serial.println("connecting...");

  if (client.connect(server, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.println("GET /search?q=arduino HTTP/1.1");
    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  
  while (client.available()) {
    char c = client.read();
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    client.stop();
  }
  
  int i = 0;
  MenuItem item;

  char title[10] = "Potato";

  for(i = 0; i < 20; i++) {
    item.title[0] = 0x0;
    memcpy(&item.title, (void *)title, String("Potato").length() + 1);
    item.price = 1 + i;
    memcpy(&menu_list[i], &item, sizeof(MenuItem));
  }

  draw_page();
 
  pinMode(13, OUTPUT);
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

    if ((p.x >= PREVIOUS_BOX_X && p.x <= (PREVIOUS_BOX_X + BOXSIZE)) && (p.y >= PREVIOUS_BOX_Y && p.y <= (PREVIOUS_BOX_Y + BOXSIZE))) {
      if(current_page > 1) {
        current_page--;
      }

      draw_page();
    }

    if ((p.x >= NEXT_BOX_X && p.x <= (NEXT_BOX_X + BOXSIZE)) && (p.y >= NEXT_BOX_Y && p.y <= (NEXT_BOX_Y + BOXSIZE))) {
      if(current_page < 5) {
        current_page++;
      }

      draw_page();
    }
  }
}
