/**
 * Web runtime - Utils, CVarTable and CElement
 * run: npm run test-web
 */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRuntimeEnv } from './helpers/runtimeEnv.mjs';

test('Utils.isTrueValue / isFalseValue - numbers, null and strings', () => {
    const { eval: ev } = createRuntimeEnv();
    const isTrue = (v) => ev('Utils').isTrueValue(v);
    assert.equal(isTrue(0), false, '0 is false');
    assert.equal(isTrue(NaN), false, 'NaN is false');
    assert.equal(isTrue(null), false, 'null is false');
    assert.equal(isTrue(undefined), false, 'undefined is false');
    assert.equal(isTrue(1), true);
    assert.equal(isTrue(true), true);
    assert.equal(isTrue(false), false);
    assert.equal(isTrue('0'), false);
    assert.equal(isTrue(' Off '), false);
    assert.equal(isTrue('1'), true);
});

test('Utils.capitalizeFirstLetter returns the result', () => {
    const { eval: ev } = createRuntimeEnv();
    assert.equal(ev('Utils.capitalizeFirstLetter("status")'), 'Status');
    assert.equal(ev('Utils.capitalizeFirstLetter(42)'), 42);
});

test('CVarTable.setVar - "", 0 and false are values, undefined/null delete the var', () => {
    const { eval: ev } = createRuntimeEnv();
    const t = ev('new CVarTable()');
    t.setVar('empty', '').setVar('zero', 0).setVar('no', false).setVar('del', 'x');
    assert.equal(t.getValue('empty', '<missing>'), '');
    assert.equal(t.getValue('zero', '<missing>'), 0);
    assert.equal(t.getValue('no', '<missing>'), false);
    t.setVar('del', undefined);
    assert.equal(t.getValue('del', '<missing>'), '<missing>');
    t.setVar('zero', null);
    assert.equal(t.getValue('zero', '<missing>'), '<missing>');
});

test('CVarTable.setVars - from another CVarTable, an object and an array', () => {
    const { eval: ev } = createRuntimeEnv();
    const src = ev('new CVarTable()').setVar('a', '1');
    const t = ev('new CVarTable()');
    t.setVars(src).setVars({ b: '2' });
    assert.equal(t.getValue('a'), '1');
    assert.equal(t.getValue('b'), '2');
});

test('CVarTable.subst - values are inserted literally, nested vars, no endless loop', () => {
    const { eval: ev } = createRuntimeEnv();
    const t = ev('new CVarTable()');
    t.setVar('price', '$& $$ $1').setVar('name', '$(first) $(last)').setVar('first', 'Jo').setVar('last', 'Doe');
    t.setVar('self', '$(self)!').setVar('n', 0);
    assert.equal(t.subst('cost: $(price)'), 'cost: $& $$ $1');
    assert.equal(t.subst('Hello $(name)'), 'Hello Jo Doe');
    assert.equal(t.subst('n=$(n)'), 'n=0');
    assert.equal(t.subst('$(unknown) stays'), '$(unknown) stays');
    // self reference must terminate (iteration limit)
    assert.match(t.subst('$(self)'), /^\$\(self\)!+$/);
});

test('CVarTable.setPageVars reads the VarTable JSON (also from a template)', () => {
    const env = createRuntimeEnv();
    const oTpl = env.document.createElement('template');
    oTpl.innerHTML = '<PageVars style="display:none">{"col":"col-xs-4"}</PageVars><div></div>';
    const t = env.eval('new CVarTable()');
    t.setPageVars(oTpl.content, 'PageVars');
    assert.equal(t.getValue('col'), 'col-xs-4');
});

test('CElement.disable() / enable()', () => {
    const env = createRuntimeEnv({ html: '<button id="b"></button>' });
    const e = env.eval('EC("#b")');
    e.disable();
    assert.equal(env.document.getElementById('b').disabled, true);
    e.disable(false);
    assert.equal(env.document.getElementById('b').disabled, false);
});

test('CElement.evtArgs - strings and objects are returned as stored', () => {
    const env = createRuntimeEnv({ html: '<button id="a"></button><button id="b"></button>' });
    const a = env.eval('EC("#a")');
    a.evtArgs({ cmd: 'scan', n: 2 });
    assert.deepEqual({ ...a.evtArgs() }, { cmd: 'scan', n: 2 });
    const b = env.eval('EC("#b")');
    b.evtArgs('savePage');
    assert.equal(b.evtArgs(), 'savePage');
    assert.equal(env.eval('EC(document.createElement("i"))').evtArgs(), undefined);
});

test('CElement.setText - i18n key or plain text (plain text removes an old key)', () => {
    const env = createRuntimeEnv({ html: '<span id="s" data-i18n="old.key">x</span>' });
    const e = env.eval('EC("#s")');
    e.setText('i18n:new.key');
    assert.equal(e.getBase().dataset.i18n, 'new.key');
    e.setText('plain');
    assert.equal(e.getBase().innerHTML, 'plain');
    assert.equal(e.getBase().dataset.i18n, undefined);
});

test('CElement.setValue / getValue - range, hidden, textarea, number, checkbox, radio', () => {
    const env = createRuntimeEnv({ html: `
        <input id="r" type="range" min="0" max="100"><input id="h" type="hidden">
        <textarea id="t"></textarea><input id="n" type="number">
        <input id="c" type="checkbox"><input id="ra" type="radio" value="1">
        <span id="sp"></span><button id="btn">B</button>` });
    const ec = (id) => env.eval(`EC("#${id}")`);
    for (const [id, v] of [['r', '42'], ['h', 'secret'], ['t', 'multi\nline'], ['n', '7']]) {
        ec(id).setValue(v);
        assert.equal(ec(id).getValue(), v, id);
    }
    ec('c').setValue(0);
    assert.equal(ec('c').getValue(), '0', 'checkbox with numeric 0 is unchecked');
    ec('c').setValue(true);
    assert.equal(ec('c').getValue(), '1');
    ec('ra').setValue('1');
    assert.equal(ec('ra').getValue(), '1');
    ec('sp').setValue('text');
    assert.equal(env.document.getElementById('sp').textContent, 'text');
    assert.equal(ec('sp').getValue(), undefined, 'no value for plain elements');
    ec('btn').setValue('ignored');
    assert.equal(env.document.getElementById('btn').textContent, 'B', 'buttons are not changed by setValue');
    assert.equal(ec('btn').getValue(), undefined);
});

test('CElement.setValue on select - unknown values are added as text (no html)', () => {
    const env = createRuntimeEnv({ html: '<select id="s"><option value="a">A</option></select>' });
    const strEvil = '<img src=x onerror="window.hacked=1">';
    env.eval('EC("#s")').setValue(strEvil);
    const oSel = env.document.getElementById('s');
    assert.equal(oSel.querySelector('img'), null, 'no html element created');
    assert.equal(oSel.options.length, 2);
    assert.equal(oSel.value, strEvil);
    assert.equal(oSel.options[1].textContent, strEvil);
});

test('CElement.isVisible respects the class "isHidden" (data-auth hidden elements)', () => {
    const env = createRuntimeEnv({ html: '<div id="p" class="isHidden"><input id="i"></div><input id="v">' });
    assert.equal(env.eval('EC("#i")').isVisible(), false);
    assert.equal(env.eval('EC("#v")').isVisible(), true);
});

test('CElement.fadeOut without element does not throw', async () => {
    const env = createRuntimeEnv();
    await env.eval('new CElement(null)').fadeOut(0);
});
