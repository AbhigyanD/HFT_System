(function () {
  'use strict';
  const $ = (id) => document.getElementById(id);
  const css = (name) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();

  // ---------- chart ----------
  class Chart {
    constructor(canvas) {
      this.c = canvas; this.ctx = canvas.getContext('2d');
      this.values = []; this.markers = []; this.ref = null; this.prefix = '$'; this.color = '--accent'; this.hover = -1;
      new ResizeObserver(() => this.draw()).observe(canvas.parentElement);
      canvas.addEventListener('mousemove', (e) => {
        if (this.values.length < 2) return;
        const r = this.rect(), x = e.offsetX;
        this.hover = Math.max(0, Math.min(this.values.length - 1, Math.round((x - r.l) / (r.r - r.l) * (this.values.length - 1))));
        this.draw();
      });
      canvas.addEventListener('mouseleave', () => { this.hover = -1; this.draw(); });
    }
    set(values, markers, ref) { this.values = values; this.markers = markers || []; this.ref = ref == null ? null : ref; this.draw(); }
    rect() { const w = this.c.clientWidth, h = this.c.clientHeight; return { l: 64, r: w - 14, t: 44, b: h - 14 }; }
    draw() {
      const c = this.c, dpr = window.devicePixelRatio || 1, w = c.clientWidth, h = c.clientHeight;
      if (!w || !h) return;
      if (c.width !== Math.round(w * dpr) || c.height !== Math.round(h * dpr)) { c.width = Math.round(w * dpr); c.height = Math.round(h * dpr); }
      const g = this.ctx; g.setTransform(dpr, 0, 0, dpr, 0, 0); g.clearRect(0, 0, w, h);
      const text = css('--text'), dim = css('--dim'), border = css('--border'), panel = css('--panel');
      const v = this.values;
      g.font = '12px system-ui, sans-serif';
      if (v.length < 2) { g.fillStyle = dim; g.textAlign = 'center'; g.fillText('No data yet', w / 2, h / 2); return; }
      const R = this.rect();
      let lo = Math.min(...v), hi = Math.max(...v);
      if (this.ref != null) { lo = Math.min(lo, this.ref); hi = Math.max(hi, this.ref); }
      const pad = Math.max((hi - lo) * 0.08, 1e-6); lo -= pad; hi += pad;
      const X = (i) => R.l + (R.r - R.l) * i / (v.length - 1);
      const Y = (val) => R.b - (R.b - R.t) * (val - lo) / (hi - lo);
      g.textAlign = 'right'; g.textBaseline = 'middle';
      for (let i = 0; i <= 4; i++) {
        const y = R.t + (R.b - R.t) * i / 4, val = hi - (hi - lo) * i / 4;
        g.strokeStyle = border; g.lineWidth = 1; g.beginPath(); g.moveTo(R.l, y); g.lineTo(R.r, y); g.stroke();
        g.fillStyle = dim; g.fillText(this.prefix + val.toLocaleString(undefined, { maximumFractionDigits: hi > 1000 ? 0 : 2, minimumFractionDigits: hi > 1000 ? 0 : 2 }), R.l - 8, y);
      }
      if (this.ref != null) { g.setLineDash([5, 4]); g.strokeStyle = dim; g.beginPath(); g.moveTo(R.l, Y(this.ref)); g.lineTo(R.r, Y(this.ref)); g.stroke(); g.setLineDash([]); }
      g.strokeStyle = css(this.color); g.lineWidth = 2; g.lineJoin = 'round'; g.beginPath();
      // Decimate long series to ~2 points per pixel.
      const step = Math.max(1, Math.floor(v.length / ((R.r - R.l) * 2)));
      for (let i = 0; i < v.length; i += step) { i === 0 ? g.moveTo(X(i), Y(v[i])) : g.lineTo(X(i), Y(v[i])); }
      g.lineTo(X(v.length - 1), Y(v[v.length - 1])); g.stroke();
      const showMarkers = this.markers.length <= 600;
      if (showMarkers) for (const m of this.markers) {
        if (m.i < 0 || m.i >= v.length) continue;
        const x = X(m.i), y = Y(v[m.i]), s = 5;
        g.fillStyle = css(m.buy ? '--up' : '--down'); g.strokeStyle = panel; g.lineWidth = 1.5; g.beginPath();
        if (m.buy) { g.moveTo(x, y - 2); g.lineTo(x - s, y + 9); g.lineTo(x + s, y + 9); }
        else { g.moveTo(x, y + 2); g.lineTo(x - s, y - 9); g.lineTo(x + s, y - 9); }
        g.closePath(); g.fill(); g.stroke();
      }
      if (this.hover >= 0) {
        const x = X(this.hover), y = Y(v[this.hover]);
        g.strokeStyle = dim; g.setLineDash([2, 3]); g.beginPath(); g.moveTo(x, R.t); g.lineTo(x, R.b); g.stroke(); g.setLineDash([]);
        g.fillStyle = css(this.color); g.beginPath(); g.arc(x, y, 4, 0, 7); g.fill();
        const label = '#' + this.hover + '   ' + this.prefix + v[this.hover].toFixed(2);
        const bw = g.measureText(label).width + 16, bx = Math.min(x + 10, R.r - bw);
        g.fillStyle = panel; g.strokeStyle = border; g.beginPath(); g.roundRect(bx, R.t + 2, bw, 24, 6); g.fill(); g.stroke();
        g.fillStyle = text; g.textAlign = 'center'; g.fillText(label, bx + bw / 2, R.t + 14);
      }
    }
  }
  const liveChart = new Chart($('c-live'));
  const eqChart = new Chart($('c-eq')); eqChart.color = '--purple';
  const pxChart = new Chart($('c-px'));
  matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => [liveChart, eqChart, pxChart].forEach((c) => c.draw()));

  // ---------- helpers ----------
  const fmt = (n, d = 2) => n.toLocaleString(undefined, { minimumFractionDigits: d, maximumFractionDigits: d });
  const signed = (n, d = 2) => (n >= 0 ? '+' : '−') + fmt(Math.abs(n), d);
  function setKpi(id, text, sign) { const el = $(id); el.textContent = text; el.className = sign > 0 ? 'up' : sign < 0 ? 'down' : ''; }

  // ---------- engine ----------
  if (typeof NanoEX !== 'function') { $('fatal').hidden = false; return; }
  NanoEX().then((m) => {
    const setParams = m.cwrap('nx_set_params', null, Array(10).fill('number'));
    const backtest = m.cwrap('nx_backtest', 'string', ['number', 'number', 'number', 'number']);
    const simStart = m.cwrap('nx_sim_start', 'string', ['number', 'number', 'number']);
    const simStep = m.cwrap('nx_sim_step', 'string', []);

    const num = (id) => parseFloat($(id).value);
    function applyParams() {
      setParams(parseInt($('kind').value, 10), num('posSize'), num('stopLoss'), num('takeProfit'),
        Math.round(num('shortP')), Math.round(num('longP')), num('momThr'),
        Math.round(num('lookback')), num('entryPct'), num('exitPct'));
    }
    $('kind').addEventListener('change', () => {
      const mom = $('kind').value === '0'; $('p-mom').hidden = !mom; $('p-mr').hidden = mom;
    });

    // tabs
    function showTab(bt) {
      $('tab-live').setAttribute('aria-selected', String(!bt)); $('tab-bt').setAttribute('aria-selected', String(bt));
      $('view-live').hidden = bt; $('view-bt').hidden = !bt;
      requestAnimationFrame(() => [liveChart, eqChart, pxChart].forEach((c) => c.draw()));
    }
    $('tab-live').onclick = () => showTab(false); $('tab-bt').onclick = () => showTab(true);

    // ---------- backtest ----------
    function runBacktest() {
      applyParams();
      const r = JSON.parse(backtest(Math.round(num('btBars')), num('btDrift'), num('btVol'), Math.round(num('btSeed'))));
      if (!r.ok) { alert(r.error); return; }
      setKpi('b-ret', signed(r.return_pct) + '%', r.return_pct);
      setKpi('b-bh', signed(r.buy_hold_pct) + '%', r.buy_hold_pct);
      setKpi('b-trades', String(r.trades));
      setKpi('b-win', r.trades ? fmt(r.win_rate, 0) + '%' : '—');
      setKpi('b-dd', fmt(r.max_dd) + '%');
      setKpi('b-sharpe', fmt(r.sharpe));
      eqChart.set(r.equity, [], r.initial);
      pxChart.set(r.prices, r.markers.map(([i, b]) => ({ i, buy: b === 1 })));
    }
    $('runBt').onclick = runBacktest;

    // ---------- live simulation ----------
    let timer = null, prices = [], markers = [], firstIndex = 0, rows = 0;
    const MAX_POINTS = 600;

    function stop() { clearInterval(timer); timer = null; $('run').textContent = '▶ Start'; }
    function start() {
      applyParams();
      const seed = Math.floor(Math.random() * 1e9);
      const res = JSON.parse(simStart(num('liveVol'), 100, seed));
      if (!res.ok) { alert(res.error); return; }
      prices = []; markers = []; firstIndex = 0; rows = 0;
      $('log').innerHTML = ''; $('log-empty').hidden = false; liveChart.set([], []);
      $('run').textContent = '■ Stop';
      timer = setInterval(tick, parseInt($('speed').value, 10));
      tick();
    }
    function tick() {
      const s = JSON.parse(simStep());
      prices.push(s.price);
      if (prices.length > MAX_POINTS) { prices.shift(); firstIndex++; }
      if (s.trade) {
        markers.push({ i: s.index, buy: s.trade.buy }); addRow(s);
      }
      markers = markers.filter((mk) => mk.i >= firstIndex);
      liveChart.set(prices, markers.map((mk) => ({ i: mk.i - firstIndex, buy: mk.buy })));
      $('k-price').textContent = '$' + fmt(s.price);
      $('k-equity').textContent = '$' + fmt(s.equity);
      setKpi('k-pnl', signed(s.realized) , s.realized); $('k-pnl').textContent = (s.realized >= 0 ? '+$' : '−$') + fmt(Math.abs(s.realized));
      setKpi('k-pos', s.position > 0 ? 'Long ' + fmt(s.position, 0) : 'Flat');
      setKpi('k-trades', String(s.trades));
      setKpi('k-win', s.trades ? fmt(100 * s.wins / s.trades, 0) + '%' : '—');
    }
    function addRow(s) {
      const t = s.trade, tr = document.createElement('tr');
      const cell = (txt, cls) => { const td = document.createElement('td'); td.textContent = txt; if (cls) td.className = cls; tr.appendChild(td); };
      cell(String(s.index)); cell(t.buy ? 'BUY' : 'SELL', t.buy ? 'buy' : 'sell');
      cell('$' + fmt(s.price)); cell(fmt(t.qty, 0));
      t.buy ? cell('—') : cell((t.pnl >= 0 ? '+$' : '−$') + fmt(Math.abs(t.pnl)) + ' (' + signed(t.pnl_pct) + '%)', t.pnl >= 0 ? 'up' : 'down');
      cell(t.reason, 'reason'); tr.lastChild.title = t.reason;
      const body = $('log'); body.insertBefore(tr, body.firstChild);
      if (++rows > 200) { body.removeChild(body.lastChild); rows--; }
      $('log-empty').hidden = true;
    }
    $('run').onclick = () => (timer ? stop() : start());
    $('speed').onchange = () => { if (timer) { clearInterval(timer); timer = setInterval(tick, parseInt($('speed').value, 10)); } };

    runBacktest();  // show something immediately
    start();        // and a running market
  }).catch(() => { $('fatal').hidden = false; });
})();
