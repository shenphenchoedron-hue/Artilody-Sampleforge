#!/usr/bin/env python3
"""Renders docs/manual/manual-*.md into PDF manuals with ReportLab.

    python3 -m pip install reportlab
    python3 docs/build-manuals.py

Supported Markdown subset: '# ', '## ', '### ' headings, paragraphs,
'- ' bullets, '1. ' numbered lists, '> ' notes, '```' code blocks,
'| a | b |' tables, **bold**, *italic* and `code`.
"""
import re
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import (Image, KeepTogether, ListFlowable, ListItem,
                                PageBreak, Paragraph, Preformatted,
                                SimpleDocTemplate, Spacer, Table, TableStyle)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = Path(__file__).resolve().parent
ICON = ROOT.parent / "packaging" / "linux" / "icons" / "256.png"
VIOLET = colors.HexColor("#351b58")
GOLD = colors.HexColor("#bb6a26")
RUBY = colors.HexColor("#be386e")
GREY = colors.HexColor("#555555")

MANUALS = {
    "manual-en.md": ("Artilody-Sampleforge-Manual-EN.pdf", "User Manual", "Contents", "Page"),
    "manual-da.md": ("Artilody-Sampleforge-Manual-DA.pdf", "Brugermanual", "Indhold", "Side"),
}

base = dict(fontName="Helvetica", fontSize=10.5, leading=15, textColor=colors.black)
S = {
    "body": ParagraphStyle("body", spaceAfter=6, **base),
    "h1": ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=19, leading=24,
                         textColor=VIOLET, spaceBefore=4, spaceAfter=10),
    "h2": ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=13.5, leading=18,
                         textColor=RUBY, spaceBefore=10, spaceAfter=5),
    "h3": ParagraphStyle("h3", fontName="Helvetica-Bold", fontSize=11.5, leading=15,
                         textColor=GOLD, spaceBefore=8, spaceAfter=3),
    "note": ParagraphStyle("note", backColor=colors.HexColor("#f6efe0"),
                           borderColor=GOLD, borderWidth=0.6, borderPadding=6,
                           spaceBefore=6, spaceAfter=10, leftIndent=6, rightIndent=6, **base),
    "code": ParagraphStyle("code", fontName="Courier", fontSize=9, leading=12,
                           backColor=colors.HexColor("#f1eef6"), borderPadding=5,
                           spaceBefore=4, spaceAfter=10, leftIndent=6, rightIndent=6),
    "cell": ParagraphStyle("cell", fontName="Helvetica", fontSize=9.5, leading=12.5),
    "title": ParagraphStyle("title", fontName="Helvetica-Bold", fontSize=30, leading=36,
                            alignment=TA_CENTER, textColor=VIOLET),
    "subtitle": ParagraphStyle("subtitle", fontName="Helvetica", fontSize=15, leading=20,
                               alignment=TA_CENTER, textColor=RUBY),
    "small": ParagraphStyle("small", fontName="Helvetica", fontSize=10, leading=14,
                            alignment=TA_CENTER, textColor=GREY),
    "toc1": ParagraphStyle("toc1", fontName="Helvetica-Bold", fontSize=11, leading=17, leftIndent=0),
    "toc2": ParagraphStyle("toc2", fontName="Helvetica", fontSize=10, leading=14, leftIndent=16),
}


def inline(text):
    text = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    text = re.sub(r"`([^`]+)`", r'<font face="Courier" color="#5a2d82">\1</font>', text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    text = re.sub(r"(?<![\w*])\*([^*]+)\*(?![\w*])", r"<i>\1</i>", text)
    text = re.sub(r"(https?://[^\s<)]+)", r'<link href="\1" color="#5a2d82">\1</link>', text)
    return text


class Doc(SimpleDocTemplate):
    def afterFlowable(self, flowable):
        if isinstance(flowable, Paragraph) and flowable.style.name in ("h1", "h2"):
            level = 0 if flowable.style.name == "h1" else 1
            text = flowable.getPlainText()
            key = "h%d" % id(flowable)
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(text, key, level=level)
            self.notify("TOCEntry", (level, text, self.page, key))


def parse(md):
    lines = md.splitlines()
    meta, story, i = {}, [], 0
    while i < len(lines) and lines[i].startswith("%"):
        k, _, v = lines[i][1:].partition(":")
        meta[k.strip()] = v.strip()
        i += 1
    para, items, kind = [], [], None

    def flush():
        nonlocal para, items, kind
        if para:
            story.append(Paragraph(inline(" ".join(para)), S["body"]))
            para = []
        if items:
            flow = [ListItem(Paragraph(inline(t), S["body"]), leftIndent=14) for t in items]
            story.append(ListFlowable(flow, bulletType="1" if kind == "ol" else "bullet",
                                      start="1" if kind == "ol" else None,
                                      bulletFontName="Helvetica", bulletFontSize=9,
                                      leftIndent=14, bulletColor=GOLD))
            items, kind = [], None

    while i < len(lines):
        line = lines[i]
        s = line.strip()
        if s.startswith("```"):
            flush()
            block = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                block.append(lines[i])
                i += 1
            story.append(Preformatted("\n".join(block), S["code"]))
        elif s == "\\pagebreak":
            flush()
            story.append(PageBreak())
        elif s.startswith("|"):
            flush()
            rows = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                cells = [c.strip() for c in lines[i].strip().strip("|").split("|")]
                if not all(re.fullmatch(r":?-+:?", c) for c in cells):
                    rows.append(cells)
                i += 1
            i -= 1
            data = [[Paragraph(("<b>%s</b>" if r == 0 else "%s") % inline(c), S["cell"])
                     for c in row] for r, row in enumerate(rows)]
            ncol = len(rows[0])
            longest = max(len(re.sub(r"[`*]", "", row[0])) for row in rows)
            first = min(112, max(32, longest * 2.05 + 4)) * mm
            widths = [first] + [(170 * mm - first) / (ncol - 1)] * (ncol - 1) if ncol > 1 else None
            t = Table(data, colWidths=widths, repeatRows=1)
            t.setStyle(TableStyle([
                ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#ece4f5")),
                ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#c9bcd9")),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
            ]))
            story.append(t)
            story.append(Spacer(1, 8))
        elif s.startswith("### "):
            flush()
            story.append(Paragraph(inline(s[4:]), S["h3"]))
        elif s.startswith("## "):
            flush()
            story.append(Paragraph(inline(s[3:]), S["h2"]))
        elif s.startswith("# "):
            flush()
            if any(isinstance(f, Paragraph) and f.style.name == "h1" for f in story):
                story.append(PageBreak())
            story.append(Paragraph(inline(s[2:]), S["h1"]))
        elif s.startswith("> "):
            flush()
            note = [s[2:]]
            while i + 1 < len(lines) and lines[i + 1].strip().startswith(">"):
                i += 1
                note.append(lines[i].strip().lstrip(">").strip())
            story.append(Paragraph(inline(" ".join(note)), S["note"]))
        elif re.match(r"^- ", s):
            if kind != "ul":
                flush()
            kind = "ul"
            items.append(s[2:])
        elif re.match(r"^\d+\. ", s):
            if kind != "ol":
                flush()
            kind = "ol"
            items.append(re.sub(r"^\d+\. ", "", s))
        elif not s:
            flush()
        elif items and line.startswith("  "):
            items[-1] += " " + s
        else:
            para.append(s)
        i += 1
    flush()
    return meta, story


def build(src, out, label, toc_title, page_word):
    meta, body = parse((ROOT / "manual" / src).read_text(encoding="utf-8"))
    version = meta.get("version", "")

    def footer(canvas, doc):
        canvas.saveState()
        canvas.setFont("Helvetica", 8.5)
        canvas.setFillColor(GREY)
        canvas.drawString(20 * mm, 12 * mm, "Artilody Sampleforge %s - %s" % (version, label))
        canvas.drawRightString(190 * mm, 12 * mm, "%s %d" % (page_word, doc.page))
        canvas.setStrokeColor(GOLD)
        canvas.setLineWidth(0.5)
        canvas.line(20 * mm, 16 * mm, 190 * mm, 16 * mm)
        canvas.restoreState()

    cover = [Spacer(1, 35 * mm)]
    if ICON.exists():
        cover.append(Image(str(ICON), width=55 * mm, height=55 * mm))
    cover += [Spacer(1, 12 * mm), Paragraph("Artilody Sampleforge", S["title"]),
              Spacer(1, 4 * mm), Paragraph(label, S["subtitle"]), Spacer(1, 10 * mm),
              Paragraph(meta.get("tagline", ""), S["small"]), Spacer(1, 4 * mm),
              Paragraph("Version %s &nbsp;|&nbsp; %s" % (version, meta.get("date", "")), S["small"]),
              Spacer(1, 4 * mm), Paragraph("macOS &nbsp;|&nbsp; Windows &nbsp;|&nbsp; Debian / Ubuntu Linux", S["small"]),
              Spacer(1, 30 * mm),
              Paragraph(inline("https://github.com/shenphenchoedron-hue/Artilody-Sampleforge"), S["small"]),
              PageBreak()]
    toc = TableOfContents()
    toc.levelStyles = [S["toc1"], S["toc2"]]
    story = cover + [Paragraph(toc_title, ParagraphStyle("toch", parent=S["h1"])), toc, PageBreak()] + body

    doc = Doc(str(ROOT / out), pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm,
              topMargin=20 * mm, bottomMargin=22 * mm, title="Artilody Sampleforge - " + label,
              author="Artilody", subject="Artilody Sampleforge %s" % version)
    doc.multiBuild(story, onFirstPage=lambda c, d: None, onLaterPages=footer)
    print("Wrote", ROOT / out)


if __name__ == "__main__":
    for src, (out, label, toc_title, page_word) in MANUALS.items():
        build(src, out, label, toc_title, page_word)
