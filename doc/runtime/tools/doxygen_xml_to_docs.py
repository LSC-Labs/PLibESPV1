#!/usr/bin/env python3
"""Create compact Markdown and PDF documentation from Doxygen XML output."""

from __future__ import annotations

import argparse
import datetime as dt
import os
import textwrap
import xml.etree.ElementTree as ET
from pathlib import Path


TITLE = "PLibESPV1 Runtime Library"


def text_of(node: ET.Element | None) -> str:
    if node is None:
        return ""
    parts: list[str] = []
    if node.text:
        parts.append(node.text)
    for child in node:
        parts.append(text_of(child))
        if child.tail:
            parts.append(child.tail)
    return " ".join("".join(parts).split())


def markdown_escape(text: str) -> str:
    return text.replace("|", "\\|")


def load_compounds(xml_dir: Path) -> list[dict]:
    index_path = xml_dir / "index.xml"
    root = ET.parse(index_path).getroot()
    compounds: list[dict] = []
    for compound in root.findall("compound"):
        kind = compound.get("kind", "")
        refid = compound.get("refid", "")
        name = text_of(compound.find("name"))
        if not refid or kind in {"dir", "page"}:
            continue
        xml_path = xml_dir / f"{refid}.xml"
        if not xml_path.exists():
            continue
        detail = ET.parse(xml_path).getroot().find("compounddef")
        if detail is None:
            continue
        compounds.append(
            {
                "kind": kind,
                "name": name,
                "brief": text_of(detail.find("briefdescription")),
                "detail": text_of(detail.find("detaileddescription")),
                "sections": detail.findall("sectiondef"),
            }
        )
    return sorted(compounds, key=lambda item: (item["kind"], item["name"].lower()))


def member_signature(member: ET.Element) -> str:
    definition = text_of(member.find("definition"))
    args = text_of(member.find("argsstring"))
    name = text_of(member.find("name"))
    if definition:
        return f"{definition}{args}"
    return name


def member_rows(sections: list[ET.Element]) -> list[tuple[str, str, str]]:
    rows: list[tuple[str, str, str]] = []
    for section in sections:
        section_kind = section.get("kind", "")
        if section_kind not in {
            "public-func",
            "public-static-func",
            "public-attrib",
            "public-static-attrib",
            "protected-func",
            "protected-attrib",
            "private-func",
        }:
            continue
        for member in section.findall("memberdef"):
            kind = member.get("kind", "")
            if kind not in {"function", "variable"}:
                continue
            signature = member_signature(member)
            brief = text_of(member.find("briefdescription"))
            detail = text_of(member.find("detaileddescription"))
            desc = brief or detail
            rows.append((section_kind, signature, desc))
    return rows


def write_markdown(compounds: list[dict], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    generated = dt.datetime.now().strftime("%Y-%m-%d %H:%M")
    lines: list[str] = [
        f"# {TITLE}",
        "",
        f"Generated from Doxygen XML on {generated}.",
        "",
        "## Overview",
        "",
        "This document is a compact Markdown companion to the full Doxygen HTML output.",
        "It lists the documented classes, structs, files and their public API members.",
        "",
        "## Contents",
        "",
    ]
    for compound in compounds:
        anchor = compound["name"].lower().replace("::", "").replace(" ", "-")
        lines.append(f"- [{compound['kind']} `{compound['name']}`](#{anchor})")
    lines.append("")

    for compound in compounds:
        lines.append(f"## {compound['kind']} `{compound['name']}`")
        lines.append("")
        if compound["brief"]:
            lines.append(compound["brief"])
            lines.append("")
        if compound["detail"] and compound["detail"] != compound["brief"]:
            lines.append(compound["detail"])
            lines.append("")
        rows = member_rows(compound["sections"])
        if rows:
            lines.extend(["| Scope | Member | Description |", "| --- | --- | --- |"])
            for scope, signature, desc in rows:
                lines.append(
                    f"| {markdown_escape(scope)} | `{markdown_escape(signature)}` | {markdown_escape(desc)} |"
                )
            lines.append("")
    output.write_text("\n".join(lines), encoding="utf-8")


def pdf_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")


def build_pdf_lines(markdown_path: Path) -> list[str]:
    lines: list[str] = []
    for raw in markdown_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line:
            lines.append("")
            continue
        line = line.replace("`", "")
        line = line.replace("| --- | --- | --- |", "")
        if line.startswith("| "):
            cells = [cell.strip() for cell in line.strip("|").split("|")]
            line = " - ".join(cells)
        if line.startswith("# "):
            lines.append(line[2:].upper())
        elif line.startswith("## "):
            lines.append("")
            lines.append(line[3:])
        elif line.startswith("- "):
            lines.append(line)
        else:
            lines.extend(textwrap.wrap(line, width=100) or [""])
    return lines


def make_pdf(markdown_path: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    logical_lines = build_pdf_lines(markdown_path)
    pages: list[list[str]] = []
    page: list[str] = []
    for line in logical_lines:
        page.append(line)
        if len(page) >= 54:
            pages.append(page)
            page = []
    if page:
        pages.append(page)

    objects: list[str] = []

    def add(obj: str) -> int:
        objects.append(obj)
        return len(objects)

    catalog_id = add("<< /Type /Catalog /Pages 2 0 R >>")
    pages_id = add("PLACEHOLDER")
    font_id = add("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>")
    page_ids: list[int] = []
    for page_lines in pages:
        stream_lines = ["BT", "/F1 10 Tf", "50 800 Td", "14 TL"]
        for idx, line in enumerate(page_lines):
            if idx:
                stream_lines.append("T*")
            stream_lines.append(f"({pdf_escape(line)}) Tj")
        stream_lines.append("ET")
        stream = "\n".join(stream_lines)
        content_id = add(f"<< /Length {len(stream.encode('latin-1', 'replace'))} >>\nstream\n{stream}\nendstream")
        page_id = add(
            f"<< /Type /Page /Parent {pages_id} 0 R /MediaBox [0 0 595 842] "
            f"/Resources << /Font << /F1 {font_id} 0 R >> >> /Contents {content_id} 0 R >>"
        )
        page_ids.append(page_id)
    objects[pages_id - 1] = f"<< /Type /Pages /Kids [{' '.join(f'{pid} 0 R' for pid in page_ids)}] /Count {len(page_ids)} >>"

    data = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
    offsets = [0]
    for idx, obj in enumerate(objects, start=1):
        offsets.append(len(data))
        data.extend(f"{idx} 0 obj\n{obj}\nendobj\n".encode("latin-1", "replace"))
    xref = len(data)
    data.extend(f"xref\n0 {len(objects) + 1}\n0000000000 65535 f \n".encode("ascii"))
    for offset in offsets[1:]:
        data.extend(f"{offset:010d} 00000 n \n".encode("ascii"))
    data.extend(
        f"trailer\n<< /Size {len(objects) + 1} /Root {catalog_id} 0 R >>\n"
        f"startxref\n{xref}\n%%EOF\n".encode("ascii")
    )
    output.write_bytes(data)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--xml", required=True, type=Path)
    parser.add_argument("--markdown", required=True, type=Path)
    parser.add_argument("--pdf", required=True, type=Path)
    args = parser.parse_args()

    compounds = load_compounds(args.xml)
    write_markdown(compounds, args.markdown)
    make_pdf(args.markdown, args.pdf)
    print(f"Wrote {args.markdown}")
    print(f"Wrote {args.pdf}")
    print(f"Documented compounds: {len(compounds)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
