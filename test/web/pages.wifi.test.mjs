/**
 * WiFi page - found networks must be displayed as text (SSIDs are chosen by foreign networks)
 * run: npm run test-web
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRuntimeEnv, readLibFile } from './helpers/runtimeEnv.mjs';

test('WiFi page - scanned SSIDs are inserted as text, not as html', async () => {
    const env = createRuntimeEnv({
        files: { 'i18n/en.json': {} },
        html: '<div id="WiFiPage"></div><dialog id="scanWiFiDialog"><div class="dialogHeader"></div><select id="scan_ssid"></select></dialog>',
    });
    env.load(readLibFile('src/web/pages/WiFi/page.js'));
    const page = env.eval('new CWiFiPage({ Config: {}, Translator: new CTranslator() })');
    const strEvil = '<img src=x onerror="window.hacked=1">';
    page.insertFoundNetworks([{ ssid: strEvil, rssi: '-60', bssid: 'AA:BB' }, { ssid: 'Home', rssi: '-40', bssid: 'CC:DD' }]);
    await env.flush();
    const oSelect = env.document.getElementById('scan_ssid');
    assert.equal(oSelect.querySelector('img'), null, 'no html element created from the SSID');
    assert.equal(oSelect.options.length, 2);
    assert.equal(oSelect.options[0].value, strEvil);
    assert.equal(oSelect.options[0].textContent, `${strEvil} (80%) - AA:BB`);
    assert.equal(oSelect.options[1].textContent, 'Home (100%) - CC:DD');
});
