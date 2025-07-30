
/**
 * Page Handler class for Backup / Restore Page
 * 
 * @PageBuilder:register(BackupPage,CBackupPage)
 */

class CBackupPage extends CPageHandler {
    constructor(oApp) {
        super("backup",document.getElementById("BackupPage"),oApp.Config)
        this._oApp = oApp;
    }

     /**
     * catch the button click events.
     * @param {*} oContent 
     * @param {*} oElement 
     * @param {*} strCmd 
     */
    on(oContent,oElement,strCmd) {
        super.on(oContent,oElement);
        // Backup requested ? - ask the device for a backup file... (catch the result...)
        if(Utils.isElement(oElement)) {
            switch(oElement.id) {
                case "backupSet": 
                    this._oApp.sendSocketCommand(DEFAULTS.GET_BACKUP_COMMAND);
                    break;
                case "restoreSet": 
                    let oAlert = new CAlertDialog({ title: "i18n:SysCfg.Restore.title"})
                    if ("files" in oElement) {
                        if (oElement.files.length === 0) {
                            oAlert.showModal({ body: "i18n:main.noFileSelected"});
                            // alert("You did not select file to restore!");
                        } else {
                            let oReader = new FileReader();
                            oReader.onload = function() {
                                try {
                                    let oConfigData = JSON.parse(oReader.result);
                                    // Check if structure could be a config file...
                                    if (oConfigData.wifi && oConfigData.wifi.hasOwnProperty("ap_mode")) {
                                        if (confirm("File seems to be valid, do you wish to continue?")) {
                                            this.m_oAPP.WS.sendCommandMessage(RESTORE_BACKUP_COMMAND,"",oConfigData);
                                        }
                                    } else {
                                        oAlert.showModal({body: "i18n:SysCfg.Restore.invalidFile"});
                                        // alert("Not a valid backup file!");
                                    }
                                } catch (e) {
                                    oAlert.showModal({body: "i18n:SysCfg.Restore.invalidFile"});
                                    // alert("Invalid backup file!");
                                return;
                                }
        
                            };
                            oReader.readAsText(oElement.files[0]);
                        }
                    }    
                    break;
            }
        }
    }
    
     /**
     * listen on the socket message and check if the message is a "ssidlist" message.
     * @param {*} pView 
     * @param {*} oMsg 
     */
    onBackupMessageReceived(pView, oMsg) {
        if(oMsg) {
            switch(oMsg.data) {
                case "config":  // A backup request for config received...
                    // Simulate a download click...
                    let oDataObj = oMsg.payload;
                    let strData =  "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(oDataObj, null, 2));
                    let dlAnchorElem = pView.selById("downloadSet");
                    dlAnchorElem.setAttribute("href", strData);
                    dlAnchorElem.setAttribute("download", `${this._oApp.Vars.getValue("prog_name")} - settings.json`);
                    dlAnchorElem.click();
                    break;
            }
        }
    }
}