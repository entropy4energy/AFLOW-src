#!/usr/bin/env python3
"""Refuse ICSD-derived structures in anything public (AGENTS.md, Licensed Data). Mentioning the database,
or a number from it, is fine.

    check_public_icsd.py tree DIR                 every file under DIR
    check_public_icsd.py history GITDIR           every object of a clone, reachable or not
    check_public_icsd.py range GITDIR BASE HEAD   what HEAD adds to BASE
    check_public_icsd.py selftest                 fails unless every rule fires

    --baseline REF      lines and files of upstream's release REF pass (a clone with that tag)
    --approved FILE     hashes Corey has passed (default check_public_icsd_approved.txt beside this script)
    --no-fingerprints   only where .icsd_fingerprints cannot be, as on GitHub's runners

Rules: CIF content; a CIF file; PRIVATE, the internal records files; RECORD, a JHU record in the public
code; VALUES, a list of five or more numbers that is an ICSD-derived parameter set (hashes in
.icsd_fingerprints, at the workspace root, never committed); NEWFILE, with --baseline, a file that neither
upstream nor the approved release list has, so a whole tree can never go out by accident.


A line counts as CIF content when it carries a data key with its value (_cell_length_a 4.0, for example),
embedded in a string or not, in any file type. Code that writes CIFs passes. A .cif, .mcif or .icsd file counts whole.
Exit status 1 on a finding, 2 when the check cannot run. The report gives places only, never the content.
"""
import argparse, hashlib, os, re, subprocess, sys
from pathlib import Path

# a data key with its value: code that merely writes CIFs has a format or a variable there, not a number
KEY = re.compile(r"_cell_length_[abc](?:\s|\\n|\\t)+\d|_atom_site_fract_[xyz](?:\s|\\n|\\t)+\S*\s*[A-Za-z0-9]+[\w+-]*\s+[A-Za-z0-9+-]+\s+-?\d|_database_code_\w+\s+\d|"
                 r"_symmetry_cell_setting\s+'?[a-z]|_chemical_formula_sum\s+'[A-Z]", re.I)
FILETYPE = re.compile(r"\.(cif|mcif|icsd)(\.(gz|xz|zip|bz2))?$", re.I)
PRIVATE = re.compile(r"(^|/)aflow_prototypes_jhu_records[^/]*$")
RECORD = re.compile(r'\{"[A-Z][A-Za-z0-9]*_[^"]+",\s*\d{3,}\s*,')
NUMS = re.compile(r"-?\d+\.\d+(?:[eE][-+]?\d+)?|-?\d+(?=[,\s\"';\]])")
SELF = {"check_public_icsd.py", "check_public_icsd_approved.txt"}
BINARY = (".pdf", ".png", ".jpg", ".jpeg", ".gif", ".ico", ".woff", ".woff2", ".mp4", ".pyc")


def sha(s):
    return hashlib.sha256(s.encode("utf-8", "surrogateescape")).hexdigest()


def norm(line):
    return " ".join(line.split())


def fingerprints_of(values):
    """Hashes of one list of numbers at two precisions, so a rounded copy still matches."""
    return [sha(",".join("%.*f" % (d, float(v)) for v in values))[:24] for d in (3, 2)]


class Scan:
    def __init__(self, approved, baseline, fingerprints=(), paths=(), upstream=None):
        self.approved, self.baseline, self.findings = approved, baseline, []
        self.fp, self.paths, self.upstream = fingerprints, paths, upstream
        self.internal = False

    def line(self, where, line):
        n = norm(line)
        if n in self.baseline or sha(n) in self.approved:
            return
        if KEY.search(line):
            self.findings.append("CIF content  " + where)
        if self.fp and len(line) < 200000:
            for chunk in re.split(r"[\"';]", line):
                vals = NUMS.findall(chunk)
                if len(vals) >= 5:
                    try:
                        if any(f in self.fp for f in fingerprints_of(vals)):
                            self.findings.append("VALUES       " + where)
                            break
                    except ValueError:
                        pass

    def path(self, name):
        if self.internal:
            return
        if PRIVATE.search(name):
            self.findings.append("PRIVATE file " + name)
        if self.upstream is not None and name not in self.upstream and sha(name) not in self.paths and os.path.basename(name) not in SELF:
            self.findings.append("NEW file     " + name)

    def file(self, name, data):
        self.path(name)
        if os.path.basename(name) in SELF:
            return
        if not self.internal and os.path.basename(name) == "aflow_prototypes_jhu.cpp" and RECORD.search(data.decode("utf-8", "surrogateescape")):
            self.findings.append("RECORD       " + name)
        if FILETYPE.search(name) and norm(name) not in self.baseline:
            self.findings.append("CIF file     " + name)
        if name.lower().endswith(BINARY):
            return
        for i, l in enumerate(data.decode("utf-8", "surrogateescape").splitlines(), 1):
            self.line("%s:%d" % (name, i), l)


def git(gd, *a):
    p = subprocess.run(["git", "-C", gd] + list(a), capture_output=True)
    if p.returncode:
        print("git %s failed: %s" % (a[0], p.stderr.decode()[:200]), file=sys.stderr)
        sys.exit(2)
    return p.stdout


def baseline_of(gd, ref):
    base, files = set(), set()
    for f in git(gd, "ls-tree", "-r", "-z", "--name-only", ref).decode("utf-8", "surrogateescape").split("\0"):
        if f:
            base.add(norm(f)); files.add(f)
            for l in git(gd, "cat-file", "blob", "%s:%s" % (ref, f)).decode("utf-8", "surrogateescape").splitlines():
                if KEY.search(l) or len(NUMS.findall(l)) >= 5:
                    base.add(norm(l))
    return base, files


def tree(s, root):
    root = Path(root)
    for p in sorted(root.rglob("*")):
        rel = p.relative_to(root)
        if p.is_file() and ".git" not in rel.parts:
            s.file(str(rel), p.read_bytes())


def history(s, gd):
    names = {}
    for l in git(gd, "rev-list", "--all", "--objects").decode("utf-8", "surrogateescape").splitlines():
        if " " in l:
            o, n = l.split(" ", 1)
            names.setdefault(o, n)
    objs = git(gd, "cat-file", "--batch-all-objects", "--batch-check=%(objectname) %(objecttype)", "--unordered").decode().splitlines()
    for o in objs:
        oid, typ = o.split()
        if typ == "blob":
            s.file(names.get(oid, "blob-" + oid[:10]), git(gd, "cat-file", "blob", oid))
    print("%s: %d objects read" % (gd, len(objs)))


def span(s, gd, base, head):
    if set(base) == {"0"}:
        base = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"  # the empty tree: a new branch
    cur, n = "?", 0
    for l in git(gd, "diff", "--no-color", "--unified=0", "--no-ext-diff", base, head).decode("utf-8", "surrogateescape").splitlines():
        if l.startswith("+++ "):
            cur = l[4:].removeprefix("b/")
            if cur != "/dev/null":
                s.path(cur)
                if FILETYPE.search(cur) and os.path.basename(cur) not in SELF and norm(cur) not in s.baseline:
                    s.findings.append("CIF file     " + cur)
                if not s.internal and os.path.basename(cur) == "aflow_prototypes_jhu.cpp" and RECORD.search(git(gd, "show", "%s:%s" % (head, cur)).decode("utf-8", "surrogateescape")):
                    s.findings.append("RECORD       " + cur)
        elif l.startswith("@@"):
            m = re.search(r"\+(\d+)", l)
            n = int(m.group(1)) if m else 0
        elif l.startswith("+"):
            if os.path.basename(cur) not in SELF:
                s.line("%s:%d" % (cur, n), l[1:])
            n += 1


def selftest():
    vals = "1.234,2.345,3.456,4.567,5.678"
    fp = set(fingerprints_of(vals.split(",")))
    s = Scan(set(), set(), fp, set(), {"src/a.cpp"})
    s.file("src/a.cpp", ('x = "%s";' % vals).encode())
    s.file("src/b.json", b'["1.2341,2.3449,3.4562,4.5668,5.6781"]')
    s.file("src/aflow_prototypes_jhu_records.inc", b"")
    s.file("src/aflow_prototypes_jhu.cpp", b'{"AB_cF8_225_a_b", 1001, "a", "4.0", "AB", "n", "-", "r", "XX", false},')
    rules = [f.split()[0] for f in s.findings]
    print(sorted(set(rules)))
    if not {"VALUES", "PRIVATE", "RECORD", "NEW"} <= set(rules) or rules.count("VALUES") != 2:
        print("MISSED a rule", rules)
        return 1
    s = Scan(set(), {'x = "%s";' % vals}, fp)
    s.file("src/a.cpp", ('x = "%s";' % vals).encode())
    s.file("src/aflow_prototypes_jhu.cpp", b'static const std::vector<Record> vrecords = {\n};')
    if s.findings:
        print("FALSE HIT", s.findings)
        return 1
    must = [("x.cif", b""), ("x.CIF.gz", b""), ("a.icsd", b""), ("t.json", b'{"cif_x": "_cell_length_a 4.0\\n_cell_length_b 4.0"}'),
            ("t.cpp", b'const char* c = "_cell_length_a\\n12.1";'), ("t.txt", b"_database_code_ICSD 1"), ("t.cif.txt", b"_cell_length_c   9.000"),
            ("data.csv", b"_chemical_formula_sum 'Al3 Ti1'"), ("n.txt", b"_cell_length_a\t3.5")]
    fine = [("t.cpp", b"// structure from ICSD #25553"), ("r.md", b"the ICSD lists this phase, collection code 15430"),
            ("p.txt", b"Al6Ti2\n1.0\n4 0 0\n"), ("c.py", b"import re\nx = 1"),
            ("w.py", b'f.write(f"_cell_length_a {a:.6f}")'), ("w.js", b"lines.push('_cell_length_a ' + a);"),
            ("r.cpp", b'if (key == "_atom_site_fract_x") {ix = i;}'), ("p.py", b'"_cell_length_a %.6f\\n" % a')]
    bad = 0
    for name, data in must:
        s = Scan(set(), set())
        s.file(name, data)
        if not s.findings:
            print("MISSED", name)
            bad += 1
    for name, data in fine:
        s = Scan(set(), set())
        s.file(name, data)
        if s.findings:
            print("FALSE HIT", name, s.findings)
            bad += 1
    s = Scan({sha("_cell_length_a 1.0 approved")}, {"_cell_length_a 2.0 upstream"})
    s.line("t:1", "  _cell_length_a   1.0 approved")
    s.line("t:2", "_cell_length_a 2.0 upstream")
    if s.findings:
        print("FALSE HIT", s.findings)
        bad += 1
    print("selftest: %d cases, %d failed" % (len(must) + len(fine) + 1, bad))
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["tree", "history", "range", "selftest"])
    ap.add_argument("path", nargs="?")
    ap.add_argument("base", nargs="?")
    ap.add_argument("head", nargs="?")
    ap.add_argument("--baseline")
    ap.add_argument("--git", help="clone that holds --baseline; default PATH")
    ap.add_argument("--approved")
    ap.add_argument("--no-fingerprints", action="store_true")
    ap.add_argument("--internal", action="store_true", help="a private repository: the records may be there, CIFs may not")
    a = ap.parse_args()
    if a.mode == "selftest":
        return selftest()
    if not a.path:
        ap.error("PATH is required")
    ok = Path(a.approved) if a.approved else Path(__file__).resolve().parent / "check_public_icsd_approved.txt"
    if not ok.is_file():
        print("cannot find %s: refusing to report clean" % ok, file=sys.stderr)
        return 2
    rows = [l.split() for l in ok.read_text().splitlines() if l and not l.startswith("#")]
    approved = {r[1] for r in rows if r[0] == "line"}
    paths = {r[1] for r in rows if r[0] == "path"}
    fp = set()
    if not a.no_fingerprints:
        f = next((d / ".icsd_fingerprints" for d in Path(__file__).resolve().parents if (d / ".icsd_fingerprints").is_file()), None)
        if f is None:
            print("cannot find .icsd_fingerprints: refusing to report clean", file=sys.stderr)
            return 2
        fp = {l.strip() for l in f.read_text().splitlines() if l.strip() and not l.startswith("#")}
    base, upstream = baseline_of(a.git or a.path, a.baseline) if a.baseline else (set(), None)
    s = Scan(approved, base, fp, paths, upstream)
    s.internal = a.internal
    if a.mode == "tree":
        tree(s, a.path)
    elif a.mode == "history":
        history(s, a.path)
    else:
        if not (a.base and a.head):
            ap.error("range needs BASE and HEAD")
        span(s, a.path, a.base, a.head)
    for f in s.findings[:200]:
        print(f)
    if s.findings:
        print("BLOCKED: %d finding(s) in %s. Nothing public until they are gone." % (len(s.findings), a.path))
        return 1
    print("clean: %s (%s)" % (a.path, a.mode))
    return 0


if __name__ == "__main__":
    sys.exit(main())
