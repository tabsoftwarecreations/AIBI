#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define SDA_PIN 21
#define SCL_PIN 22
#define TOUCH_PIN 27

const char* WIFI_NAME = "AIBI-ROBOT";
const char* WIFI_PASSWORD = "aibi12345";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

int lookX = 0;
int targetLookX = 0;
int eyeHeight = 26;
int targetEyeHeight = 26;
int eyeWidth = 20;
int targetEyeWidth = 20;

unsigned long nextLookAt = 0;
unsigned long nextBlinkAt = 0;
unsigned long blinkUntil = 0;
unsigned long secondBlinkAt = 0;
unsigned long happyUntil = 0;
unsigned long wakeUntil = 0;
unsigned long attentionUntil = 0;
unsigned long talkUntil = 0;
unsigned long serialSleepUntil = 0;
unsigned long autoSmileUntil = 0;
unsigned long lastInteractionAt = 0;
unsigned long nextMicroActionAt = 0;
unsigned long nextMouthAt = 0;
unsigned long touchStartedAt = 0;

bool lastTouch = false;
int mouthFrame = 0;
String serialLine = "";

void runCommand(String command, unsigned long now);
void markInteraction(unsigned long now);
void startWifiControl();

const char CONTROL_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>AIBI</title>
  <style>
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      background: #101214;
      color: #eef4f2;
      font-family: Arial, sans-serif;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 18px;
    }
    main {
      width: min(100%, 440px);
    }
    .face {
      width: 104px;
      height: 66px;
      margin: 0 auto 18px;
      border-radius: 14px;
      background: #050607;
      position: relative;
      border: 1px solid #30383d;
    }
    .face:before,
    .face:after {
      content: "";
      position: absolute;
      top: 20px;
      width: 14px;
      height: 26px;
      border-radius: 999px;
      background: #55d67a;
      box-shadow: 0 0 16px rgba(85, 214, 122, 0.7);
    }
    .face:before { left: 29px; }
    .face:after { right: 29px; }
    h1 {
      text-align: center;
      font-size: 28px;
      margin: 0 0 6px;
    }
    p {
      text-align: center;
      margin: 0 0 22px;
      color: #91a09a;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    button {
      min-height: 64px;
      border: 1px solid #30383d;
      border-radius: 10px;
      background: #20262a;
      color: #eef4f2;
      font-size: 18px;
    }
    button:active {
      transform: scale(0.98);
    }
    .happy { background: #123420; border-color: #31884d; }
    .talk { background: #13283a; border-color: #2f6f9d; }
    .sleep { background: #332713; border-color: #9d7830; }
    .wake { background: #20262a; border-color: #4b5960; }
    textarea {
      width: 100%;
      min-height: 96px;
      margin: 14px 0 10px;
      border: 1px solid #30383d;
      border-radius: 10px;
      background: #090b0c;
      color: #eef4f2;
      padding: 12px;
      font-size: 16px;
      resize: vertical;
    }
    .wide {
      width: 100%;
      background: #13283a;
      border-color: #2f6f9d;
    }
    #status {
      margin-top: 14px;
      min-height: 22px;
      color: #55d67a;
      text-align: center;
      font-size: 14px;
    }
  </style>
</head>
<body>
  <main>
    <div class="face"></div>
    <h1>AIBI</h1>
    <p>Phone control</p>
    <div class="grid">
      <button class="happy" onclick="send('HAPPY 2500')">Happy</button>
      <button class="talk" onclick="send('TALK 4000')">Talk</button>
      <button class="sleep" onclick="send('SLEEP 3000')">Sleepy</button>
      <button class="wake" onclick="send('WAKE 1600')">Wake</button>
      <button class="happy" onclick="send('PAT 1800')">Pat</button>
      <button class="wake" onclick="send('CALL 2200')">Call</button>
    </div>
    <textarea id="text" placeholder="Type something for AIBI to perform"></textarea>
    <button class="wide" onclick="say()">Say</button>
    <div id="status">Ready</div>
  </main>
  <script>
    const status = document.getElementById('status');
    const text = document.getElementById('text');

    function durationFor(value) {
      const length = value.trim().length;
      return Math.min(12000, Math.max(1200, 800 + length * 75));
    }

    async function send(command) {
      status.textContent = 'Sending ' + command;
      try {
        const response = await fetch('/cmd?c=' + encodeURIComponent(command));
        status.textContent = await response.text();
      } catch (error) {
        status.textContent = 'Could not reach AIBI';
      }
    }

    function say() {
      send('TALK ' + durationFor(text.value));
    }
  </script>
</body>
</html>
)rawliteral";

void drawHappyEye(int centerX, int centerY) {
  for (int thick = 0; thick < 2; thick++) {
    display.drawLine(centerX - 12, centerY + 5 + thick, centerX - 5, centerY - 1 + thick, SSD1306_WHITE);
    display.drawLine(centerX - 5, centerY - 1 + thick, centerX, centerY - 3 + thick, SSD1306_WHITE);
    display.drawLine(centerX, centerY - 3 + thick, centerX + 5, centerY - 1 + thick, SSD1306_WHITE);
    display.drawLine(centerX + 5, centerY - 1 + thick, centerX + 12, centerY + 5 + thick, SSD1306_WHITE);
  }
}

void drawHappyEyes(int offsetX) {
  drawHappyEye(41 + offsetX, 31);
  drawHappyEye(91 + offsetX, 31);
}

void drawWakeEyes(int offsetX, int width, int height) {
  int leftX = 41 - (width / 2) + offsetX;
  int rightX = 91 - (width / 2) + offsetX;
  int eyeY = 31 - (height / 2);
  int pupilShift = constrain(offsetX / 3, -3, 3);

  display.fillRoundRect(leftX, eyeY, width, height, 8, SSD1306_WHITE);
  display.fillRoundRect(rightX, eyeY, width, height, 8, SSD1306_WHITE);

  display.fillRoundRect(leftX + (width / 2) - 3 + pupilShift, eyeY + (height / 2) - 5, 6, 10, 3, SSD1306_BLACK);
  display.fillRoundRect(rightX + (width / 2) - 3 + pupilShift, eyeY + (height / 2) - 5, 6, 10, 3, SSD1306_BLACK);

  display.fillCircle(leftX + (width / 2) + 1 + pupilShift, eyeY + (height / 2) - 2, 1, SSD1306_WHITE);
  display.fillCircle(rightX + (width / 2) + 1 + pupilShift, eyeY + (height / 2) - 2, 1, SSD1306_WHITE);

  display.drawLine(leftX - 2, eyeY - 4, leftX + 10, eyeY - 7, SSD1306_WHITE);
  display.drawLine(rightX + width - 10, eyeY - 7, rightX + width + 2, eyeY - 4, SSD1306_WHITE);
}

void drawClosedBlinkEyes(int offsetX, int width) {
  int leftX = 41 - (width / 2) + offsetX;
  int rightX = 91 - (width / 2) + offsetX;

  display.fillRoundRect(leftX, 31, width, 4, 2, SSD1306_WHITE);
  display.fillRoundRect(rightX, 31, width, 4, 2, SSD1306_WHITE);
}

void drawOpenEyesWithPupils(int offsetX, int width, int height) {
  int leftX = 41 - (width / 2) + offsetX;
  int rightX = 91 - (width / 2) + offsetX;
  int eyeY = 32 - (height / 2);
  int pupilShift = constrain(offsetX / 3, -3, 3);
  int pupilH = constrain(height - 10, 8, 14);

  display.fillRoundRect(leftX, eyeY, width, height, 8, SSD1306_WHITE);
  display.fillRoundRect(rightX, eyeY, width, height, 8, SSD1306_WHITE);

  display.fillRoundRect(leftX + (width / 2) - 3 + pupilShift, 32 - (pupilH / 2), 6, pupilH, 3, SSD1306_BLACK);
  display.fillRoundRect(rightX + (width / 2) - 3 + pupilShift, 32 - (pupilH / 2), 6, pupilH, 3, SSD1306_BLACK);
}

void drawMouth(bool happy, bool sleepy, bool attention, bool talking, bool waking) {
  if (sleepy) {
    display.drawFastHLine(58, 56, 12, SSD1306_WHITE);
  } else if (waking) {
    display.drawPixel(59, 55, SSD1306_WHITE);
    display.drawPixel(60, 56, SSD1306_WHITE);
    display.drawFastHLine(61, 57, 6, SSD1306_WHITE);
    display.drawPixel(67, 56, SSD1306_WHITE);
    display.drawPixel(68, 55, SSD1306_WHITE);
  } else if (talking) {
    if (mouthFrame == 0) {
      display.drawFastHLine(58, 55, 12, SSD1306_WHITE);
    } else if (mouthFrame == 1) {
      display.drawRoundRect(59, 52, 10, 7, 3, SSD1306_WHITE);
    } else {
      display.fillRoundRect(60, 51, 8, 9, 4, SSD1306_WHITE);
      display.fillRoundRect(62, 53, 4, 5, 2, SSD1306_BLACK);
    }
  } else if (happy) {
    display.drawPixel(54, 52, SSD1306_WHITE);
    display.drawPixel(55, 54, SSD1306_WHITE);
    display.drawPixel(56, 55, SSD1306_WHITE);
    display.drawFastHLine(57, 56, 14, SSD1306_WHITE);
    display.drawPixel(71, 55, SSD1306_WHITE);
    display.drawPixel(72, 54, SSD1306_WHITE);
    display.drawPixel(73, 52, SSD1306_WHITE);
    display.drawFastHLine(59, 57, 10, SSD1306_WHITE);
  } else if (attention) {
    display.drawCircle(64, 55, 3, SSD1306_WHITE);
  } else {
    display.drawFastHLine(60, 55, 8, SSD1306_WHITE);
  }
}

void drawEyes(int offsetX, int width, int height, bool happy, bool sleepy, bool attention, bool talking, bool waking, bool blinking) {
  display.clearDisplay();

  int leftX = 41 - (width / 2) + offsetX;
  int rightX = 91 - (width / 2) + offsetX;
  int eyeY = 32 - (height / 2);

  if (sleepy) {
    display.fillRoundRect(leftX, 31, width, 4, 2, SSD1306_WHITE);
    display.fillRoundRect(rightX, 31, width, 4, 2, SSD1306_WHITE);
  } else if (blinking && !happy) {
    drawClosedBlinkEyes(offsetX, width);
  } else if (waking) {
    drawWakeEyes(offsetX, width, height);
  } else if (happy) {
    drawHappyEyes(offsetX);
  } else {
    drawOpenEyesWithPupils(offsetX, width, height);
  }

  drawMouth(happy, sleepy, attention, talking, waking);
  display.display();
}

void wakeUp() {
  display.clearDisplay();
  display.display();
  delay(300);

  for (int h = 2; h <= 26; h += 2) {
    drawEyes(0, 20, h, false, false, false, false, false, false);
    delay(35);
  }

  drawEyes(0, 22, 26, false, false, false, false, true, false);
  delay(450);
}

void setup() {
  pinMode(TOUCH_PIN, INPUT);
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) {
      delay(100);
    }
  }

  randomSeed(analogRead(34));
  wakeUp();

  Serial.println();
  Serial.println("AIBI ready");
  Serial.println("Commands: HAPPY, TALK 4000, SLEEP 3000, WAKE, PAT, CALL");
  Serial.println("Shortcuts: h, t, s, w");
  startWifiControl();

  lastInteractionAt = millis();
  nextMicroActionAt = millis() + random(5000, 11000);
  nextLookAt = millis() + 1000;
  nextBlinkAt = millis() + 2200;
}

void markInteraction(unsigned long now) {
  lastInteractionAt = now;
  autoSmileUntil = 0;
  nextMicroActionAt = now + random(5000, 11000);
}

void runCommand(String command, unsigned long now) {
  command.trim();
  if (command.length() == 0) {
    return;
  }

  command.toUpperCase();

  int spaceIndex = command.indexOf(' ');
  String action = spaceIndex == -1 ? command : command.substring(0, spaceIndex);
  unsigned long value = 0;

  if (spaceIndex != -1) {
    value = command.substring(spaceIndex + 1).toInt();
  }

  if (action == "H" || action == "HAPPY") {
    unsigned long duration = value > 0 ? value : 2500;
    Serial.print("ok HAPPY ");
    Serial.println(duration);
    markInteraction(now);
    happyUntil = now + duration;
    attentionUntil = now + 900;
    wakeUntil = 0;
    serialSleepUntil = 0;
  } else if (action == "T" || action == "TALK") {
    unsigned long duration = value > 0 ? value : 4000;
    Serial.print("ok TALK ");
    Serial.println(duration);
    markInteraction(now);
    talkUntil = now + duration;
    attentionUntil = now + 1200;
    wakeUntil = 0;
    serialSleepUntil = 0;
  } else if (action == "S" || action == "SLEEP") {
    unsigned long duration = value > 0 ? value : 3000;
    Serial.print("ok SLEEP ");
    Serial.println(duration);
    markInteraction(now);
    serialSleepUntil = now + duration;
    happyUntil = 0;
    wakeUntil = 0;
    talkUntil = 0;
    attentionUntil = 0;
  } else if (action == "W" || action == "WAKE") {
    unsigned long duration = value > 0 ? value : 1400;
    Serial.print("ok WAKE ");
    Serial.println(duration);
    markInteraction(now);
    serialSleepUntil = 0;
    wakeUntil = now + duration;
    happyUntil = 0;
    talkUntil = 0;
    attentionUntil = now + duration;
  } else if (action == "P" || action == "PAT") {
    unsigned long duration = value > 0 ? value : 1800;
    Serial.print("ok PAT ");
    Serial.println(duration);
    markInteraction(now);
    serialSleepUntil = 0;
    happyUntil = now + duration;
    attentionUntil = now + 700;
    wakeUntil = 0;
    talkUntil = 0;
  } else if (action == "C" || action == "CALL") {
    unsigned long duration = value > 0 ? value : 2200;
    Serial.print("ok CALL ");
    Serial.println(duration);
    markInteraction(now);
    serialSleepUntil = 0;
    wakeUntil = now + duration;
    attentionUntil = now + duration;
    happyUntil = 0;
    talkUntil = 0;
  } else {
    Serial.print("unknown ");
    Serial.println(command);
  }
}

void handleRoot() {
  server.send_P(200, "text/html", CONTROL_PAGE);
}

void handleWebCommand() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "missing command");
    return;
  }

  String command = server.arg("c");
  runCommand(command, millis());
  command.trim();
  server.send(200, "text/plain", "AIBI: " + command);
}

void startWifiControl() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);

  server.on("/", handleRoot);
  server.on("/cmd", handleWebCommand);
  server.begin();

  Serial.print("WiFi name: ");
  Serial.println(WIFI_NAME);
  Serial.print("WiFi password: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("Open on phone: http://");
  Serial.println(WiFi.softAPIP());
}

void handleSerialCommands(unsigned long now) {
  while (Serial.available()) {
    char incoming = Serial.read();

    if (incoming == '\n' || incoming == '\r') {
      runCommand(serialLine, now);
      serialLine = "";
      continue;
    }

    serialLine += incoming;

    if (serialLine.length() > 40) {
      serialLine = "";
      Serial.println("error command too long");
    }
  }
}

void loop() {
  unsigned long now = millis();
  server.handleClient();
  handleSerialCommands(now);

  bool touched = digitalRead(TOUCH_PIN) == HIGH;

  if (touched && !lastTouch) {
    touchStartedAt = now;
    markInteraction(now);
    attentionUntil = now + 500;
  }

  if (!touched && lastTouch) {
    unsigned long pressTime = now - touchStartedAt;
    if (pressTime < 650) {
      markInteraction(now);
      happyUntil = now + 900;
      attentionUntil = now + 700;
      talkUntil = now + 1600;
      targetEyeHeight = 26;
    }
  }

  lastTouch = touched;

  unsigned long quietTime = now - lastInteractionAt;
  bool autoCurious = quietTime > 20000 && quietTime <= 55000;
  bool autoSleepy = quietTime > 55000;
  bool manualSleepyHold = (touched && (now - touchStartedAt > 650)) || now < serialSleepUntil;
  bool sleepyHold = manualSleepyHold || autoSleepy;
  bool waking = now < wakeUntil;
  bool attention = now < attentionUntil;
  bool talking = now < talkUntil;

  if (!sleepyHold && !waking && !attention && !talking && now > nextMicroActionAt) {
    int action = random(0, autoCurious ? 3 : 4);

    if (action == 0) {
      blinkUntil = now + random(160, 240);
    } else if (action == 1) {
      targetLookX = random(-16, 17);
    } else if (action == 2 && !autoCurious) {
      autoSmileUntil = now + random(700, 1200);
    }

    nextMicroActionAt = now + random(autoCurious ? 1800 : 5000, autoCurious ? 4200 : 12000);
  }

  bool happy = (now < happyUntil || now < autoSmileUntil) && !autoSleepy;

  if (sleepyHold) {
    targetEyeHeight = 5;
    targetEyeWidth = 24;
    targetLookX = 0;
  } else if (waking) {
    targetEyeHeight = 28;
    targetEyeWidth = 24;
    targetLookX = 0;
  } else if (attention || talking) {
    targetEyeHeight = 28;
    targetEyeWidth = 22;
    targetLookX = 0;
  } else if (autoCurious) {
    targetEyeHeight = 28;
    targetEyeWidth = 22;
  } else if (now < blinkUntil) {
    targetEyeHeight = 2;
    targetEyeWidth = 22;
  } else {
    int breathe = (now / 900) % 2;
    targetEyeHeight = breathe == 0 ? 26 : 24;
    targetEyeWidth = 20;
  }

  if (!sleepyHold && !attention && !talking && !waking && now > nextLookAt) {
    if (autoCurious) {
      targetLookX = random(-16, 17);
      nextLookAt = now + random(350, 950);
    } else {
      targetLookX = random(-12, 13);
      nextLookAt = now + random(1000, 2600);
    }
  }

  if (talking && now > nextMouthAt) {
    mouthFrame = random(0, 3);
    nextMouthAt = now + random(90, 180);
  }

  if (!sleepyHold && now > secondBlinkAt && secondBlinkAt != 0) {
    blinkUntil = now + random(140, 190);
    secondBlinkAt = 0;
  }

  if (!sleepyHold && now > nextBlinkAt) {
    blinkUntil = now + random(160, 220);
    if (random(0, 4) == 0) {
      secondBlinkAt = now + random(240, 340);
    }
    nextBlinkAt = now + random(autoCurious ? 1300 : 2600, autoCurious ? 3000 : 5600);
  }

  if (lookX < targetLookX) lookX++;
  if (lookX > targetLookX) lookX--;

  if (eyeHeight < targetEyeHeight) eyeHeight += 2;
  if (eyeHeight > targetEyeHeight) eyeHeight -= 2;
  eyeHeight = constrain(eyeHeight, 2, 28);
  if (eyeWidth < targetEyeWidth) eyeWidth++;
  if (eyeWidth > targetEyeWidth) eyeWidth--;
  eyeWidth = constrain(eyeWidth, 18, 24);

  bool blinking = now < blinkUntil;
  drawEyes(lookX, eyeWidth, eyeHeight, happy && !sleepyHold && !waking, sleepyHold, attention || autoCurious, talking && !sleepyHold, waking && !sleepyHold, blinking && !sleepyHold);
  delay(30);
}
