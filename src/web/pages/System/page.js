/**
 * The System Page with Restart / Reset
 * 
 * @PageBuilder:register(SystemPage,CSystemPage)
 */
class CSystemPage extends CPageHandler {
    constructor(oApp) {
        super("system",EC("#SystemPage"),oApp.Config);
        this._App = oApp;
    }
    showWait(oOptions) {
        let oDlg = new CWaitDialog(oOptions);
        oDlg.showModal();
    }

    navToHome() {
        console.log("calling home page...");
        location.href = location.href; 
    }

    on(pView, oElement) {
        if(Utils.isElement(oElement)) {
            switch(oElement.id) {
                case "restart": this.showWait({
                    timeOutSecs: 10,
                    onFinished: this.navToHome.bind(this)
                });
                this._App.sendSocketCommand(DEFAULTS.RESTART_COMMAND);
            }
        }
    }
}