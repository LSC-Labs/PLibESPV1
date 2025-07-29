# PLibESPV1
ESP Runtime library

As the smart devices need to be initialized and to be configured, some solutions are available in the net that are more or less without an interface. So you have to insert the configuration in an arduino sketch or use automatic systems to deploy images with the configuration hardcoded in the source.

If a web interface should come in place, things become complicated, cause you have to build the webservice, the sockets and the intial configuration of your wifi - and you have to take care about the credentials you are using.

This library tries to simplify this basic coding, so you can concentrate on building your service.

## Functionality
The library offers you:

- Wifi access point with a web gui, so you can initialize the device with the wifi credentials, without coding it into the source code.
- An admin web interface, so you can configure your device with all needed settings.
- Web GUI with user/public functionality.
- Authentication (basic) functions.
- Security functions like en/decryption.
- Authentication and authorization on REST, Socket or web GUI.
- Backup and Restore the settings
- Over the air update for your device.
- Plugable architecture, so you can select which modules you want to use and insert your own modules.
- Basic services like buttons and LED's

## Default Modules
- WiFi with Access Point and Station Mode for initial configuration or Stand Alone operating.
- MQTT service to connect to an existing message broker service, to be able to send and receive heartbeats or commands.
- HTTP Web service to handle the admin web pages and to offer a REST api.
- Websocket service for realtime communication with your device
- REST API to setup and update the device.
- RF433 remote controls (simple commands)
- LED module, to control a simple (RGB) LED's with PWM, wave or blink commands.
- Button module with enhanced functions like interrupt triggers.


## Architecture
All communication between the modules is done by a message bus system.
In addition there are some interfaces to follow for configuration and statis in place, the modules will follow.

### A Module
A Module registers itself on a main object, called "Appl" with an id that should be unique. The WiFi module i.E. is using the id "wifi". This id is used by the applicationto reserve this area for the module. All config, statis and other infos are stored in this "area".

The id will become also the key to the web GUI, as the settings and the statis for this module will be sent to the GUI within this areas.



### The Appl
The Appl is a global instance of the class CAppl and is the coordinator for the communication and functions.

The Appl is offering a message bus system and a standard
The library uses a common instance of the class CAppl, the "Appl"