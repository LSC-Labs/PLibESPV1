/**
 * Load the chartJS library from content delivery (current version)
 * If internet is available, the library is in place afterwards.
 * 
 * To check if it is in place, use "isChartJSAvailable()" or check the function "Chart".
 * 
 */
(function() {
    console.log("Loading chartJS....")
    const oScript = document.createElement("script");
    oScript.setAttribute("type", "text/javascript");
    oScript.setAttribute("src", "https://cdn.jsdelivr.net/npm/chart.js/dist/chart.umd.min.js");
    document.getElementsByTagName("head")[0].appendChild(oScript);   
})();

/**
 * global function shows if chartJS is available or not...
 */
function isChartJSAvailable() { return( typeof Chart !== 'undefined')};