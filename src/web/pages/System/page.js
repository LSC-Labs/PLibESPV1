/**
 * The System Page with Restart / Reset
 * 
 * @PageBuilder:register(SystemPage,CSystemPage)
 */
class CSystemPage extends CPageHandler {
    constructor(oApp) {
        super("system",EC("#SystemPage"),oApp.Config);
        this._App = oApp;
        this.Transe = new CTranslator();
    }

    async restartDevice() {
        let strAsk = await this.Transe.getKeyData("System.Restart.confirm");
        if (confirm(strAsk ?? "restart ?")) {
            this._App.restartDevice();
            let oDlg = new CWaitDialog({
                title: "i18n:System.Restart.title",
                title: "i18n:System.Restart.waitmsg",
                onFinished: function() { location.href = location.href; }
            });
            oDlg.showModal();
        }
    }

    async resetDevice() {
        let strAsk = await this.Transe.getKeyData("System.Reset.confirm");
        if (confirm(strAsk ?? "reset ?")) {
            this._App.resetDevice();
            let oDlg = new CWaitDialog({
                title: "i18n:System.Reset.title",
                title: "i18n:System.Reset.waitmsg",
                onFinished: function() { location.href = location.href; }
            });
            oDlg.showModal();
        }
    }

    on(pView, oElement) {
        if(Utils.isElement(oElement)) {
            switch(oElement.id) {
                case "restart": this.restartDevice(); break;
                case "reset":   this.resetDevice(); break;
            }
        }
    }
}