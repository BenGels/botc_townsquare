#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>
#include <AccelStepper.h>
#include <EEPROM.h>
#include "index_html.h"

const int stepsPerRevolution = 2 * 2048;

// Access Point: der ESP spannt sein eigenes WLAN auf.
// Name und Passwort stehen in secrets.h (siehe secrets.example.h).
#include "secrets.h"
const char* apSsid = AP_SSID;
const char* apPassword = AP_PASSWORD;
// Hostname für mDNS -> http://townsquare.local
const char* hostName = "townsquare";

// Webserver auf Port 80 (erreichbar unter http://townsquare.local bzw. http://192.168.4.1)
ESP8266WebServer server(80);
#define IN1 5
#define IN2 4
#define IN3 12
#define IN4 13

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepper(8, IN1, IN3, IN2, IN4);

// Das Getriebe des 28BYJ-48 hat Spiel. Der Zeiger fährt ein Ziel deshalb immer
// aus derselben Richtung an und überfährt es rückwärts um diese Schritte.
#define BACKLASH_STEPS 64
#define CONFIG_MAGIC 0x54535131

// Bleibt im EEPROM erhalten: Zeigerposition und Kalibrierung auf erste/letzte nutzbare LED.
struct HandConfig {
  uint32_t magic;
  long position;
  long first;
  long last;
};
HandConfig handConfig;
long handTarget = 0;
bool handDirty = false;
long calFirst = 0;
long calSpread = 0;

// Anzahl der LEDs im Ring
#define NUM_LEDS 58
// Die ersten und letzten LEDs des Rings bleiben immer aus
#define LED_MARGIN 4
#define USABLE_LEDS (NUM_LEDS - 2 * LED_MARGIN)
// Datenpin für die LEDs (D5 = GPIO 14)
#define DATA_PIN 14
// Anzahl der LEDs im Vote-Tower
#define TOWER_LEDS 10

#define MIN_PLAYERS 5
#define MAX_PLAYERS 20
#define MAX_NAME_LENGTH 64

// NeoPixel-Objekt erstellen
Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel voteTower(TOWER_LEDS, 2, NEO_GRB + NEO_KHZ800);
int playerCount = 0;
int blockedPlayer = -1;
// Farben für Farbenblinde: Gold gegen Blau, plus Helligkeit als zweites Merkmal
const uint32_t colorDead = strip.Color(0, 8, 24);
const uint32_t colorDeadVote = strip.Color(0, 90, 255);
const uint32_t colorAlive = strip.Color(255, 170, 30);
int currentMaxVotes = 0;
String names[MAX_PLAYERS];
boolean dead[MAX_PLAYERS];
boolean ghostvoted[MAX_PLAYERS];


void setup() {
  Serial.begin(9600);
  delay(3000);

  strip.begin();
  strip.show();
  strip.setBrightness(255);

  voteTower.begin();
  voteTower.setBrightness(255);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  if (WiFi.softAP(apSsid, apPassword)) {
    Serial.print("Access Point gestartet: ");
    Serial.println(apSsid);
  } else {
    Serial.println("Access Point konnte nicht gestartet werden");
  }
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/state", sendState);
  server.on("/set", handleSetLED);
  server.on("/setup", handleSetupGame);
  server.on("/name", handleSaveName);
  server.on("/status", handleSetStatus);
  server.on("/add", handleAddPlayer);
  server.on("/remove", handleRemovePlayer);
  server.on("/kill", handleKillPlayer);
  server.on("/block", handleBlockPlayer);
  server.on("/deadvote", handleDeadVote);
  server.on("/resurrect", handleResurrect);
  server.on("/degree", handleDegree);
  server.on("/reset", handleReset);
  server.on("/resetStepper", handleResetStepper);
  server.on("/increaseMaxVote", handleIncVote);
  server.on("/setVotes", handleSetVotes);
  server.on("/handjog", handleHandJog);
  server.on("/calibrate", handleCalibrate);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP-Server gestartet");

  if (MDNS.begin(hostName)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS gestartet: http://" + String(hostName) + ".local");
  } else {
    Serial.println("mDNS konnte nicht gestartet werden");
  }

  initCircle();
  initTower();

  stepper.setMaxSpeed(1000.0);
  stepper.setAcceleration(50.0);
  stepper.setSpeed(200);
  loadHandConfig();
}

void loop() {
  updateHand();
  stepper.run();
  server.handleClient();
  MDNS.update();
  delay(1);
}

void handleRoot() {
  // Nie aus dem Browser-Cache laden, sonst läuft nach einem Update noch die alte Seite
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", INDEX_HTML);
}

// Liest einen Platz-Parameter und prüft, ob dort ein Spieler sitzt.
bool readSeat(const char* argName, int& seat) {
  if (!server.hasArg(argName)) {
    server.send(400, "text/plain", "Fehlende Parameter");
    return false;
  }
  seat = server.arg(argName).toInt();
  if (seat < 0 || seat >= playerCount) {
    server.send(400, "text/plain", "Ungültiger Platz");
    return false;
  }
  return true;
}

String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if ((unsigned char)c < 0x20) {
      out += ' ';
    } else {
      out += c;
    }
  }
  return out;
}

// 0 = lebendig, 1 = tot (Geisterstimme noch da), 2 = Geisterstimme verbraucht
int playerStatus(int seat) {
  if (ghostvoted[seat]) return 2;
  if (dead[seat]) return 1;
  return 0;
}

// Alle Endpunkte der Oberfläche antworten mit dem kompletten Spielstand.
void sendState() {
  String json = "{\"count\":" + String(playerCount) +
                ",\"votes\":" + String(currentMaxVotes) +
                ",\"hand\":" + String(blockedPlayer) +
                ",\"min\":" + String(MIN_PLAYERS) +
                ",\"max\":" + String(MAX_PLAYERS) +
                ",\"players\":[";
  for (int i = 0; i < playerCount; i++) {
    if (i > 0) json += ',';
    json += "{\"name\":\"" + jsonEscape(names[i]) + "\",\"state\":" + String(playerStatus(i)) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleSaveName() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Fehlende Parameter");
    return;
  }
  int seat;
  if (!readSeat("seat", seat)) return;
  names[seat] = server.arg("name").substring(0, MAX_NAME_LENGTH);
  Serial.println("Seat " + String(seat) + " has name: " + names[seat]);
  sendState();
}

void handleSetStatus() {
  int seat;
  if (!readSeat("player", seat)) return;
  switch (server.arg("state").toInt()) {
    case 0:
      dead[seat] = false;
      ghostvoted[seat] = false;
      setPlayerToColor(seat, colorAlive);
      break;
    case 1:
      ghostvoted[seat] = false;
      // Nur ein echter Tod setzt den Vote-Tower zurück, nicht das Zurücknehmen der Geisterstimme
      if (!dead[seat]) killPlayer(seat);
      else setPlayerToColor(seat, colorDeadVote);
      break;
    case 2:
      deadVote(seat);
      break;
    default:
      server.send(400, "text/plain", "Ungültiger Status");
      return;
  }
  sendState();
}

void clearSeat(int seat) {
  names[seat] = "";
  dead[seat] = false;
  ghostvoted[seat] = false;
}

// Nach Änderung der Spielerzahl LEDs und Zeiger neu ausrichten.
void applyPlayerCount() {
  drawBoard();
  if (blockedPlayer >= 0) setClockhandToPlayer(blockedPlayer);
}

void handleAddPlayer() {
  if (!server.hasArg("at")) {
    server.send(400, "text/plain", "Fehlende Parameter");
    return;
  }
  int at = server.arg("at").toInt();
  if (at < 0 || at > playerCount) {
    server.send(400, "text/plain", "Ungültiger Platz");
    return;
  }
  if (playerCount >= MAX_PLAYERS) {
    server.send(400, "text/plain", "Maximal " + String(MAX_PLAYERS) + " Spieler");
    return;
  }
  for (int i = playerCount; i > at; i--) {
    names[i] = names[i - 1];
    dead[i] = dead[i - 1];
    ghostvoted[i] = ghostvoted[i - 1];
  }
  clearSeat(at);
  playerCount++;
  if (blockedPlayer >= at) blockedPlayer++;
  applyPlayerCount();
  sendState();
}

void handleRemovePlayer() {
  int seat;
  if (!readSeat("player", seat)) return;
  if (playerCount <= MIN_PLAYERS) {
    server.send(400, "text/plain", "Mindestens " + String(MIN_PLAYERS) + " Spieler");
    return;
  }
  for (int i = seat; i < playerCount - 1; i++) {
    names[i] = names[i + 1];
    dead[i] = dead[i + 1];
    ghostvoted[i] = ghostvoted[i + 1];
  }
  playerCount--;
  clearSeat(playerCount);
  if (blockedPlayer == seat) blockedPlayer = -1;
  else if (blockedPlayer > seat) blockedPlayer--;
  applyPlayerCount();
  sendState();
}

void handleDegree() {
  if (server.hasArg("steps")) {
    moveHandTo(handTarget + server.arg("steps").toInt());
  }
  server.send(200, "text/plain", "OK");
}

void loadHandConfig() {
  EEPROM.begin(sizeof(HandConfig));
  EEPROM.get(0, handConfig);
  if (handConfig.magic != CONFIG_MAGIC) {
    handConfig.magic = CONFIG_MAGIC;
    handConfig.position = 0;
    handConfig.first = (long)stepsPerRevolution * LED_MARGIN / NUM_LEDS;
    handConfig.last = (long)stepsPerRevolution * (NUM_LEDS - LED_MARGIN - 1) / NUM_LEDS;
  }
  stepper.setCurrentPosition(handConfig.position);
  handTarget = handConfig.position;
}

// Zeigerposition speichern, sobald er steht: Nach dem Ausschalten weiß er so noch, wo er ist.
void saveHandConfig() {
  handConfig.position = stepper.currentPosition();
  EEPROM.put(0, handConfig);
  EEPROM.commit();
  handDirty = false;
}

void moveHandTo(long target) {
  handTarget = target;
  handDirty = true;
  stepper.moveTo(target < stepper.currentPosition() ? target - BACKLASH_STEPS : target);
}

void updateHand() {
  if (stepper.distanceToGo() != 0) return;
  if (stepper.currentPosition() != handTarget) stepper.moveTo(handTarget);
  else if (handDirty) saveHandConfig();
}

long stepsForLed(int led) {
  return handConfig.first + (long)(led - LED_MARGIN) * (handConfig.last - handConfig.first) / (USABLE_LEDS - 1);
}

// Ruhestellung im LED-freien Bereich
void moveHandToRest() {
  moveHandTo(stepsForLed(0));
}

void handleHandJog() {
  if (!server.hasArg("steps")) {
    server.send(400, "text/plain", "Fehlende Parameter");
    return;
  }
  moveHandTo(handTarget + server.arg("steps").toInt());
  sendState();
}

void showOnlyLed(int led) {
  strip.clear();
  strip.setPixelColor(led, strip.Color(255, 255, 255));
  strip.show();
}

void finishCalibration() {
  drawBoard();
  if (blockedPlayer >= 0) setClockhandToPlayer(blockedPlayer);
  else moveHandToRest();
}

// Ablauf: first -> Zeiger auf erste LED stellen, last -> auf letzte LED stellen, done -> speichern.
void handleCalibrate() {
  String phase = server.arg("phase");
  if (phase == "first") {
    calSpread = handConfig.last - handConfig.first;
    showOnlyLed(LED_MARGIN);
    moveHandTo(handConfig.first);
  } else if (phase == "last") {
    calFirst = handTarget;
    showOnlyLed(NUM_LEDS - LED_MARGIN - 1);
    moveHandTo(calFirst + calSpread);
  } else if (phase == "flip") {
    calSpread = -calSpread;
    moveHandTo(calFirst + calSpread);
  } else if (phase == "done") {
    if (abs(handTarget - calFirst) < USABLE_LEDS) {
      server.send(400, "text/plain", "Zeiger steht zu nah an der ersten LED");
      return;
    }
    handConfig.first = calFirst;
    handConfig.last = handTarget;
    finishCalibration();
  } else if (phase == "cancel") {
    finishCalibration();
  } else {
    server.send(400, "text/plain", "Ungültiger Schritt");
    return;
  }
  sendState();
}

void handleIncVote() {
  Serial.println("Incoming Request handleIncVote");
  setMaxVote(currentMaxVotes + 1);
  sendState();
}

void handleSetVotes() {
  if (!server.hasArg("votes")) {
    server.send(400, "text/plain", "Fehlende Parameter");
    return;
  }
  setMaxVote(constrain(server.arg("votes").toInt(), 0, MAX_PLAYERS));
  sendState();
}

void handleResetStepper() {
  stepper.setCurrentPosition(0);
  handTarget = 0;
  handDirty = true;
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  resetGame();
  sendState();
}


void initCircle() {
  for (int i = LED_MARGIN; i < NUM_LEDS - LED_MARGIN; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
    strip.show();
    delay(20);
    strip.setPixelColor(i, strip.Color(0, 0, 0));
    strip.show();
  }
}

void initTower() {
  for (int i = 0; i < TOWER_LEDS; i++) {
    voteTower.setPixelColor(i, voteTower.Color(255, 255, 255));
    voteTower.show();
    delay(100);
    voteTower.setPixelColor(i, voteTower.Color(0, 0, 0));
    voteTower.show();
  }
}

// Bis 10 Stimmen: eine rote LED pro Stimme.
// Ab 11 Stimmen: unterste LED weiß (= 10), darüber eine rote LED pro weiterer Stimme.
void setMaxVote(int voteCount) {
  if (voteCount < 0) voteCount = 0;
  currentMaxVotes = voteCount;

  int remaining = voteCount;
  int startWith = 0;
  if (remaining > TOWER_LEDS) {
    voteTower.setPixelColor(0, voteTower.Color(255, 255, 255));
    remaining -= TOWER_LEDS;
    startWith = 1;
  }
  for (int i = startWith; i < TOWER_LEDS; i++) {
    if (i - startWith < remaining) {
      voteTower.setPixelColor(i, voteTower.Color(255, 0, 0));
    } else {
      voteTower.setPixelColor(i, voteTower.Color(0, 0, 0));
    }
  }
  voteTower.show();
}


// LED-Position eines Spielers: gleichmäßig von der ersten bis zur letzten nutzbaren LED, gerundet
int ledForPlayer(int playerID) {
  if (playerCount <= 1) return LED_MARGIN;
  int gaps = playerCount - 1;
  return LED_MARGIN + (playerID * (USABLE_LEDS - 1) + gaps / 2) / gaps;
}

String setPlayerToColor(int playerID, uint32_t color) {
  int seat = ledForPlayer(playerID);
  String message = "Set light for player " + String(playerID) + " on seat " + String(seat) + " auf Farbe =" + String(color) +
                   " gesetzt.";
  Serial.println(message);
  strip.setPixelColor(seat, color);
  strip.show();
  return message;
}

void handleResurrect() {
  int seat;
  if (!readSeat("player", seat)) return;
  dead[seat] = false;
  ghostvoted[seat] = false;
  setPlayerToColor(seat, colorAlive);
  sendState();
}

void handleDeadVote() {
  int seat;
  if (!readSeat("player", seat)) return;
  deadVote(seat);
  sendState();
}

void deadVote(int seat) {
  dead[seat] = true;
  ghostvoted[seat] = true;
  setPlayerToColor(seat, colorDead);
}

void handleKillPlayer() {
  int seat;
  if (!readSeat("player", seat)) return;
  killPlayer(seat);
  sendState();
}

void killPlayer(int seat) {
  dead[seat] = true;
  setMaxVote(0);
  setPlayerToColor(seat, colorDeadVote);
}

void handleBlockPlayer() {
  int seat;
  if (!readSeat("player", seat)) return;
  blockedPlayer = seat;
  setClockhandToPlayer(seat);
  sendState();
}

// Zielposition direkt aus der LED-Position berechnen, damit sich kein Rundungsfehler aufsummiert.
void setClockhandToPlayer(int playerID) {
  moveHandTo(stepsForLed(ledForPlayer(playerID)));
}

// Zeichnet alle Plätze entsprechend ihrem Status neu.
void drawBoard() {
  for (int r = 0; r < NUM_LEDS; r++) {
    strip.setPixelColor(r, strip.Color(0, 0, 0));
  }
  const uint32_t colors[] = { colorAlive, colorDeadVote, colorDead };
  for (int i = 0; i < playerCount; i++) {
    strip.setPixelColor(ledForPlayer(i), colors[playerStatus(i)]);
  }
  strip.show();
}

void resetGame() {
  for (int o = 0; o < MAX_PLAYERS; o++) {
    clearSeat(o);
  }
  blockedPlayer = -1;
  moveHandToRest();
  setMaxVote(0);
  drawBoard();
}

void handleSetupGame() {
  if (!server.hasArg("player")) {
    server.send(400, "text/plain", "Fehlende Parameter");
    return;
  }
  playerCount = constrain(server.arg("player").toInt(), MIN_PLAYERS, MAX_PLAYERS);
  blockedPlayer = -1;
  moveHandToRest();
  setMaxVote(0);
  applyPlayerCount();
  sendState();
}

void handleSetLED() {
  if (server.hasArg("led") && server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    int ledIndex = server.arg("led").toInt();
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();

    if (ledIndex >= LED_MARGIN && ledIndex < NUM_LEDS - LED_MARGIN) {
      strip.setPixelColor(ledIndex, strip.Color(r, g, b));
      strip.show();
      String message = "LED " + String(ledIndex) + " auf Farbe R=" + String(r) +
                       ", G=" + String(g) + ", B=" + String(b) + " gesetzt.";
      server.send(200, "text/plain", message);
    } else {
      server.send(400, "text/plain", "Ungültige LED-Nummer");
    }
  } else {
    server.send(400, "text/plain", "Fehlende Parameter");
  }
}

// 404-Seite
void handleNotFound() {
  server.send(404, "text/plain", "Seite nicht gefunden");
}
