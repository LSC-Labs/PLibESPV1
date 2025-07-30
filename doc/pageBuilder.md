## Page Builder
The script helps you to manage your web pages and to keep standard files in sync with your project.

### Web Page
A web page consists of several parts that will be joined and inserted in your project with this script.

- The html description
- An optional java script for the page
- Language definitions

You include this pages by inserting a pages.json file in the root of your directory.

Properties of pages.json
| Section | Property | Description| 
|-- |--- |---|
|out|| Properties to control the page generation
||pageFile | All html files are written in this file
||pageScript | All java script page parts | will be written in this file
||i18nLoc | A directory where the language files will be written.
||writePageRegistration | All found pages will be registered at the main application 


The defaut pages are stored in "src/web/pages".
- WiFi Page

