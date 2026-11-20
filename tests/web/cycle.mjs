// Prueba el ciclo COMPLETO en un Chrome de verdad (sin instalar nada: usa el Chrome del PC y el WebSocket de Node):
//   Jugar -> escena -> combate -> mensaje final y récord -> fundido -> vuelta a la portada con el resultado -> día bloqueado.
//
//   node tests/web/cycle.mjs "http://localhost:8081/?debug=1&daily=1&seed=900002" salida/   (victoria segura)
//   node tests/web/cycle.mjs "http://localhost:8081/?debug=1&daily=1&seed=900001" salida/   (derrota segura)
// Necesita la ROM de pruebas (scripts\build-wsl.ps1 -Debug) y el servidor (node scripts/serve.mjs 8081).
import { spawn } from 'node:child_process';
import { mkdirSync, writeFileSync, mkdtempSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const [url, outDir = 'tests/web/out'] = process.argv.slice(2);
const CHROME = process.env.CHROME || 'C:/Program Files/Google/Chrome/Application/chrome.exe';
const PORT = 9333 + Math.floor(Math.random() * 500);
mkdirSync(outDir, { recursive: true });
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const profile = mkdtempSync(join(tmpdir(), 'ds-chrome-'));
const chrome = spawn(CHROME, [`--headless=new`, `--remote-debugging-port=${PORT}`, `--user-data-dir=${profile}`,
  '--window-size=900,720', '--autoplay-policy=no-user-gesture-required', '--no-first-run', 'about:blank'], { stdio: 'ignore' });

async function pageWs() {
  for (let i = 0; i < 60; i++) {
    try {
      const list = await (await fetch(`http://127.0.0.1:${PORT}/json`)).json();
      const p = list.find((t) => t.type === 'page');
      if (p) return p.webSocketDebuggerUrl;
    } catch { /* aún no */ }
    await sleep(500);
  }
  throw new Error('Chrome no arrancó');
}

const ws = new WebSocket(await pageWs());
await new Promise((r) => (ws.onopen = r));
let nextId = 1;
const pending = new Map();
ws.onmessage = (e) => {
  const m = JSON.parse(e.data);
  if (m.id && pending.has(m.id)) { pending.get(m.id)(m); pending.delete(m.id); }
};
const send = (method, params = {}) => new Promise((res) => {
  const id = nextId++;
  pending.set(id, res);
  ws.send(JSON.stringify({ id, method, params }));
});
const evalJs = async (expr) => (await send('Runtime.evaluate', { expression: expr, returnByValue: true })).result?.result?.value;
const shot = async (name) => {
  const r = await send('Page.captureScreenshot', { format: 'png' });
  writeFileSync(join(outDir, `${name}.png`), Buffer.from(r.result.data, 'base64'));
};
const key = async (k, code, vk) => {
  for (const type of ['keyDown', 'keyUp']) {
    await send('Input.dispatchKeyEvent', { type, key: k, code, windowsVirtualKeyCode: vk, nativeVirtualKeyCode: vk });
    await sleep(60);
  }
};
const state = () => evalJs("document.querySelector('.app')?.dataset.state");
const stats = () => evalJs("localStorage.getItem('hortelano:dailysteven:stats-debug') || localStorage.getItem('hortelano:dailysteven:stats')");

await send('Page.enable');
await send('Page.navigate', { url });
await sleep(3000);
console.log('portada:', await state());
await shot('00-portada');
await evalJs("document.getElementById('play').click()");

let sawPlaying = false;
let t = 0;
const log = [];
for (; t < 420; t++) {
  const st = await state();
  if (st === 'playing') sawPlaying = true;
  if (t % 10 === 0) { log.push(`${t}s estado=${st}`); console.log(`${t}s estado=${st}`); }
  if (t % 20 === 0) await shot(`${String(t).padStart(3, '0')}-${st}`);
  if (sawPlaying && st === 'title') break;
  if (st === 'error') break;
  if (t > 8) await key('x', 'KeyX', 88);   // A: avanza textos y elige el primer ataque
  await sleep(900);
}
await sleep(1500);
const finalState = await state();
const finalStats = await stats();
await shot('99-final');
console.log('ESTADO FINAL:', finalState);
console.log('STATS:', finalStats);
const lockedText = await evalJs("document.getElementById('played')?.hidden === false ? document.getElementById('played-result').textContent + ' | ' + document.getElementById('played-next').textContent : 'NO BLOQUEADO'");
console.log('PORTADA:', lockedText);
console.log('Botón Jugar visible:', await evalJs("!document.getElementById('play').hidden"));
writeFileSync(join(outDir, 'resumen.txt'), [...log, `final=${finalState}`, `stats=${finalStats}`, `portada=${lockedText}`].join('\n'));
ws.close();
chrome.kill();
process.exit(finalState === 'title' ? 0 : 1);
