/**
 * Test environment for the web runtime (src/web/js/runtime.js).
 *
 * Loads the runtime into a jsdom window - without the auto start at the end
 * of the file - so all classes (CElement, CVarTable, CDialog, ...) can be
 * tested in isolation. Missing browser functions are replaced by fakes:
 * - HTMLDialogElement.showModal()/close()  (strict: showModal() on an open dialog throws)
 * - fetch()                                (answers from env.files, counts the calls)
 * - WebSocket                              (FakeWebSocket, readyState controlled by the test)
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { JSDOM } from 'jsdom';

// PLIB_LIB_ROOT: run the tests against another checkout (e.g. an old version to verify the tests detect the defects)
const LIB_ROOT = process.env.PLIB_LIB_ROOT ?? path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../..');
export const RUNTIME_FILE = path.join(LIB_ROOT, 'src/web/js/runtime.js');
const AUTO_START = '(function() {\n    const APP         = new CAppl();';

/** runtime source without the auto start (IIFE at the end of the file) */
export function runtimeSource() {
    const strSource = fs.readFileSync(RUNTIME_FILE, 'utf8');
    const nPos = strSource.lastIndexOf(AUTO_START);
    if (nPos < 0) throw new Error('auto start of runtime.js not found - adjust the test helper');
    return strSource.substring(0, nPos);
}

export function readLibFile(strRelPath) {
    return fs.readFileSync(path.join(LIB_ROOT, strRelPath), 'utf8');
}

/**
 * create a new window with the runtime loaded.
 * @param {object} oOptions  html: body content, files: { url: object|string } for fetch
 * @returns env with window, eval (run code in the window) and helpers
 */
export function createRuntimeEnv(oOptions = {}) {
    const dom = new JSDOM(`<!DOCTYPE html><html lang="en"><body>${oOptions.html ?? ''}</body></html>`, {
        runScripts: 'outside-only',
        url: oOptions.url ?? 'http://device.local/',
    });
    const window = dom.window;
    const env = { window, document: window.document, files: oOptions.files ?? {}, fetchCalls: [], sockets: [] };

    // --- <dialog> (not implemented by jsdom) - strict like older browsers
    window.HTMLDialogElement.prototype.showModal = function () {
        if (this.open) throw new window.DOMException('dialog is already open', 'InvalidStateError');
        this.setAttribute('open', '');
    };
    window.HTMLDialogElement.prototype.close = function () {
        if (!this.open) return;
        this.removeAttribute('open');
        this.dispatchEvent(new window.Event('close'));
    };

    // --- innerText (not implemented by jsdom) - close enough for tests: textContent
    if (!('innerText' in window.HTMLElement.prototype)) {
        Object.defineProperty(window.HTMLElement.prototype, 'innerText', {
            get() { return this.textContent; },
            set(v) { this.textContent = v; },
            configurable: true,
        });
    }

    // --- fetch
    window.fetch = async (strUrl) => {
        env.fetchCalls.push(String(strUrl));
        const oData = env.files[String(strUrl)];
        const bFound = oData !== undefined;
        return {
            ok: bFound, status: bFound ? 200 : 404, statusText: bFound ? 'OK' : 'Not Found',
            json: async () => (typeof oData === 'string' ? JSON.parse(oData) : oData),
            text: async () => (typeof oData === 'string' ? oData : JSON.stringify(oData)),
        };
    };

    // --- WebSocket
    window.WebSocket = class FakeWebSocket {
        static CONNECTING = 0; static OPEN = 1; static CLOSING = 2; static CLOSED = 3;
        constructor(strUrl) {
            this.url = strUrl; this.readyState = 0; this.sent = []; this._listeners = {};
            env.sockets.push(this);
        }
        addEventListener(strEvt, fn) { (this._listeners[strEvt] ??= []).push(fn); }
        send(oMsg) { this.sent.push(oMsg); }
        close() { this.fakeClose(); }
        // test helpers
        fakeOpen() { this.readyState = 1; this.onopen?.({}); }
        fakeClose() { this.readyState = 3; this.onclose?.({}); }
        fakeMessage(oData) { (this._listeners.message ?? []).forEach(fn => fn.call(this, { data: JSON.stringify(oData) })); }
    };

    env.load = (strSource) => {
        // top level classes/functions/consts of an eval'd script are not visible in later evals - export them
        const tNames = [...strSource.matchAll(/^(?:class|function|const)\s+([A-Za-z_$][\w$]*)/gm)].map(m => m[1]);
        window.eval(`${strSource}\n;Object.assign(globalThis, { ${tNames.join(', ')} });`);
    };
    env.load(runtimeSource());
    env.eval = (strCode) => window.eval(strCode);
    env.eval('new CLog().setLogLevel(0)');      // keep the test output readable
    /** wait until pending promises (translations, fetch) are processed */
    env.flush = async (nRounds = 5) => {
        for (let i = 0; i < nRounds; i++) await new Promise(resolve => setTimeout(resolve, 0));
    };
    return env;
}
