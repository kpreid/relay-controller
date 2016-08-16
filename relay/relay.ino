#define DRIVER_STBY 4
#define DRIVER_AIN1 3
#define DRIVER_AIN2 2
#define DRIVER_BIN1 5
#define DRIVER_BIN2 6
#define ST_LED1 13

#define SWITCH_A1 8
#define SWITCH_A2 9
#define SWITCH_B1 10
#define SWITCH_B2 11

static void driver_send(bool which, bool state) {
  digitalWrite(which ? DRIVER_BIN1 : DRIVER_AIN1, state ? HIGH : LOW);
  digitalWrite(which ? DRIVER_BIN2 : DRIVER_AIN2, state ? LOW : HIGH);
  // other remains hiZ
  digitalWrite(which ? DRIVER_AIN1 : DRIVER_BIN1, LOW);
  digitalWrite(which ? DRIVER_AIN2 : DRIVER_BIN2, LOW);

  digitalWrite(DRIVER_STBY, 1);
  digitalWrite(ST_LED1, 1);
  delay(10);
  digitalWrite(DRIVER_STBY, 0);
  digitalWrite(ST_LED1, 0);
}

struct relay {
  const int which;
  const char key;
  int curState;

  bool set(bool state) {
    bool changed = false;
    if (curState != state) {
      curState = state;
      driver_send(which, curState);
      changed = true;
    }
    // Report new state unconditionally for consistent results.
    Serial.write(key);
    Serial.print(curState);
    Serial.write(';');
    return changed;
  }
};

static relay relays[] = {
  {0, 'A', -1},
  {1, 'B', -1},
};

void setup() {
  pinMode(DRIVER_STBY, OUTPUT);
  pinMode(DRIVER_AIN1, OUTPUT);
  pinMode(DRIVER_AIN2, OUTPUT);
  pinMode(DRIVER_BIN1, OUTPUT);
  pinMode(DRIVER_BIN2, OUTPUT);

  pinMode(ST_LED1, OUTPUT);

  pinMode(SWITCH_A1, INPUT_PULLUP);
  pinMode(SWITCH_A2, INPUT_PULLUP);
  pinMode(SWITCH_B1, INPUT_PULLUP);
  pinMode(SWITCH_B2, INPUT_PULLUP);

  relays[0].set(1);
  relays[1].set(1);

  Serial.begin(115200);
  Serial.write("READY;");
}

bool serWhich = 0;
bool serState = 0;

void loop() {
  // TODO: If any switch input is active, ignore opposing serial commands.
  if (!digitalRead(SWITCH_A1)) { relays[0].set(0); }
  if (!digitalRead(SWITCH_A2)) { relays[0].set(1); }
  if (!digitalRead(SWITCH_B1)) { relays[1].set(0); }
  if (!digitalRead(SWITCH_B2)) { relays[1].set(1); }
  if (Serial.available() > 0) {
    int symbol = Serial.read();
    switch (symbol) {
      case 'A': serWhich = 0; break;
      case 'B': serWhich = 1; break;
      case '0': serState = 0; break;
      case '1': serState = 1; break;
      case ';': {
        bool changed = relays[serWhich].set(serState);
        Serial.write(changed ? "" : "NOP;");
        break;
      }
      default:
        Serial.write("ERROR;");
    }
  }
}

