const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const test = require('node:test');
const source = fs.readFileSync(path.join(__dirname, '../data-source.js'), 'utf8');

function setup() {
    const links = [];
    const open = {};
    const get = {};
    const tx = { objectStore: () => ({ get(key) { tx.key = key; return get; } }) };
    let closed = false, aborted = false;
    const db = {
        objectStoreNames: { contains: () => true },
        transaction(store, mode) { assert.equal(store, 'METADATA'); assert.equal(mode, 'readonly'); return tx; },
        close() { closed = true; }
    };
    open.result = db;
    open.transaction = { abort() { aborted = true; } };
    const context = {
        location: { pathname: '/demo/static/index.html' },
        indexedDB: { open(name, version) { assert.equal(name, 'EM_PRELOAD_CACHE'); assert.equal(version, 1); return open; } },
        document: { createElement: () => ({}), head: { append(link) { links.push(link); } } }
    };
    context.window = context;
    vm.runInNewContext(source, context);
    return { context, links, open, get, tx, db, closed: () => closed, aborted: () => aborted };
}

test('cold cache aborts schema creation and preloads the gzip URL with matching credentials', () => {
    const s = setup();
    s.open.onupgradeneeded();
    assert.ok(s.aborted());
    s.open.onerror();
    assert.equal(s.links.length, 1);
    assert.equal(s.links[0].href, s.context.OpenDEDataSource.url);
    assert.equal(s.links[0].as, 'fetch');
    assert.equal(s.links[0].crossOrigin, 'anonymous');
    assert.match(s.links[0].href, /deusex\.data\.gz$/);
});

test('cached metadata suppresses network hints and closes the read-only connection', () => {
    const s = setup();
    s.open.onsuccess();
    assert.equal(s.tx.key, 'metadata/%2Fdemo%2Fstatic%2Fbin/index.data');
    s.get.result = { uuid: 'cached-package' };
    s.get.onsuccess();
    s.tx.oncomplete();
    assert.equal(s.links.length, 0);
    assert.ok(s.closed());
});

test('an existing database without this page package triggers preload', () => {
    const s = setup();
    s.open.onsuccess();
    s.get.onsuccess();
    assert.equal(s.links.length, 1);
});

test('late and blocked probes do not launch duplicate preloads', () => {
    const late = setup();
    late.context.OpenDEDataSource.loaderStarted = true;
    late.open.onsuccess();
    late.get.onsuccess();
    assert.equal(late.links.length, 0);
    const blocked = setup();
    blocked.open.onblocked();
    blocked.open.onsuccess();
    assert.ok(blocked.closed());
    assert.equal(blocked.links.length, 0);
});

test('storage errors leave cache recovery to the loader', () => {
    const s = setup();
    s.open.onerror();
    assert.equal(s.links.length, 0);
});
