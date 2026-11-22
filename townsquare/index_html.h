// Web-Oberfläche des Townsquare. Wird von handleRoot() ausgeliefert und
// holt sich den Spielstand per fetch() als JSON von /state.
#pragma once

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#140a16">
<title>Townsquare</title>
<style>
:root{
  --parch:#efe4c8;--ink:#2b1c12;--brass:#8d7650;--brass-light:#c9b58a;
  --panel-line:#4b3552;--divider:#2e1f35;--field:#0f0811;
  --text:#f2ead8;--muted:#b9aabf;--danger:#ff7b7b;
  --alive:#ffaa1e;--dead:#2f7bff;--ghost:#24406f;
  --serif:"Palatino Linotype",Palatino,"Book Antiqua","URW Palladio L",Georgia,serif;
  color-scheme:dark;
}
*{box-sizing:border-box}
html,body{margin:0;height:100%}
body{
  min-height:100dvh;display:flex;align-items:center;justify-content:center;overflow:hidden;
  color:var(--text);font-family:var(--serif);-webkit-tap-highlight-color:transparent;
  background:
    radial-gradient(ellipse 70% 50% at 25% 15%,#4a2650 0,transparent 70%),
    radial-gradient(ellipse 60% 45% at 80% 85%,#3d1f44 0,transparent 70%),
    radial-gradient(ellipse at center,#2a152d 0,#140a16 100%);
  background-color:#140a16;
}
body::before{
  content:"";position:fixed;inset:0;pointer-events:none;opacity:.35;mix-blend-mode:soft-light;
  background:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='480' height='480'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.006' numOctaves='4' seed='7' stitchTiles='stitch'/%3E%3CfeColorMatrix values='0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 0 0 0 1.3 -.35'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E");
  background-size:480px;
}
button{touch-action:manipulation}

#board{position:relative;flex:none}
.seat{
  position:absolute;padding:0;border:0;background:none;border-radius:50%;cursor:pointer;font-family:inherit;
  filter:drop-shadow(0 4px 6px rgba(0,0,0,.55));transition:transform .15s;
}
.seat:active{transform:scale(.95)}
.seat:focus-visible{outline:2px solid var(--text);outline-offset:3px}
.seat.blocked{filter:drop-shadow(0 0 10px rgba(255,60,60,.85))}
.seat svg{display:block;width:100%;height:100%}
.seat .num{font:700 34px var(--serif);fill:var(--ink)}
.seat .arc{font-family:var(--serif);font-weight:700;letter-spacing:1px;fill:var(--ink)}
.label{
  position:absolute;left:50%;top:calc(100% + 3px);transform:translateX(-50%);
  display:flex;align-items:center;gap:.35em;max-width:calc(var(--s) * 1.7);
  padding:.15em .55em;font-size:var(--lf);line-height:1.25;white-space:nowrap;color:var(--text);
  background:#120a14;border:1px solid #33243a;border-radius:3px;box-shadow:0 2px 6px rgba(0,0,0,.5);
}
.label span{min-width:0;overflow:hidden;text-overflow:ellipsis}
.small .label{top:auto;bottom:-.55em}
.label i{flex:none;width:.55em;height:.55em;border-radius:50%}
.st2 .label{color:var(--muted)}

.reminder{
  position:absolute;display:flex;align-items:center;justify-content:center;pointer-events:none;
  border-radius:50%;font-size:max(9px,calc(var(--rs) * .22));font-weight:700;color:var(--text);
  background:radial-gradient(circle at 35% 30%,#5d3a70,#2a1533 75%);
  border:1.5px solid #120814;box-shadow:0 3px 6px rgba(0,0,0,.6);
}
.reminder.block{background:radial-gradient(circle at 35% 30%,#8a2a2a,#3a0f12 75%)}

/* Zeiger: nur der Teil zwischen Mitte und Sitz, dreht um die Brettmitte */
.hand{
  position:absolute;z-index:1;width:8px;opacity:0;pointer-events:none;
  background:linear-gradient(var(--parch),var(--brass));
  clip-path:polygon(50% 0,100% 10px,100% 100%,0 100%,0 10px);
  filter:drop-shadow(0 0 4px rgba(0,0,0,.7));
  transition:transform .8s ease,opacity .3s;
}
.hand.on{opacity:.95}

#center{
  position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);
  display:flex;flex-direction:column;align-items:center;gap:.6em;text-align:center;font-size:var(--if);
}
.moon{width:2.4em;height:2.4em;filter:drop-shadow(0 0 8px rgba(233,217,166,.35))}
.compact .moon{display:none}
.plaque{
  position:relative;width:100%;padding:.35em .8em;color:var(--ink);
  font-size:var(--pf);font-weight:700;letter-spacing:.06em;font-variant:small-caps;
  background:linear-gradient(#f6eedb,#e3d4b0);border:2px solid #3b2a20;
  box-shadow:0 0 0 1px var(--brass) inset,0 6px 14px rgba(0,0,0,.6);
}
.plaque::before,.plaque::after{
  content:"";position:absolute;top:50%;width:10px;height:10px;
  background:#a08a60;border:1px solid #3b2a20;transform:translateY(-50%) rotate(45deg);
}
.plaque::before{left:-7px}
.plaque::after{right:-7px}
.info{color:var(--muted)}
.info b{color:var(--text);font-size:1.25em}
.link{
  font:inherit;color:inherit;background:none;border:0;padding:0;cursor:pointer;
  text-decoration:underline dotted;text-underline-offset:3px;
}
.corner{
  position:fixed;z-index:5;font-size:20px;padding:.3em .65em;
  top:max(12px,env(safe-area-inset-top));right:max(12px,env(safe-area-inset-right));
}
.btn{
  font:inherit;color:var(--text);padding:.5em 1em;cursor:pointer;
  background:linear-gradient(#2a1830,#170d1a);border:1px solid var(--panel-line);border-radius:4px;
  box-shadow:0 3px 8px rgba(0,0,0,.5);
}
.btn:active{transform:translateY(1px)}
.btn.primary{border-color:var(--brass)}

#backdrop{
  position:fixed;inset:0;z-index:10;display:flex;align-items:center;justify-content:center;padding:16px;
  background:rgba(8,3,10,.62);-webkit-backdrop-filter:blur(3px);backdrop-filter:blur(3px);
}
#backdrop[hidden]{display:none}
.panel{
  position:relative;width:min(100%,380px);max-height:calc(100dvh - 32px);overflow-x:hidden;overflow-y:auto;padding:16px;
  background:linear-gradient(#21132a,#140b17);border:1px solid var(--panel-line);border-radius:4px;
  box-shadow:0 18px 40px rgba(0,0,0,.7);animation:pop .16s ease-out;
}
.panel::after{
  content:"";position:absolute;top:-1px;right:-1px;width:22px;height:22px;pointer-events:none;
  border-top:3px solid var(--text);border-right:3px solid var(--text);
}
@keyframes pop{from{transform:scale(.96)}}
.panel header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
.panel h2{margin:0;font-size:22px}
.x{font:inherit;font-size:28px;line-height:1;padding:2px 8px;background:none;border:0;color:var(--muted);cursor:pointer}
.field{display:block;margin-bottom:14px}
.field label{display:block;margin-bottom:4px;font-size:13px;color:var(--muted)}
.namerow{display:flex;gap:8px}
.namerow .btn{flex:none;font-size:16px;padding:0 .8em}
.field input{
  flex:1;min-width:0;width:100%;padding:10px 12px;font:inherit;font-size:18px;color:var(--text);
  background:var(--field);border:1px solid var(--panel-line);border-radius:4px;
}
.field input:focus{outline:none;border-color:var(--brass-light)}
.seg{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:6px;margin-bottom:14px}
.seg button{
  display:flex;flex-direction:column;align-items:center;gap:6px;padding:10px 6px;cursor:pointer;
  font:inherit;font-size:14px;line-height:1.2;color:var(--text);
  background:var(--field);border:1px solid var(--panel-line);border-radius:4px;
}
.seg button i{width:12px;height:12px;border-radius:50%}
.seg button[aria-checked=true]{background:#3a2244;border-color:var(--brass-light);box-shadow:0 0 0 1px var(--brass-light) inset}
.menu{list-style:none;margin:0;padding:0;border-top:1px solid var(--divider)}
.menu button{
  width:100%;display:flex;align-items:center;gap:12px;padding:12px 4px;cursor:pointer;text-align:left;
  font:inherit;font-size:18px;color:var(--text);background:none;border:0;border-bottom:1px solid var(--divider);
}
.menu button:disabled{color:#6f6075;cursor:default}
.menu .danger{color:var(--danger)}
.menu svg{width:22px;height:22px;flex:none;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}
.hint{margin:10px 0 0;font-size:14px;color:var(--muted)}
.cal{margin:0 0 14px;font-size:16px;color:var(--text)}
.jogs{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px;margin-bottom:14px}
.jogs .btn,.calacts .btn{font-size:17px;padding:.6em .4em}
.calacts{display:flex;gap:8px}
.calacts .btn{flex:1}
.picks{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin-top:4px}
.pick{
  aspect-ratio:1;border-radius:50%;cursor:pointer;font:700 22px var(--serif);color:var(--ink);
  background:radial-gradient(circle at 40% 35%,#f7efdc,#e6d8b6 60%,#bfa878);
  border:1.5px solid #4a3a28;box-shadow:0 3px 8px rgba(0,0,0,.6);
}
.pick.cur{box-shadow:0 0 0 3px var(--brass-light),0 3px 8px rgba(0,0,0,.6)}
#toast{
  position:fixed;left:50%;bottom:20px;z-index:20;transform:translateX(-50%);padding:10px 16px;
  font-size:15px;color:var(--text);background:#3a0f16;border:1px solid #8a2a2a;border-radius:4px;
  opacity:0;transition:opacity .2s;pointer-events:none;
}
#toast.on{opacity:1}
#toast.ok{background:#1d2a4a;border-color:#2f7bff}
</style>
</head>
<body>
<svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>
  <radialGradient id="parch" cx="45%" cy="40%" r="65%">
    <stop offset="0" stop-color="#f7efdc"/><stop offset=".6" stop-color="#e6d8b6"/><stop offset="1" stop-color="#bfa878"/>
  </radialGradient>
</defs></svg>

<main id="board">
  <div id="hand" class="hand"></div>
  <div id="seats"></div>
  <div id="center">
    <svg class="moon" viewBox="0 0 40 40" aria-hidden="true"><path d="M26 5.2A16 16 0 1 0 35.8 26 12 12 0 0 1 26 5.2Z" fill="#e9d9a6"/></svg>
    <div class="plaque">Townsquare</div>
    <div class="info"><b id="count">0</b> Spieler · <button class="link" id="votesBtn" aria-label="Stimmenanzahl setzen"><b id="votes">0</b> Stimmen</button></div>
    <button class="btn primary" id="vote">Stimme +1</button>
  </div>
</main>
<button class="btn corner" id="menu" aria-label="Spielmenü">☰</button>

<div id="backdrop" hidden><div id="panel" class="panel" role="dialog" aria-modal="true"></div></div>
<div id="toast" role="status"></div>

<script>
const $ = s => document.querySelector(s);
const STATES = ['Lebendig', 'Tot', 'Geisterstimme verbraucht'];
const LED = ['var(--alive)', 'var(--dead)', 'var(--ghost)'];
const IC = {
  block: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 12V6.5M12 12l3.5 2"/></svg>',
  add: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 8v8M8 12h8"/></svg>',
  remove: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M8 12h8"/></svg>',
  votes: '<svg viewBox="0 0 24 24"><path d="M5 20V10M10 20V6M15 20v-8M20 20V4"/></svg>',
  hand: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 12l4-5"/><path d="M12 3v2M21 12h-2M12 21v-2M3 12h2"/></svg>',
  count: '<svg viewBox="0 0 24 24"><circle cx="12" cy="5" r="2"/><circle cx="19" cy="12" r="2"/><circle cx="12" cy="19" r="2"/><circle cx="5" cy="12" r="2"/></svg>',
  reset: '<svg viewBox="0 0 24 24"><path d="M4 12a8 8 0 1 0 2.5-5.8"/><path d="M4 4v4h4"/></svg>'
};
let S = {count: 0, votes: 0, hand: -1, min: 5, max: 20, players: []};
let seatOpen = -1;
let calibrating = false;

const esc = t => String(t).replace(/[&<>"']/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c]));

let toastTimer;
function toast(msg, ok) {
  const t = $('#toast');
  t.textContent = msg;
  t.classList.toggle('ok', !!ok);
  t.classList.add('on');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove('on'), 2500);
}

function applyState(state) {
  S = state;
  render();
  // Nach einem Neustart des ESP ist kein Spiel mehr aktiv
  if (!S.count) {
    seatOpen = -1;
    calibrating = false;
    openPicker();
  }
}

// Jeder Endpunkt antwortet mit dem kompletten Spielstand als JSON.
async function api(path) {
  try {
    const r = await fetch(path, {cache: 'no-store'});
    if (!r.ok) {
      const msg = await r.text();
      // Abgelehnt: Die Seite kennt evtl. einen alten Stand, also neu abgleichen
      applyState(await (await fetch('/state', {cache: 'no-store'})).json());
      throw new Error(S.count ? msg : 'Der Townsquare wurde neu gestartet. Bitte Spielerzahl neu wählen.');
    }
    applyState(await r.json());
    return true;
  } catch (e) {
    toast(e instanceof TypeError ? 'Keine Verbindung zum Townsquare' : e.message);
    return false;
  }
}

function token(i, p, small) {
  const name = p.name.toUpperCase();
  const showArc = !small && name;
  const fs = Math.min(13, Math.max(6, (100 / Math.max(name.length, 1) - 1) / .72));
  let svg = '<svg viewBox="0 0 100 100" aria-hidden="true">'
    + '<circle cx="50" cy="50" r="48" fill="url(#parch)" stroke="#4a3a28" stroke-width="1.5"/>'
    + '<circle cx="50" cy="50" r="43" fill="none" stroke="#8a744f" stroke-opacity=".5" stroke-width=".8"/>'
    + `<text class="num" x="50" y="${showArc ? 46 : small ? 40 : 52}" text-anchor="middle" dominant-baseline="central">${i + 1}</text>`;
  if (showArc) {
    svg += `<path id="arc${i}" d="M15 52A35 35 0 0 0 85 52" fill="none"/>`
      + `<text class="arc" font-size="${fs.toFixed(1)}"><textPath href="#arc${i}" xlink:href="#arc${i}" startOffset="50%" text-anchor="middle">${esc(name)}</textPath></text>`;
  }
  if (p.state) {
    // Tote Spieler: abgedunkelt und mit Leichentuch oben
    svg += `<circle cx="50" cy="50" r="48" fill="#120816" fill-opacity="${p.state === 2 ? .62 : .45}"/>`
      + '<path d="M37 2.5H63V30L50 23L37 30Z" fill="#17131a" stroke="#000"/>';
  }
  return svg + '</svg>';
}

// Sitze gleichmäßig nach Bogenlänge auf einer Ellipse verteilen: Sitz 1 oben, dann im Uhrzeigersinn.
function seatPositions(n, cx, cy, rx, ry) {
  const N = 720, pts = [], len = [0];
  for (let k = 0; k <= N; k++) {
    const t = k / N * 2 * Math.PI;
    pts.push([cx + rx * Math.sin(t), cy - ry * Math.cos(t)]);
    if (k) len.push(len[k - 1] + Math.hypot(pts[k][0] - pts[k - 1][0], pts[k][1] - pts[k - 1][1]));
  }
  const out = [];
  let k = 0;
  for (let i = 0; i < n; i++) {
    const target = len[N] * i / n;
    while (len[k + 1] < target) k++;
    const f = (target - len[k]) / (len[k + 1] - len[k] || 1);
    out.push([pts[k][0] + (pts[k + 1][0] - pts[k][0]) * f, pts[k][1] + (pts[k + 1][1] - pts[k][1]) * f]);
  }
  return out;
}

function render() {
  // Hochkant am Handy wird der Kreis zur Ellipse, damit die Plätze größer werden können
  let W = Math.min(innerWidth - 16, 900), H = Math.min(innerHeight - 16, 900);
  W = Math.floor(Math.min(W, H * 1.5));
  H = Math.floor(Math.min(H, W * 1.5));
  const n = Math.max(S.count, 1);
  const perimeter = 2 * Math.PI * Math.sqrt(((W * .4) ** 2 + (H * .4) ** 2) / 2);
  const s = Math.min(Math.min(W, H) * .19, 130, perimeter / n * .7);
  const small = s < 56;
  const rem = Math.max(34, s * .42);
  const rx = W / 2 - s / 2 - Math.max(6, s * .35 + 4);
  const ry = H / 2 - s / 2 - (small ? 10 : Math.max(16, s * .24));
  const cx = W / 2, cy = H / 2;

  const board = $('#board');
  board.style.width = W + 'px';
  board.style.height = H + 'px';
  board.classList.toggle('small', small);
  board.style.setProperty('--s', s + 'px');
  board.style.setProperty('--lf', Math.max(10, Math.min(18, s * .2)) + 'px');

  const pos = seatPositions(n, cx, cy, rx, ry);
  let html = '';
  S.players.forEach((p, i) => {
    const [x, y] = pos[i];
    const d = Math.hypot(cx - x, cy - y) || 1, ux = (cx - x) / d, uy = (cy - y) / d;
    const label = p.name || 'Sitz ' + (i + 1);
    html += `<button class="seat st${p.state}${i === S.hand ? ' blocked' : ''}" data-seat="${i}"`
      + ` style="left:${x - s / 2}px;top:${y - s / 2}px;width:${s}px;height:${s}px"`
      + ` aria-label="Sitz ${i + 1}${p.name ? ', ' + esc(p.name) : ''}, ${STATES[p.state]}">`
      + token(i, p, small)
      + `<span class="label"><i style="background:${LED[p.state]}"></i><span>${esc(label)}</span></span></button>`;

    const reminders = [];
    if (p.state === 1) reminders.push(['Stimme', 'vote']);
    if (i === S.hand) reminders.push(['Block', 'block']);
    reminders.forEach(([text, cls], k) => {
      // überlappt den Innenrand des Tokens leicht, wie ein angelegter Marker
      const off = s / 2 + rem * .2 + k * rem * .9;
      html += `<span class="reminder ${cls}" style="left:${x + ux * off - rem / 2}px;top:${y + uy * off - rem / 2}px;`
        + `width:${rem}px;height:${rem}px;--rs:${rem}px">${text}</span>`;
    });
  });
  $('#seats').innerHTML = html;

  // Mitte so groß wie möglich, aber innerhalb der freien Fläche (Marker eingerechnet)
  const ax = rx - s / 2 - rem * .75, ay = ry - s / 2 - rem * .75;
  const cw = Math.max(120, Math.min(300, ax * 1.6));
  const center = $('#center');
  center.classList.toggle('compact', Math.min(ax, ay) < 120);
  center.style.width = cw + 'px';
  center.style.setProperty('--pf', Math.max(15, Math.min(26, cw * .11)) + 'px');
  center.style.setProperty('--if', Math.max(13, Math.min(17, cw * .075)) + 'px');
  $('#count').textContent = S.count;
  $('#votes').textContent = S.votes;
  const k = Math.max(.8, Math.min(1, .97 / Math.hypot(center.offsetWidth / 2 / ax, center.offsetHeight / 2 / ay)));
  center.style.transform = `translate(-50%,-50%) scale(${k})`;

  // Zeiger beginnt am Rand der Mitte (als Ellipse um Schild und Knöpfe genähert)
  const hand = $('#hand');
  let visible = false;
  if (S.hand >= 0 && S.hand < S.count) {
    const [x, y] = pos[S.hand];
    const angle = Math.atan2(x - cx, cy - y);
    const ea = center.offsetWidth * k / 2 + 6, eb = center.offsetHeight * k / 2 + 6;
    const inner = ea * eb / Math.hypot(eb * Math.sin(angle), ea * Math.cos(angle));
    const outer = Math.hypot(x - cx, y - cy) - s / 2 - 4;
    visible = outer - inner > 14;
    if (visible) {
      hand.style.left = (cx - 4) + 'px';
      hand.style.top = (cy - outer) + 'px';
      hand.style.height = (outer - inner) + 'px';
      hand.style.transformOrigin = `50% ${outer}px`;
      hand.style.transform = `rotate(${angle}rad)`;
    }
  }
  hand.classList.toggle('on', visible);
}

function showPanel(html, closable = true) {
  const panel = $('#panel');
  panel.innerHTML = html;
  panel.dataset.closable = closable;
  $('#backdrop').hidden = false;
}

// Liefert false, wenn der Name nicht gespeichert werden konnte.
async function saveName() {
  const input = $('#name');
  if (!input || seatOpen < 0) return true;
  const value = input.value.trim();
  const p = S.players[seatOpen];
  if (!p || value === p.name) return true;
  return api(`/name?seat=${seatOpen}&name=${encodeURIComponent(value)}`);
}

async function closePanel(force) {
  if (!force && $('#panel').dataset.closable === 'false') return;
  // Schlägt das Speichern fehl, bleibt das Fenster offen, damit die Eingabe nicht verloren geht
  if (seatOpen >= 0 && !await saveName()) return;
  seatOpen = -1;
  if (calibrating) {
    calibrating = false;
    api('/calibrate?phase=cancel');
  }
  $('#backdrop').hidden = true;
}

function openPlayer(i) {
  seatOpen = i;
  const p = S.players[i];
  const full = S.count >= S.max, few = S.count <= S.min, blocked = i === S.hand;
  showPanel(`<header><h2>Sitz ${i + 1}</h2><button class="x" data-close aria-label="Schließen">×</button></header>
    <div class="field"><label for="name">Name</label><div class="namerow"><input id="name" maxlength="24" autocomplete="off" autocapitalize="words" enterkeyhint="done" placeholder="Name eingeben" value="${esc(p.name)}"><button class="btn primary" data-savename>Speichern</button></div></div>
    <div class="seg" role="radiogroup" aria-label="Status">${STATES.map((l, k) =>
      `<button role="radio" aria-checked="${p.state === k}" data-state="${k}"><i style="background:${LED[k]}"></i>${l}</button>`).join('')}</div>
    <ul class="menu">
      <li><button data-act="block"${blocked ? ' disabled' : ''}>${IC.block}${blocked ? 'Ist auf dem Block' : 'Auf den Block'}</button></li>
      <li><button data-act="before"${full ? ' disabled' : ''}>${IC.add}Spieler davor einfügen</button></li>
      <li><button data-act="after"${full ? ' disabled' : ''}>${IC.add}Spieler danach einfügen</button></li>
      <li><button data-act="remove" class="danger"${few ? ' disabled' : ''}>${IC.remove}Aus dem Spiel entfernen</button></li>
    </ul>
    ${full ? `<p class="hint">Maximal ${S.max} Spieler.</p>` : ''}${few ? `<p class="hint">Mindestens ${S.min} Spieler.</p>` : ''}`);
  const input = $('#name');
  input.addEventListener('change', saveName);
  input.addEventListener('keydown', e => { if (e.key === 'Enter') input.blur(); });
}

function openMenu() {
  seatOpen = -1;
  showPanel(`<header><h2>Spiel</h2><button class="x" data-close aria-label="Schließen">×</button></header>
    <ul class="menu">
      <li><button data-act="votes">${IC.votes}Stimmenanzahl setzen</button></li>
      <li><button data-act="count">${IC.count}Spieleranzahl ändern</button></li>
      <li><button data-act="calibrate">${IC.hand}Zeiger kalibrieren</button></li>
      <li><button data-act="reset" class="danger">${IC.reset}Alles zurücksetzen</button></li>
    </ul>`);
}

function openPicker() {
  seatOpen = -1;
  const closable = S.count > 0;
  let picks = '';
  for (let k = S.min; k <= S.max; k++) picks += `<button class="pick${k === S.count ? ' cur' : ''}" data-pick="${k}">${k}</button>`;
  showPanel(`<header><h2>Wie viele Spieler?</h2>${closable ? '<button class="x" data-close aria-label="Schließen">×</button>' : ''}</header>
    <div class="picks">${picks}</div>
    ${closable ? '<p class="hint">Namen und Status bleiben erhalten. Zum Leeren „Alles zurücksetzen“ im Menü wählen.</p>' : ''}`, closable);
}

// Stimmen direkt setzen: 0 bis Anzahl der Spieler
function openVotes() {
  seatOpen = -1;
  let picks = '';
  for (let k = 0; k <= Math.max(S.count, 1); k++) picks += `<button class="pick${k === S.votes ? ' cur' : ''}" data-votes="${k}">${k}</button>`;
  showPanel(`<header><h2>Stimmen setzen</h2><button class="x" data-close aria-label="Schließen">×</button></header>
    <div class="picks">${picks}</div>`);
}

function calibrationPanel(step, text, actions) {
  const jogs = [-500, -50, -5, 5, 50, 500];
  showPanel(`<header><h2>Zeiger kalibrieren (${step}/2)</h2><button class="x" data-close aria-label="Abbrechen">×</button></header>
    <p class="hint cal">${text}</p>
    <div class="jogs">${jogs.map(j => `<button class="btn" data-jog="${j}">${j > 0 ? '+' : '−'}${Math.abs(j)}</button>`).join('')}</div>
    <div class="calacts">${actions}</div>`);
}

async function openCalibration() {
  seatOpen = -1;
  if (!await api('/calibrate?phase=first')) return;
  calibrating = true;
  calibrationPanel(1, 'Stelle den Zeiger genau auf die weiß leuchtende LED. Das ist die erste Sitz-Position.',
    '<button class="btn primary" data-cal="last">Weiter</button>');
}

async function openCalibrationLast() {
  if (!await api('/calibrate?phase=last')) return;
  calibrationPanel(2, 'Der Zeiger fährt jetzt zur letzten LED. Stelle ihn genau auf die weiß leuchtende LED. Fährt er in die falsche Richtung, tippe auf „Falsch herum“.',
    '<button class="btn" data-cal="flip">Falsch herum</button><button class="btn primary" data-cal="done">Speichern</button>');
}

$('#panel').addEventListener('click', async e => {
  if (e.target.closest('[data-savename]')) {
    if (await saveName() && seatOpen >= 0) {
      closePanel();
      toast('Name gespeichert', true);
    }
    return;
  }
  const jog = e.target.closest('[data-jog]');
  if (jog) {
    api(`/handjog?steps=${jog.dataset.jog}`);
    return;
  }
  const cal = e.target.closest('[data-cal]');
  if (cal) {
    if (cal.dataset.cal === 'last') openCalibrationLast();
    else if (cal.dataset.cal === 'flip') api('/calibrate?phase=flip');
    else if (await api('/calibrate?phase=done')) {
      calibrating = false;
      closePanel();
      toast('Zeiger kalibriert', true);
    }
    return;
  }
  const votesBtn = e.target.closest('[data-votes]');
  if (votesBtn) {
    if (await api(`/setVotes?votes=${votesBtn.dataset.votes}`)) closePanel();
    return;
  }
  const stateBtn = e.target.closest('[data-state]');
  if (stateBtn && seatOpen >= 0) {
    if (await api(`/status?player=${seatOpen}&state=${stateBtn.dataset.state}`)) {
      $('#panel').querySelectorAll('[data-state]').forEach(b => b.setAttribute('aria-checked', b === stateBtn));
    }
    return;
  }
  const pick = e.target.closest('[data-pick]');
  if (pick) {
    if (await api(`/setup?player=${pick.dataset.pick}`)) closePanel(true);
    return;
  }
  const btn = e.target.closest('[data-act]');
  if (!btn) return;
  const i = seatOpen;
  switch (btn.dataset.act) {
    case 'block':
      if (await api(`/block?player=${i}`)) {
        btn.disabled = true;
        btn.lastChild.textContent = 'Ist auf dem Block';
      }
      break;
    case 'before':
    case 'after': {
      await saveName();
      const at = btn.dataset.act === 'before' ? i : i + 1;
      if (await api(`/add?at=${at}`)) openPlayer(at);
      break;
    }
    case 'remove': {
      const p = S.players[i];
      if (!confirm(`Sitz ${i + 1}${p.name ? ' (' + p.name + ')' : ''} wirklich aus dem Spiel entfernen?`)) return;
      seatOpen = -1;
      if (await api(`/remove?player=${i}`)) closePanel();
      else seatOpen = i;
      break;
    }
    case 'votes':
      openVotes();
      break;
    case 'count':
      openPicker();
      break;
    case 'calibrate':
      openCalibration();
      break;
    case 'reset':
      if (confirm('Alle Namen und Status zurücksetzen?') && await api('/reset')) closePanel();
      break;
  }
});

$('#backdrop').addEventListener('click', e => {
  if (e.target.id === 'backdrop' || e.target.closest('[data-close]')) closePanel();
});
addEventListener('keydown', e => { if (e.key === 'Escape' && !$('#backdrop').hidden) closePanel(); });
$('#seats').addEventListener('click', e => {
  const seat = e.target.closest('.seat');
  if (seat) openPlayer(+seat.dataset.seat);
});
$('#vote').addEventListener('click', () => api('/increaseMaxVote'));
$('#menu').addEventListener('click', openMenu);
$('#votesBtn').addEventListener('click', openVotes);
addEventListener('resize', render);
document.addEventListener('visibilitychange', () => { if (!document.hidden) api('/state'); });

render();
api('/state');
</script>
</body>
</html>
)rawliteral";
