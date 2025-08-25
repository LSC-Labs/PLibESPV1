## Page Builder
The script helps you to manage your web pages and to keep standard files in sync with your project.

### Web Page
A web page consists of several parts that will be joined and inserted in your project with this script.

- The html description
- An optional java script for the page
- Language definitions

You include this pages by inserting a pages.json file in the root of your directory.

Properties of pages.json


 "locations": {
        "webSource": "src/web",
        "webDist"  : "dist/web",
        "webPacked": "dist/tmp",
        "includes" : "include"
    },

| Section | Property | Type | Description| 
|-- |--- |-- |---|
|usePages||Array[string]| Define here, which page definitions/functionalities should be available in your application front end. 
|sync||object|Elements to be synced
||files|Array[string]|Array with sync commands like "source > dest" 
|locations| |object|Defines locations of elements
||webSource | string | Base directory, where the web elements are stored. 
||webDist   | string | Web server area. Generated files will be stored here, so they can be distributed to a web server.
||webPacked | string | The generated web files in a compressed form (gzip).
||includes | string |c++ include directory, to be used to include the compiled files into the source code.


