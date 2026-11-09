/**
 * Web runtime - CTranslator, CTemplates, CView, dialogs, WebSocket, CAppl
 * run: npm run test-web
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRuntimeEnv } from './helpers/runtimeEnv.mjs';

const LANG_FILES = {
    'i18n/en.json': { main: { ok: 'OK', cancel: 'Cancel' }, t: { hello: 'Hello $(who)', title: 'Tip' } },
};

// #region CTranslator
test('CTranslator.getKeyData returns the default, if the key is not found', async () => {
    const env = createRuntimeEnv({ files: LANG_FILES });
    const tr = env.eval('new CTranslator()');
    assert.equal(await tr.getKeyData('t.missing', 'en', 'DEFAULT'), 'DEFAULT');
    assert.equal(await tr.getKeyData('main.ok', 'en', 'DEFAULT'), 'OK');
});

test('CTranslator.translateI18n substitutes the vars', async () => {
    const env = createRuntimeEnv({ files: LANG_FILES });
    const vars = env.eval('new CVarTable()').setVar('who', 'World');
    assert.equal(await env.eval('new CTranslator()').translateI18n('t.hello', 'en', vars), 'Hello World');
});

test('CTranslator.getLanguageDef - parallel requests load a language file only once', async () => {
    const env = createRuntimeEnv({ files: { 'i18n/fr.json': { a: 'b' } } });
    const tr = env.eval('new CTranslator()');
    const tResults = await Promise.all([1, 2, 3, 4, 5].map(() => tr.getLanguageDef('fr')));
    assert.equal(env.fetchCalls.filter(u => u === 'i18n/fr.json').length, 1);
    tResults.forEach(o => assert.equal(o.a, 'b'));
    // a missing language is also cached (no endless retries)
    await Promise.all([tr.getLanguageDef('xx'), tr.getLanguageDef('xx')]);
    assert.equal(env.fetchCalls.filter(u => u === 'i18n/xx.json').length, 1);
});

test('CTranslator.translate - attribute targets: substituted, missing key leaves attribute alone', async () => {
    const env = createRuntimeEnv({
        files: LANG_FILES,
        html: '<span id="a" data-i18n="@title:t.hello"></span><span id="b" title="keep" data-i18n="@title:t.nope"></span>',
    });
    const vars = env.eval('new CVarTable()').setVar('who', 'You');
    const tr = env.eval('new CTranslator()');
    await tr.translate(env.document.body, 'en', vars);
    await env.flush();
    assert.equal(env.document.getElementById('a').title, 'Hello You');
    assert.equal(env.document.getElementById('b').title, 'keep');
});
// #endregion

// #region CTemplates / CView
test('CTemplates.createIcon - non svg icon by name gets its class', () => {
    const env = createRuntimeEnv();
    const tpl = env.eval('new CTemplates({ icons: { d: [ { name: "Font", type: "font", class: "fa fa-star" } ] } })');
    const oIcon = tpl.createIcon('Font').getBase();
    assert.equal(oIcon.tagName, 'I');
    assert.equal(oIcon.className, 'fa fa-star');
});

test('CTemplates.findIconDef without icon definitions returns null', () => {
    const env = createRuntimeEnv();
    assert.equal(env.eval('new CTemplates({})').findIconDef('x'), null);
});

test('CTemplates.createElements - processed elements are not processed again', () => {
    const env = createRuntimeEnv({ html: '<div id="c"><span data-element-type="save-btn" id="s"></span></div>' });
    const app = { n: 0, savePage() { this.n++; } };
    const tpl = env.eval('new CTemplates({})');
    tpl.createElements(env.document.getElementById('c'), app);
    const oBtn = env.document.querySelector('#c button');
    assert.ok(oBtn.classList.contains('saveBtn'));
    assert.equal(oBtn.hasAttribute('data-element-type'), false, 'marker removed from the replacement');
    tpl.createElements(env.document.getElementById('c'), app);
    assert.equal(env.document.querySelectorAll('#c button').length, 1);
    oBtn.click();
    assert.equal(app.n, 1);
});

test('CView.loadView - save button calls savePage once, PageVars are used', () => {
    const env = createRuntimeEnv({ html: '<div id="main"></div>' });
    const app = {
        saves: 0, events: [],
        savePage() { this.saves++; },
        onEvent(e) { this.events.push(env.eval('EC')(e.currentTarget).evtArgs()); },
    };
    const view = env.eval('new CView("#main", {})');
    const strPage = '<PageVars style="display:none">{"cls":"col-xs-4"}</PageVars>' +
                    '<div class="$(cls)" id="row"></div>' +
                    '<button data-element-type="save-btn"></button>' +
                    '<button data-evt-args="doIt" id="other"></button>';
    assert.equal(view.loadView('TestPage', strPage, env.eval('new CVarTable()'), app), true);
    assert.equal(env.document.getElementById('row').className, 'col-xs-4');
    env.document.querySelector('#main .saveBtn').click();
    assert.equal(app.saves, 1, 'savePage exactly once');
    env.document.getElementById('other').click();
    assert.deepEqual(app.events, ['doIt']);
});

test('CView.loadView - content as CElement', () => {
    const env = createRuntimeEnv({ html: '<div id="main"></div><div id="src"><b id="x">X</b></div>' });
    const view = env.eval('new CView("#main", {})');
    assert.equal(view.loadView('P', env.eval('EC("#src")'), env.eval('new CVarTable()')), true);
    assert.equal(env.document.querySelector('#main b').textContent, 'X');
});
// #endregion

// #region Dialogs
const DIALOGS_HTML = `
    <dialog id="reviewDialog"><h4 class="modal-title"></h4><div class="dialogBody"></div>
        <button type="button" class="btn-secondary" data-dismiss="modal"></button>
        <button type="button" class="btn-primary"></button></dialog>
    <dialog id="waitDialog"><div class="modal-title"></div><div class="dialogBody"></div>
        <div class="progressBar"><div class="progressStatus"></div></div></dialog>
    <dialog id="signinDialog"><form><input name="password" type="password"><button type="submit">in</button></form></dialog>`;

test('CDialog - OK handler runs once, also if the dialog was shown before', async () => {
    const env = createRuntimeEnv({ html: DIALOGS_HTML, files: LANG_FILES });
    let nFirst = 0, nSecond = 0;
    const dlg1 = env.eval('new CDialog("#reviewDialog")');
    dlg1.showModal({ onOK: () => nFirst++, okBtn: 'i18n:main.ok', cancelBtn: true });
    await env.flush();
    env.document.querySelector('#reviewDialog').close();
    const dlg2 = env.eval('new CDialog("#reviewDialog")');
    dlg2.showModal({ onOK: () => nSecond++ });
    await env.flush();
    env.document.querySelector('#reviewDialog .btn-primary').click();
    assert.equal(nFirst, 0, 'handler of the first call is removed');
    assert.equal(nSecond, 1);
});

test('CDialog - button texts from the options (i18n key or plain text)', async () => {
    const env = createRuntimeEnv({ html: DIALOGS_HTML, files: LANG_FILES });
    env.eval('new CDialog("#reviewDialog")').showModal({ okBtn: 'i18n:main.ok', cancelBtn: 'Abort' });
    await env.flush();
    const oOK = env.document.querySelector('#reviewDialog .btn-primary');
    const oCancel = env.document.querySelector('#reviewDialog .btn-secondary');
    assert.equal(oOK.dataset.i18n, 'main.ok');
    assert.equal(oOK.textContent, 'OK');
    assert.equal(oCancel.textContent, 'Abort');
    assert.equal(env.document.getElementById('reviewDialog').open, true);
});

test('CWaitDialog.showModal - no second showModal() on the open dialog', async () => {
    const env = createRuntimeEnv({ html: DIALOGS_HTML, files: LANG_FILES });
    const dlg = env.eval('new CWaitDialog({ timeOutSecs: 100 })');
    dlg.showModal();
    await env.flush();      // the translation callback would call showModal() again
    assert.equal(env.document.getElementById('waitDialog').open, true);
    env.window.clearInterval(dlg._ActTimerID);
});

test('CSigninDialog - only the current dialog instance handles the submit', () => {
    const env = createRuntimeEnv({ html: DIALOGS_HTML });
    const tCalls = [];
    env.eval('new CSigninDialog({})')._Options.onFinished = (p) => tCalls.push('first:' + p);
    const dlg2 = env.eval('new CSigninDialog({})');
    dlg2._Options.onFinished = (p) => tCalls.push('second:' + p);
    env.document.querySelector('#signinDialog [name=password]').value = 'pw';
    const oForm = env.document.querySelector('#signinDialog form');
    oForm.dispatchEvent(new env.window.Event('submit', { cancelable: true }));
    assert.deepEqual(tCalls, ['second:pw']);
});
// #endregion

// #region WebSocket
test('CWebSocketV2 - no second socket while connecting, old socket does not change the state', () => {
    const env = createRuntimeEnv({ html: '<div id="statusInfo"></div>' });
    const tStatus = [];
    const ws = env.eval('CWebSocketV2') && new (env.eval('CWebSocketV2'))({ onStatusChanged: s => tStatus.push(s) });
    ws._openSocket();
    ws._monCon();               // monitor timer while the socket is still connecting
    assert.equal(env.sockets.length, 1, 'no second socket while CONNECTING');
    env.sockets[0].fakeClose(); // connection failed
    ws._monCon();
    assert.equal(env.sockets.length, 2, 'reconnect after close');
    env.sockets[1].fakeOpen();
    env.sockets[0].onopen({});  // late event of the old socket - ignored
    env.sockets[0].onclose({});
    assert.equal(ws._isConnectionAvailable, true);
    assert.deepEqual(tStatus, ['close', 'open']);
});

test('CWebSocketV2 - monitor timer is stopped by close() and not duplicated', () => {
    const env = createRuntimeEnv({ html: '<div id="statusInfo"></div>' });
    const ws = new (env.eval('CWebSocketV2'))({});
    ws.connect(true);
    const nTimer = ws.oMonTimerID;
    assert.ok(nTimer);
    ws.connect(true);
    assert.equal(ws.oMonTimerID, nTimer, 'no second timer');
    ws.close();
    assert.equal(ws.oMonTimerID, undefined);
});

test('CWebSocketV2 - socket uri keeps the port of the page', () => {
    const uri = (strUrl) => new (createRuntimeEnv({ url: strUrl }).eval('CWebSocketV2'))({})._getConnectionUri();
    assert.equal(uri('http://192.168.4.1/'), 'ws://192.168.4.1/ws');
    assert.equal(uri('http://192.168.4.1:8081/'), 'ws://192.168.4.1:8081/ws');
    assert.equal(uri('https://dev.local/'), 'wss://dev.local/ws');
});
// #endregion

// #region CAppl
test('CAppl.setNewStatus keeps prog_name/prog_ver, if the status has no value', () => {
    const env = createRuntimeEnv();
    const app = env.eval('new CAppl()');
    app.Vars.setVar('prog_ver', '1.0.0').setVar('prog_name', 'Default');
    app.setNewStatus({ prog_name: 'Device' });
    assert.equal(app.Vars.getValue('prog_name'), 'Device');
    assert.equal(app.Vars.getValue('prog_ver'), '1.0.0');
});
// #endregion
