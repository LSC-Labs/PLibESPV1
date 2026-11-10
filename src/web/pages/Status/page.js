/**
  @PageBuilder:register(StatusPage,CStatusPage)
 */

class CStatusPage extends CPageHandler {
    /**
     * 
     * @param {CAPP} oApp 
     */
    constructor(oApp) {
        // don't use this, before initialized !
        super("status",new CElement("#StatusPage"),oApp.Config);
        this._App = oApp;
       // this.Translator = new CTranslator();
    }

        
    // #region Update Status on a regular base
    disposePage(pView, pApp) {
        super.disposePage(pView, pApp);
        this.RefreshTimer = clearInterval(this.RefreshTimer);
        this.RefreshTimer = null;
    }

    preparePage(pView, pApp) {
        super.preparePage(pView, pApp);
        // Set an interval to refresh the status infos every 5 seconds...
        this.RefreshTimer = setInterval( this.refreshStatus.bind(this, pView, pApp), 5000);
    }
    
    refreshStatus(pView, pApp) {
        // System status (heap, flash, file system) is only sent by apiV1/sysstatus -
        // request it also, otherwise the memory values stay at the values of the page load.
        // The device returns the last snapshot and builds a new one (=> values max. one interval old).
        pApp.requestActDeviceStatus();
        pApp.requestActStatus();
    }

    /**
     * Triggered, when a new status message is received from the web socket.
     * @param {CView} pView 
     * @param {CApp} pApp 
     * @returns false to avoid loadPageConfig is called again (!) and the game starts again...
     */
    onUpdateStatusReceived(pView, pApp) {
        this.loadPageConfig(pView, pApp);
        return(false)
    }
    // #endregion




    /**
     * Load the page config...
     * Request an actual status from the device...
     * This will trigger an update message, which will be handled in onUpdate_status_Received.
     * @param {CView} pView 
     * @param {CApp} pApp 
     */
    loadPageConfig(pView, pApp) {
        let oStatus = pApp.DeviceStatus;
       
        // set the root element infos - no prefix...
        this.setConfigValues(pView,oStatus);
        for(let strName in oStatus) {
            let oData = oStatus[strName];
            if(oData && Utils.isObj(oData)) {
                this.setConfigValues(pView,oData,strName);
            }
        }

        let oVars = new CVarTable();
        oVars.setVars(pApp.tVars);
        oVars.setVars(oStatus);
        this.showMemoryUsage(pView, oStatus, oVars);
        this.showBattery(pView, oStatus);
        return(oStatus);
    }

    /**
     * The battery row is only shown, if the device has a battery status handler ("bat")
     * and a battery is connected (bat.available).
     */
    showBattery(pView, oStatus) {
        let oRow = pView.sel("#batteryRow");
        if(!oRow) return;
        const bAvailable = !!(oStatus && Utils.isObj(oStatus.bat) && oStatus.bat.available === true);
        oRow.style.display = bAvailable ? "" : "none";
    }

    // #region Memory usage (program memory, file system, heap)

    /** value as KB (1 decimal for small values) */
    static toKB(nBytes) {
        const nKB = nBytes / 1024;
        return(nKB < 100 ? Math.round(nKB * 10) / 10 : Math.round(nKB));
    }

    /** true, if all values are numbers */
    static isNum(...tValues) {
        return(tValues.every(n => typeof n === "number" && isFinite(n)));
    }

    /**
     * Sets a usage bar (width, text, color) and the translated tooltip.
     * @param {CView} pView
     * @param {string} strSelector selector of the bar (.usageStatus element)
     * @param {number} nPercent used in percent (0..100)
     * @param {string} strText text inside the bar
     * @param {boolean} bWarn true => warning color
     * @param {string} strI18n i18n key of the tooltip
     * @param {CVarTable} oVars vars for the tooltip
     */
    setUsageBar(pView, strSelector, nPercent, strText, bWarn, strI18n, oVars) {
        let oBar = pView.sel(strSelector);
        if(!oBar) return;
        const nWidth = Math.min(100, Math.max(0, nPercent));
        oBar.style.width = nWidth + "%";
        oBar.innerText = strText;
        let oUB = EC(oBar);
        if(bWarn) oUB.rc("usageOK").ac("usageWarn");
        else      oUB.ac("usageOK").rc("usageWarn");
        (new CTranslator()).translateI18n(strI18n, undefined, oVars)
            .then(str => { oUB.attr("title", str); });
    }

    /**
     * Shows program memory, file system and heap usage.
     *
     * Program memory: the sketch area is sketch_size + sketch_free_size
     * (sketch_free_size is the space for a new firmware - OTA update).
     * An OTA update is only possible, if the new sketch fits into the free space
     * => warning, if the current sketch is larger than the free space (> 50% on ESP8266).
     */
    showMemoryUsage(pView, oStatus, oVars) {
        // Program memory (flash) / OTA
        const nSketch = oStatus.sketch_size;
        const nFree   = oStatus.sketch_free_size;
        if(CStatusPage.isNum(nSketch, nFree) && (nSketch + nFree) > 0) {
            const nArea    = nSketch + nFree;
            const nPercent = Math.round(nSketch * 100 / nArea);
            oVars.setVar("sketch_kb",          CStatusPage.toKB(nSketch));
            oVars.setVar("sketch_free_kb",     CStatusPage.toKB(nFree));
            oVars.setVar("sketch_area_kb",     CStatusPage.toKB(nArea));
            oVars.setVar("flash_used_percent", nPercent);
            const bWarn = nSketch > nFree;
            this.setUsageBar(pView, "#flashUsage", nPercent,
                             CStatusPage.toKB(nSketch) + " KB (" + nPercent + "%)",
                             bWarn, bWarn ? "Status.memWarn" : "Status.memOK", oVars);
        }

        // File system - as text (a bar is not readable with a small usage)
        let oFs = pView.sel("#fsUsage");
        const nFsUsed  = oStatus.fs_used;
        const nFsTotal = oStatus.fs_total;
        if(oFs && CStatusPage.isNum(nFsUsed, nFsTotal) && nFsTotal > 0) {
            const nPercent = Math.round(nFsUsed * 100 / nFsTotal);
            oVars.setVar("fs_used_kb",      CStatusPage.toKB(nFsUsed));
            oVars.setVar("fs_total_kb",     CStatusPage.toKB(nFsTotal));
            oVars.setVar("fs_used_percent", nPercent);
            const bWarn = nPercent >= 80;
            let oEF = EC(oFs);
            if(bWarn) oEF.ac("textWarn"); else oEF.rc("textWarn");
            (new CTranslator()).translateI18n(bWarn ? "Status.fsWarn" : "Status.fsOK", undefined, oVars)
                .then(str => { oFs.innerText = str; });
        }

        // Heap - ESP8266 has no total heap size, so free / largest block / fragmentation is shown.
        // (heap_max is the largest block on ESP8266 - heap_max_block is the same value on all platforms)
        let oHeap = pView.sel("#heapUsage");
        const nHeapFree  = oStatus.heap_free;
        const nHeapBlock = CStatusPage.isNum(oStatus.heap_max_block) ? oStatus.heap_max_block : oStatus.heap_max;
        if(oHeap && CStatusPage.isNum(nHeapFree, nHeapBlock)) {
            let nFrag = CStatusPage.isNum(oStatus.heap_frag)
                            ? oStatus.heap_frag
                            : (nHeapFree > 0 ? Math.round(100 - nHeapBlock * 100 / nHeapFree) : 0);
            nFrag = Math.min(100, Math.max(0, nFrag));
            oVars.setVar("heap_free_kb",  CStatusPage.toKB(nHeapFree));
            oVars.setVar("heap_block_kb", CStatusPage.toKB(nHeapBlock));
            oVars.setVar("heap_frag",     nFrag);
            // Minimum free heap since start - a value that decreases over time indicates a leak
            oVars.setVar("heap_min_kb",   CStatusPage.isNum(oStatus.heap_min) && oStatus.heap_min > 0
                                            ? CStatusPage.toKB(oStatus.heap_min) : "-");
            // Less than 8 KB free or a largest block below 4 KB: requests / TLS / JSON may fail
            const bWarn = nHeapFree < 8192 || nHeapBlock < 4096 || nFrag > 50;
            let oEH = EC(oHeap);
            if(bWarn) oEH.ac("textWarn"); else oEH.rc("textWarn");
            let oTranse = new CTranslator();
            oTranse.translateI18n("Status.heapInfo", undefined, oVars)
                .then(str => { oHeap.innerText = str; });
            if(bWarn) oTranse.translateI18n("Status.heapWarn", undefined, oVars)
                        .then(str => { oEH.attr("title", str); });
            else oHeap.removeAttribute("title");
        }
    }

    // #endregion
}