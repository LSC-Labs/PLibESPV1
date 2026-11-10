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
        "includes" : "include/web"
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



## Sync files (`npm run sync-RuntimeFiles`)
`scripts/syncFiles.js` processes the instructions in `sync.files` of pages.json: `"<source> <cmd> <target>"`.
The source is searched in the project first, then in `sync.locations` (default `.` and `lib/PLibESPV1`).

|cmd|Description|
|--|--|
|`=`|copy only, if the target does not exist (initial copy, i.e. templates)|
|`>`|copy source to target, if the source is newer|
|`<`|copy the target back to the source, if the target is newer|
|`<>`|copy in the direction of the newer file|

If the source is a **folder**, all files of the folder (including sub folders) are synced into the
target folder with the same command, e.g. to take over a page of the library into the project:

    "lib/PLibESPV1/src/web/pages/Status  =  src/web/pages/Status"

Each file is reported with its result:
🆕 new (copied) · 🔄 updated (overwritten) · ⬅️ copied back · ✅ up to date · ⏭️ kept (`=`, target exists) · ⚠️ source missing · ❌ error.
A summary is printed at the end, the exit code is 1 if an error occurred.
