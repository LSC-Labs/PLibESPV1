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
- Multi language support.
- Authentication (basic) functions.
- Security functions like en/decryption.
- Authentication and authorization on REST, Socket or web GUI.
- Backup and Restore the settings
- Over the air update for your device.
- Modulare Plugable architecture, so you can select which modules you want to use and insert your own modules.
- Basic services like buttons and LED's

## Default Modules
- WiFi with Access Point and Station Mode for initial configuration or Stand Alone operating.
- Multicast DNS service (Apple Bonjour) to simple publish your service.
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


### The Application
The Application coordinates the communication and does the most work for you.

To plug into the application you have to write a "Module" and register the available
interface in the application object.

#### On the device
The Application is the global object "App" as in instance of CAppl.

It offers a message bus, an interface for configuration and an interface for status information.

##### Message Bus
Every module or component can use the Message Bus to send or receive messages as events.

If a module needs to be informed about these events, it has to register on the Applications Message Bus.
- Send and receive messages from Websocket
- Status changes from other modules
- Own module message

The messages will be sent synchron, and each module has the chance to stop the further processing and 
send a "feedback". So a module can stop a reboot if it is currently not the best time to do so.

##### Config Interface
When a module registers itself with a (unique) key, the key will be used
to build a separate area in the main configuration object and the module will be inserted into
the config file processing (readConfig()/writeConfig()).

Everytime, when the application is asked to load or to save the device configuration,
the registered modules interface is called to fill in or to read the configuration by handling
the interface this area (JSON Object)

##### Status Interface
The key, that is used on registration of the module is used
to build a separate area in the main status object.

When the application needs the current status of the module (i.E. when the GUI as asking for),
the module will be asked to fill in it's status.

#### Web GUI
In the GUI, the Application is an instance of CAppl and coordinates the configuration and status
pages and offers a websocket communication with the device.



### Writing a Module
A module consists of the runtime and Config/ Status Web Pages.

Each module uses a unique key to exchange status und config information.

#### Runtime
At the device, write your logic and support the interfaces
- IConfigHandler for configuration
- IStatusHandler for status information
- IMessageEventReceiver to participate from system messages
- and send messages if needed, via the Application Message Bus


#### Web Page / User Frontend
The Web Page can have the following components:
- The HTML description of the Web.
- Menu entries for the navigation bar.
- Java script to enhance the HTML page.
- Language files for multi language support.


A Module registers itself on a main object, called "Appl" with an id that should be unique. The WiFi module i.E. is using the id "wifi". This id is used by the applicationto reserve this area for the module. All config, statis and other infos are stored in this "area".

The id will become also the key to the web GUI, as the settings and the statis for this module will be sent to the GUI within this areas.

## Where to go from here...
If you like, you can use the [ESP-ProjectTemplate](https://github.com/LSC-Labs/ESP-ProjectTemplate) template.

This is a template that is using this library and supports you with additional scripts in context to develop and prepares most of the environmet for you:
- Visiual Studio Code
- PlatformIO Development Environment
- This library as a runtime lib
