// Projektin on tehnyt Einari Mäkiranta osana KAMK:in projektiopintoja.

#include <SPI.h>
#include <avr/pgmspace.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Fonts/TomThumb.h>

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

#define CardRankRows 5
#define CardRankColumns 8

const uint8_t PROGMEM card_ranks[13][5] {
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
  {
    0b00001100,
    0b00001100,
    0b00001100,
    0b01101100,
    0b00111100,
  },
  {
    0b11111100,
    0b11001100,
    0b11001100,
    0b11111100,
    0b00000110,
  },
  {
    0b11101100,
    0b01101100,
    0b01110000,
    0b01111100,
    0b01101100,
  },
  {
    0b01111100,
    0b01100100,
    0b01111100,
    0b01100100,
    0b01100100,
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
    0b0001000,
    0b0111110,
    0b0011100,
    0b0001000,
    0b0111110,
  },
};

#define ArrowRows 5
#define ArrowColumns 7

const uint8_t PROGMEM left_arrow[5] {
  0b0001000,
  0b0110000,
  0b1011111,
  0b0110000,
  0b0011000,
};

enum class Hand : uint8_t {
  HighCard        = 0,
  Pair            = 1,
  TwoPair         = 2,
  ThreeOfAKind    = 3,
  Flush           = 4,
  FullHouse       = 5,
  Straight        = 6,
  FourOfAKind     = 7,
  StraightFlush   = 8,
  RoyalFlush      = 9,
  FiveOfAKind     = 10,
  FlushHouse      = 11,
  FlushFive       = 12,
  None            = 13,
};

struct HandInfo {
  const char* name;
  float mult;
  float chips;
};

HandInfo hands[13] {
  {
    .name = "High Card",
    .mult = 1,
    .chips = 15,
  },
  {
    .name = "Pair",
    .mult = 1,
    .chips = 25,
  },
  {
    .name = "Two Pair",
    .mult = 2,
    .chips = 30,
  },
  {
    .name = "Three of a Kind",
    .mult = 3,
    .chips = 40,
  },
  {
    .name = "Flush",
    .mult = 4,
    .chips = 40,
  },
  {
    .name = "Full House",
    .mult = 4,
    .chips = 40,
  },
  {
    .name = "Straight",
    .mult = 5,
    .chips = 50,
  },
  {
    .name = "Four of a Kind",
    .mult = 6,
    .chips = 50,
  },
  {
    .name = "Straight Flush",
    .mult = 8,
    .chips = 80,
  },
  {
    .name = "Royal Flush",
    .mult = 10,
    .chips = 100,
  },
  {
    .name = "Five of a Kind",
    .mult = 14,
    .chips = 120,
  },
  {
    .name = "Flush House",
    .mult = 16,
    .chips = 160,
  },
  {
    .name = "Flush Five",
    .mult = 18,
    .chips = 180,
  },
};

struct HandHistogram {
  uint8_t rankCounts[13] {};
  uint8_t suitCounts[13] {};
};

struct CardProperties {
  CardRank rank;
  CardSuit suit;
};

CardProperties deck[52] {};
uint8_t deckSize = 52;
uint8_t shuffledDeckIDs[52] {};
uint8_t shuffledDeckRemaining = 0;

void ShuffleDeck() {
  for (uint8_t i = 0; i < deckSize; i++) {
    shuffledDeckIDs[i] = i;
  }
  for (uint8_t i = 51; i > 0; i--) {
    uint8_t j = random(0, i + 1);
    uint8_t tmp = shuffledDeckIDs[i];
    shuffledDeckIDs[i] = shuffledDeckIDs[j];
    shuffledDeckIDs[j] = tmp;
  }
  shuffledDeckRemaining = deckSize;
}

// returns CardProperties index from deck
bool DrawRandomCard(uint8_t& outIndex) {
  if (shuffledDeckRemaining == 0) return false;
  outIndex = shuffledDeckIDs[--shuffledDeckRemaining];
  return true;
}

const uint8_t CLK = 6; // digital
const uint8_t DIN = 5; // digital
const uint8_t DC = 4; // digital
const uint8_t CE = 3; // digital
const uint8_t RST = 2; // digital

Adafruit_PCD8544 display = Adafruit_PCD8544(CLK, DIN, DC, CE, RST);

struct Button {
private:

  uint32_t _longPressTimer = 0; // milliseconds
  uint16_t _longPressTime;
  bool _wasDown = false;
  bool _wasLongPressed = false;
  uint8_t _pin;

public:

  enum PressType {
    None = 0,
    Short = 1,
    Long = 2,
  };

  Button(uint8_t digitalPin, uint16_t longPressTime)
      : _pin(digitalPin), _longPressTime(longPressTime) {
    pinMode(digitalPin, INPUT_PULLUP);
  }

  PressType WasPressed() {
    if (digitalRead(_pin) == HIGH) {
      _wasLongPressed = false;
      if (_wasDown && millis() - _longPressTimer < _longPressTime) {
        _wasDown = false;
        _longPressTimer = millis();
        return Short;
      }
      _longPressTimer = millis();
      return None;
    }
    if (!_wasLongPressed && millis() - _longPressTimer > _longPressTime) {
      _wasDown = false;
      _wasLongPressed = true;
      return Long;
    }
    if (!_wasLongPressed) {
      _wasDown = true;
    }
    return None;
  }
};

struct UIButton {
private:
  
  const char* text;

public:

  UIButton(const char* text)
    : text(text) {}
  
  void Draw(uint16_t left, uint16_t top) {
    display.setCursor(left, top);
    display.print(text);
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

  uint8_t id;
  CardFrame frame;
  Vec<float> position;
  bool selected;

  void Draw() const {
    uint16_t left = ceil(position.x);
    uint16_t top = ceil((float)display.height() - position.y);
    DrawSprite16(left, top, card_frames[frame], CardFrameRows, CardFrameColumns);
    CardProperties& properties = deck[id];
    if (frame == CardFrame_Back) {
      return;
    }
    DrawSprite8(left + 1 + 1 * (properties.rank != CardRank_10),
      top + 2, card_ranks[properties.rank], CardRankRows, CardRankColumns);
    DrawSprite8(left + 2, top + 8, card_suits_small[properties.suit], CardSuitSmallRows, CardSuitSmallColumns);
  }
};

template <uint8_t Count_T>
struct CardSet {
private:

  Card _cards[Count_T] = {};
  Vec<int> _center;
  float _hoveredCardT = 0.0f;
  int16_t _hoveredCard = -1;
  uint8_t _count = Count_T;
  uint8_t _selectedCount = 0;

public:

  CardSet(Vec<int> center) : _center(center) {
    memset(_cards, 0, Count_T * sizeof(Card));
    for (uint32_t i = 0; i < _count; i++) {
      _cards[i].position = { _center.x, center.y };
    }
  }

  ~CardSet() {}
  
  void Draw() const {
    for (uint8_t i = 0; i < _count; i++) {
      _cards[i].Draw();
    }
  }

  uint8_t Count() const {
    return _count;
  }

  uint8_t SelectedCount() const {
    return _selectedCount;
  }

  void SetCardProperties(uint16_t index, CardRank rank, CardSuit suit) {
    if (index >= _count) {
      return;
    }
    assert(rank <= CardRank_Identity);
    assert(suit <= CardSuit_Identity);
    CardProperties& card = deck[_cards[index].id];
    if (rank != CardRank_Identity) {
      card.rank = rank;
    }
    if (suit != CardSuit_Identity) {
      card.suit = suit;
    }
  }

  void SetCardFrame(uint16_t index, CardFrame frame) {
    if (index >= _count) {
      return;
    }
    assert(frame <= CardFrame_Identity);
    Card& card = _cards[index];
    card.frame = frame;
  }

  void SetActive() {
    _hoveredCard = 0;
    _hoveredCardT = 0.0f;
  }

  void SetInactive() {
    _hoveredCard = -1;
  }

  void GoRight() {
    if (_hoveredCard != -1) {
      _hoveredCard = ((_hoveredCard + 1) % _count);
      _hoveredCardT = 0.0f;
    }
  }

  void GoLeft() {
    if (_hoveredCard != -1) {
      _hoveredCard = _hoveredCard ? ((_hoveredCard - 1) % _count) : _count - 1;
      _hoveredCardT = 0.0f;
    }
  }

  void SelectCard() {
    if (_hoveredCard == -1) {
      return;
    }
    Card& card = _cards[_hoveredCard];
    if (!card.selected && _selectedCount < 5) {
      card.selected = true;
      ++_selectedCount;
      _hoveredCardT = 0;
    }
    else if (card.selected) {
      assert(_selectedCount);
      card.selected = false;
      --_selectedCount;
      _hoveredCardT = M_PI;
    }
  }

  void DeselectAll() {
    for (Card& card : _cards) {
      card.selected = false;
    }
    _selectedCount = 0;
  }

  void GetSelected(uint8_t& outCount, uint8_t (&outIndices)[5]) {
    outCount = 0;
    for (Card& card : _cards) {
      if (card.selected) {
        outIndices[outCount++] = card.id;
      }
    }
  }

  void SortCards() {
    for (uint8_t i = 1; i < _count; i++) {
      Card key = _cards[i];
      int8_t j = i - 1;
      while (j >= 0 && deck[_cards[j].id].rank < deck[key.id].rank) {
        _cards[j + 1] = _cards[j];
        j--;
      }
      _cards[j + 1] = key;
    }
  }

  void RemoveCard(uint8_t index) {
    for (uint8_t i = index; i < _count - 1; i++) {
      _cards[i] = _cards[i + 1];
    }
    --_count;
    if (_count == 0) {
      _hoveredCard = -1;
    }
    else {
      _hoveredCard = 0;
    }
  }

  void RedrawSelected() {
    for (uint8_t i = 0; i < _count; i++) {
      Card& card = _cards[i];
      if (card.selected) {
        card.selected = false;
        if (!DrawRandomCard(card.id)) {
          RemoveCard(i);
          --i;
        }
      }
    }
    _selectedCount = 0;
  }

  bool DrawRandomCards() {
    for (uint16_t i = 0; i < _count; i++) {
      Card& card = _cards[i];
      if (!DrawRandomCard(card.id)) {
        RemoveCard(i);
      }
    }
  }

  void UpdatePositions() {
    Vec<float> p = {
        _center.x - (_count * 0.5f * CardFrameColumns),
        _center.y + (CardFrameRows >> 1),
    };
    for (uint32_t i = 0; i < _count; i++) {
      Card& card = _cards[i];
      Vec<float> tp = p;
      if (_hoveredCard == i) {
        if (card.selected) {
          tp.y += 8 + (cos(_hoveredCardT) + 1.0f) * 2.0f;
        }
        else {
          tp.y += 2 + (cos(_hoveredCardT) + 1.0f) * 2;
        }
        _hoveredCardT = fmod(_hoveredCardT + 0.5f, M_PI * 2.0f);
      }
      else if (card.selected) {
        tp.y += 8;
      }
      card.position = Vec<float>::Lerp(
        { 
          abs(card.position.x - tp.x) < 1.15f ? tp.x : card.position.x,
          abs(card.position.y - tp.y) < 1.15f ? tp.y : card.position.y,
        },
        tp, 0.5f);
      p.x += CardFrameColumns;
    }
  }
};

Button button_one(10, 0); // initialize button one at digital pin 10
Button button_two(9, 300); // initialize button two at digital pin 9
Button button_three(8, 0); // initialize button three at digital pin 8

const float tao = M_PI * 2;

const Vec<int16_t> display_center = { display.width() >> 1, display.height() >> 1 };

void DrawLine(int x0, int y0, float rot, int length, uint16_t color = BLACK) {
  display.drawLine(x0, y0,
    x0 + length * cos(rot), y0 + length * sin(rot),
    color
  );
}

CardSet<7> card_set({ display_center.x, display_center.y - 10 });

UIButton play_hand_button("Play hand");
UIButton discard_hand_button("Discard");

void setup()   {
  randomSeed(analogRead(A0));
  display.begin();
  display.setFont(&TomThumb);
  display.setContrast(60);
  display.display();
  display.clearDisplay();
  card_set.SetActive();
  for (uint16_t i = 0; i < 4; i++) {
    for (uint16_t j = 0; j < 13; j++) {
      CardProperties& card = deck[i * 13 + j];
      card.suit = i;
      card.rank = j;
    }
  }
  ShuffleDeck();
  card_set.DrawRandomCards();
  for (uint16_t i = 0; i < card_set.Count(); i++) {
    card_set.SetCardFrame(i, CardFrame_Front);
  }
}

enum class RoundMenu {
  Hand,
  PlayDiscard,
};

RoundMenu round_menu = RoundMenu::Hand;

enum HandResult : uint8_t {
  None = 0,
  Victory = 1,
  Loss = 2,
};

Hand GetCurrentHand() {
  uint8_t cardCount;
  uint8_t cards[5];
  card_set.GetSelected(cardCount, cards);
  if (cardCount == 0) {
    return Hand::None;
  }
  uint8_t rankCounts[13] {};
  uint8_t suitCounts[4] {};
  for (uint8_t i = 0; i < cardCount; i++) {
    CardProperties& card = deck[cards[i]];
    rankCounts[card.rank]++;
    suitCounts[card.suit]++;
  }
  bool hasFlush = false;
  for (uint8_t i = 0; i < 4; i++) {
    if (suitCounts[i] >= 5) {
      hasFlush = true;
      break;
    }
  }
  bool hasFiveOfAKind = false;
  bool hasFourOfAKind = false;
  CardRank threeOfAKindRank = CardRank_Identity;
  uint8_t pairCount = 0;
  for (uint8_t i = 0; i < 13; i++) {
    if (rankCounts[i] >= 4) {
      hasFiveOfAKind = rankCounts[i] == 5;
      hasFourOfAKind = true;
      break;
    }
    if (rankCounts[i] == 3) {
      threeOfAKindRank = i;
    }
    if (rankCounts[i] == 2) {
      ++pairCount;
    }
  }
  if (hasFiveOfAKind) {
    if (hasFlush) {
      return Hand::FlushFive;
    }
    return Hand::FiveOfAKind;
  }
  if (hasFourOfAKind) {
    return Hand::FourOfAKind;
  }
  if (threeOfAKindRank != CardRank_Identity) {
    if (pairCount == 1) { // is full house
      if (hasFlush) {
        return Hand::FlushHouse;
      }
      return Hand::FullHouse;
    }
    return Hand::ThreeOfAKind;
  }
  if (pairCount == 2) {
    return Hand::TwoPair;
  }
  if (pairCount == 1) {
    return Hand::Pair;
  }
  // detect straight
  bool hasStraight = false;
  bool isRoyal = false;
  for (uint8_t i = 0; i < 9; i++) {
    hasStraight = true;
    uint8_t jEnd = i + 5;
    for (uint8_t j = i; j < jEnd; j++) {
      if (rankCounts[j] == 0) {
        hasStraight = false;
        break;
      }
    }
    if (hasStraight) {
      if (i == 7) { // 10 to Ace straight
        isRoyal = true;
      }
      break;
    }
  }
  // detect Ace -> 5 straight
  if (!hasStraight) {
    hasStraight = true;
    for (uint8_t i = 0; i < 4; i++) {
      if (rankCounts[i] == 0) {
        hasStraight = false;
      }
    }
    hasStraight = hasStraight && rankCounts[CardRank_Ace] != 0;
  }
  if (hasStraight) {
    if (hasFlush) {
      if (isRoyal) {
        return Hand::RoyalFlush;
      }
      return Hand::StraightFlush;
    }
    return Hand::Straight;
  }
  if (hasFlush) {
    return Hand::Flush;
  }
  return Hand::HighCard;
}

uint8_t PlayCurrentHand() {
  return HandResult::None;
}

void loop() {
  display.clearDisplay();
  HandResult res = None;
  if (round_menu == RoundMenu::Hand) {
    Hand hand = GetCurrentHand();
    if (card_set.SelectedCount()) {
      display.setCursor(display_center.x / 2, 11);
      display.print(hands[(uint8_t)hand].name);
    }
    if (button_one.WasPressed()) {
      card_set.GoRight();
    }
    else if (button_three.WasPressed()) {
      card_set.GoLeft();
    }
    auto b2_press = button_two.WasPressed();
    if (b2_press == Button::Long && card_set.SelectedCount()) {
      round_menu = RoundMenu::PlayDiscard;
      card_set.SetInactive();
    }
    else if (b2_press == Button::Short) {
      card_set.SelectCard();
    }
  }
  else {
    static uint8_t arrow_pos = 0;
    uint16_t x = display_center.x / 2;
    play_hand_button.Draw(x, 5);
    discard_hand_button.Draw(x, 11);
    if (!arrow_pos) {
      DrawSprite8(x + 50, 0, left_arrow, ArrowRows, ArrowColumns);
    }
    else {
      DrawSprite8(x + 50, 6, left_arrow, ArrowRows, ArrowColumns);
    }
    if (button_one.WasPressed() || button_three.WasPressed()) {
      arrow_pos = !arrow_pos;
    }
    auto b2_press = button_two.WasPressed(); 
    if (b2_press == Button::Short) {
      if (!arrow_pos) {
        res = PlayCurrentHand();
      }
      else {
        card_set.RedrawSelected();
      }
      round_menu = RoundMenu::Hand;
      card_set.SetActive();
    }
    else if (b2_press) {
      round_menu = RoundMenu::Hand;
      card_set.SetActive();
    }
  }
  card_set.SortCards();
  card_set.UpdatePositions();
  card_set.Draw();
  display.display();
  switch (res) {
    case None:
      break;
    case Victory:
      break;
    case Loss:
      break;
  };
}
