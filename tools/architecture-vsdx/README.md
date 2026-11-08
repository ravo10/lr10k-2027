# Visio file of the architecture diagrams

`docs/architecture/lr10k-architecture.vsdx` is made from the Mermaid diagrams in `docs/architecture/README.md`, so the Visio file, the diagrams on GitHub and the SysML model show the same thing. It has one page per diagram. Mermaid's layout is kept: every box, line and text is where Mermaid put it.

- **Groups:** each node (a block, a state, a box with its text) is one Visio group. Move it as one piece, or double-click into the group to edit a text.
- **Lines:** lines are separate shapes with arrowheads, so they do not follow when a box is moved.
- **Composition diamonds:** drawn as small filled shapes, because Visio has no filled diamond arrowhead.

After changing a diagram in `docs/architecture/README.md`, make the Visio file again:

1. **Get Mermaid's script once.** `npm pack` only downloads the package; it runs nothing:

   ```bash
   npm pack mermaid@11.4.1 && tar -xzf mermaid-11.4.1.tgz package/dist/mermaid.min.js
   ```

2. **Draw the diagrams and read out their shapes.** This needs Node.js with Playwright (`npm install playwright`, then `npx playwright install chromium`):

   ```bash
   node tools/architecture-vsdx/extract.js docs/architecture/README.md shapes.json package/dist/mermaid.min.js
   ```

3. **Write the Visio file.** This needs Python 3 only (standard library):

   ```bash
   python3 tools/architecture-vsdx/mermaid_to_vsdx.py shapes.json docs/architecture/lr10k-architecture.vsdx "1 Block definition" "2 Internal block" "3 State machine" "4 Activity" "5 Sequence"
   ```

To check the result without Visio: `soffice --headless --convert-to pdf docs/architecture/lr10k-architecture.vsdx`. LibreOffice reads `.vsdx`, and the free draw.io can open it too.

Edits made in Visio are not carried back to the Mermaid diagrams or the SysML model. Change those first, and make the Visio file again from them.
