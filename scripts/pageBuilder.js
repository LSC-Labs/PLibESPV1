/**
 * Page Builder
 * 
 * A page definition contains :
 *  - the page definition as html (page.html),
 *  - necessary javascripts       (page.js),
 *  - language definitions        (i18n/*.json)
 * 
 * 
 * As the pages defines also the structure of the menu,
 * it is necessary to join them in the correct order (!)
 * 
 * Therefore a file "pages.json" in the root of the project is used.
 * 
 * {
 *      out: {
 *          path:        (default is src/web)
 *          pageFile:    (default is pages.html)
 *          pageScript:  (default is pages.js)
 *      }
 *      include: [
 *              "pathToPageDir",
 *              "pathToPages/*"
 *          ] 
 *              
 *              
 * }
 */
const fs = require("fs");
const path = require("path");

const CTRL_FILE = "pages.json";
const PAGE_HTML = "page.html";
const PAGE_SCRIPT = "page.js";
const PAGE_I18N = "i18n";

const DEFAULTS = {
    "out": {
        "path": "src/web",
        "pageFile": "pages.html",
        "pageScript": "pages.js",
        "i18nLoc" : "i18n"
    },
    "include": {},
    "getHtmlTargetName": function() {
        return(path.join(Settings.out.path,Settings.out.pageFile));
    },
    "getScriptTargetName": function() {
        return(path.join(this.out.path,this.out.pageScript));
    },
    "getLanguagePath": function() {
        let strPath = path.join(this.out.path,this.out.i18nLoc);
        if(!fs.existsSync(strPath)) fs.mkdirSync(strPath);
        return(strPath);
    }
}

var Settings = {};

function loadControlFile(strCtrlFile) {
    console.log(" - loading control file");
    let strCtrl = fs.readFileSync(strCtrlFile ?? CTRL_FILE)
    let oCtrl = JSON.parse(strCtrl);
    Settings = { ...DEFAULTS, ...oCtrl};
    Settings.out = {...DEFAULTS.out, ...oCtrl.out}
}

function joinHtmlPage(strPagePath) {
    let strInput = path.join(strPagePath,PAGE_HTML);
    if(fs.existsSync(strInput)) {
        let strOut = Settings.getHtmlTargetName();
        console.log("   -> appending HTML to : " + strOut);
        let strData = fs.readFileSync(strInput);
        fs.appendFileSync(strOut,strData);
    }
}

function joinScriptPage(strPagePath) {
    let strInput = path.join(strPagePath,PAGE_SCRIPT);
    if(fs.existsSync(strInput)) {
        let strOut = Settings.getHtmlTargetName();
        console.log("   -> appending Script to : " + strOut);
        let strData = fs.readFileSync(strInput);
        fs.appendFileSync(strOut,strData);
    }
}

function getJsonFromFile(strFile) {
    let oData = {};
    if(fs.existsSync(strFile)) {
        let strData = fs.readFileSync(strFile);
        oData = JSON.parse(strData);
    }
    return(oData);
}

function joinLanguages(strPagePath) {
    let strInputPath = path.join(strPagePath,PAGE_I18N); {
        let oRX = new RegExp("^[a-z]{2}\.json$")
        if(fs.existsSync(strInputPath)) {
            fs.readdirSync(strInputPath).forEach((strFile) => {
                if(oRX.test(strFile)) {
                    console.log("   -> joining language : " + strFile);
                    // Join the language....
                    let strTargetFile = path.join(Settings.getLanguagePath(),strFile);
                    let oTarget = getJsonFromFile(strTargetFile);

                    let strInputFile = path.join(strInputPath, strFile);
                    let oInput = getJsonFromFile(strInputFile);
                    let oResult = {...oTarget,...oInput}
                    fs.writeFileSync(strTargetFile,JSON.stringify(oResult,null,2));
                }
            });
        }
    }
}


function removeTargets() {
    if(fs.existsSync(Settings.getHtmlTargetName())) fs.rmSync(Settings.getHtmlTargetName())
    if(fs.existsSync(Settings.getScriptTargetName())) fs.rmSync(Settings.getScriptTargetName())
}


function processPages() {
    let strPath = Settings.out.path;
    if(!fs.existsSync(strPath)) fs.mkdirSync(strPath);
    for(let strName of Settings.include) {
        console.log(" - processing : " + strName);
        joinHtmlPage(strName);
        joinScriptPage(strName);
        joinLanguages(strName);
    }
}

console.log("=======================================");
console.log(" pageBuilder - create pages for project")
console.log("=======================================");
loadControlFile();
removeTargets();
processPages();
