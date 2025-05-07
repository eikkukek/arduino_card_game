#include <SPI.h>
#include <avr/pgmspace.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>

enum CardFrame : uint8_t {
  CardFrame_Back     = 0,
  CardFrame_Front    = 1,
  CardFrame_Identity = 2,
};

#define CardFrameRows 15
#define CardFrameColumns 11

const uint16_t PROGMEM card_frames[2][15] {
  {
    0b11111111111,
    0b10011011001,
    0b01100100110,
    0b10011011001,
    0b01100100110,
    0b10011011001,
    0b01100100110,
    0b10011011001,
    0b01100100110,
    0b10011011001,
    0b01100100110,
    0b10011011001,
    0b01100100110,
    0b10011011001,
    0b11111111111,
  },
  {
    0b11111111111,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b10000000001,
    0b11111111111,
  },
};

enum CardRank : uint8_t {
  CardRank_2        = 0,
  CardRank_3        = 1,
  CardRank_4        = 2,
  CardRank_5        = 3,
  CardRank_6        = 4,
  CardRank_7        = 5,
  CardRank_8        = 6,
  CardRank_9        = 7,
  CardRank_10       = 8,
  CardRank_Jack     = 9,
  CardRank_Queen    = 10,
  CardRank_King     = 11,
  CardRank_Ace      = 12,
  CardRank_Identity = 13,
};

#define CardNumRankRows 5
#define CardNumRankColumns 8

const uint8_t PROGMEM card_num_ranks[9][5] {
  {
    0b00111000,
    0b00001000,
    0b00111000,
    0b00100000,
    0b00111000,
  },
  {
    0b00111000,
    0b00001000,
    0b00111000,
    0b00001000,
    0b00111000,
  },
  {
    0b00011000,
    0b00101000,
    0b01111000,
    0b00001000,
    0b00001000,
  },
  {
    0b00111000,
    0b00100000,
    0b00111000,
    0b00001000,
    0b00111000,
  },
  {
    0b00111000,
    0b00100000,
    0b00111000,
    0b00101000,
    0b00111000,
  },
  {
    0b00111000,
    0b00001000,
    0b00001000,
    0b00001000,
    0b00001000,
  },
  {
    0b00111000,
    0b00101000,
    0b00111000,
    0b00101000,
    0b00111000,
  },
  {
    0b00111000,
    0b00101000,
    0b00111000,
    0b00001000,
    0b00111000,
  },
  {
    0b01101110,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101110,
  },
};

enum CardSuit : uint8_t {
  CardSuit_Spade     = 0,
  CardSuit_Heart     = 1,
  CardSuit_Diamond   = 2,
  CardSuit_Club      = 3,
  CardSuit_Identity  = 4,
};

#define CardSuitSmallRows 5
#define CardSuitSmallColumns 7

const uint8_t PROGMEM card_suits_small[4][5] {
  {
    0b0001000,
    0b0011100,
    0b0111110,
    0b0001000,
    0b0111110,
  },
  {
    0b0100010,
    0b1110111,
    0b0111110,
    0b0011100,
    0b0001000,
  },
  {
    0b0001000,
    0b0011100,
    0b0111110,
    0b0011100,
    0b0001000,
  },
  {
    0b0001100,
    0b0111110,
    0b0011100,
    0b0001000,
    0b0111110,
  },
};

const uint8_t CLK = 6; // digital
const uint8_t DIN = 5; // digital
const uint8_t DC = 4; // digital
const uint8_t CE = 3; // digital
const uint8_t RST = 2; // digital

Adafruit_PCD8544 display = Adafruit_PCD8544(CLK, DIN, DC, CE, RST);

struct Button {
private:

  uint16_t _lastPressTime = 0; // milliseconds
  uint16_t _delay; // milliseconds
  uint8_t _pin;

public:

  Button(uint8_t digitalPin, uint16_t delay) : _pin(digitalPin), _delay(delay) {
    pinMode(digitalPin, INPUT_PULLUP);
  }

  bool WasPressed() {
    if (millis() - _lastPressTime < _delay) {
      return false;
    }
    if (digitalRead(_pin) == LOW) {
      _lastPressTime = millis();
      return true;
    }
    return false;
  }

};

float Clamp(float v, float x, float y) {
  v = v < x ? x : v;
  return v > y ? y : v;
}

template <typename T>
struct Vec  {

  T x, y;

  Vec Rotated(float r) {
    float c = cos(r);
    float s = sin(r);
    Vec res {
      x * c - y * s,
      x * s + y * c,
    };
    return res;
  }

  static Vec Lerp(Vec a, Vec b, float t) {
    return { a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t };
  }
};

void DrawSprite16(uint16_t left, uint16_t top, uint16_t* sprite PROGMEM, uint8_t rows, uint8_t columns) {
  for (uint8_t i = 0; i < rows; i++) {
    uint16_t bits = pgm_read_word(sprite + i);
    uint8_t x = left;
    for (int16_t j = columns - 1; j >=0; j--) {
      if (bits & (1 << j)) {
        display.drawPixel(x, top, BLACK);
      }
      ++x;
    }
    ++top;
  }
}

void DrawSprite8(uint16_t left, uint16_t top, uint8_t* sprite PROGMEM, uint8_t rows, uint8_t columns) {
  for (uint8_t i = 0; i < rows; i++) {
    uint8_t bits = pgm_read_byte(sprite + i);
    uint8_t x = left;
    for (int16_t j = columns - 1; j >=0; j--) {
      if (bits & (1 << j)) {
        display.drawPixel(x, top, BLACK);
      }
      ++x;
    }
    ++top;
  }
}


struct Card {

  Vec<float> position;
  CardRank rank;
  CardSuit suit;
  CardFrame frame;

  inline void Draw() const {
    uint16_t left = ceil(position.x);
    uint16_t top = ceil((float)display.height() - position.y);
    DrawSprite16(left, top, card_frames[frame], CardFrameRows, CardFrameColumns);
    if (frame == CardFrame_Back) {
      return;
    }
    if (rank < CardRank_Jack) {
      DrawSprite8(left + 1 + 1 * (rank < CardRank_10), top + 2, card_num_ranks[rank], CardNumRankRows, CardNumRankColumns);
    }
    DrawSprite8(left + 2, top + 8, card_suits_small[suit], CardSuitSmallRows, CardSuitSmallColumns);
  }
};

struct CardSet {
private:

  Card* _cards = nullptr;
  Vec<int> _center;
  uint16_t _count = 0;
  int _selectedCard = -1;

public:

  CardSet(Vec<int> center, uint16_t count) : _center(center) {
    if (count) {
      uint32_t allocSize = count * sizeof(Card);
      _cards = (Card*)malloc(allocSize);
      if (_cards) {
        memset(_cards, 0, allocSize);
        _count = count;
        for (uint32_t i = 0; i < count; i++) {
          _cards[i].position = { _center.x, center.y };
        }
      }
    }
  }

  ~CardSet() {
    free(_cards);
  }
  
  void Draw() const {
    for (uint32_t i = 0; i < _count; i++) {
      _cards[i].Draw();
    }
    display.display();
  }

  uint16_t Count() const {
    return _count;
  }

  void SetCardProperties(uint16_t index, CardRank rank, CardSuit suit, CardFrame frame) {
    if (index >= _count) {
      return;
    }
    assert(rank <= CardRank_Identity);
    assert(suit <= CardSuit_Identity);
    assert(frame <= CardFrame_Identity);
    Card& card = _cards[index];
    if (rank != CardRank_Identity) {
      card.rank = rank;
    }
    if (suit != CardSuit_Identity) {
      card.suit = suit;
    }
    if (frame != CardFrame_Identity) {
      card.frame = frame;
    }
  }

  void SetActive() {
    _selectedCard = 0;
  }

  void SetInactive() {
    _selectedCard = -1;
  }

  void GoRight() {
    if (_selectedCard != -1) {
      _selectedCard = ((_selectedCard + 1) % _count);
    }
  }

  void GoLeft() {
    if (_selectedCard != -1) {
      _selectedCard = _selectedCard ? ((_selectedCard - 1) % _count) : _count - 1;
    }
  }

  void UpdatePositions() {
    Vec<float> p = { _center.x - (int)_count * (CardFrameColumns >> 1), _center.y + (CardFrameRows >> 1) };
    for (uint32_t i = 0; i < _count; i++) {
      Card& card = _cards[i];
      Vec<float> tp = p;
      if (_selectedCard == i) {
        tp.y += 3;
      }
      card.position = Vec<float>::Lerp(
        { 
          abs(card.position.x - tp.x) < 1.15f ? tp.x : card.position.x ,
          abs(card.position.y - tp.y) < 1.15f ? tp.y : card.position.y,
        },
        tp, 0.5f);
      p.x += CardFrameColumns;
    }
  }
};

Button button_one(10, 200); // initialize button one at digital pin 10
Button button_two(9, 500); // initialize button two at digital pin 9
Button button_three(8, 200); // initialize button three at digital pin 8

const float tao = M_PI * 2;

const Vec<int> display_center = { display.width() / 2, display.height() / 2 };

void DrawLine(int x0, int y0, float rot, int length, uint16_t color = BLACK) {
  display.drawLine(x0, y0,
    x0 + length * cos(rot), y0 + length * sin(rot),
    color
  );
}

void Test() {
  const static float d = tao / 20;
  const static float rSpeed = M_PI / 20;
  const static float orSpeed = M_PI / 10;
  static uint8_t toggle = 0;
  float rOff = 0;
  Vec<float> oo = { 10.0f, 0.0f };
  while (1) {
    for (int i = 0; i < 20; i++) {
      DrawLine(
        (display.width() >> 1) + oo.x,
        (display.height() >> 1) + oo.y,
        d * i + rOff,
        10
      );
    }
    display.display();
    delay(50);
    display.clearDisplay();   
    if (button_one.WasPressed()) {
      toggle = !toggle;
      Serial.println("button one was pressed");
    }
    if (button_two.WasPressed()) {
        Serial.println("button two was pressed");
    }
    if (button_three.WasPressed()) {
        Serial.println("button three was pressed");
    }
    rOff = fmod(toggle ? rOff + rSpeed : rOff - rSpeed, tao);
    oo = oo.Rotated(toggle ? orSpeed : -orSpeed);
  }
}

CardSet cardSet(display_center, 5);

void setup()   {
  Serial.begin(9600);
  display.begin();
  display.setContrast(75);
  display.display();
  display.clearDisplay();
  cardSet.SetActive();
  CardRank ranks[5] {
    CardRank_6,
    CardRank_7,
    CardRank_8,
    CardRank_9,
    CardRank_10,
  };
  CardSuit suits[5] {
    CardSuit_Spade,
    CardSuit_Heart,
    CardSuit_Diamond,
    CardSuit_Club,
    CardSuit_Diamond,
  };
  for (uint16_t i = 0; i < 5; i++) {
    cardSet.SetCardProperties(i, ranks[i], suits[i], CardFrame_Front);
  }
}

void loop() {
  display.clearDisplay();
  if (button_one.WasPressed()) {
    cardSet.GoRight();
    Serial.println("button one pressed!");
  }
  else if (button_three.WasPressed()) {
    cardSet.GoLeft();
    Serial.println("button two pressed!");
  }
  cardSet.UpdatePositions();
  cardSet.Draw();
}