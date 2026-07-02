# PLibESPV1 Runtime Library

PLibESPV1 stellt wiederverwendbare Runtime-Bausteine fuer ESP-basierte
Anwendungen bereit. Die Library buendelt Konfiguration, Statusausgabe,
Event-Bus, Logging, Netzwerkdienste, WebSocket/WebServer, MQTT, einfache
JSON-Verarbeitung, Security-Helfer und Hardware-nahe Controller.

## Einstiegspunkte

- `CAppl` ist der zentrale Anwendungshub fuer Module, Konfiguration, Status,
  Logging und den Message-Bus.
- `IModule`, `IConfigHandler`, `IStatusHandler` und `IMsgEventReceiver`
  beschreiben die wichtigsten Erweiterungspunkte.
- `CJsonNode`, `CVarTable`, `CNamedValueTable` und `LSCUtils` bilden die
  allgemeinen Utility-Bausteine.
- `CWiFiController`, `CWebServer`, `CWebSocket`, `CMQTTController`,
  `CNTPHandler` und `CMDNSController` kapseln Netzwerkfunktionen.
- `COutputPinController`, `CInputPinController`, `CLightSwitch`, `CRGBLed`,
  `CButton` und `CBatteryMeasure` decken typische Hardware-Interaktionen ab.

## Generierte Formate

Diese Runtime-Dokumentation wird aus Headern, Source-Dateien und vorhandenen
Markdown-Seiten erzeugt:

- HTML: `doc/runtime/html/index.html`
- Markdown: `doc/runtime/markdown/PLibESPV1_Runtime_Library.md`
- PDF: `doc/runtime/pdf/PLibESPV1_Runtime_Library.pdf`

Die Markdown- und PDF-Dateien werden aus der Doxygen-XML-Ausgabe abgeleitet,
damit alle Formate auf derselben API-Basis beruhen.
