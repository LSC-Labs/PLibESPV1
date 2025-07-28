import sys
import shutil
import glob
import os
import json
import argparse
from pathlib import Path


parser = argparse.ArgumentParser(
                    prog='joinFiles',
                    description='Join files from a directory to a target by using a contorl file',
                    epilog='(c) LSC Labs')

parser.add_argument('-ctrl', '--controlFile',nargs="?", help="control file with instructions.")
parser.add_argument('-src' , '--sourceDir', nargs="?", help="base source directory")
parser.add_argument('-target' , '--targetFile', help="target file name")
parser.add_argument('-mask', '--fileMask',nargs='*', help='fileMask to be used (overrides control file)')

args = parser.parse_args()


SourceDir = "."
TargetFile = "_out.join"
ControlFile = None
FileMask = None

DefaultCtrl = {
    "files": [
        "*.py2"
    ],
}

def loadControlFile(strFilename):
    global DefaultCtrl
    global TargetFile
    bResult = False
    if(os.path.exists(strFilename)):
        print(" - loading ctrl file: " + strFilename)
        with open(strFilename, 'r') as oFP:
            DefaultCtrl = json.load(oFP)
            oFP.close()
            if(DefaultCtrl["targetfile"]):
                TargetFile = str(DefaultCtrl["targetfile"])
            bResult = True
    return(bResult)
    

def setArguments():
    global SourceDir
    global TargetFile
    global FileMask
    if(args.sourceDir):
        SourceDir = args.sourceDir

    if(args.targetFile):
        TargetFile = args.targetFile

    if(args.fileMask):
        FileMask = args.fileMask

    if(args.controlFile): 
        if(not loadControlFile(args.controlFile)):
            print("ERROR: Controlfile does not exist : " + args.controlFile)
            exit(-1)

def joinFile(strFilename, oFP):
    print(" - joining file : " + strFilename)
    with open(strFilename,'r') as oFPRead:
        shutil.copyfileobj(oFPRead,oFP)
        oFPRead.close()

  
def writeTargetFile(strTargetFile):
    print(" - writing result to: " + strTargetFile )
    with open(strTargetFile,'w+') as oFP:
        
        # Use FileMask if specified by cmd line, otherwise use control file
        ScanList = DefaultCtrl["files"]
        if(FileMask): ScanList = FileMask

        for fileMask in ScanList :
            # already a full qualified name ?
            if(os.path.exists(fileMask)):
                joinFile(fileMask,oFP)
            else:
                # it's a file name or a mask, so iterate...
                for strFilename in glob.glob(os.path.join(SourceDir,fileMask)):
                    joinFile(strFilename,oFP)
        oFP.close()
        
        
setArguments()

print("====== Joining files =========")
writeTargetFile(TargetFile)
print("==============================")

