/**
 * WebRoutes.cpp
 *
 * Is part of PLibESPV1 - CWebServer
 *
 * Insert your application specific static pages here, also
 * as the defaults and the registration/handling of API's
 *
 * Notes:
 * - All handlers run in the async context (ESP32: AsyncTCP task,
 *   ESP8266: SYS context). Never build status/config data here -
 *   use the snapshots of Appl (getStatusSnapshot(), getSystemStatusSnapshot()).
 * - The embedded pages are delivered gzipped with an ETag. The browser
 *   revalidates them (Cache-Control: no-cache) and gets a "304 Not Modified"
 *   as long as the firmware (this file) was not rebuilt with new pages.
 */
#ifndef DEBUG_LSC_WEBROUTES
    #undef DEBUGINFOS
#endif

#include <WebRoutes.h>
#include <WebServer.h>
#include <Appl.h>
#include <web/index.html.gz.h>
#include <web/app.css.gz.h>
#include <web/index.js.gz.h>
#include <web/settings.js.gz.h>
#include <web/_runtime.css.gz.h>
#include <web/_runtime.js.gz.h>
#include <web/_pages.html.gz.h>
#include <web/_pages.js.gz.h>
#include <web/_pages.css.gz.h>
#include <web/de.json.gz.h>
#include <web/en.json.gz.h>

// Changes with every build of this file, i.e. whenever an included page changes.
#ifndef WEBROUTES_ETAG
    #define WEBROUTES_ETAG "\"" __DATE__ " " __TIME__ "\""
#endif

/**
 * @brief Registers a GET route delivering an embedded (PROGMEM) gzip file.
 *
 * Sends "304 Not Modified" if the browser already has the current version
 * (If-None-Match == ETag), otherwise the gzipped content.
 */
static void addGzipRoute(CWebServer &oWebServer, const char *pszUri, const char *pszContentType,
                         const uint8_t *pData, size_t nLen) {
    oWebServer.on(pszUri, HTTP_GET, [pszContentType, pData, nLen](AsyncWebServerRequest *pRequest) {
        DEBUG_INFOS("WEB: sending %s",pRequest->url().c_str());
        const AsyncWebHeader *pIfNoneMatch = pRequest->getHeader("If-None-Match");
        AsyncWebServerResponse *pResponse;
        if(pIfNoneMatch && pIfNoneMatch->value() == WEBROUTES_ETAG) {
            pResponse = pRequest->beginResponse(304);
        } else {
            pResponse = pRequest->beginResponse(200, pszContentType, pData, nLen);
            pResponse->addHeader("Content-Encoding", "gzip");
        }
        pResponse->addHeader("ETag", WEBROUTES_ETAG);
        pResponse->addHeader("Cache-Control", "no-cache");    // always revalidate (new pages after OTA)
        pRequest->send(pResponse);
    });
}

void registerWebRoutes(CWebServer &oWebServer) {
    oWebServer.registerDefaults();
    oWebServer.registerFileAccess();

    // Embedded web pages
    addGzipRoute(oWebServer, "/index.html",       "text/html",        index_html_gz,    index_html_gz_len);
    addGzipRoute(oWebServer, "/_pages.html",      "text/html",        _pages_html_gz,   _pages_html_gz_len);
    addGzipRoute(oWebServer, "/css/_runtime.css", "text/css",         _runtime_css_gz,  _runtime_css_gz_len);
    addGzipRoute(oWebServer, "/css/_pages.css",   "text/css",         _pages_css_gz,    _pages_css_gz_len);
    addGzipRoute(oWebServer, "/css/app.css",      "text/css",         app_css_gz,       app_css_gz_len);
    addGzipRoute(oWebServer, "/js/settings.js",   "text/javascript",  settings_js_gz,   settings_js_gz_len);
    addGzipRoute(oWebServer, "/js/index.js",      "text/javascript",  index_js_gz,      index_js_gz_len);
    addGzipRoute(oWebServer, "/js/_runtime.js",   "text/javascript",  _runtime_js_gz,   _runtime_js_gz_len);
    addGzipRoute(oWebServer, "/js/_pages.js",     "text/javascript",  _pages_js_gz,     _pages_js_gz_len);
    addGzipRoute(oWebServer, "/i18n/de.json",     "application/json", de_json_gz,       de_json_gz_len);
    addGzipRoute(oWebServer, "/i18n/en.json",     "application/json", en_json_gz,       en_json_gz_len);

    // API - snapshots built in the loop (safe copies for the async context)
    oWebServer.on("/apiV1/status", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        pRequest->send(200, "application/json", Appl.getStatusSnapshot());
    });

    oWebServer.on("/apiV1/sysstatus", HTTP_GET, [](AsyncWebServerRequest *pRequest) {
        // Last snapshot - a fresh one is built by the next loop (file system access)
        pRequest->send(200, "application/json", Appl.getSystemStatusSnapshot());
    });
}
