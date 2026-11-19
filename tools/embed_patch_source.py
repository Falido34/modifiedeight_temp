#!/usr/bin/env python3
"""Bundle buildable client source tree into a zlib-compressed .incbin blob
so standalone binaries can compile & apply C++ code patches without a git repo.

Usage:
  embed_patch_source.py --root <repo_root> --client <modifiedeight|modifiedeight-newadditions> \
                        --out-bin <path.zbin> --out-asm <path.S>
"""
import argparse
import os
import re
import struct
import sys
import zlib


PCM_ARRAY_RE = re.compile(r"uint8_t\s+([A-Za-z0-9_]+)\s*\[\s*\]\s*=\s*\{([^}]*)\}\s*;")


def compact_pcm_data(pcm_c_path):
    with open(pcm_c_path, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    raw_chunks = bytearray()
    asm_lines = [
        "#if defined(MCPE_IOS) || defined(__APPLE__)",
        ".section __TEXT,__const",
        "#else",
        ".section .rodata",
        "#endif",
        "",
    ]

    found = 0
    for m in PCM_ARRAY_RE.finditer(text):
        sym = m.group(1)
        body = m.group(2)
        vals = bytes(int(tok, 0) & 0xFF for tok in body.split(",") if tok.strip())
        while len(raw_chunks) % 4 != 0:
            raw_chunks.append(0)
        off = len(raw_chunks)
        sz = len(vals)
        raw_chunks.extend(vals)
        asm_lines.append(".balign 4")
        asm_lines.append(f".globl {sym}")
        asm_lines.append(f".globl _{sym}")
        asm_lines.append(f"{sym}:")
        asm_lines.append(f"_{sym}:")
        asm_lines.append(f'.incbin "impl/pcm_data_raw.bin", {off}, {sz}')
        found += 1

    if found == 0:
        return None, None
    asm_lines.append("#if defined(__linux__)")
    asm_lines.append('.section .note.GNU-stack,"",@progbits')
    asm_lines.append("#endif")
    return bytes(raw_chunks), ("\n".join(asm_lines) + "\n").encode("utf-8")


def collect_entries(repo_root, client):
    entries = {}

    def add_file(rel_path, data=None):
        rel = rel_path.replace("\\", "/")
        if data is None:
            full = os.path.join(repo_root, rel)
            if not os.path.isfile(full):
                return
            with open(full, "rb") as f:
                data = f.read()
        entries[rel] = data

    def add_tree(rel_dir, skip_dirs=None, skip_files=None, only_exts=None):
        skip_dirs = set(skip_dirs or ())
        skip_files = set(skip_files or ())
        base = os.path.join(repo_root, rel_dir)
        if not os.path.isdir(base):
            return
        for dp, dns, fns in os.walk(base):
            dns[:] = sorted(d for d in dns if d not in skip_dirs and not d.startswith("."))
            for fn in sorted(fns):
                if fn in skip_files or fn.startswith("."):
                    continue
                if only_exts is not None:
                    low = fn.lower()
                    if fn != "CMakeLists.txt" and not any(low.endswith(e) for e in only_exts):
                        continue
                full = os.path.join(dp, fn)
                if not os.path.isfile(full):
                    continue
                rel = os.path.relpath(full, repo_root).replace("\\", "/")
                add_file(rel)

    add_file("CMakeLists.txt")
    add_tree("stb", skip_dirs={"tests", "docs", "tools", "deprecated"}, only_exts={".h", ".hpp"})
    add_tree("glm/glm", skip_dirs={"test", "doc"}, only_exts={".h", ".hpp", ".inl", ".cpp"})
    add_file("OpenAES/CMakeLists.txt")
    add_tree("OpenAES/inc")
    add_tree("OpenAES/src")
    add_file("RakNet/CMakeLists.txt")
    add_tree("RakNet/CmakeIncludes")
    add_tree("RakNet/Source")
    add_file("RakNet/Lib/CMakeLists.txt")
    add_tree("RakNet/Lib/LibStatic")
    add_file("jsoncpp/jsoncpp/CMakeLists.txt")
    add_file("jsoncpp/jsoncpp/version")
    add_tree("jsoncpp/jsoncpp/include")
    add_file("jsoncpp/jsoncpp/src/CMakeLists.txt")
    add_tree("jsoncpp/jsoncpp/src/lib_json")

    for fn in ("CMakeLists.txt", "utils.cmake", "libutf8proc.pc.cmakein", "utf8proc.c", "utf8proc.h", "utf8proc_data.c"):
        add_file(f"utf8proc/{fn}")
    add_tree("utf8proc/cmake")

    add_file(f"{client}/CMakeLists.txt")
    add_tree(f"{client}/headers")
    add_tree(f"{client}/impl", skip_files={"pcm_data.c"})

    pcm_c = os.path.join(repo_root, client, "impl", "pcm_data.c")
    if os.path.isfile(pcm_c):
        raw_bin, asm_bytes = compact_pcm_data(pcm_c)
        if raw_bin is not None and asm_bytes is not None:
            add_file(f"{client}/impl/pcm_data_raw.bin", raw_bin)
            add_file(f"{client}/impl/pcm_data_embed.S", asm_bytes)
        else:
            add_file(f"{client}/impl/pcm_data.c")

    entries[f"{client}/headers/patch/PatchedBuildStamp.hpp"] = (
        b'#pragma once\n#define MCPE_ACTIVE_CODE_FINGERPRINT ""\n'
    )

    return entries


def write_if_different(path, data_bytes):
    if os.path.isfile(path):
        with open(path, "rb") as f:
            if f.read() == data_bytes:
                return False
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data_bytes)
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    ap.add_argument("--client", required=True)
    ap.add_argument("--out-bin", required=True)
    ap.add_argument("--out-asm", required=True)
    args = ap.parse_args()

    entries = collect_entries(os.path.abspath(args.root), args.client)
    items = sorted(entries.items())

    raw = bytearray()
    for rel, data in items:
        pb = rel.encode("utf-8")
        raw.extend(struct.pack("<H", len(pb)))
        raw.extend(pb)
        raw.extend(struct.pack("<I", len(data)))
        raw.extend(data)

    comp = zlib.compress(bytes(raw), 6)
    blob = b"M8SB" + struct.pack("<II", len(raw), len(items)) + comp

    out_bin_abs = os.path.abspath(args.out_bin).replace("\\", "/")
    write_if_different(args.out_bin, blob)

    asm = (
        "#if defined(MCPE_IOS) || defined(__APPLE__)\n"
        ".section __TEXT,__const\n"
        "#else\n"
        ".section .rodata\n"
        "#endif\n"
        ".balign 8\n"
        ".globl m8_embedded_src_start\n"
        ".globl _m8_embedded_src_start\n"
        "m8_embedded_src_start:\n"
        "_m8_embedded_src_start:\n"
        f'.incbin "{out_bin_abs}"\n'
        ".globl m8_embedded_src_end\n"
        ".globl _m8_embedded_src_end\n"
        "m8_embedded_src_end:\n"
        "_m8_embedded_src_end:\n"
        "#if defined(__linux__)\n"
        '.section .note.GNU-stack,"",@progbits\n'
        "#endif\n"
    ).encode("utf-8")
    write_if_different(args.out_asm, asm)
    print(f"[embed_patch_source] {args.client}: {len(items)} files, raw={len(raw)} bytes, zbin={len(blob)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
