/* POSEIDON site script. Null-guarded so subpages can share it
 * even when they don't have hero / particles / caustic canvases. */

/* Shared environment flags (mobile perf gating — added 2026-06-18).
 * Desktop keeps full-power effects; phones throttle/disable heavy loops. */
var POSEIDON_MOBILE = matchMedia('(max-width:768px)').matches;
var POSEIDON_REDUCED = matchMedia('(prefers-reduced-motion: reduce)').matches;

// Decorative canvases are presentation-only — hide from assistive tech (a11y, 2026-06-17).
addEventListener('DOMContentLoaded',function(){document.querySelectorAll('canvas').forEach(function(c){c.setAttribute('aria-hidden','true');});});


// Boot sequence
window.addEventListener('load', () => {
  setTimeout(() => {
    const bo = document.getElementById('bootOverlay');
    const hud = document.getElementById('hud');
    if (bo) bo.classList.add('hidden');
    if (hud) setTimeout(() => hud.classList.add('visible'), 400);
  }, 1200);
});

// Scroll progress + nav
const bar = document.getElementById('scrollProgress');
const mainNav = document.getElementById('mainNav');
window.addEventListener('scroll', () => {
  if (bar) {
    const pct = window.scrollY / (document.documentElement.scrollHeight - window.innerHeight) * 100;
    bar.style.width = pct + '%';
  }
  if (mainNav) mainNav.classList.toggle('scrolled', window.scrollY > 50);
}, { passive: true });

// Mobile nav (hamburger). No-op on desktop: the injected button is CSS-hidden
// above 768px and we only wire toggling behaviour at small viewports. Added 2026-06-18.
(function mobileNav() {
  const nav = document.getElementById('mainNav');
  if (!nav) return;
  const links = nav.querySelector('.nav-links');
  if (!links) return;

  // Inject an accessible hamburger button (hidden by CSS on desktop).
  const btn = document.createElement('button');
  btn.className = 'nav-toggle';
  btn.type = 'button';
  btn.setAttribute('aria-label', 'Toggle navigation menu');
  btn.setAttribute('aria-expanded', 'false');
  btn.setAttribute('aria-controls', 'navLinks');
  btn.innerHTML = '<span class="nt-bars" aria-hidden="true"></span>';
  if (!links.id) links.id = 'navLinks';
  nav.appendChild(btn);

  const mq = matchMedia('(max-width:768px)');
  const isMobile = () => mq.matches;

  function close() {
    links.classList.remove('open');
    btn.setAttribute('aria-expanded', 'false');
  }
  function open() {
    links.classList.add('open');
    btn.setAttribute('aria-expanded', 'true');
  }
  function toggle() {
    if (links.classList.contains('open')) close(); else open();
  }

  // Button only acts as a toggle on small screens. On desktop it's display:none,
  // so clicks/keys can't reach it — but guard anyway to stay a strict no-op.
  btn.addEventListener('click', () => { if (isMobile()) toggle(); });

  // Close on link tap, Escape, or outside tap — only while in mobile mode.
  links.addEventListener('click', (e) => {
    if (isMobile() && e.target.closest('a')) close();
  });
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && links.classList.contains('open')) { close(); btn.focus(); }
  });
  document.addEventListener('click', (e) => {
    if (!links.classList.contains('open')) return;
    if (!nav.contains(e.target)) close();
  });

  // If the viewport grows past the breakpoint, drop any open state so desktop
  // nav is untouched.
  const onChange = () => { if (!isMobile()) close(); };
  if (mq.addEventListener) mq.addEventListener('change', onChange);
  else if (mq.addListener) mq.addListener(onChange);
})();

// Scroll reveal
const obs = new IntersectionObserver((entries) => {
  entries.forEach(e => {
    if (e.isIntersecting) {
      e.target.classList.add('visible');
      obs.unobserve(e.target);
    }
  });
}, { threshold: 0.1 });
document.querySelectorAll('.reveal,.reveal-left').forEach(el => obs.observe(el));

// Stat number counters
const counterObs = new IntersectionObserver((entries) => {
  entries.forEach(e => {
    if (!e.isIntersecting) return;
    const el = e.target;
    const target = parseInt(el.dataset.count, 10);
    const suffix = el.dataset.suffix || '';
    const dur = 1400;
    const start = performance.now();
    const tick = (t) => {
      const p = Math.min(1, (t - start) / dur);
      const eased = 1 - Math.pow(1 - p, 3);
      el.textContent = Math.round(target * eased) + (p >= 1 ? suffix : '');
      if (p < 1) requestAnimationFrame(tick);
    };
    requestAnimationFrame(tick);
    counterObs.unobserve(el);
  });
}, { threshold: 0.6 });
document.querySelectorAll('[data-count]').forEach(el => counterObs.observe(el));

// Floating particles (ocean depth) — optional on any page
(function particles() {
  const pc = document.getElementById('particles');
  if (!pc) return;
  // Mobile perf: reduced-motion users get nothing; phones get far fewer
  // particles and a capped frame rate so the page stays smooth + battery-friendly.
  if (POSEIDON_REDUCED) return;
  const px = pc.getContext('2d');
  function resize() { pc.width = window.innerWidth; pc.height = window.innerHeight; }
  resize();
  window.addEventListener('resize', resize);
  const COUNT = POSEIDON_MOBILE ? 18 : 50;
  const MIN_DT = POSEIDON_MOBILE ? 1000 / 30 : 0; // cap ~30fps on phones
  const pts = Array.from({ length: COUNT }, () => ({
    x: Math.random() * pc.width, y: Math.random() * pc.height,
    vx: (Math.random() - 0.5) * 0.3, vy: (Math.random() - 0.5) * 0.3,
    r: Math.random() * 2 + 0.8, hue: Math.random() > 0.6 ? 190 : 205
  }));
  let last = 0;
  (function draw(t) {
    requestAnimationFrame(draw);
    if (t - last < MIN_DT) return;
    last = t || 0;
    px.clearRect(0, 0, pc.width, pc.height);
    pts.forEach(p => {
      p.x += p.vx + Math.sin(Date.now() * 0.001 + p.y * 0.01) * 0.2;
      p.y += p.vy;
      if (p.x < 0) p.x = pc.width;
      if (p.x > pc.width) p.x = 0;
      if (p.y < 0) p.y = pc.height;
      if (p.y > pc.height) p.y = 0;
      px.beginPath(); px.arc(p.x, p.y, p.r, 0, Math.PI * 2);
      px.fillStyle = `hsla(${p.hue},80%,60%,.45)`; px.fill();
      px.beginPath(); px.arc(p.x, p.y, p.r * 3, 0, Math.PI * 2);
      px.fillStyle = `hsla(${p.hue},80%,60%,.06)`; px.fill();
    });
  })(0);
})();

// Hero constellation — home page only
(function heroConstellation() {
  const hc = document.getElementById('heroCanvas');
  if (!hc) return;
  // Mobile perf: skip the O(n²) constellation link-up entirely for reduced-motion;
  // phones get fewer nodes (cheaper inner loop) and a capped frame rate.
  if (POSEIDON_REDUCED) return;
  const hx = hc.getContext('2d');
  function resize() { hc.width = hc.parentElement.clientWidth; hc.height = hc.parentElement.clientHeight; }
  resize();
  window.addEventListener('resize', resize);
  const NODE_COUNT = POSEIDON_MOBILE ? 12 : 24;
  const MIN_DT = POSEIDON_MOBILE ? 1000 / 30 : 0;
  const nodes = Array.from({ length: NODE_COUNT }, () => ({
    x: Math.random() * 1400, y: Math.random() * 900,
    vx: (Math.random() - 0.5) * 0.35, vy: (Math.random() - 0.5) * 0.35
  }));
  let last = 0;
  (function draw(t) {
    requestAnimationFrame(draw);
    if (t - last < MIN_DT) return;
    last = t || 0;
    hx.clearRect(0, 0, hc.width, hc.height);
    const cx = hc.width / 2, cy = hc.height / 2;
    nodes.forEach(n => {
      n.x += n.vx; n.y += n.vy;
      if (n.x < 0 || n.x > hc.width) n.vx *= -1;
      if (n.y < 0 || n.y > hc.height) n.vy *= -1;
    });
    for (let i = 0; i < nodes.length; i++) {
      for (let j = i + 1; j < nodes.length; j++) {
        const dx = nodes[i].x - nodes[j].x, dy = nodes[i].y - nodes[j].y;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < 220) {
          hx.beginPath(); hx.moveTo(nodes[i].x, nodes[i].y); hx.lineTo(nodes[j].x, nodes[j].y);
          hx.strokeStyle = `rgba(14,165,233,${0.2 * (1 - dist / 220)})`; hx.lineWidth = 0.6; hx.stroke();
        }
      }
    }
    nodes.forEach(n => {
      hx.beginPath(); hx.arc(n.x, n.y, 2, 0, Math.PI * 2);
      hx.fillStyle = 'rgba(14,165,233,.6)'; hx.fill();
      hx.beginPath(); hx.arc(n.x, n.y, 6, 0, Math.PI * 2);
      hx.fillStyle = 'rgba(14,165,233,.08)'; hx.fill();
    });
    const now = Date.now() * 0.001;
    for (let w = 0; w < 2; w++) {
      const waveR = ((now + w * 1.3) % 4) * 180;
      hx.beginPath(); hx.arc(cx, cy, waveR, 0, Math.PI * 2);
      hx.strokeStyle = `rgba(14,165,233,${0.18 * (1 - waveR / 720)})`; hx.lineWidth = 1; hx.stroke();
    }
  })(0);
})();

// Caustic water shader — home page only
(function caustic() {
  const cc = document.getElementById('causticLayer');
  if (!cc) return;
  // Mobile perf: the caustic shader is a per-pixel ImageData loop every frame —
  // by far the heaviest effect. Skip it outright on phones and reduced-motion.
  if (POSEIDON_MOBILE || POSEIDON_REDUCED) { cc.style.display = 'none'; return; }
  const cx2 = cc.getContext('2d');
  function resize() { cc.width = cc.parentElement.clientWidth; cc.height = cc.parentElement.clientHeight; }
  resize();
  window.addEventListener('resize', resize);
  (function draw() {
    const t = Date.now() * 0.0006;
    const w = cc.width, h = cc.height;
    const img = cx2.createImageData(Math.floor(w / 4), Math.floor(h / 4));
    const iw = img.width, ih = img.height;
    for (let y = 0; y < ih; y++) {
      for (let x = 0; x < iw; x++) {
        const u = x / iw * 6, v = y / ih * 4;
        const n = Math.sin(u + t) + Math.sin(v * 1.3 + t * 1.2) + Math.sin((u + v) * 0.8 + t * 0.7);
        const c = Math.pow(Math.max(0, n + 1) / 4, 3);
        const i = (y * iw + x) * 4;
        img.data[i] = 14 * c; img.data[i + 1] = 165 * c; img.data[i + 2] = 233 * c;
        img.data[i + 3] = Math.min(255, c * 140);
      }
    }
    cx2.putImageData(img, 0, 0);
    cx2.imageSmoothingEnabled = true;
    cx2.globalCompositeOperation = 'copy';
    cx2.drawImage(cc, 0, 0, iw, ih, 0, 0, w, h);
    cx2.globalCompositeOperation = 'source-over';
    requestAnimationFrame(draw);
  })();
})();

// Click ripple — all pages
(function clickRipple() {
  document.addEventListener('click', e => {
    const r = document.createElement('div');
    r.style.cssText = `position:fixed;left:${e.clientX}px;top:${e.clientY}px;width:0;height:0;border-radius:50%;background:radial-gradient(circle,rgba(14,165,233,.4),transparent);transform:translate(-50%,-50%);pointer-events:none;z-index:9999;animation:ripple .7s ease-out forwards`;
    document.body.appendChild(r);
    setTimeout(() => r.remove(), 700);
  });
  const rippleStyle = document.createElement('style');
  rippleStyle.textContent = '@keyframes ripple{to{width:280px;height:280px;opacity:0}}';
  document.head.appendChild(rippleStyle);
})();

// Hero parallax — home page only
(function heroParallax() {
  const heroContent = document.querySelector('.hero-content');
  if (!heroContent) return;
  document.addEventListener('mousemove', e => {
    if (window.scrollY > 400) return;
    const cx = window.innerWidth / 2, cy = window.innerHeight / 2;
    const dx = (e.clientX - cx) / cx * 6, dy = (e.clientY - cy) / cy * 6;
    heroContent.style.transform = `translate(${dx}px,${dy}px)`;
  });
})();

/* ─────────────────────────────────────────
 *  PREEM PASS — cyber/tech polish
 * ───────────────────────────────────────── */

// Matrix rain layer. Injects a <canvas id="matrixRain"> and runs it if the page
// has a <body data-matrix>. Safe on every page.
(function matrixRain() {
  if (document.body.dataset.matrix === undefined) return;
  // Mobile perf: reduced-motion users skip the always-on rain; phones use a
  // wider column step (fewer columns) and a capped frame rate.
  if (POSEIDON_REDUCED) return;
  const STEP = POSEIDON_MOBILE ? 22 : 14;     // px between columns (fewer cols on phones)
  const MIN_DT = POSEIDON_MOBILE ? 1000 / 24 : 0;
  const c = document.createElement('canvas');
  c.id = 'matrixRain';
  document.body.prepend(c);
  const x = c.getContext('2d');
  let cols, drops;
  const glyphs = 'アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロ0123456789<>/*[]{}|=+#$%▸◂┃━╱╲'.split('');
  function resize() {
    c.width = window.innerWidth;
    c.height = window.innerHeight;
    cols = Math.floor(c.width / STEP);
    drops = Array.from({ length: cols }, () => Math.random() * c.height / STEP);
  }
  resize();
  window.addEventListener('resize', resize);
  x.font = '12px "JetBrains Mono", monospace';
  let last = 0;
  (function draw(t) {
    requestAnimationFrame(draw);
    if (t - last < MIN_DT) return;
    last = t || 0;
    x.fillStyle = 'rgba(4,8,18,0.08)';
    x.fillRect(0, 0, c.width, c.height);
    for (let i = 0; i < cols; i++) {
      const g = glyphs[Math.floor(Math.random() * glyphs.length)];
      const py = drops[i] * STEP;
      // Random leading character: brighter
      if (Math.random() > 0.97) x.fillStyle = 'rgba(125,211,252,.9)';
      else x.fillStyle = 'rgba(14,165,233,' + (0.25 + Math.random() * 0.35) + ')';
      x.fillText(g, i * STEP, py);
      drops[i] = py > c.height && Math.random() > 0.975 ? 0 : drops[i] + 1;
    }
  })(0);
})();

// CRT sweep element
(function crtSweep() {
  if (document.getElementById('crtSweep')) return;
  const el = document.createElement('div');
  el.id = 'crtSweep';
  document.body.appendChild(el);
})();

// Cursor spotlight
(function spotlight() {
  if (matchMedia('(hover:none)').matches) return;
  const sp = document.createElement('div');
  sp.id = 'spotlight';
  document.body.appendChild(sp);
  document.body.classList.add('has-cursor');
  let tx = 0, ty = 0, cx = 0, cy = 0;
  document.addEventListener('mousemove', e => { tx = e.clientX; ty = e.clientY; });
  (function tick() {
    cx += (tx - cx) * 0.08;
    cy += (ty - cy) * 0.08;
    sp.style.left = cx + 'px';
    sp.style.top = cy + 'px';
    requestAnimationFrame(tick);
  })();
})();

// Glitch attribute — stamp data-text on every .hero-title for the ::before/::after
(function glitchPrime() {
  document.querySelectorAll('.hero-title,.page-head h1').forEach(h => {
    if (!h.dataset.text) h.dataset.text = h.textContent.trim();
  });
})();

// Frame corners helper — stamp 2 extra divs in any .frame so all four corners render
(function frames() {
  document.querySelectorAll('.frame').forEach(f => {
    if (!f.querySelector('.cnr-bl')) {
      const bl = document.createElement('span');
      const br = document.createElement('span');
      bl.className = 'cnr-bl';
      br.className = 'cnr-br';
      f.appendChild(bl);
      f.appendChild(br);
    }
  });
})();
