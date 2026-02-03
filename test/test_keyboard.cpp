/****************************************************************************/
/* Test harness for Sanyo MBC 550/555 keyboard adapter firmware             */
/*                                                                          */
/* Compiles natively on the host machine (not on Arduino).                  */
/* Mocks Arduino APIs (Serial, GPIO, PS2KeyAdvanced) and exercises          */
/* the scan code translation logic.                                         */
/*                                                                          */
/* Build & run:  cd test && make test                                       */
/****************************************************************************/

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>
#include <string>

// =========================================================================
// Arduino constants (normally from Arduino.h)
// =========================================================================
#define OUTPUT 1
#define HIGH   1
#define LOW    0
#define SERIAL_8E2  0x2E
#define SERIAL_8O2  0x3E

typedef uint8_t byte;

// =========================================================================
// PS2KeyAdvanced key code definitions (from the library header)
// These must be defined BEFORE including the .ino file.
// =========================================================================
#define PS2_KEY_A         0x41
#define PS2_KEY_B         0x42
#define PS2_KEY_C         0x43
#define PS2_KEY_D         0x44
#define PS2_KEY_E         0x45
#define PS2_KEY_F         0x46
#define PS2_KEY_G         0x47
#define PS2_KEY_H         0x48
#define PS2_KEY_I         0x49
#define PS2_KEY_J         0x4A
#define PS2_KEY_K         0x4B
#define PS2_KEY_L         0x4C
#define PS2_KEY_M         0x4D
#define PS2_KEY_N         0x4E
#define PS2_KEY_O         0x4F
#define PS2_KEY_P         0x50
#define PS2_KEY_Q         0x51
#define PS2_KEY_R         0x52
#define PS2_KEY_S         0x53
#define PS2_KEY_T         0x54
#define PS2_KEY_U         0x55
#define PS2_KEY_V         0x56
#define PS2_KEY_W         0x57
#define PS2_KEY_X         0x58
#define PS2_KEY_Y         0x59
#define PS2_KEY_Z         0x5A

#define PS2_KEY_0         0x30
#define PS2_KEY_1         0x31
#define PS2_KEY_2         0x32
#define PS2_KEY_3         0x33
#define PS2_KEY_4         0x34
#define PS2_KEY_5         0x35
#define PS2_KEY_6         0x36
#define PS2_KEY_7         0x37
#define PS2_KEY_8         0x38
#define PS2_KEY_9         0x39

#define PS2_KEY_F1        0x61
#define PS2_KEY_F2        0x62
#define PS2_KEY_F3        0x63
#define PS2_KEY_F4        0x64
#define PS2_KEY_F5        0x65
#define PS2_KEY_F6        0x66
#define PS2_KEY_F7        0x67
#define PS2_KEY_F8        0x68
#define PS2_KEY_F9        0x69
#define PS2_KEY_F10       0x6A

#define PS2_KEY_KP0       0x20
#define PS2_KEY_KP1       0x21
#define PS2_KEY_KP2       0x22
#define PS2_KEY_KP3       0x23
#define PS2_KEY_KP4       0x24
#define PS2_KEY_KP5       0x25
#define PS2_KEY_KP6       0x26
#define PS2_KEY_KP7       0x27
#define PS2_KEY_KP8       0x28
#define PS2_KEY_KP9       0x29

#define PS2_KEY_ENTER     0x1E
#define PS2_KEY_KP_ENTER  0x2B
#define PS2_KEY_DELETE    0x1A
#define PS2_KEY_BS        0x1C
#define PS2_KEY_INSERT    0x19
#define PS2_KEY_TAB       0x1D
#define PS2_KEY_BREAK     0x0F
#define PS2_KEY_ESC       0x1B
#define PS2_KEY_SPACE     0x1F

#define PS2_KEY_HOME      0x11
#define PS2_KEY_END       0x12
#define PS2_KEY_PGUP      0x13
#define PS2_KEY_PGDN      0x14
#define PS2_KEY_L_ARROW   0x15
#define PS2_KEY_R_ARROW   0x16
#define PS2_KEY_UP_ARROW  0x17
#define PS2_KEY_DN_ARROW  0x18

#define PS2_KEY_KP_DOT    0x2A
#define PS2_KEY_KP_PLUS   0x2C
#define PS2_KEY_KP_MINUS  0x2D
#define PS2_KEY_KP_TIMES  0x2E
#define PS2_KEY_KP_DIV    0x2F
#define PS2_KEY_KP_EQUAL  0x3F

#define PS2_KEY_DOT       0x3D
#define PS2_KEY_DIV       0x3E
#define PS2_KEY_EQUAL     0x5F
#define PS2_KEY_MINUS     0x3C
#define PS2_KEY_COMMA     0x3B
#define PS2_KEY_APOS      0x3A
#define PS2_KEY_SEMI      0x5B
#define PS2_KEY_OPEN_SQ   0x5D
#define PS2_KEY_CLOSE_SQ  0x5E
#define PS2_KEY_BACK      0x5C

// =========================================================================
// Arduino mock layer
// =========================================================================

struct SerialEvent {
  enum Type { WRITE, BEGIN, END_EV, FLUSH } type;
  int value;       // written byte or baud rate
  int config;      // for BEGIN: parity config; for WRITE: active parity
};

static std::vector<SerialEvent> serialLog;
static int lastParityConfig = SERIAL_8E2;

struct MockSerial {
  void begin(int baud, int config) {
    lastParityConfig = config;
    serialLog.push_back({SerialEvent::BEGIN, baud, config});
  }
  void end() {
    serialLog.push_back({SerialEvent::END_EV, 0, 0});
  }
  void flush() {
    serialLog.push_back({SerialEvent::FLUSH, 0, 0});
  }
  size_t write(int c) {
    serialLog.push_back({SerialEvent::WRITE, c, lastParityConfig});
    return 1;
  }
  // Print overloads (no-ops for testing, but needed to compile)
  void print(const char*) {}
  void print(int, int = 10) {}
  void print(char c) { (void)c; }
  void println(const char*) {}
} Serial;

// GPIO mock
struct PinEvent {
  int pin;
  int value;
};
static std::vector<PinEvent> pinLog;

void pinMode(int pin, int mode) { (void)pin; (void)mode; }
void digitalWrite(int pin, int value) {
  pinLog.push_back({pin, value});
}
void delay(int ms) { (void)ms; }

// PS2KeyAdvanced mock — must be defined before the .ino is included
static uint16_t mockScanCodeQueue[64];
static int mockQueueHead = 0;
static int mockQueueTail = 0;

class PS2KeyAdvanced {
public:
  void begin(int, int) {}
  void setNoBreak(int) {}
  void setNoRepeat(int) {}
  bool available() { return mockQueueHead != mockQueueTail; }
  uint16_t read() {
    if (mockQueueHead == mockQueueTail) return 0;
    uint16_t val = mockScanCodeQueue[mockQueueHead];
    mockQueueHead = (mockQueueHead + 1) % 64;
    return val;
  }
};

class PS2KeyMap {
public:
  void begin() {}
};

// =========================================================================
// Include the firmware source (compiles with our mocks above)
// The -I. flag in the Makefile ensures our stub PS2KeyAdvanced.h and
// PS2KeyMap.h are found instead of the real library headers.
// =========================================================================
#include "../sanyombc-keyboard.ino"

// =========================================================================
// Test helpers
// =========================================================================

static uint16_t makeScanCode(int keyCode, bool ctrl = false, bool alt = false,
                              bool altGr = false, bool shift = false,
                              bool capsLock = false) {
  uint16_t sc = keyCode & 0xFF;
  if (ctrl)     sc |= (1 << 13);
  if (altGr)    sc |= (1 << 10);
  if (alt)      sc |= (1 << 11);
  if (capsLock) sc |= (1 << 12);
  if (shift)    sc |= (1 << 14);
  return sc;
}

static void enqueue(uint16_t sc) {
  mockScanCodeQueue[mockQueueTail] = sc;
  mockQueueTail = (mockQueueTail + 1) % 64;
}

// Run one loop iteration, return the bytes written to Serial
static std::vector<int> runKey(uint16_t scanCode) {
  serialLog.clear();
  pinLog.clear();
  enqueue(scanCode);
  loop();
  std::vector<int> written;
  for (auto& e : serialLog) {
    if (e.type == SerialEvent::WRITE) written.push_back(e.value);
  }
  return written;
}

static int runKeyExpectOne(uint16_t scanCode) {
  auto out = runKey(scanCode);
  assert(out.size() == 1);
  return out[0];
}

// =========================================================================
// Test counters and macros
// =========================================================================
static int testsPassed = 0;
static int testsFailed = 0;

#define RUN_TEST(fn) do { \
    printf("  %-50s", #fn); \
    try { fn(); testsPassed++; printf("PASS\n"); } \
    catch (...) { testsFailed++; printf("FAIL\n"); } \
  } while(0)

#define ASSERT_EQ(actual, expected) do { \
    auto _a = (actual); auto _e = (expected); \
    if (_a != _e) { \
      fprintf(stderr, "\n    ASSERT_EQ failed: got 0x%X, expected 0x%X at line %d\n", \
              (unsigned)_a, (unsigned)_e, __LINE__); \
      assert(false); \
    } \
  } while(0)

// =========================================================================
// Tests: Regular characters
// =========================================================================

void test_lowercase_letters() {
  for (int k = PS2_KEY_A; k <= PS2_KEY_Z; k++) {
    int out = runKeyExpectOne(makeScanCode(k));
    ASSERT_EQ(out, 'a' + (k - PS2_KEY_A));
  }
}

void test_uppercase_letters_shift() {
  for (int k = PS2_KEY_A; k <= PS2_KEY_Z; k++) {
    int out = runKeyExpectOne(makeScanCode(k, false, false, false, true));
    ASSERT_EQ(out, 'A' + (k - PS2_KEY_A));
  }
}

void test_uppercase_letters_capslock() {
  for (int k = PS2_KEY_A; k <= PS2_KEY_Z; k++) {
    int out = runKeyExpectOne(makeScanCode(k, false, false, false, false, true));
    ASSERT_EQ(out, 'A' + (k - PS2_KEY_A));
  }
}

void test_digits_unshifted() {
  const char* expected = "0123456789";
  for (int k = PS2_KEY_0; k <= PS2_KEY_9; k++) {
    int out = runKeyExpectOne(makeScanCode(k));
    ASSERT_EQ(out, expected[k - PS2_KEY_0]);
  }
}

void test_digits_shifted() {
  const char* expected = ")!@#$%^&*(";
  for (int k = PS2_KEY_0; k <= PS2_KEY_9; k++) {
    int out = runKeyExpectOne(makeScanCode(k, false, false, false, true));
    ASSERT_EQ(out, expected[k - PS2_KEY_0]);
  }
}

// =========================================================================
// Tests: Function keys
// =========================================================================

void test_function_keys() {
  for (int k = PS2_KEY_F1; k <= PS2_KEY_F10; k++) {
    int out = runKeyExpectOne(makeScanCode(k));
    ASSERT_EQ(out, MBC_F1 + (k - PS2_KEY_F1));
  }
}

// =========================================================================
// Tests: Keypad
// =========================================================================

void test_keypad_digits() {
  for (int k = PS2_KEY_KP0; k <= PS2_KEY_KP9; k++) {
    int out = runKeyExpectOne(makeScanCode(k));
    ASSERT_EQ(out, '0' + (k - PS2_KEY_KP0));
  }
}

void test_keypad_operators() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_EQUAL)), '=');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_MINUS)), '-');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_PLUS)),  '+');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_DIV)),   '/');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_TIMES)), '*');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_DOT)),   '.');
}

// =========================================================================
// Tests: Navigation keys
// =========================================================================

void test_navigation_keys() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_HOME)),     MBC_HOME);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_END)),      MBC_END);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_PGUP)),     MBC_PG_UP);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_PGDN)),     MBC_PG_DOWN);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_L_ARROW)),  MBC_CRS_LEFT);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_R_ARROW)),  MBC_CRS_RIGHT);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_UP_ARROW)), MBC_CRS_UP);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_DN_ARROW)), MBC_CRS_DOWN);
}

// =========================================================================
// Tests: Special keys
// =========================================================================

void test_enter_keys() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_ENTER)),    MBC_RETURN);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_ENTER)), MBC_ENTER);
}

void test_backspace_delete() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_BS)),     MBC_BACKSPACE);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_DELETE)),  MBC_BACKSPACE);
}

void test_tab() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_TAB)),  MBC_TAB);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_TAB, false, false, false, true)), MBC_BACKTAB);
}

void test_escape() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_ESC)), MBC_ESC);
}

void test_space() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_SPACE)), ' ');
}

void test_insert() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_INSERT)), MBC_INSERT);
}

// =========================================================================
// Tests: Punctuation
// =========================================================================

void test_punctuation_unshifted() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_DOT)),      '.');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_DIV)),      '/');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_EQUAL)),    '=');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_MINUS)),    '-');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_COMMA)),    ',');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_APOS)),     '\'');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_SEMI)),     ';');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_OPEN_SQ)),  '[');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_CLOSE_SQ)), ']');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_BACK)),     '\\');
}

void test_punctuation_shifted() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_DOT,      false, false, false, true)), '>');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_DIV,      false, false, false, true)), '?');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_EQUAL,    false, false, false, true)), '+');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_MINUS,    false, false, false, true)), '_');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_COMMA,    false, false, false, true)), '<');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_APOS,     false, false, false, true)), '"');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_SEMI,     false, false, false, true)), ':');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_OPEN_SQ,  false, false, false, true)), '{');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_CLOSE_SQ, false, false, false, true)), '}');
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_BACK,     false, false, false, true)), '|');
}

// =========================================================================
// Tests: CTRL combinations
// =========================================================================

void test_ctrl_letters_parity() {
  for (int k = PS2_KEY_A; k <= PS2_KEY_Z; k++) {
    if (k == PS2_KEY_C) continue;  // CTRL-C is special

    serialLog.clear();
    runKey(makeScanCode(k, true));

    int writtenChar = -1;
    bool hadFlush = false;
    bool hadOddParity = false;
    bool hadEvenRestore = false;
    for (auto& e : serialLog) {
      if (e.type == SerialEvent::FLUSH) hadFlush = true;
      if (e.type == SerialEvent::BEGIN && e.config == SERIAL_8O2) hadOddParity = true;
      if (e.type == SerialEvent::WRITE) {
        writtenChar = e.value;
        ASSERT_EQ(e.config, SERIAL_8O2);
      }
      if (e.type == SerialEvent::BEGIN && e.config == SERIAL_8E2) hadEvenRestore = true;
    }
    assert(hadFlush);
    assert(hadOddParity);
    assert(hadEvenRestore);
    char expected = 'a' + (k - PS2_KEY_A);
    ASSERT_EQ(writtenChar, expected);
  }
}

// Regression test: CTRL+I used to send 'j' due to a copy-paste bug
void test_ctrl_i_sends_i_not_j() {
  serialLog.clear();
  runKey(makeScanCode(PS2_KEY_I, true));
  for (auto& e : serialLog) {
    if (e.type == SerialEvent::WRITE) {
      ASSERT_EQ(e.value, (int)'i');
      return;
    }
  }
  assert(false);  // no write found
}

// Regression test: CTRL+U used to send 'y' due to a copy-paste bug
void test_ctrl_u_sends_u_not_y() {
  serialLog.clear();
  runKey(makeScanCode(PS2_KEY_U, true));
  for (auto& e : serialLog) {
    if (e.type == SerialEvent::WRITE) {
      ASSERT_EQ(e.value, (int)'u');
      return;
    }
  }
  assert(false);  // no write found
}

void test_ctrl_c_no_parity_error() {
  serialLog.clear();
  auto out = runKey(makeScanCode(PS2_KEY_C, true));
  ASSERT_EQ((int)out.size(), 1);
  ASSERT_EQ(out[0], CTRL_C);
  for (auto& e : serialLog) {
    if (e.type == SerialEvent::BEGIN && e.config == SERIAL_8O2) {
      assert(false);  // should not switch to odd parity for CTRL-C
    }
  }
}

void test_ctrl_function_keys() {
  for (int k = PS2_KEY_F1; k <= PS2_KEY_F10; k++) {
    int out = runKeyExpectOne(makeScanCode(k, true));
    ASSERT_EQ(out, CTRL_F1 + (k - PS2_KEY_F1));
  }
}

void test_ctrl_special_keys() {
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_OPEN_SQ,  true)), CTRL_OPEN_SQ);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_CLOSE_SQ, true)), CTRL_CLOSE_SQ);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_END,      true)), CTRL_END);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_PGDN,     true)), CTRL_PGDN);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_TAB,      true)), CTRL_TAB);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_ENTER,    true)), CTRL_ENTER);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_KP_ENTER, true)), CTRL_ENTER);
  ASSERT_EQ(runKeyExpectOne(makeScanCode(PS2_KEY_HOME,     true)), CTRL_HOME);
}

// =========================================================================
// Tests: Special combos
// =========================================================================

void test_ctrl_alt_del_reset() {
  pinLog.clear();
  serialLog.clear();
  runKey(makeScanCode(PS2_KEY_DELETE, true, true));
  bool foundLow = false, foundHigh = false;
  for (auto& p : pinLog) {
    if (p.pin == 6 && p.value == LOW) foundLow = true;
    if (p.pin == 6 && p.value == HIGH) foundHigh = true;
  }
  assert(foundLow);
  assert(foundHigh);
}

void test_ctrl_alt_a_enables_capture() {
  captureMode = false;
  serialLog.clear();
  runKey(makeScanCode(PS2_KEY_A, true, true));
  assert(captureMode == true);
  disableCaptureMode();
}

// =========================================================================
// Tests: Capture mode
// =========================================================================

void test_capture_mode_valid_hex() {
  captureMode = true;
  bufferIndex = 0;
  hexBuffer[0] = '\0';

  // PS2_KEY_4 = 0x34 = '4', PS2_KEY_A = 0x41 = 'A'
  // Entering "4A" should output 0x4A
  serialLog.clear();
  enqueue(makeScanCode(PS2_KEY_4));
  loop();
  {
    bool anyWrite = false;
    for (auto& e : serialLog) {
      if (e.type == SerialEvent::WRITE) anyWrite = true;
    }
    assert(!anyWrite);  // first digit — no output yet
  }

  serialLog.clear();
  enqueue(makeScanCode(PS2_KEY_A));
  loop();
  {
    int writtenVal = -1;
    for (auto& e : serialLog) {
      if (e.type == SerialEvent::WRITE) writtenVal = e.value;
    }
    ASSERT_EQ(writtenVal, 0x4A);
  }
  assert(captureMode == false);
}

void test_capture_mode_invalid_char() {
  captureMode = true;
  bufferIndex = 0;
  hexBuffer[0] = '\0';

  // PS2_KEY_SPACE = 0x1F, not a hex digit
  serialLog.clear();
  enqueue(makeScanCode(PS2_KEY_SPACE));
  loop();

  int writtenVal = -1;
  for (auto& e : serialLog) {
    if (e.type == SerialEvent::WRITE) writtenVal = e.value;
  }
  ASSERT_EQ(writtenVal, (int)'?');
  assert(captureMode == false);
}

// Regression test: toggling off capture mode used to leave stale buffer
void test_capture_toggle_off_clears_buffer() {
  captureMode = true;
  bufferIndex = 1;
  hexBuffer[0] = 'F';
  hexBuffer[1] = '\0';

  serialLog.clear();
  runKey(makeScanCode(PS2_KEY_A, true, true));

  assert(captureMode == false);
  assert(bufferIndex == 0);
  assert(hexBuffer[0] == '\0');
}

// =========================================================================
// Tests: Parity switching
// =========================================================================

void test_parity_flush_sequence() {
  serialLog.clear();
  runKey(makeScanCode(PS2_KEY_B, true));  // CTRL+B -> writeWithParityError

  // Expected: FLUSH, END, BEGIN(odd), WRITE, FLUSH, END, BEGIN(even)
  enum State {
    EXPECT_FLUSH1, EXPECT_END1, EXPECT_BEGIN_ODD,
    EXPECT_WRITE, EXPECT_FLUSH2, EXPECT_END2, EXPECT_BEGIN_EVEN, DONE
  } state = EXPECT_FLUSH1;

  for (auto& e : serialLog) {
    switch (state) {
      case EXPECT_FLUSH1:
        if (e.type == SerialEvent::FLUSH) state = EXPECT_END1;
        break;
      case EXPECT_END1:
        if (e.type == SerialEvent::END_EV) state = EXPECT_BEGIN_ODD;
        break;
      case EXPECT_BEGIN_ODD:
        if (e.type == SerialEvent::BEGIN && e.config == SERIAL_8O2) state = EXPECT_WRITE;
        break;
      case EXPECT_WRITE:
        if (e.type == SerialEvent::WRITE) state = EXPECT_FLUSH2;
        break;
      case EXPECT_FLUSH2:
        if (e.type == SerialEvent::FLUSH) state = EXPECT_END2;
        break;
      case EXPECT_END2:
        if (e.type == SerialEvent::END_EV) state = EXPECT_BEGIN_EVEN;
        break;
      case EXPECT_BEGIN_EVEN:
        if (e.type == SerialEvent::BEGIN && e.config == SERIAL_8E2) state = DONE;
        break;
      case DONE:
        break;
    }
  }
  assert(state == DONE);
}

// =========================================================================
// Tests: Graph mode
// =========================================================================

void test_graph_mode_a() {
  int out = runKeyExpectOne(makeScanCode(PS2_KEY_A, false, false, true));
  ASSERT_EQ(out, GRAPH_A);
}

// =========================================================================
// Tests: Utilities
// =========================================================================

void test_hex_to_int() {
  ASSERT_EQ(hex_to_int("00"), 0x00);
  ASSERT_EQ(hex_to_int("FF"), 0xFF);
  ASSERT_EQ(hex_to_int("4A"), 0x4A);
  ASSERT_EQ(hex_to_int("0D"), 0x0D);
}

// =========================================================================
// Main
// =========================================================================

int main() {
  printf("Running Sanyo MBC keyboard adapter tests...\n\n");

  // Initialize firmware state (skip setup() which would talk to hardware)
  captureMode = false;
  bufferIndex = 0;
  hexBuffer[0] = '\0';

  printf("[Regular characters]\n");
  RUN_TEST(test_lowercase_letters);
  RUN_TEST(test_uppercase_letters_shift);
  RUN_TEST(test_uppercase_letters_capslock);
  RUN_TEST(test_digits_unshifted);
  RUN_TEST(test_digits_shifted);

  printf("\n[Function keys]\n");
  RUN_TEST(test_function_keys);

  printf("\n[Keypad]\n");
  RUN_TEST(test_keypad_digits);
  RUN_TEST(test_keypad_operators);

  printf("\n[Navigation keys]\n");
  RUN_TEST(test_navigation_keys);

  printf("\n[Special keys]\n");
  RUN_TEST(test_enter_keys);
  RUN_TEST(test_backspace_delete);
  RUN_TEST(test_tab);
  RUN_TEST(test_escape);
  RUN_TEST(test_space);
  RUN_TEST(test_insert);

  printf("\n[Punctuation]\n");
  RUN_TEST(test_punctuation_unshifted);
  RUN_TEST(test_punctuation_shifted);

  printf("\n[CTRL combinations]\n");
  RUN_TEST(test_ctrl_letters_parity);
  RUN_TEST(test_ctrl_i_sends_i_not_j);
  RUN_TEST(test_ctrl_u_sends_u_not_y);
  RUN_TEST(test_ctrl_c_no_parity_error);
  RUN_TEST(test_ctrl_function_keys);
  RUN_TEST(test_ctrl_special_keys);

  printf("\n[Special combos]\n");
  RUN_TEST(test_ctrl_alt_del_reset);
  RUN_TEST(test_ctrl_alt_a_enables_capture);

  printf("\n[Capture mode]\n");
  RUN_TEST(test_capture_mode_valid_hex);
  RUN_TEST(test_capture_mode_invalid_char);
  RUN_TEST(test_capture_toggle_off_clears_buffer);

  printf("\n[Parity switching]\n");
  RUN_TEST(test_parity_flush_sequence);

  printf("\n[Graph mode]\n");
  RUN_TEST(test_graph_mode_a);

  printf("\n[Utilities]\n");
  RUN_TEST(test_hex_to_int);

  printf("\n========================================\n");
  printf("Results: %d passed, %d failed\n", testsPassed, testsFailed);
  printf("========================================\n");

  return testsFailed > 0 ? 1 : 0;
}
