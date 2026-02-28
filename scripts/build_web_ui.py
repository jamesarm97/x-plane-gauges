Import("env")
import gzip
import os

# ── Inject framework library include paths ──────────────────────────
# Arduino ESP32 3.x framework libraries (Network, FS, WebServer, etc.)
# need their include paths added explicitly for ESPAsyncWebServer deps
framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if framework_dir:
    framework_libs = ["Network", "FS", "WebServer", "Update", "DNSServer"]
    for lib in framework_libs:
        inc_path = os.path.join(framework_dir, "libraries", lib, "src")
        if os.path.isdir(inc_path):
            env.Append(CPPPATH=[inc_path])
            print(f"[build_web_ui] Added framework include: {inc_path}")

# ── Generate web_ui.h from index.html ──────────────────────────────
def build_web_ui(source, target, env):
    src_path = os.path.join(env["PROJECT_DIR"], "src", "web", "index.html")
    out_path = os.path.join(env["PROJECT_DIR"], "src", "web_ui.h")

    if not os.path.exists(src_path):
        print(f"[build_web_ui] WARNING: {src_path} not found, creating empty placeholder")
        with open(out_path, "w") as f:
            f.write("#pragma once\n#include <pgmspace.h>\n")
            f.write("const uint8_t WEB_UI_HTML_GZ[] PROGMEM = {0};\n")
            f.write("const size_t WEB_UI_HTML_GZ_LEN = 0;\n")
        return

    with open(src_path, "r") as f:
        html = f.read()

    compressed = gzip.compress(html.encode("utf-8"), compresslevel=9)

    lines = []
    lines.append("#pragma once")
    lines.append("#include <pgmspace.h>")
    lines.append("")
    lines.append("// Auto-generated from src/web/index.html — do not edit")
    lines.append(f"// Uncompressed: {len(html)} bytes, Compressed: {len(compressed)} bytes")
    lines.append("")
    lines.append("const uint8_t WEB_UI_HTML_GZ[] PROGMEM = {")
    for i in range(0, len(compressed), 16):
        chunk = compressed[i:i+16]
        row = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {row},")
    lines.append("};")
    lines.append(f"const size_t WEB_UI_HTML_GZ_LEN = {len(compressed)};")
    lines.append("")

    with open(out_path, "w") as f:
        f.write("\n".join(lines))

    print(f"[build_web_ui] {src_path} -> {out_path} ({len(html)} -> {len(compressed)} bytes)")

env.AddPreAction("buildprog", env.VerboseAction(build_web_ui, "Generating web_ui.h from index.html"))
