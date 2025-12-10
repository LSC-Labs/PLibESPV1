# Classes

## WebSocket
The Websocket is part of the Webserver.

If you want to use Websocket communication, you have to instanciate the class CWebSocket,
register it at the WebSever object and call dispatch() in the loop.

As the Websocket runs async, an incomming message will be stored in the message queue of the CWebSocket object,
to avoid timeouts in the interrupt. @see also documentation of the AsyncWebserver documentation.


Per default, there are already some functionalities implemented in the dispatch() function that will be processed by default. The expected message is a json message in the format:
```
{
    "command" : "<command>"
    "token"   : "<AccessToken>"
    "payload" : { <The payload of the command> }
}
```

The command is not case sensitive. If an authentication is neede, the call needs a valid access token, which is generated due the signin phase. If a function is called without an authentication token, the command will be revoced with an access denied message.

|Command|Auth|Functionality|Payload
|-- |-- |-- |--
|getstatus|no|the status of the device (without sys information) as a json object. All your modules will have their own section inside this status data|the status as a json object
|getconfig|no|the device will send its configuration data, passwords are not shown (!)|the current configuration as a json object
|getbackup|yes|sends the configuration file with passwords in clear text (!). Be aware to keep this file on a save place to ensure your credentials (access, wifi, ...).|the config of the device with passwords
|saveconfig|yes|Known configuration values will be stored persistent in the device. The device will reboot after the data is persistent. Visible passwords are only inside, if the user (admin) changed the password to a new value|the changed configuration as a json object.
|restorebackup|yes|the payload will be the new configuration file. If passwords are inside, they have to be either in the hidden or in cleartext form.|the new configuration file.
|restart|yes|restarts the device | - none -
|factoryreset|yes|deletes the configuration file and starts, as it was in initial state (an access point will be opened)| - none -
|scanwifi|no|Sends "MSG_WIFI_SCAN" on the application message bus. The WiFi module recognizes this command on the message bus. As soon as the scan is finished, a new message will be sent on the message bus (async).
|scanrf433|no|Sends "MSG_RF433_SCAN" on the application message bus. The RF433 module recognizes this command on the message bus. As soon as the scan is finished, a new message will be sent on the message bus (async).

### Access control
Per default, there are 2 states, authenticated (administrator) or not to get access to a function.

To set which function is allowed to access, there are the following possibilities:
- define at compile time  "WS_NEEDS_AUTH"  with the string that contains the command names
- set the allowed commands at runtime via the function `setNeedsAuth(strFunctionNames)`.
To see which commands needs an authenticated user, use the `getNeedsAuth()`.


### Own commands
To implement or enhance the existing commands, 
override the function `dispatchMessage(WebSocketMessage *pMessage)`

