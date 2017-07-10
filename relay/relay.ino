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
    int pin = pin_coil_pos1;
    switch (p) {
      case POS_1: pin = pin_coil_pos1; break;
      case POS_2: pin = pin_coil_pos2; break;
      default: Serial.write("BAD_DRIVE;"); return;
    }
    digitalWrite(pin, HIGH);
    delay(20);
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

  void set(position p) {
    bool changed = false;
    if (last_seen_position != p) {
      conf.drive_coil(p);
      changed = true;
    }
    // drive_coil blocks so we should get a good reading.
    // Though in principle the pulse might take less time than the state change.
    read_indicators(true);
  }

  void setup() {
    conf.setup();
    read_indicators(true);
  }

  void read_indicators(bool always_report) {
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
    if (new_position != last_seen_position || always_report) {
      last_seen_position = new_position;
      char code;
      switch (last_seen_position) {
        case POS_UNPLUGGED: code = 'U'; break;
        case POS_FAULT: code = 'F'; break;
        case POS_1: code = '1'; break;
        case POS_2: code = '2'; break;
      }
      Serial.write(conf.key);
      Serial.write(code);
      Serial.write(';');
    }
  }
};

static channel_state the_state[CHANNELS] = {
  {{ 8,  9,  2,  3, 'A'}, POS_FAULT},
  {{10, 11,  4,  5, 'B'}, POS_FAULT},
  {{12, 13,  6,  7, 'C'}, POS_FAULT},
};

class command_parser {
  channel_state *cmd_channel = NULL;
  position cmd_position = POS_FAULT;

  public:

  void clear() {
    cmd_channel = NULL;
    cmd_position = POS_FAULT;
  }

  void check() {
    if (Serial.available() > 0) {
      int symbol = Serial.read();
      switch (symbol) {
        case 'A': cmd_channel = &the_state[0]; break;
        case 'B': cmd_channel = &the_state[1]; break;
        case 'C': cmd_channel = &the_state[2]; break;
        case '1': cmd_position = POS_1; break;
        case '2': cmd_position = POS_2; break;
        case ';':
          if (cmd_channel == NULL || cmd_position == POS_FAULT) {
            Serial.write("INCOMPLETE_COMMAND;");
          } else {
            cmd_channel->set(cmd_position);
          }
          break;
        default:
          Serial.write("BAD_CHARACTER;");
          clear();
          break;
      }
    }
  }
};

static command_parser the_parser;

void setup() {
  Serial.begin(115200);
  Serial.write("BOOTING;");
  for (auto& c : the_state) {
    c.setup();
  }
  Serial.write("READY;");
}

void loop() {
  for (auto& c : the_state) {
    c.read_indicators(false);
  }
  the_parser.check();
}

