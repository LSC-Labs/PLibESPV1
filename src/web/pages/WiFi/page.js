/**
 * Represents a WiFi Setting page
 * 
 * @PageBuilder:register(WiFiPage,CWiFiPage) 
 */
class CWiFiPage extends CPageHandler {
    /**
     * 
     * @param {CAPP} oApp 
     */
    constructor(oApp) {
        // don't use this, before initialized !
        super("wifi",new CElement("#WiFiPage"),oApp.Config);
        this._App = oApp;
       // this.Translator = new CTranslator();
    }

    /**
     * catch the useWiFiBtn click event from the scan dialog to insert the selected network.
     * @param {CView} pView 
     * @param {HTMLElement} oElement 
     * @param {string} strCmd 
     */
    on(pView,oElement,strCmd) {
        super.on(pView,oElement);
        if(Utils.isElement(oElement)) {
            if(oElement.id == "useWifiBtn") {
                let oSource = pView.selById("scan_ssid");
                let oSelected = oSource.options[oSource.selectedIndex];
                if(oSelected) {
                    pView.sel("[data-cfg = 'ssid']").value  = oSelected.value;
                    pView.sel("[data-cfg = 'bssid']").value = oSelected.bssidvalue; 
                }
                this.closeDialog(pView,oElement);
            } 
            // Close action for modal dialog ?
            if(oElement.dataset.dismiss == "modal")  this.closeDialog(pView,oElement);
        }
        
    }

    /**
     * listen on the web socket message and check if the message is a "ssidlist" message.
     * @param {CView} pView 
     * @param {object} oMsg 
     */
    onSocketMessage(pView, oMsg) {
        if(oMsg && oMsg.data == "ssidlist") {
            this.insertFoundNetworks(oMsg.payload);
        }
    }

    /**
     * display the scan network dialog.
     * @param {CView} pView 
     * @param {HTMLElement} oElement
     */
    showScanDialog(pView,oElement) {
        let oThis = this;
        let oDlg = new CDialog("#scanWiFiDialog", { cancelBtn:true });
        // Reset to scan start....
        try { oDlg.sel(".dialogHeader").dataset.i18n = "WiFi.scan.title"; } catch {}
        try { oDlg.sel("#scan_ssid").innerHTML = ""; } catch {}
        this._App.Translator.translate(oDlg.getBase())
            .then(e => {
                oDlg.showModal();
                oThis._App.sendSocketCommand(DEFAULTS.SCAN_WIFI);
            });
    }

    /**
     * insert the found (scanned) networks into the select element.
     * @param {*} oListOfSSIDs 
     */
    insertFoundNetworks(oListOfSSIDs) {
        let oDlg = new CDialog("#scanWiFiDialog");
        let oSelect = oDlg.sel("#scan_ssid");
        try { oDlg.sel(".dialogHeader").dataset.i18n = "WiFi.scan.titleDone"; } catch {}
        this._App.Translator.translate(oDlg.getBase())
            .then(e => {
                for(let i=0; i < oListOfSSIDs.length; i++) {
                    let x = parseInt(oListOfSSIDs[i].rssi);
                    let nPercent = Math.min(Math.max(2 * (x + 100), 0), 100);
                    let oOpt = document.createElement("option");
                    oOpt.value = oListOfSSIDs[i].ssid;
                    oOpt.bssidvalue = oListOfSSIDs[i].bssid;
                    oOpt.innerHTML = oListOfSSIDs[i].ssid + " (" + nPercent + "%) - " + oListOfSSIDs[i].bssid ;
                    oSelect.appendChild(oOpt);
                }
            });
        
        
    }
}
