// Renders each ```mermaid block of a Markdown file with Mermaid in headless Chromium (SVG text labels, no HTML
// labels) and writes what is drawn - outlines, lines with their markers, texts - with absolute coordinates to a
// JSON file, for mermaid_to_vsdx.py. See README.md in this folder.
//
//   node extract.js <markdown file> <shapes.json> <mermaid.min.js>
const { chromium } = require('playwright');
const fs = require('fs');
const os = require('os');
const path = require('path');

const [readmePath, outPath, mermaidJs] = process.argv.slice(2);
if (!readmePath || !outPath || !mermaidJs) { console.error('usage: node extract.js <markdown> <shapes.json> <mermaid.min.js>'); process.exit(2); }
const readme = fs.readFileSync(readmePath, 'utf8');
const blocks = [...readme.matchAll(/```mermaid\n([\s\S]*?)```/g)].map(m => m[1]);
const pageDir = fs.mkdtempSync(path.join(os.tmpdir(), 'mermaid-vsdx-'));
fs.copyFileSync(mermaidJs, path.join(pageDir, 'mermaid.min.js'));
fs.writeFileSync(path.join(pageDir, 'page.html'),
  '<!doctype html><html><body style="margin:0"><div id="out"></div><script src="mermaid.min.js"></script></body></html>');

(async () => {
  const browser = await chromium.launch();
  const p = await browser.newPage({ viewport: { width: 2400, height: 1600 } });
  await p.goto('file://' + path.join(pageDir, 'page.html'));
  await p.waitForFunction(() => window.mermaid !== undefined);
  const diagrams = await p.evaluate(async (bl) => {
    mermaid.initialize({ startOnLoad: false, theme: 'default', htmlLabels: false,
      flowchart: { htmlLabels: false, useMaxWidth: false }, class: { htmlLabels: false, useMaxWidth: false },
      sequence: { useMaxWidth: false }, state: { useMaxWidth: false } });
    const out = [];
    for (let i = 0; i < bl.length; i++) {
      const { svg } = await mermaid.render('g' + i, bl[i]);
      const host = document.getElementById('out');
      host.innerHTML = svg;
      const root = host.querySelector('svg');
      root.removeAttribute('style'); root.style.maxWidth = 'none';
      const rb = root.getBoundingClientRect();
      const prims = [];
      const toRoot = (el, x, y) => {
        const m = el.getScreenCTM(); const pt = root.createSVGPoint(); pt.x = x; pt.y = y;
        const q = pt.matrixTransform(m); return [q.x - rb.left, q.y - rb.top];
      };
      // Which node (group) an element belongs to: the nearest <g> that is a node / cluster / actor
      const groupOf = (el) => {
        let g = el.parentElement;
        while (g && g !== root) {
          const c = (g.getAttribute('class') || '');
          if (/\b(node|statediagram-state|actor|classGroup|cluster)\b/.test(c) && g.id) return g.id;
          g = g.parentElement;
        }
        return null;
      };
      const style = (el) => {
        const cs = getComputedStyle(el);
        return { fill: cs.fill, stroke: cs.stroke, sw: parseFloat(cs.strokeWidth) || 1, dash: cs.strokeDasharray,
                 opacity: parseFloat(cs.opacity) || 1, fillOpacity: parseFloat(cs.fillOpacity) };
      };
      const visible = (el) => { const cs = getComputedStyle(el); return cs.display !== 'none' && cs.visibility !== 'hidden'; };
      root.querySelectorAll('rect, path, line, polyline, polygon, circle, ellipse').forEach(el => {
        if (!visible(el) || el.closest('marker') || el.closest('defs')) return;
        const st = style(el);
        const tag = el.tagName.toLowerCase();
        const markerEnd = el.getAttribute('marker-end') || '', markerStart = el.getAttribute('marker-start') || '';
        let pts = [];
        if (tag === 'rect') {
          const x = +el.getAttribute('x') || 0, y = +el.getAttribute('y') || 0, w = +el.getAttribute('width'), h = +el.getAttribute('height');
          if (!(w > 0 && h > 0)) return;
          pts = [toRoot(el, x, y), toRoot(el, x + w, y), toRoot(el, x + w, y + h), toRoot(el, x, y + h)];
          prims.push({ kind: 'poly', closed: true, pts, rx: +el.getAttribute('rx') || 0, ...st, group: groupOf(el) });
          return;
        }
        if (tag === 'circle' || tag === 'ellipse') {
          const cx = +el.getAttribute('cx') || 0, cy = +el.getAttribute('cy') || 0;
          const rx = +(el.getAttribute('r') || el.getAttribute('rx')), ry = +(el.getAttribute('r') || el.getAttribute('ry'));
          const c = toRoot(el, cx, cy), e = toRoot(el, cx + rx, cy + ry);
          prims.push({ kind: 'ellipse', c, rx: Math.abs(e[0] - c[0]), ry: Math.abs(e[1] - c[1]), ...st, group: groupOf(el) });
          return;
        }
        if (tag === 'line') {
          pts = [toRoot(el, +el.getAttribute('x1'), +el.getAttribute('y1')), toRoot(el, +el.getAttribute('x2'), +el.getAttribute('y2'))];
        } else if (tag === 'polygon' || tag === 'polyline') {
          pts = Array.from(el.points).map(q => toRoot(el, q.x, q.y));
        } else {
          let len = 0; try { len = el.getTotalLength(); } catch (e) { return; }
          if (!(len > 0)) return;
          const d = el.getAttribute('d') || '';
          const curved = /[CcQqSsTtAa]/.test(d);
          if (!curved) {
            // Straight segments: the vertices themselves
            const segs = d.match(/[MLHVZmlhvz][^MLHVZmlhvz]*/g) || [];
            let cur = [0, 0], start = [0, 0];
            for (const s of segs) {
              const c = s[0], nums = (s.slice(1).match(/-?\d*\.?\d+(?:e-?\d+)?/gi) || []).map(Number);
              if (c === 'M' || c === 'L') { for (let k = 0; k + 1 < nums.length; k += 2) { cur = [nums[k], nums[k + 1]]; if (c === 'M' && k === 0) start = cur; pts.push(toRoot(el, cur[0], cur[1])); } }
              else if (c === 'm' || c === 'l') { for (let k = 0; k + 1 < nums.length; k += 2) { cur = [cur[0] + nums[k], cur[1] + nums[k + 1]]; if (c === 'm' && k === 0) start = cur; pts.push(toRoot(el, cur[0], cur[1])); } }
              else if (c === 'H') { cur = [nums[0], cur[1]]; pts.push(toRoot(el, cur[0], cur[1])); }
              else if (c === 'h') { cur = [cur[0] + nums[0], cur[1]]; pts.push(toRoot(el, cur[0], cur[1])); }
              else if (c === 'V') { cur = [cur[0], nums[0]]; pts.push(toRoot(el, cur[0], cur[1])); }
              else if (c === 'v') { cur = [cur[0], cur[1] + nums[0]]; pts.push(toRoot(el, cur[0], cur[1])); }
              else if (c === 'Z' || c === 'z') { pts.push(toRoot(el, start[0], start[1])); cur = start; }
            }
          } else {
            const n = Math.min(60, Math.max(8, Math.ceil(len / 6)));
            for (let k = 0; k <= n; k++) { const q = el.getPointAtLength(len * k / n); pts.push(toRoot(el, q.x, q.y)); }
          }
        }
        if (pts.length < 2) return;
        const first = pts[0], last = pts[pts.length - 1];
        const closed = (tag === 'polygon') || (Math.hypot(first[0] - last[0], first[1] - last[1]) < 0.5 && pts.length > 3);
        prims.push({ kind: 'poly', closed, pts, ...st, markerEnd, markerStart, group: groupOf(el) });
      });
      root.querySelectorAll('text').forEach(el => {
        if (!visible(el)) return;
        // Lines: the text's own (outer) tspans; Mermaid nests the words in inner tspans
        const outer = Array.from(el.children).filter(c => c.tagName.toLowerCase() === 'tspan');
        let lines = outer.length ? outer.map(s => s.textContent) : [el.textContent];
        lines = lines.map(t => t.replace(/\s+/g, ' ').trim()).filter(t => t);
        if (!lines.length) return;
        const bb = el.getBBox(); const a = toRoot(el, bb.x, bb.y), b = toRoot(el, bb.x + bb.width, bb.y + bb.height);
        // The colour of the words: Mermaid sets it on the inner tspans for some diagrams (e.g. sequence actors)
        const leaf = el.querySelector('tspan tspan') || el.querySelector('tspan') || el;
        const cs = getComputedStyle(el); const leafFill = getComputedStyle(leaf).fill;
        prims.push({ kind: 'text', lines, box: [a[0], a[1], b[0], b[1]], size: parseFloat(cs.fontSize) || 16,
                     weight: cs.fontWeight, color: leafFill, anchor: cs.textAnchor, group: groupOf(el) });
      });
      out.push({ width: rb.width, height: rb.height, prims });
    }
    return out;
  }, blocks);
  fs.writeFileSync(outPath, JSON.stringify(diagrams));
  diagrams.forEach((d, i) => console.log('diagram ' + (i + 1) + ': ' + Math.round(d.width) + ' x ' + Math.round(d.height) + ' px, ' + d.prims.length + ' primitives'));
  await browser.close();
})();
