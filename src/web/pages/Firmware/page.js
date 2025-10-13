/**
 * Firmware Download / Install page
 * 
 * @PageBuilder:register(FirmwarePage,CFirmwarePage)
 */
class CFirmwarePage extends CPageHandler {
    _fwInstTime = 30;   // Time needed to install firmware..
    constructor(oApp) {
        super("firmware",EC("#FirmwarePage"),oApp.Config);
        this._App = oApp;
        
    }

    loadPageConfig(pView, pApp) {
        super.loadPageConfig(pView,pApp);
        EC("#fwSelect").on("change",this.onFile.bind(this));
        let strURL = pApp.Settings.getData("app.latest.url");
        fetch(strURL)
            .then((response) => {
                return response.json();
            })
            .then((oRelease) => {
                let oAsset = oRelease.assets[0];
                let oDTCreated = new Date(oAsset.created_at);
                let oDTUpdated = new Date(oAsset.updated_at)
                pView.sel("#Date").textContent = oDTCreated.toLocaleDateString() + " / " + oDTUpdated.toLocaleDateString();
                pView.sel("#Name").textContent = oRelease.name;
                pView.sel("#Version").textContent = oRelease.tag_name;
                pView.sel("#Desc").innerHTML = oRelease.body.replaceAll("\n","<br/>");
                pView.gel("#downloadupdate").attr("href",oAsset.browser_download_url);
                pView.sel("#download").disabled = false;
            })
            .catch(ex => {
            });

    }
    onFile(oEvt) {
        EC("#fwInstall").getBase().disabled = false;
    }

    on(pView, oElement) {
        if(oElement) {
            switch(oElement.id) {

                case 'download':
                    // A firmware download was clicked, trigger the download...
                    pView.sel("#downloadupdate").click();
                    break;

                case 'fwSelect':
                    // A Firnware was selected, enable the install button
                    let oFW = pView.sel('#fwInstall');
                    oFW.disabled = false;
                    break;

                case 'fwInstall':
                    // Install the selected Firmware file
                    
                    // formData.append("bin", pView.sel("#fwSelect").files[0]);
                    
                    let oDlg = new CWaitDialog({
                        title: "installing firmware - please wait",
                        timeOutSecs: this._fwInstTime,
                        onFinished: function() { location.href = location.href; }
                    });
                    oDlg.showModal();
                    let oFile = pView.sel("#fwSelect").files[0];
                    let oFormData = new FormData();
                    oFormData.append("bin", oFile, oFile.name);
                    
                    fetch('/update',{ 
                        method:'POST', 
                        body: oFormData
                        // formData 
                    });
                    break;
            }
        }
    }

}