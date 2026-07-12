/**
 * Firmware Download / Install page
 * 
 * @PageBuilder:register(FirmwarePage,CFirmwarePage)
 */
class CFirmwarePage extends CPageHandler {
    _fwInstTime = 30;   // Time needed to install firmware..
    _fwMaxHistory = 8;  // Max number of firmware history entries to show
    constructor(oApp) {
        super("firmware",EC("#FirmwarePage"),oApp.Config);
        this._App = oApp;
        
    }

    /**
     * 
     * @param {*} oReleases 
     * @returns the complete block ot the newest version
     */
    getNewestRelease(oReleases) {
        let oMainRelease = {};
        for(let oRelease of oReleases) {
            if(oRelease.draft != undefined && oRelease.draft == false) {
                oMainRelease = oRelease;
                break;
            }
        }
        return oMainRelease;
    }

    getReleasesStripped(oReleases) {
        let oStripped = [];
        let nCount = 0;
        for(let oRelease of oReleases) {
            if(oRelease.draft != undefined && oRelease.draft == false) {
                let oAsset = oRelease.assets[0];
                let oDTCreated = new Date(oAsset.created_at);
                let oDTUpdated = new Date(oAsset.updated_at);
                let strName = oRelease.name;
                let strDateUpdated = oDTUpdated.toLocaleDateString();
                let strDateCreated = oDTCreated.toLocaleDateString();
                let strDate = strDateCreated + " / " + strDateUpdated;
                oStripped.push({
                    name: strName,
                    date: strDate,
                    updated_at: strDateUpdated,
                    created_at: strDateCreated,
                    desc: oRelease.body.replaceAll("\n","<br/>"),
                    browser_download_url: oAsset.browser_download_url,
                });
                nCount++;
                if(nCount >= this._fwMaxHistory) {
                    break;
                }
            }
        }
        return oStripped;
    }


    loadPageConfig(pView, pApp) {
        super.loadPageConfig(pView,pApp);
        EC("#fwSelect").on("change",this.onFile.bind(this));
        let strURL = pApp.Settings.getData("app.firmware.list");
        fetch(strURL)
            .then((response) => {
                return response.json();
            })
            .then((oReleases) => {
                let oRelease = this.getNewestRelease(oReleases);
                let oAsset = oRelease.assets[0];
                let oDTCreated = new Date(oAsset.created_at);
                let oDTUpdated = new Date(oAsset.updated_at)
                pView.sel("#Date").textContent = oDTCreated.toLocaleDateString() + " / " + oDTUpdated.toLocaleDateString();
                pView.sel("#Name").textContent = oRelease.name;
                pView.sel("#Version").textContent = oRelease.tag_name;
                pView.sel("#Desc").innerHTML = oRelease.body.replaceAll("\n","<br/>");
                pView.gel("#downloadupdate").attr("href",oAsset.browser_download_url);
                pView.sel("#download").disabled = false;
                let strReleasePage = pApp.Settings.getData("app.GitHubPage");
                
                if(strReleasePage) {
                    let oLink = pView.gel("#fwReleasePageLink");
                    oLink.attr("href",strReleasePage + "/releases");
                    oLink.show();
                }
                let oHistory = pView.sel("#fwHistory");
                for(let oRelease of this.getReleasesStripped(oReleases)) {
                    let oDiv = document.createElement("div");
                    oDiv.classList.add("release");
                    oDiv.innerHTML = `<div><hr/><span class="fwReleaseName">${oRelease.name}</span> - ${oRelease.created_at}</div><hr/><div>${oRelease.desc}</div>`;
                    oHistory.appendChild(oDiv);
                }
            })
            .catch(ex => {
                console.log("Error loading firmware list: " + ex);
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
                    });
                    break;
            }
        }
    }

}