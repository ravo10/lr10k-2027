"""Copy the README and the user documentation into a folder for the website.

Usage: python docs/site/stage.py <output folder>     (run from the repository root)

The website (GitHub Pages, built by .github/workflows/docs.yml) shows the README as
its front page and the Markdown pages under docs/ as normal pages. This script:
  - copies those pages, their pictures and the Jekyll settings in docs/site/,
  - points links to files that are not on the website (source code, LICENSE, ...)
    at the file on GitHub instead,
  - turns the README's video link (a GitHub upload) into a video player, as on GitHub.
"""
import os
import re
import shutil
import sys

REPO = "https://github.com/ravo10/lr10k-2027"
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

# Markdown pages on the website (paths from the repository root)
PAGES = ["README.md", "BUILDING.md", "ARCHITECTURE.md",
         "docs/index.md", "docs/developer.md", "docs/extensometer-setup.md",
         "docs/extensometer-markers.md", "docs/architecture/README.md"]
# Folders copied as they are (pictures)
FOLDERS = ["docs/images", "docs/themes", "docs/extensometer"]

LINK = re.compile(r'(\]\(|src=")([^)"\s#]+)([^)"]*)')
VIDEO = re.compile(r'^(https://github\.com/user-attachments/assets/[0-9a-f-]+)\s*$', re.M)


def on_site(path):
    """True if the repository file `path` is copied to the website."""
    return path in PAGES or any(path.startswith(f + "/") for f in FOLDERS)


def fix_links(text, page):
    page_dir = os.path.dirname(page)

    def repl(m):
        target = m.group(2)
        if re.match(r'^[a-z]+:', target) or target.startswith("/"):
            return m.group(0)
        path = os.path.normpath(os.path.join(page_dir, target)).replace(os.sep, "/")
        if on_site(path) or path.startswith(".."):
            return m.group(0)
        kind = "tree" if os.path.isdir(os.path.join(ROOT, path)) else "blob"
        return f"{m.group(1)}{REPO}/{kind}/main/{path}{m.group(3)}"

    return LINK.sub(repl, text)


def main():
    out = os.path.abspath(sys.argv[1])
    shutil.copytree(os.path.join(ROOT, "docs", "site"), out, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("stage.py", "__pycache__"))
    for page in PAGES:
        with open(os.path.join(ROOT, page), encoding="utf-8") as f:
            text = f.read()
        text = fix_links(text, page)
        if page == "README.md":
            text = VIDEO.sub(r'<video src="\1" controls muted playsinline width="100%"></video>', text)
        dest = os.path.join(out, page)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "w", encoding="utf-8") as f:
            f.write(text)
    for folder in FOLDERS:
        shutil.copytree(os.path.join(ROOT, folder), os.path.join(out, folder), dirs_exist_ok=True)


if __name__ == "__main__":
    main()
