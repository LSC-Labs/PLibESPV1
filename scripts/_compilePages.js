import fs from 'fs';
import path from 'path';
import gulp from 'gulp';
import gzip from 'gulp-gzip';
import debug from 'gulp-debug';
import htmlmin from 'gulp-htmlmin';
import uglify from 'gulp-uglify';
import cleancss from 'gulp-clean-css';
import jsonmin from 'gulp-jsonmin';
import flatmap from 'gulp-flatmap';
import pump from 'pump';
import { CConfig } from './_common.js';

const DEFAULTS = {
    "locations": {
        "webSource": "src/web",
        "webDist"  : "dist/web",
        "webPacked": "dist/tmp",
        "includes" : "dist/include"
    }
}
var Settings = new CConfig(DEFAULTS);


// #region Minimize functions

function minimizeHTML(cb) {
    let strSourcePath = Settings.getWebSourcePath() + "/*.html";
    let strTargetPath = Settings.getWebDistPath();
    let strPackedPath = Settings.getWebPackedPath();

    gulp.src(strSourcePath)
    .pipe(debug({ title: 'min HTML :'}))
    .pipe(htmlmin({collapseWhitespace: true, minifyJS: true, removeComments:true}))
    .pipe(gulp.dest(strTargetPath))
    .pipe(gzip({append:true}))
    .pipe(gulp.dest(strPackedPath));
    cb();
}

function minimizeScripts(cb) {
    let strSourcePath = Settings.getWebSourcePath() + "/js/*.js";
    let strTargetPath = Settings.getWebDistPath() + "/js";
    let strPackedPath = Settings.getWebPackedPath();
    
    gulp.src(strSourcePath)
    .pipe(debug({ title: 'min JS   :'}))
    .pipe(uglify())
    .pipe(gulp.dest(strTargetPath))
    .pipe(gzip({append:true}))
    .pipe(gulp.dest(strPackedPath));
    cb();
}

function minimizeCSS(cb) {
    let strSourcePath = Settings.getWebSourcePath() + "/css/*.css";
    let strTargetPath = Settings.getWebDistPath() + "/css";
    let strPackedPath = Settings.getWebPackedPath();
    console.log("CSS out : " + strTargetPath);
    gulp.src(strSourcePath)
    .pipe(debug({ title: 'min CSS  :'}))
    .pipe(cleancss())
    .pipe(gulp.dest(strTargetPath))
    .pipe(gzip({append:true}))
    .pipe(gulp.dest(strPackedPath));
    cb();
}

function minimizeLanguages(cb) {
    let strSourcePath = Settings.getWebSourcePath() + "/i18n/*.json";
    let strTargetPath = Settings.getWebDistPath() + "/i18n";
    let strPackedPath = Settings.getWebPackedPath();
    gulp.src(strSourcePath)
    .pipe(debug({ title: 'min i18n :'}))
    .pipe(jsonmin())
    .pipe(gulp.dest(strTargetPath))
    .pipe(gzip({append:true}))
    .pipe(gulp.dest(strPackedPath));
    cb();
}

// #endregion

// #region build of cpp header information of packed files

function buildHeaderFiles(cb) {

    // let strScanMask = Settings.getWebPackedPath() + "/*.gz";
    let fTotalSize = 0;
    let strIncludePath = Settings.getIncludePath();
    let strPackedPath = Settings.getWebPackedPath();
    fs.readdirSync(strPackedPath).forEach(strFileName => {
        if(strFileName.endsWith(".gz")) {
            let strPackedFileName = path.join(strPackedPath,strFileName);
            let strHeaderFileName = path.join(strIncludePath,strFileName+ ".h");
            let nPackedFileSize = fs.statSync(strPackedFileName).size;
            let tData = fs.readFileSync(strPackedFileName);
            if(nPackedFileSize == tData.length) {
                console.log(` - file size (${nPackedFileSize})\tis OK ${strPackedFileName}\t ==> ${strHeaderFileName}`);
                let strIncludeName = path.basename(strPackedFileName);
                let oWS = fs.createWriteStream(strHeaderFileName);
                oWS.on("error", function(oErr) { gutil.log(oErr) });
                oWS.write("#pragma once\n");
                oWS.write("#define " + strIncludeName.replace(/\.|-/g, "_") + "_len " + nPackedFileSize + "\n");
                oWS.write("const uint8_t " + strIncludeName.replace(/\.|-/g, "_") + "[] PROGMEM = {")
                for(let i = 0; i < tData.length; i++) {
                    if (i % 1000 == 0) oWS.write("\n");
                    oWS.write('0x' + ('00' + tData[i].toString(16)).slice(-2));
                    if (i < tData.length - 1) oWS.write(',');
                }
                oWS.write("\n};");
                oWS.end();
                fTotalSize += nPackedFileSize;
            }
        }
    });
    console.log("==================================================================");
    console.log("Web Frontend size in memory : " + fTotalSize + " bytes");
    console.log("==================================================================");
    let strTouchFile = Settings.getData("touchAfterPagesCompiled");
    console.log("Checking touch file : " + strTouchFile);
    if(strTouchFile && fs.existsSync(strTouchFile)) {
        console.log("touching file : " + strTouchFile);
        let changedModifiedTime = new Date();
        let changedAccessTime = new Date();
        fs.utimesSync(strTouchFile, changedAccessTime, changedModifiedTime);

        // as touching does not trigger the compile, append a line as comment...
        fs.appendFileSync(strTouchFile,"/* touched by page compiler : " + changedModifiedTime.getTime() + " */\n");   
    }
    cb();
}

// #endregion


export async function runCompilePages(cb, oSettings) {
    console.log("---- compile....");
    Settings.addConfig(oSettings);
    
    const runJob = gulp.series(   
                                    minimizeHTML,
                                    minimizeCSS,
                                    minimizeScripts,
                                    minimizeLanguages,
                                    buildHeaderFiles
                                );
    return await runJob();
}