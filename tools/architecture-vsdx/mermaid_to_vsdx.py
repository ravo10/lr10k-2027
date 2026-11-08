"""Writes a Visio drawing (.vsdx) from the shapes extracted from Mermaid diagrams (extract.js -> shapes.json).

One page per diagram. Mermaid's own layout is kept: every outline, line and text at its position, in inches.
Each Mermaid node (a box with its texts, a state, a class) becomes one Visio group, so it can be moved and edited
as one piece; lines are separate shapes with arrowheads. Standard library only (zipfile, xml escaping).

    python3 mermaid_to_vsdx.py shapes.json out.vsdx "Page name 1" "Page name 2" ...
"""
import datetime
import html
import json
import re
import sys
import zipfile
from xml.sax.saxutils import escape

PX = 96.0          # Mermaid works in CSS pixels; Visio in inches
MARGIN = 0.5       # inches around each diagram
NS = 'xmlns="http://schemas.microsoft.com/office/visio/2012/main" ' \
     'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xml:space="preserve"'


def colour(css, default=None):
    """'rgb(r, g, b)' / 'rgba(...)' / '#rrggbb' -> '#RRGGBB', or None for none/transparent."""
    if not css or css in ('none', 'transparent'):
        return default
    m = re.match(r'rgba?\(([^)]*)\)', css)
    if m:
        parts = [float(x) for x in m.group(1).replace('/', ',').split(',') if x.strip()]
        if len(parts) == 4 and parts[3] == 0:
            return default
        return '#%02X%02X%02X' % tuple(int(round(v)) for v in parts[:3])
    if css.startswith('#') and len(css) in (4, 7):
        if len(css) == 4:
            css = '#' + ''.join(c * 2 for c in css[1:])
        return css.upper()
    return default


def cell(n, v, u=None):
    if isinstance(v, float):
        v = ('%.6f' % v).rstrip('0').rstrip('.')
    return '<Cell N="%s" V="%s"%s/>' % (n, escape(str(v)), (' U="%s"' % u) if u else '')


def arrow_for(marker):
    """Visio arrowhead index for a Mermaid marker id (0 = none)."""
    m = (marker or '').lower()
    if not m:
        return 0
    if 'composition' in m:
        return 0    # drawn as a filled diamond shape instead (see diamond_for)
    if 'aggregation' in m:
        return 21   # open diamond
    if 'extension' in m:
        return 21
    if 'cross' in m:
        return 0
    return 4        # filled triangle


class Page:
    def __init__(self, diagram):
        self.w_px, self.h_px = diagram['width'], diagram['height']
        self.prims = diagram['prims']
        self.next_id = 1
        self.width = self.w_px / PX + 2 * MARGIN
        self.height = self.h_px / PX + 2 * MARGIN

    # page coordinates (inches, y up)
    def X(self, x):
        return x / PX + MARGIN

    def Y(self, y):
        return (self.h_px - y) / PX + MARGIN

    def new_id(self):
        i = self.next_id
        self.next_id += 1
        return i

    # ---- one primitive -> one <Shape>, with coordinates relative to (ox, oy) -------------------------
    def shape_xml(self, p, ox, oy):
        sid = self.new_id()
        if p['kind'] == 'text':
            x0, y0, x1, y1 = p['box']
            lines = [html.unescape(html.unescape(t)) for t in p['lines']]
            w = (x1 - x0) / PX * 1.18 + 0.12
            h = max((y1 - y0) / PX, 0.12) + 0.04
            cx = self.X((x0 + x1) / 2) - ox
            cy = self.Y((y0 + y1) / 2) - oy
            align = {'start': 0, 'end': 2}.get(p.get('anchor'), 1)
            if align == 0:
                cx = self.X(x0) - ox + w / 2 - 0.06
            elif align == 2:
                cx = self.X(x1) - ox - w / 2 + 0.06
            bold = 1 if str(p.get('weight', '400')) in ('bold', '600', '700', '800', '900') else 0
            size_in = p.get('size', 16) * 0.75 / 72.0
            col = colour(p.get('color'), '#333333')
            return ('<Shape ID="%d" Type="Shape" LineStyle="0" FillStyle="0" TextStyle="0">' % sid
                    + cell('PinX', cx) + cell('PinY', cy) + cell('Width', w) + cell('Height', h)
                    + cell('LocPinX', w / 2) + cell('LocPinY', h / 2)
                    + cell('LinePattern', 0) + cell('FillPattern', 0)
                    + cell('LeftMargin', 0.0, 'PT') + cell('RightMargin', 0.0, 'PT')
                    + cell('TopMargin', 0.0, 'PT') + cell('BottomMargin', 0.0, 'PT')
                    + cell('VerticalAlign', 1)
                    + '<Section N="Character"><Row IX="0">' + cell('Size', size_in, 'PT') + cell('Color', col)
                    + cell('Style', bold) + '</Row></Section>'
                    + '<Section N="Paragraph"><Row IX="0">' + cell('HorzAlign', align) + '</Row></Section>'
                    + '<Text>' + escape('\n'.join(lines)) + '</Text></Shape>')

        line_col = colour(p.get('stroke'))
        fill_col = colour(p.get('fill'))
        if p.get('fillOpacity') == 0:
            fill_col = None
        weight_in = max(p.get('sw', 1), 0.5) / PX
        dashed = any(float(v) > 0 for v in re.findall(r'\d*\.?\d+', p.get('dash') or ''))

        if p['kind'] == 'ellipse':
            cx, cy = self.X(p['c'][0]) - ox, self.Y(p['c'][1]) - oy
            w, h = 2 * p['rx'] / PX, 2 * p['ry'] / PX
            geo = ('<Section N="Geometry" IX="0">' + cell('NoFill', 0 if fill_col else 1)
                   + cell('NoLine', 0 if line_col else 1)
                   + '<Row T="Ellipse" IX="1">' + cell('X', w / 2) + cell('Y', h / 2) + cell('A', w) + cell('B', h / 2)
                   + cell('C', w / 2) + cell('D', h) + '</Row></Section>')
        else:
            xs = [self.X(q[0]) for q in p['pts']]
            ys = [self.Y(q[1]) for q in p['pts']]
            minx, maxx, miny, maxy = min(xs), max(xs), min(ys), max(ys)
            w, h = max(maxx - minx, 0.001), max(maxy - miny, 0.001)
            cx, cy = (minx + maxx) / 2 - ox, (miny + maxy) / 2 - oy
            closed = p.get('closed')
            rows = []
            for k, (x, y) in enumerate(zip(xs, ys)):
                rows.append('<Row T="%s" IX="%d">' % ('MoveTo' if k == 0 else 'LineTo', k + 1)
                            + cell('X', x - minx) + cell('Y', y - miny) + '</Row>')
            if closed and (abs(xs[0] - xs[-1]) > 1e-6 or abs(ys[0] - ys[-1]) > 1e-6):
                rows.append('<Row T="LineTo" IX="%d">' % (len(xs) + 1) + cell('X', xs[0] - minx) + cell('Y', ys[0] - miny) + '</Row>')
            geo = ('<Section N="Geometry" IX="0">' + cell('NoFill', 0 if (closed and fill_col) else 1)
                   + cell('NoLine', 0 if line_col else 1) + ''.join(rows) + '</Section>')

        extra = ''
        if p['kind'] == 'poly' and not p.get('closed'):
            end, start = arrow_for(p.get('markerEnd')), arrow_for(p.get('markerStart'))
            if end:
                extra += cell('EndArrow', end) + cell('EndArrowSize', 2)
            if start:
                extra += cell('BeginArrow', start) + cell('BeginArrowSize', 2)
        rounding = p.get('rx', 0) / PX if p.get('closed') else 0
        return ('<Shape ID="%d" Type="Shape" LineStyle="0" FillStyle="0" TextStyle="0">' % sid
                + cell('PinX', cx) + cell('PinY', cy) + cell('Width', w) + cell('Height', h)
                + cell('LocPinX', w / 2) + cell('LocPinY', h / 2)
                + cell('LineWeight', weight_in) + cell('LineColor', line_col or '#000000')
                + cell('LinePattern', (2 if dashed else 1) if line_col else 0)
                + cell('FillForegnd', fill_col or '#FFFFFF') + cell('FillPattern', 1 if fill_col else 0)
                + (cell('Rounding', rounding) if rounding else '') + extra + geo + '</Shape>')

    @staticmethod
    def diamond_for(p, at_start):
        """A filled diamond (SysML composition) at the line's start or end, along the line (Mermaid draws it as a
        marker; Visio has no filled diamond arrowhead)."""
        pts = p['pts'] if at_start else p['pts'][::-1]
        x0, y0 = pts[0]
        for x1, y1 in pts[1:]:
            if abs(x1 - x0) + abs(y1 - y0) > 1:
                break
        L = max(((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5, 1e-6)
        ux, uy = (x1 - x0) / L, (y1 - y0) / L
        vx, vy = -uy, ux
        length, half = 18.0, 6.0
        tip, mid, back = (x0, y0), (x0 + ux * length / 2, y0 + uy * length / 2), (x0 + ux * length, y0 + uy * length)
        return {'kind': 'poly', 'closed': True, 'stroke': p.get('stroke'), 'fill': p.get('stroke'), 'sw': p.get('sw', 1),
                'dash': 'none', 'pts': [list(tip), [mid[0] + vx * half, mid[1] + vy * half], list(back),
                                        [mid[0] - vx * half, mid[1] - vy * half], list(tip)], 'group': None}

    def add_diamonds(self):
        extra = []
        for p in self.prims:
            if p['kind'] == 'poly' and not p.get('closed'):
                if 'composition' in (p.get('markerStart') or '').lower():
                    extra.append(self.diamond_for(p, True))
                if 'composition' in (p.get('markerEnd') or '').lower():
                    extra.append(self.diamond_for(p, False))
        self.prims.extend(extra)

    def bounds(self, prims):
        xs, ys = [], []
        for p in prims:
            if p['kind'] == 'text':
                x0, y0, x1, y1 = p['box']
                pad = (x1 - x0) * 0.1
                xs += [x0 - pad, x1 + pad]; ys += [y0, y1]
            elif p['kind'] == 'ellipse':
                xs += [p['c'][0] - p['rx'], p['c'][0] + p['rx']]; ys += [p['c'][1] - p['ry'], p['c'][1] + p['ry']]
            else:
                xs += [q[0] for q in p['pts']]; ys += [q[1] for q in p['pts']]
        return min(xs), min(ys), max(xs), max(ys)

    def assign_groups(self):
        """Mermaid's own node groups; and texts without one go with the smallest box around them."""
        boxes = [p for p in self.prims if p['kind'] == 'poly' and p.get('closed') and not p.get('group')]
        area = lambda p: (max(q[0] for q in p['pts']) - min(q[0] for q in p['pts'])) * (max(q[1] for q in p['pts']) - min(q[1] for q in p['pts']))
        page_area = self.w_px * self.h_px
        for n, t in enumerate(p for p in self.prims if p['kind'] == 'text' and not p.get('group')):
            x0, y0, x1, y1 = t['box']
            cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
            inside = [b for b in boxes
                      if min(q[0] for q in b['pts']) <= cx <= max(q[0] for q in b['pts'])
                      and min(q[1] for q in b['pts']) <= cy <= max(q[1] for q in b['pts'])
                      and area(b) < page_area * 0.05]
            if inside:
                b = min(inside, key=area)
                gid = b.get('group') or ('box%d' % id(b))
                b['group'] = gid
                t['group'] = gid

    def xml(self):
        self.add_diamonds()
        self.assign_groups()
        order, groups = [], {}
        for p in self.prims:
            g = p.get('group')
            if g:
                if g not in groups:
                    groups[g] = []
                    order.append(('g', g))
                groups[g].append(p)
            else:
                order.append(('p', p))
        out = []
        for kind, item in order:
            if kind == 'p':
                out.append(self.shape_xml(item, 0.0, 0.0))
                continue
            members = groups[item]
            if len(members) == 1:
                out.append(self.shape_xml(members[0], 0.0, 0.0))
                continue
            x0, y0, x1, y1 = self.bounds(members)
            gx0, gy0 = self.X(x0), self.Y(y1)            # lower-left in page inches
            gw, gh = (x1 - x0) / PX, (y1 - y0) / PX
            gid = self.new_id()
            children = ''.join(self.shape_xml(m, gx0, gy0) for m in members)
            out.append('<Shape ID="%d" Type="Group" LineStyle="0" FillStyle="0" TextStyle="0">' % gid
                       + cell('PinX', gx0 + gw / 2) + cell('PinY', gy0 + gh / 2) + cell('Width', gw) + cell('Height', gh)
                       + cell('LocPinX', gw / 2) + cell('LocPinY', gh / 2)
                       + '<Shapes>' + children + '</Shapes></Shape>')
        return '<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<PageContents %s><Shapes>%s</Shapes></PageContents>' % (NS, ''.join(out))


STYLE0 = ''.join([
    cell('EnableLineProps', 1), cell('EnableFillProps', 1), cell('EnableTextProps', 1), cell('HideForApply', 0),
    cell('LineWeight', 0.01041666666666667), cell('LineColor', '#000000'), cell('LinePattern', 1), cell('Rounding', 0),
    cell('EndArrowSize', 2), cell('BeginArrow', 0), cell('EndArrow', 0), cell('LineCap', 0), cell('BeginArrowSize', 2),
    cell('LineColorTrans', 0), cell('CompoundType', 0),
    cell('FillForegnd', '#FFFFFF'), cell('FillBkgnd', '#000000'), cell('FillPattern', 1),
    cell('ShdwForegnd', '#000000'), cell('ShdwPattern', 0), cell('FillForegndTrans', 0), cell('FillBkgndTrans', 0),
    cell('ShdwForegndTrans', 0), cell('ShapeShdwType', 0), cell('ShapeShdwOffsetX', 0), cell('ShapeShdwOffsetY', 0),
    cell('ShapeShdwObliqueAngle', 0), cell('ShapeShdwScaleFactor', 1), cell('ShapeShdwBlur', 0), cell('ShapeShdwShow', 0),
    cell('LeftMargin', 0.05555555555555555, 'PT'), cell('RightMargin', 0.05555555555555555, 'PT'),
    cell('TopMargin', 0.05555555555555555, 'PT'), cell('BottomMargin', 0.05555555555555555, 'PT'),
    cell('VerticalAlign', 1), cell('TextBkgnd', 0), cell('DefaultTabStop', 0.5), cell('TextDirection', 0),
    cell('TextBkgndTrans', 0), cell('LockTextEdit', 0), cell('LockVtxEdit', 0), cell('LockCalcWH', 0),
    cell('NoAlignBox', 0), cell('DynFeedback', 0), cell('GlueType', 0), cell('WalkPreference', 0),
    cell('BegTrigger', 0), cell('EndTrigger', 0), cell('ObjType', 0), cell('Comment', ''),
    cell('IsDropSource', 0), cell('NoLiveDynamics', 0), cell('LocalizeMerge', 0), cell('NoProofing', 0),
    cell('Calendar', 0), cell('LangID', 'en-US'), cell('ShapeKeywords', ''), cell('DropOnPageScale', 1),
    '<Section N="Character"><Row IX="0">', cell('Font', 'Calibri'), cell('Color', '#000000'), cell('Style', 0),
    cell('Case', 0), cell('Pos', 0), cell('FontScale', 1), cell('Size', 0.1666666666666667, 'PT'), cell('DblUnderline', 0),
    cell('Overline', 0), cell('Strikethru', 0), cell('DoubleStrikethrough', 0), cell('Letterspace', 0), cell('ColorTrans', 0),
    cell('AsianFont', 0), cell('ComplexScriptFont', 0), cell('ComplexScriptSize', -1), cell('LangID', 'en-US'),
    '</Row></Section>',
    '<Section N="Paragraph"><Row IX="0">', cell('IndFirst', 0), cell('IndLeft', 0), cell('IndRight', 0),
    cell('SpLine', -1.2), cell('SpBefore', 0), cell('SpAfter', 0), cell('HorzAlign', 1), cell('Bullet', 0),
    cell('BulletStr', ''), cell('BulletFont', 0), cell('BulletFontSize', -1), cell('TextPosAfterBullet', 0), cell('Flags', 0),
    '</Row></Section>',
])


def write_vsdx(diagrams, names, out_path):
    pages = [Page(d) for d in diagrams]
    now = datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')
    ct = ['<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">',
          '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>',
          '<Default Extension="xml" ContentType="application/xml"/>',
          '<Override PartName="/visio/document.xml" ContentType="application/vnd.ms-visio.drawing.main+xml"/>',
          '<Override PartName="/visio/pages/pages.xml" ContentType="application/vnd.ms-visio.pages+xml"/>',
          '<Override PartName="/visio/windows.xml" ContentType="application/vnd.ms-visio.windows+xml"/>',
          '<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>',
          '<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>']
    ct += ['<Override PartName="/visio/pages/page%d.xml" ContentType="application/vnd.ms-visio.page+xml"/>' % (i + 1) for i in range(len(pages))]
    ct.append('</Types>')
    rels = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="rId1" Type="http://schemas.microsoft.com/visio/2010/relationships/document" Target="visio/document.xml"/>'
            '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>'
            '<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>'
            '</Relationships>')
    doc_rels = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                '<Relationship Id="rId1" Type="http://schemas.microsoft.com/visio/2010/relationships/pages" Target="pages/pages.xml"/>'
                '<Relationship Id="rId2" Type="http://schemas.microsoft.com/visio/2010/relationships/windows" Target="windows.xml"/>'
                '</Relationships>')
    pages_rels = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                  + ''.join('<Relationship Id="rId%d" Type="http://schemas.microsoft.com/visio/2010/relationships/page" Target="page%d.xml"/>' % (i + 1, i + 1)
                            for i in range(len(pages))) + '</Relationships>')
    document = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<VisioDocument %s>' % NS
                + '<DocumentSettings TopPage="0" DefaultTextStyle="0" DefaultLineStyle="0" DefaultFillStyle="0" DefaultGuideStyle="0">'
                + cell('GlueSettings', 9) + cell('SnapSettings', 65847) + cell('SnapExtensions', 34) + cell('SnapAngles', 0)
                + cell('DynamicGridEnabled', 1) + cell('ProtectStyles', 0) + cell('ProtectShapes', 0) + cell('ProtectMasters', 0)
                + cell('ProtectBkgnds', 0) + '</DocumentSettings>'
                + '<FaceNames><FaceName NameU="Calibri" UnicodeRanges="-536859905 -1073732485 9 0" CharSets="536871423 0" Panose="2 15 5 2 2 2 4 3 2 4" Flags="325"/></FaceNames>'
                + '<StyleSheets><StyleSheet ID="0" NameU="No Style" IsCustomNameU="1" Name="No Style" IsCustomName="1">'
                + STYLE0 + '</StyleSheet></StyleSheets></VisioDocument>')
    pages_xml = ['<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Pages %s>' % NS]
    for i, (pg, name) in enumerate(zip(pages, names)):
        pages_xml.append('<Page ID="%d" NameU="%s" IsCustomNameU="1" Name="%s" IsCustomName="1" ViewScale="-1" ViewCenterX="%s" ViewCenterY="%s">'
                         % (i, escape(name), escape(name), pg.width / 2, pg.height / 2)
                         + '<PageSheet LineStyle="0" FillStyle="0" TextStyle="0">'
                         + cell('PageWidth', pg.width) + cell('PageHeight', pg.height) + cell('ShdwOffsetX', 0.125) + cell('ShdwOffsetY', -0.125)
                         + cell('PageScale', 1, 'IN') + cell('DrawingScale', 1, 'IN') + cell('DrawingSizeType', 3) + cell('DrawingScaleType', 0)
                         + cell('InhibitSnap', 0) + cell('PageLockReplace', 0) + cell('PageLockDuplicate', 0) + cell('UIVisibility', 0)
                         + cell('ShdwType', 0) + cell('ShdwObliqueAngle', 0) + cell('ShdwScaleFactor', 1) + cell('DrawingResizeType', 1)
                         + cell('PageShapeSplit', 1) + cell('PrintPageOrientation', 1 if pg.height >= pg.width else 2)
                         + '</PageSheet><Rel r:id="rId%d"/></Page>' % (i + 1))
    pages_xml.append('</Pages>')
    windows = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Windows ClientWidth="1600" ClientHeight="900" %s>' % NS
               + '<Window ID="0" WindowType="Drawing" WindowState="1073741824" WindowLeft="-8" WindowTop="-31" WindowWidth="1616" WindowHeight="939"'
               + ' ContainerType="Page" Page="0" ViewScale="-1" ViewCenterX="%s" ViewCenterY="%s"><ShowRulers>1</ShowRulers><ShowGrid>1</ShowGrid>'
               % (pages[0].width / 2, pages[0].height / 2)
               + '<ShowPageBreaks>0</ShowPageBreaks><ShowGuides>1</ShowGuides><ShowConnectionPoints>1</ShowConnectionPoints>'
               + '<GlueSettings>9</GlueSettings><SnapSettings>65847</SnapSettings><SnapExtensions>34</SnapExtensions><SnapAngles>0</SnapAngles>'
               + '<DynamicGridEnabled>1</DynamicGridEnabled><TabSplitterPos>0.5</TabSplitterPos></Window></Windows>')
    core = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" '
            'xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:dcterms="http://purl.org/dc/terms/" xmlns:dcmitype="http://purl.org/dc/dcmitype/" '
            'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><dc:title>LR 10K Control Software architecture (SysML)</dc:title>'
            '<dc:creator>LR 10K Control Software</dc:creator><dcterms:created xsi:type="dcterms:W3CDTF">%s</dcterms:created>'
            '<dcterms:modified xsi:type="dcterms:W3CDTF">%s</dcterms:modified></cp:coreProperties>' % (now, now))
    app = ('<?xml version="1.0" encoding="utf-8" standalone="yes"?>\n<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" '
           'xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes"><Application>Microsoft Visio</Application>'
           '<Template></Template><AppVersion>15.0000</AppVersion></Properties>')
    with zipfile.ZipFile(out_path, 'w', zipfile.ZIP_DEFLATED) as z:
        z.writestr('[Content_Types].xml', ''.join(ct))
        z.writestr('_rels/.rels', rels)
        z.writestr('docProps/core.xml', core)
        z.writestr('docProps/app.xml', app)
        z.writestr('visio/document.xml', document)
        z.writestr('visio/_rels/document.xml.rels', doc_rels)
        z.writestr('visio/windows.xml', windows)
        z.writestr('visio/pages/pages.xml', ''.join(pages_xml))
        z.writestr('visio/pages/_rels/pages.xml.rels', pages_rels)
        for i, pg in enumerate(pages):
            z.writestr('visio/pages/page%d.xml' % (i + 1), pg.xml())
    return pages


if __name__ == '__main__':
    diagrams = json.load(open(sys.argv[1], encoding='utf-8'))
    names = sys.argv[3:] or ['Diagram %d' % (i + 1) for i in range(len(diagrams))]
    pages = write_vsdx(diagrams, names, sys.argv[2])
    for n, p in zip(names, pages):
        print('%-28s %.1f x %.1f in, %d shapes' % (n, p.width, p.height, p.next_id - 1))
