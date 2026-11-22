# BotC LED Townsquare

Ein Spielbrett-Helfer für *Blood on the Clocktower*: Ein ESP8266 steuert einen LED-Ring mit den Sitzplätzen, einen Vote-Tower aus LEDs und einen Schrittmotor-Zeiger für den Block. Bedient wird alles über eine Weboberfläche am Smartphone. Der ESP spannt dafür sein eigenes WLAN auf, Internet oder Router sind nicht nötig.

## Funktionen

- **Sitzkreis am Handy** im Clocktower-Stil: Tippen auf einen Spieler öffnet ein Fenster für Name, Status, Block, Einfügen und Entfernen.
- **Status pro Spieler:** lebendig, tot (Geisterstimme noch da), Geisterstimme verbraucht. Die Farben sind für Farbenblinde gewählt (Gold, Blau, schwaches Blau).
- **LED-Ring:** Die Spieler werden gleichmäßig über die nutzbaren LEDs verteilt. Die ersten und letzten 4 LEDs bleiben aus.
- **Vote-Tower:** Zeigt die höchste Stimmenzahl. Per +1 oder direkt setzbar, bei einem Tod automatisch auf 0.
- **Zeiger:** Ein Schrittmotor zeigt auf den Spieler auf dem Block. Er wird einmal kalibriert und merkt sich seine Position über Neustarts hinweg.
- **Spieler hinzufügen/entfernen** während des Spiels (5 bis 20 Spieler).

## Hardware

| Bauteil | Anschluss |
|---|---|
| ESP8266 (getestet mit NodeMCU-kompatiblem Board) | – |
| LED-Ring, 58 × WS2812/NeoPixel | Daten an GPIO 14 (D5) |
| Vote-Tower, 10 × WS2812/NeoPixel | Daten an GPIO 2 (D4) |
| Schrittmotor 28BYJ-48 mit ULN2003-Treiber | IN1 → GPIO 5, IN2 → GPIO 4, IN3 → GPIO 12, IN4 → GPIO 13 |

Anzahl der LEDs, Rand-LEDs und Pins stehen als Konstanten oben in [`townsquare/townsquare.ino`](townsquare/townsquare.ino).

## Einrichtung

1. **Zugangsdaten anlegen:** `townsquare/secrets.example.h` nach `townsquare/secrets.h` kopieren und Name und Passwort des WLANs eintragen. Das Passwort braucht mindestens 8 Zeichen. `secrets.h` ist in `.gitignore` und landet nicht im Repository.
2. **Abhängigkeiten installieren:** ESP8266-Boardpaket (`esp8266:esp8266`, getestet mit 3.1.2) und die Bibliotheken *Adafruit NeoPixel* und *AccelStepper*.
3. **Kompilieren und hochladen**, z. B. mit `arduino-cli` (Port anpassen):

   ```bash
   arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --upload --port /dev/ttyUSB0 townsquare
   ```

   Alternativ den Ordner `townsquare` in der Arduino IDE öffnen.

## Benutzung

1. Mit dem WLAN aus `secrets.h` verbinden.
2. **http://townsquare.local** öffnen (auf manchen Android-Geräten nur **http://192.168.4.1**).
3. Spielerzahl wählen und los geht's.

Bei der ersten Inbetriebnahme einmal **☰ → Zeiger kalibrieren** ausführen: Zeiger auf die weiß leuchtende erste LED stellen, dann auf die letzte, speichern. Neu kalibrieren ist nötig, wenn der Strom während einer Zeigerfahrt ausfällt oder der Zeiger von Hand verstellt wurde.

Hinweis: Namen und Status liegen nur im Arbeitsspeicher und gehen bei einem Neustart verloren. Die Zeigerposition und die Kalibrierung bleiben im EEPROM erhalten.

## HTTP-API

Alle Endpunkte sind `GET`-Anfragen. Die meisten antworten mit dem kompletten Spielstand als JSON, Fehler kommen als Text mit Status 400.

| Endpunkt | Wirkung |
|---|---|
| `/state` | Spielstand abfragen |
| `/setup?player=N` | Spiel mit N Spielern starten (Namen und Status bleiben erhalten) |
| `/name?seat=I&name=TEXT` | Namen setzen |
| `/status?player=I&state=S` | Status setzen: 0 lebendig, 1 tot, 2 Geisterstimme verbraucht |
| `/block?player=I` | Spieler auf den Block, Zeiger fährt hin |
| `/add?at=I` / `/remove?player=I` | Spieler einfügen / entfernen |
| `/increaseMaxVote` / `/setVotes?votes=N` | Vote-Tower +1 / direkt setzen |
| `/reset` | Namen, Status, Stimmen und Block zurücksetzen |
| `/handjog?steps=N`, `/calibrate?phase=…` | Zeiger verfahren und kalibrieren |

Sitze werden ab 0 gezählt, in der Oberfläche ab 1.

## Aufbau

- [`townsquare/townsquare.ino`](townsquare/townsquare.ino): Spiellogik, LEDs, Zeiger, Webserver
- [`townsquare/index_html.h`](townsquare/index_html.h): die komplette Weboberfläche (HTML, CSS, JavaScript), ohne externe Abhängigkeiten
- [`townsquare/secrets.example.h`](townsquare/secrets.example.h): Vorlage für die WLAN-Zugangsdaten
