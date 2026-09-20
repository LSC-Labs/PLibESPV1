## Module
A module is a functionality that can be enabled in your application at design time.

It support standard interfaces of the runtime and supports in principal 2 areas.
1. The code that is running on the device.
2. The web frontend.

### Code running on the device
The runtime offers several interfaces to the module code running on the device.

|Interface|Mandatory|Description
|-- | -- |--
|IConfigHandler| no |The configuration of the module.
|IStatusHandler| no |The current status of the module.
|IMsgEventReceiver   | no |Sending and receiving information via the message bus.

### Web frontend
A module offers a "page" that consists of the parts:

|Part|Description
|-- |--
|page.html| The html code of the page.
|page.js | The code supporting the page.
|page.css | Stylesheets used by the page.
| i18n/xx.json | Language files of the html code.

