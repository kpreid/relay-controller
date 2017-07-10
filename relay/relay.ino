#define CHANNELS 3

enum position {
  POS_FAULT = -1,
  POS_UNPLUGGED = 0,
  POS_1 = 1,
  POS_2 = 2,
};

class channel_config {
  public:
  
  const int pin_coil_pos1, pin_coil_pos2;
  const int pin_indicator_pos1, pin_indicator_pos2;
  const char key;

  void drive_coil(position p) {
    int pin = p ? pin_coil_pos2 : pin_coil_pos1;
    digitalWrite(pin, HIGH);
    delay(10);
    digitalWrite(pin, LOW);
  }

  void setup() {
    pinMode(pin_coil_pos1, OUTPUT);
    pinMode(pin_coil_pos2, OUTPUT);
    pinMode(pin_indicator_pos1, INPUT_PULLUP);
    pinMode(pin_indicator_pos2, INPUT_PULLUP);
  }
};

class channel_state {
  public:

  const channel_config conf;
  int last_seen_position;

  // Returns whether a change was observed.
  bool set(position p) {
    bool changed = false;
    if (last_seen_position != p) {
      last_seen_position = p;
      conf.drive_coil(p);
      changed = true;
    }
    // drive_coil blocks so we should get a good reading.
    // Though in principle the pulse might take less time than the state change.
    return read_indicators();
    // TODO: Distinguish "no-op" from "failed to change".
  }

  void setup() {
    conf.setup();
    read_indicators();
  }

  bool read_indicators() {
    const bool p1 = !digitalRead(conf.pin_indicator_pos1);
    const bool p2 = !digitalRead(conf.pin_indicator_pos2);
    position new_position;
    if (p1 && !p2) {
      new_position = POS_1;
    } else if (p2 && !p1) {
      new_position = POS_2;
    } else if (!p1 && !p2) {
      new_position = POS_UNPLUGGED;
    } else {
      new_position = POS_FAULT;
    }
    if (new_position != last_seen_position) {
      last_seen_position = new_position;
      Serial.write(conf.key);
      Serial.print(last_seen_position);
      Serial.write(';');
      return true;
    } else {
      return false;
    }
  }
};

static channel_state the_state[CHANNELS] = {
  {{ 8,  9,  2,  3, 'A'}, POS_FAULT},
  {{10, 11,  4,  5, 'B'}, POS_FAULT},
  {{12, 13,  6,  7, 'C'}, POS_FAULT},
};

void setup() {
  Serial.write("BOOTING;");
  for (auto& c : the_state) {
    c.setup();
  }
  Serial.begin(115200);
  Serial.write("READY;");
}

channel_state *serial_channel = NULL;
position serial_position = POS_FAULT;

void loop() {
  for (auto& c : the_state) {
    c.read_indicators();
  }
  if (Serial.available() > 0) {
    int symbol = Serial.read();
    switch (symbol) {
      // TODO: Change position numbering to match relay label
      case 'A': serial_channel = &the_state[0]; break;
      case 'B': serial_channel = &the_state[1]; break;
      case 'C': serial_channel = &the_state[2]; break;
      case '0': serial_position = POS_1; break;
      case '1': serial_position = POS_2; break;
      case ';':
        if (serial_channel == NULL || serial_position == POS_FAULT) {
          Serial.write("BAD_COMMAND;");
        } else {
          bool changed = serial_channel->set(serial_position);
          Serial.write(changed ? "" : "NOP;");
        }
        break;
      default:
        Serial.write("BAD_CHARACTER;");
    }
  }
}

