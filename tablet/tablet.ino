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

#define MENU_ITEM_CONTAINER_HEIGHT 40
#define HEADER_SIZE (BOXSIZE + (MARGIN * 2))

#define ITEMS_PER_PAGE 4

struct MenuItem {
  int id;
  char title[12];
  int price;
};

MenuItem menu_list[20];
char num_items = 0;
char current_page = 1;
int total_cost = 0;

int cart_menu_ids[20];
int total_cart_items = 0;

char connected_to_server = 0;

#define PREVIOUS_BOX_X MARGIN
#define PREVIOUS_BOX_Y MARGIN

#define CONFIRM_BOX_X PREVIOUS_BOX_X + BOXSIZE + MARGIN
#define CONFIRM_BOX_Y MARGIN

#define NEXT_BOX_X (tft.width() - BOXSIZE - MARGIN)
#define NEXT_BOX_Y MARGIN

IPAddress ip(192, 168, 0, 117);
EthernetClient client;

char server[] = "104.131.189.231";

void draw_page() {
  int i = 0;
  int frame_start = (MARGIN * 2) + BOXSIZE + MARGIN;
  int frame_end = tft.height() - MARGIN;
  int y_pos = 0;
  MenuItem current_item;

  tft.fillScreen(WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);

  // Page back
  tft.fillRect(PREVIOUS_BOX_X, PREVIOUS_BOX_Y, BOXSIZE, BOXSIZE, RED);
  tft.setCursor(PREVIOUS_BOX_X + MARGIN, PREVIOUS_BOX_Y + MARGIN);
  tft.print('<');

  // Confirm
  tft.fillRect(CONFIRM_BOX_X, CONFIRM_BOX_Y, tft.width() - 2 * BOXSIZE - 4 * MARGIN, BOXSIZE, GREEN);
  tft.setCursor(CONFIRM_BOX_X + MARGIN, CONFIRM_BOX_Y + MARGIN);
  tft.print("Confirm");

  // Page forward
  tft.fillRect(NEXT_BOX_X, NEXT_BOX_Y, BOXSIZE, BOXSIZE, RED);
  tft.setCursor(NEXT_BOX_X + MARGIN, NEXT_BOX_Y + MARGIN);
  tft.print('>');

  for(i = 0; i < ITEMS_PER_PAGE; i++) {
    current_item = menu_list[i + (ITEMS_PER_PAGE * (current_page - 1))];
    if(current_item.id != 0) {
      y_pos = i * (MENU_ITEM_CONTAINER_HEIGHT + MARGIN) + HEADER_SIZE;
      tft.fillRect(MARGIN, y_pos, tft.width() - MARGIN * 2, MENU_ITEM_CONTAINER_HEIGHT, BLACK);
      tft.setCursor(MARGIN * 2, y_pos + 10);
      tft.print(current_item.title);
      tft.setCursor(tft.width() - MARGIN - 40, y_pos + 10);
      tft.print("$");
      tft.print(current_item.price);
    }
  }

  tft.setTextColor(BLACK);

  // Page number
  tft.setCursor(MARGIN, tft.height() - MARGIN - 15);
  tft.print("Page ");
  tft.print((int)current_page);

  // Live total price of order
  tft.setCursor(tft.width() - MARGIN - 100, tft.height() - MARGIN - 15);
  tft.print("Cost:");
  tft.setCursor(tft.width() - MARGIN - 40, tft.height() - MARGIN - 15);
  tft.print("$");
  tft.print(total_cost);
}

void confirm_order() {
  tft.fillScreen(GREEN);
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.setCursor(tft.width() / 2 - 45, tft.height() / 2 - 11);
  tft.print("Order");
  tft.setCursor(tft.width() / 2 - 85, tft.height() / 2 + 11);
  tft.print("confirmed!");
  delay(3000);
  total_cart_items = 0;
  total_cost = 0;
  draw_page();
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
    connected_to_server = 1;
    Serial.println("connected");
    // Make a HTTP request:
    client.println("GET /kitchen/menu/ HTTP/1.1");
    client.println("Host: 104.131.189.231");
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
 
  pinMode(13, OUTPUT);
}

void get_server_content()
{
  char c = 0, in_obj = 0, in_body = 0;
  String json_raw = "";
  MenuItem new_item;
  const char * title_ptr = 0x0;
  
  while(client.available()) {
    c = client.read();

    if(in_body) {
      if((!in_obj && c == ',') || c == ']') {
        Serial.println(json_raw);
        
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(json_raw.c_str());
      
        if(!root.success()) {
          Serial.println("JSONparseFailed");
        } else {
          title_ptr = root["title"];
          memcpy(&new_item.title, title_ptr, strlen(title_ptr) + 1);
          new_item.price = root["price"];
          new_item.id = root["id"];
          memcpy(&menu_list[num_items], &new_item, sizeof(MenuItem));
          num_items++;
        }
        
        json_raw.remove(0); // Reset string.
      } else if(c != '[') {
        if(c == '{') in_obj = 1;
        else if(c == '}') in_obj = 0;
        
        json_raw.concat(c);
      }
    } else if(c == '\n' || c == '\r') {
      json_raw.concat(c);

      if(json_raw == "\r\n\r\n") {
        in_body = 1;
        json_raw.remove(0);
      }
    } else {
      json_raw.remove(0);
    }
    
    if (!client.connected()) {
      Serial.println("Disconnected!");
      client.stop();
      connected_to_server = 0;
    }
  }

  Serial.println("Drawing page.");
  draw_page();
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

  if(connected_to_server) {
    get_server_content();
  }

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    /*
    Serial.print("X = "); Serial.print(p.x);
    Serial.print("\tY = "); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);
    */
    
    // scale from 0->1023 to tft.width
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));

    // Confirm button pressed, post order to server and reset order
    if (p.x >= CONFIRM_BOX_X && p.x <= (NEXT_BOX_X - MARGIN) && p.y >= CONFIRM_BOX_Y && p.y <= (CONFIRM_BOX_Y + BOXSIZE)) {
      // Post order to server somehow
      if (total_cart_items > 0) { // and if order sent:
        confirm_order();
      }
    }

    // Menu item pressed, add to cart
    if(p.y >= HEADER_SIZE && p.x >= MARGIN && p.y <= (tft.height() - MARGIN))
    {
      int box_index_pressed = (p.y - HEADER_SIZE) / (MENU_ITEM_CONTAINER_HEIGHT + MARGIN);
      MenuItem to_add;
      
      if(box_index_pressed < ITEMS_PER_PAGE)
      {
        to_add = menu_list[box_index_pressed + (ITEMS_PER_PAGE * (current_page - 1))];
        total_cost += to_add.price;
        cart_menu_ids[total_cart_items] = to_add.id;
        total_cart_items++;
      }

      draw_page();
    }
    
    if ((p.x >= PREVIOUS_BOX_X && p.x <= (PREVIOUS_BOX_X + BOXSIZE)) && (p.y >= PREVIOUS_BOX_Y && p.y <= (PREVIOUS_BOX_Y + BOXSIZE))) {
      // "Previous Page" button pressed.
      if(current_page > 1) {
        current_page--;
      }

      draw_page();
    } else if ((p.x >= NEXT_BOX_X && p.x <= (NEXT_BOX_X + BOXSIZE)) && (p.y >= NEXT_BOX_Y && p.y <= (NEXT_BOX_Y + BOXSIZE))) {
      // "Next Page" button pressed.
      if((num_items % ITEMS_PER_PAGE == 0) ? (current_page < (num_items / ITEMS_PER_PAGE)) : (current_page < (num_items / ITEMS_PER_PAGE) + 1)) {
        current_page++;
      }

      draw_page();
    }
  }
}
