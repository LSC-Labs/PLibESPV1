/**
 * Update version info
 * gets the prog name and version info from package.json and updates the settings.js
 * It is part of the PLibESPV1 project.
 * 
 * @file scripts/syncFiles.js
 * @author LSC Labs - Peter Liebl
 * @version 1.0.8
 */

// const process = require('process');
import process from 'process';
import fs from 'fs';
import path from 'path';
import { CConfig, getProjectName,Utils } from './_common.js';

const MODULE_NAME = "updateVersionInfos";
const MODULE_VERSION = "1.0.7";


var oLocations = {
    "appsetting": path.join("src","web","js","settings.js"),
    "versionfile": path.join("data","version.json"),
    "packagefile": "package.json",
    "test": {
        statusFiles: 
        [
            path.join("test","data","status.json"),
            path.join("src","web","apiV1","status")

        ]
    }
}

var oPackageData = {
    "name": "-",
    "version": "0.0.0.1"
}

/**
 * Version Data (initial without name)
 * if name is not in place, the root of the project name will be used,
 * otherwise, it will not be touched here...
 * Version numbers will be handled:
 * major, minor and patch will be used from package information..
 * build number will be increased
 */
var oVersionData = {
	"major": 0,
	"minor": 1,
	"patch": 0,
	"build": 0
}

function loadVersionFile() {
    let bResult = false;
    let strFile = oLocations.versionfile;
     if( fs.existsSync(strFile)) {
        console.log("---> using version file: " + strFile)
        let strData = fs.readFileSync(strFile);
        oVersionData = JSON.parse(strData);
        bResult = true;
    } else {
        console.log("[E] version file not found : " + strFile)
    }
    return(bResult);
}

function loadProjectPackageFile() {
    let bResult = false;
    let strFile = oLocations.packagefile;
    if( fs.existsSync(strFile)) {
        console.log("---> using project package file: " + strFile)
        let strData = fs.readFileSync(strFile);
        oPackageData = JSON.parse(strData);
        bResult = true;
    } else {
        console.log("[E] project package file not found : " + strFile)
    }
    return(bResult);
}


function getLongVersionString() {
    return(oVersionData.major + "." + oVersionData.minor + "." + oVersionData.patch + "." + oVersionData.build);
}

function showCurrentVersion() {
    console.log(" - Prog Name : " + oVersionData.name);
    console.log(" - Version   : " + getLongVersionString());
}



function updateVersionFile() {
    console.log("---> update version file...")
    // set name only, if not already in place...
    if(!oVersionData.name) {
       oVersionData.name = path.basename(process.cwd());
    }
    // User major/minor/patch from package file
    // and increment the build number...
    let tVersion = oPackageData.version.split('.');
    oVersionData.major = tVersion[0];
    oVersionData.minor = tVersion[1];
    oVersionData.patch = tVersion[2];
    oVersionData.build++;

    // Write to filesystem - pretty print 
    let strJson = JSON.stringify(oVersionData,null,4);
    fs.writeFileSync(oLocations.versionfile, strJson);
}

function updateSettingFile() {
    console.log("---> update setting file...")
    let strFile = oLocations.appsetting;
    if(fs.existsSync(strFile)) {
        let strContent = fs.readFileSync(strFile).toString('utf-8');
        let tFileData = strContent.split(/\r?\n/);
        let nLineNumber = 1;

        var oWS = fs.createWriteStream(strFile);
        oWS.on('error', function(err) { console.log("Error on writing to " + strFile); console.log(err); });
        tFileData.forEach(function(strLine) { 
            if(strLine.match(/\"prog_name\"\s*:\s*\"([\.\-_A-Za-z0-9\s]*)\"/)) {
                strLine = strLine.substring(0, strLine.indexOf(":")) + ":" + '"' + oVersionData.name + `",`;
            }
            if(strLine.match(/\"prog_ver\"\s*:\s*\"([\.\-_A-Z0-9]*)\"/)) {
                strLine = strLine.substring(0, strLine.indexOf(":")) + ":" + '"' + oPackageData.version + `",`;
            }
            oWS.write(strLine + '\n'); 
        });
        oWS.end();

    } else {
        console.log("[E] - app setting file '" + strFile + "' not found...");
    }
}

function updateTestStatusFile(strFile) {
    if(!strFile) strFile = oLocations.test.statusFiles[0]; 
    console.log("---> update test status file : " + strFile);
    if(fs.existsSync(strFile)) {
        let strData = fs.readFileSync(strFile);
        let oTestData = JSON.parse(strData);
        oTestData.prog_name = oVersionData.name;
        oTestData.prog_version = getLongVersionString() + "-T";

        
        // Write to filesystem - pretty print 
        let strJson = JSON.stringify(oTestData,null,4);
        fs.writeFileSync(strFile, strJson);

    } else {
        console.log("[E] - app setting file '" + strFile + "' not found...");
    }
}



/**
 * build the pages from page definitions
 * 
 * @param {function()} cb        as running in context of gulp, this is a callback
 * @param {CConfig | object } oSettings user settings
 */
export async function runUpdateVersionInfos(cb, oSettings) {
    console.log("---- update version info files....");
    if(loadPackageData()) {

    } else {
        console.log("[E] package file not found: " + PACKAGE_FILE)
    }
    if(Utils.isFunction(cb)) cb();
}



console.log("********************************************");
console.log("---- " + MODULE_NAME + " Version " + MODULE_VERSION);
console.log("********************************************");
if(loadProjectPackageFile() && loadVersionFile()) {
    updateVersionFile();   
    updateSettingFile();
    oLocations.test.statusFiles.forEach(strFile => updateTestStatusFile(strFile));
}
console.log("==> udpated to Version:");
console.log("--------------------------------------------");
showCurrentVersion();
console.log("--------------------------------------------");
