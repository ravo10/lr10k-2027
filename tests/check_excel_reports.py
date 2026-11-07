"""Checks the sample reports written by tests/excel_writer_check.cpp (run by .github/workflows/tests.yml).

    python3 tests/check_excel_reports.py out_dir

Opens each file with openpyxl (a file Excel would call damaged usually fails here too) and checks the
sheets, the key report cells and the charts (series names, ranges) in the .xlsx XML.
"""
import re
import sys
import zipfile

import openpyxl

failures = 0


def check(condition, message):
    global failures
    if not condition:
        failures += 1
        print("FAIL:", message)


def cells(ws):
    """All non-empty cells as {coordinate: value}."""
    return {c.coordinate: c.value for row in ws.iter_rows() for c in row if c.value is not None}


def row_of(ws, label):
    for row in ws.iter_rows():
        if row[0].value == label:
            return [c.value for c in row]
    return None


def charts(path):
    """[(series names, ranges)] per chart, from the chart XML."""
    out = []
    with zipfile.ZipFile(path) as z:
        for name in sorted(n for n in z.namelist() if re.match(r"xl/charts/chart\d+\.xml", n)):
            xml = z.read(name).decode("utf-8")
            series = re.findall(r"<c:tx><c:v>([^<]*)</c:v></c:tx>", xml)
            ranges = re.findall(r"<c:f>([^<]*)</c:f>", xml)
            out.append((series, ranges))
    return out


def main(out_dir):
    stress_sheets = ["Graph Eng. Stress", "Graph True Stress"]

    # 1. Tension, rectangular
    path = f"{out_dir}/tension_rectangular.xlsx"
    wb = openpyxl.load_workbook(path)
    check(wb.sheetnames == ["Full Report", "Setup", "Graph N-mm"] + stress_sheets, f"{path}: sheets {wb.sheetnames}")
    report = wb["Full Report"]
    uts = row_of(report, "Ultimate Tensile Strength")
    check(uts is not None and abs(uts[1] - 20.0) < 1e-9 and abs(uts[2] - 10.0) < 1e-9, f"{path}: UTS row {uts}")
    check(uts is not None and abs(uts[3] - 22.0) < 1e-9, f"{path}: UTS true stress {uts}")
    yld = row_of(report, "Yielding Strength")
    check(yld is not None and yld[1:5] == ["-", "-", "-", "-"], f"{path}: yield (not placed) row {yld}")
    check(row_of(report, "Young's modulus (E)") is not None, f"{path}: no E-modulus line")
    check(row_of(report, "Zero point offset (slack)") is not None, f"{path}: no zero offset line")
    graph = cells(wb["Graph N-mm"])
    check(abs(graph["D2"] - 0.0) < 1e-6, f"{path}: first extension after the zero offset {graph.get('D2')}")
    check([graph.get("A2"), graph.get("A3"), graph.get("A4")] == ["Ultimate Tensile Strength", "Fracture", "E-Lower Bound"],
          f"{path}: crucial point rows {[graph.get('A2'), graph.get('A3'), graph.get('A4')]}")
    check(wb["Graph N-mm"].freeze_panes == "A2", f"{path}: header row not frozen")
    found = charts(path)
    check(len(found) == 3, f"{path}: {len(found)} charts")
    if found:
        series, ranges = found[0]
        check(series[:2] == ["Load", "Crucial point"], f"{path}: series names {series}")
        check("'Graph N-mm'!$D$2:$D$602" in ranges, f"{path}: curve range {ranges}")
        check("'Graph N-mm'!$B$2:$B$4" in ranges, f"{path}: points range {ranges}")
    if len(found) == 3:
        check(found[1][0][0] == "Engineering stress" and found[2][0][0] == "True stress", f"{path}: stress series {found[1][0]} {found[2][0]}")
    setup = wb["Setup"]
    check(row_of(setup, "Load error correction")[1] == -2.0, f"{path}: load error")
    check(row_of(setup, "Samples")[1] == 601, f"{path}: samples")

    # 2. Tension, circular: report and stress sheets
    path = f"{out_dir}/tension_circular.xlsx"
    wb = openpyxl.load_workbook(path)
    check(wb.sheetnames == ["Full Report", "Setup", "Graph N-mm"] + stress_sheets, f"{path}: sheets {wb.sheetnames}")
    check(row_of(wb["Full Report"], "Diameter") is not None, f"{path}: no diameter line")
    area = row_of(wb["Full Report"], "Cross-sectional area (A0)")
    check(area is not None and abs(area[1] - 78.5398163) < 1e-6, f"{path}: area {area}")

    # 3. 3-point bending: results, no stress sheets
    path = f"{out_dir}/bending_3point.xlsx"
    wb = openpyxl.load_workbook(path)
    check(wb.sheetnames == ["Full Report", "Setup", "Graph N-mm"], f"{path}: sheets {wb.sheetnames}")
    report = wb["Full Report"]
    check(row_of(report, "Flexural strength")[1] == 240.0, f"{path}: flexural strength")
    check(row_of(report, "Maximum bending moment")[2] == "N*mm", f"{path}: moment unit")
    check(row_of(report, "Support span (L)") is not None, f"{path}: no span line")
    check(row_of(report, "Ultimate Tensile Strength") is None, f"{path}: tension points table in a bending report")
    found = charts(path)
    check(len(found) == 1 and found[0][0] == ["Load"], f"{path}: charts {found}")  # no points: no points series

    # 4. No dimensions: note, load-extension only
    path = f"{out_dir}/no_dimensions.xlsx"
    wb = openpyxl.load_workbook(path)
    check(wb.sheetnames == ["Full Report", "Setup", "Graph N-mm"], f"{path}: sheets {wb.sheetnames}")
    note = row_of(wb["Full Report"], "Stress, strain and E-modulus")
    check(note is not None and "Not calculated" in note[1], f"{path}: note {note}")

    # 5. No data: valid file, no charts
    path = f"{out_dir}/no_data.xlsx"
    wb = openpyxl.load_workbook(path)
    check(cells(wb["Graph N-mm"]).get("G2") == "No data recorded for this test.", f"{path}: no-data note")
    check(len(charts(path)) == 0, f"{path}: charts without data")

    print("check_excel_reports:", "all checks passed" if failures == 0 else f"{failures} failure(s)")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "."))
