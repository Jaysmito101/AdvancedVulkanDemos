import os
import shutil
import re
import hashlib
from pathlib import Path
import zipfile
import urllib.request
import json
import subprocess
import datetime

AVD_FONT_MAX_GLYPHS_PY = 4096

def find_git_root():
    current_dir = Path(__file__).resolve().parent
    while current_dir != current_dir.parent:
        if (current_dir / ".git").exists():
            return current_dir
        current_dir = current_dir.parent
    raise RuntimeError("Git root not found.")

def ensure_msdf_atlas_gen(git_root):
    bins_dir = os.path.join(git_root, 'bins')
    msdf_exe_path = os.path.join(bins_dir, 'msdf-atlas-gen.exe')

    if not os.path.exists(msdf_exe_path):
        print("msdf-atlas-gen.exe not found. Downloading...")
        os.makedirs(bins_dir, exist_ok=True)
        zip_url = "https://github.com/Chlumsky/msdf-atlas-gen/releases/download/v1.3/msdf-atlas-gen-1.3-win64.zip"
        zip_path = os.path.join(bins_dir, 'msdf-atlas-gen.zip')

        # Download the zip file
        urllib.request.urlretrieve(zip_url, zip_path)

        # Extract the zip file
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(bins_dir)

        # Clean up the zip file
        os.remove(zip_path)

        extracted_dir = os.path.join(bins_dir, 'msdf-atlas-gen')
        extracted_exe_path = os.path.join(extracted_dir, 'msdf-atlas-gen.exe')
        if os.path.exists(extracted_exe_path):
            shutil.move(extracted_exe_path, msdf_exe_path)
            shutil.rmtree(extracted_dir)
        else:
            print(f"Error: {extracted_exe_path} not found after extraction.")
            return None
        print(f"msdf-atlas-gen.exe moved to {msdf_exe_path}")
    else:
        print(f"msdf-atlas-gen.exe already exists at {msdf_exe_path}")
    return msdf_exe_path

def clean_directory(directory):
    if os.path.exists(directory):
        shutil.rmtree(directory)
    os.makedirs(directory, exist_ok=True)
    return directory

def ensure_directory(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)
    return directory    

def to_correct_case(any_case_with_special_chars):
    any_case_with_special_chars = re.sub(r'[^a-zA-Z0-9]', '_', any_case_with_special_chars)
    any_case_with_special_chars = any_case_with_special_chars.strip('_')
    any_case_with_special_chars = re.sub(r'^[0-9]+', '', any_case_with_special_chars)
    any_case_with_special_chars = any_case_with_special_chars.strip('_')
    any_case_with_special_chars = any_case_with_special_chars.replace('__', '_')
    any_case_with_special_chars = any_case_with_special_chars.replace('_', '')
    return any_case_with_special_chars

def get_asset_type(file_path):
    if file_path.endswith(('.png', '.jpg', '.jpeg', '.gif')):
        return 'image'
    elif file_path.endswith(('.ttf', '.otf')):
        return 'font'
    elif file_path.endswith(('.mp3', '.wav', '.ogg')):
        return 'audio'
    elif file_path.endswith(('.mp4', '.avi', '.mov')):
        return 'video'
    elif file_path.endswith(('.glsl', ".hlsl", '.slang')):
        return 'shader'
    elif file_path.endswith('.txt'):
        return 'text'
    else:
        return None

def generate_c_code_for_bytes(data, name):
    lines = [f'static const uint8_t {name}[] = {{']
    for i in range(0, len(data), 32):
        line = '    ' + ', '.join(f'0x{byte:02x}' for byte in data[i:i+32])
        if i + 32 < len(data):
            line += ','
        lines.append(line)

    lines.append('};')

    return '\n'.join(lines)

def generate_c_code_for_string(text_data, name):
    escaped_text = text_data.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n"\n"')
    return f'static const char {name}[] = "{escaped_text}";'

def generate_c_code_for_avd_font_atlas(font_metrics_data, name):
    atlas_data = font_metrics_data["atlas"]
    metrics_data = font_metrics_data["metrics"]
    json_glyphs_list = font_metrics_data["glyphs"]

    # Initialize an array for C glyph structures with default values.
    # The C array will always have AVD_FONT_MAX_GLYPHS_PY entries.
    c_glyphs_array = []
    for i in range(AVD_FONT_MAX_GLYPHS_PY):
        c_glyphs_array.append({
            "unicodeIndex": i,  # Represents the unicode value for this slot
            "advanceX": 0.0,
            "planeBounds": {"left": 0.0, "bottom": 0.0, "right": 0.0, "top": 0.0},
            "atlasBounds": {"left": 0.0, "bottom": 0.0, "right": 0.0, "top": 0.0}
        })

    skipped_glyphs_count = 0
    max_unicode_found_in_json = -1

    # Populate the c_glyphs_array with actual glyph data from JSON.
    # Glyphs from JSON are placed at their 'unicode' index.
    for glyph_from_json in json_glyphs_list:
        unicode_val = glyph_from_json["unicode"]
        
        if unicode_val >= 0:
            max_unicode_found_in_json = max(max_unicode_found_in_json, unicode_val)

        if 0 <= unicode_val < AVD_FONT_MAX_GLYPHS_PY:
            target_glyph_struct = c_glyphs_array[unicode_val]
            
            # The unicodeIndex is already set to unicode_val during initialization,
            # but explicitly setting it here reinforces its meaning.
            target_glyph_struct["unicodeIndex"] = unicode_val 
            target_glyph_struct["advanceX"] = float(glyph_from_json["advance"])
            
            plane_bounds_json = glyph_from_json.get("planeBounds")
            if plane_bounds_json:
                target_glyph_struct["planeBounds"] = {
                    "left": float(plane_bounds_json["left"]),
                    "bottom": float(plane_bounds_json["bottom"]),
                    "right": float(plane_bounds_json["right"]),
                    "top": float(plane_bounds_json["top"])
                }
            # If not present, it keeps the default {0,0,0,0}

            atlas_bounds_json = glyph_from_json.get("atlasBounds")
            if atlas_bounds_json:
                target_glyph_struct["atlasBounds"] = {
                    "left": float(atlas_bounds_json["left"]),
                    "bottom": float(atlas_bounds_json["bottom"]),
                    "right": float(atlas_bounds_json["right"]),
                    "top": float(atlas_bounds_json["top"])
                }
            # If not present, it keeps the default {0,0,0,0}
        elif unicode_val >= AVD_FONT_MAX_GLYPHS_PY:
            skipped_glyphs_count += 1
    
    if skipped_glyphs_count > 0:
        print(f"Warning: Font '{name}' - Skipped {skipped_glyphs_count} glyphs with unicode value >= {AVD_FONT_MAX_GLYPHS_PY} (max unicode found in JSON: {max_unicode_found_in_json}).")

    lines = [f'static const AVD_FontAtlas {name} = {{']
    lines.append(f'    .info = {{')
    lines.append(f'        .distanceRange = {float(atlas_data["distanceRange"]):.8f}f,')
    if "distanceRangeMiddle" in atlas_data:
        lines.append(f'        .distanceRangeMiddle = {float(atlas_data["distanceRangeMiddle"]):.8f}f,')
    else:
        # Fallback if not in JSON, though msdf-atlas-gen typically includes it.
        lines.append(f'        .distanceRangeMiddle = {float(atlas_data["distanceRange"]) / 2.0:.8f}f,')
    lines.append(f'        .size = {float(atlas_data["size"]):.8f}f,')
    lines.append(f'        .width = {int(atlas_data["width"])},')
    lines.append(f'        .height = {int(atlas_data["height"])}')
    lines.append(f'    }},')

    lines.append(f'    .metrics = {{')
    lines.append(f'        .emSize = {float(metrics_data["emSize"]):.8f}f,')
    lines.append(f'        .lineHeight = {float(metrics_data["lineHeight"]):.8f}f,')
    lines.append(f'        .ascender = {float(metrics_data["ascender"]):.8f}f,')
    lines.append(f'        .descender = {float(metrics_data["descender"]):.8f}f,')
    lines.append(f'        .underlineY = {float(metrics_data["underlineY"]):.8f}f,')
    lines.append(f'        .underlineThickness = {float(metrics_data["underlineThickness"]):.8f}f')
    lines.append(f'    }},')

    lines.append(f'    .glyphCount = {AVD_FONT_MAX_GLYPHS_PY},') # C array size is fixed
    lines.append(f'    .glyphs = {{')
    for i, glyph_struct_for_c in enumerate(c_glyphs_array): # Iterate AVD_FONT_MAX_GLYPHS_PY times
        # glyph_struct_for_c is c_glyphs_array[i], representing unicode 'i'
        line = "        {"
        line += f'.unicodeIndex = {glyph_struct_for_c["unicodeIndex"]}, ' # This will be 'i'
        line += f'.advanceX = {glyph_struct_for_c["advanceX"]:.8f}f, '
        
        pb = glyph_struct_for_c["planeBounds"]
        line += f'.planeBounds = {{{pb["left"]:.8f}f, {pb["bottom"]:.8f}f, {pb["right"]:.8f}f, {pb["top"]:.8f}f}}, '
        
        ab = glyph_struct_for_c["atlasBounds"]
        line += f'.atlasBounds = {{{ab["left"]:.8f}f, {ab["bottom"]:.8f}f, {ab["right"]:.8f}f, {ab["top"]:.8f}f}}'

        line += "}"
        if i < AVD_FONT_MAX_GLYPHS_PY - 1:
            line += ","
        lines.append(line)
    lines.append(f'    }}')
    lines.append(f'}};')
    return '\n'.join(lines)

def create_font_asset(file_path, output_dir, temp_dir, msdf_exe_path, git_root):
    print(f"Generating font asset for {file_path} in {output_dir}")

    with open(file_path, "rb") as f:
        font_bytes = f.read()
    font_hash = hashlib.sha256(font_bytes).hexdigest()
    base_name_without_ext = os.path.splitext(os.path.basename(file_path))[0]
    font_name = to_correct_case(base_name_without_ext)

    all_files = os.listdir(output_dir + "/include") + os.listdir(output_dir + "/src")
    header_exists = any(f"avd_asset_font_{font_name}_{font_hash}" in file for file in all_files if file.endswith(".h"))
    source_exists = any(f"avd_asset_font_{font_name}_{font_hash}" in file for file in all_files if file.endswith(".c"))
    if header_exists and source_exists:
        print(f"Asset {font_name} already exists with same hash, skipping generation.")
        return

    for file in all_files:
        if file.startswith(f"avd_asset_font_{font_name}_") and file.endswith(".h"):
            os.remove(os.path.join(output_dir, "include", file))
        if file.startswith(f"avd_asset_font_{font_name}_") and file.endswith(".c"):
            os.remove(os.path.join(output_dir, "src", file))

    temp_json_path = os.path.join(temp_dir, f"{font_name}.json")
    temp_png_path = os.path.join(temp_dir, f"{font_name}.png")

    ensure_directory(temp_dir)

    cmd = [
        msdf_exe_path,
        "-font", file_path,
        "-type", "mtsdf",
        "-format", "png",
        "-dimensions", "256", "256",
        "-json", temp_json_path,
        "-pxrange", "4",
        "-imageout", temp_png_path
    ]
    print(f"Running msdf-atlas-gen: {' '.join(cmd)}")
    process = subprocess.run(cmd, capture_output=True, text=True)
    if process.returncode != 0:
        print(f"Error generating font atlas for {file_path}:")
        print(process.stdout)
        print(process.stderr)
        return
    
    print(f"msdf-atlas-gen output for {font_name}:\n{process.stdout}")
    if process.stderr:
        print(f"msdf-atlas-gen stderr for {font_name}:\n{process.stderr}")

    with open(temp_png_path, "rb") as f:
        atlas_image_bytes = f.read()
    
    with open(temp_json_path, "r") as f:
        font_metrics_data = json.load(f)

    header_source = [
        f"#ifndef AVD_ASSET_FONT_{font_name.upper()}_{font_hash.upper()}_H",
        f"#define AVD_ASSET_FONT_{font_name.upper()}_{font_hash.upper()}_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"font\"",
        f"// Asset Path: {file_path}",
        f"// Hash: {font_hash}",
        f"// Name: {font_name}",
        f"\n",
        f"#include <stdint.h>",
        f"#include <stddef.h>",
        f"#include \"font/avd_font.h\"",
        f"\n",
        f"const uint8_t* avdAssetFontAtlas_{font_name}(size_t* size);",
        f"\n",
        f"const AVD_FontAtlas* avdAssetFontMetrics_{font_name}(void);",
        f"\n",
        f"#endif // AVD_ASSET_FONT_{font_name.upper()}_{font_hash.upper()}_H"
    ]

    header_file_path = os.path.join(output_dir, "include", f"avd_asset_font_{font_name}_{font_hash}.h")
    with open(header_file_path, "w") as header_file:
        header_file.write("\n".join(header_source))

    source_source = [
        f"#include \"avd_asset_font_{font_name}_{font_hash}.h\"",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"font\"",
        f"// Asset Path: {file_path}",
        f"// Hash: {font_hash}",
        f"// Name: {font_name}",
        f"\n",
        generate_c_code_for_bytes(atlas_image_bytes, f"__avdAssetFontAtlas__{font_name}_{font_hash}"),
        f"\n",
        f"const uint8_t* avdAssetFontAtlas_{font_name}(size_t* size) {{",
        f"    if (size != NULL) {{",
        f"        *size = sizeof(__avdAssetFontAtlas__{font_name}_{font_hash});",
        f"    }}",
        f"   return &__avdAssetFontAtlas__{font_name}_{font_hash}[0];",
        f"}};",
        f"\n",
        generate_c_code_for_avd_font_atlas(font_metrics_data, f"__avdAssetFontMetrics__{font_name}_{font_hash}"),
        f"\n",
        f"const AVD_FontAtlas* avdAssetFontMetrics_{font_name}(void) {{",
        f"   return &__avdAssetFontMetrics__{font_name}_{font_hash};",
        f"}};",
        f"\n"
    ]

    source_file_path = os.path.join(output_dir, "src", f"avd_asset_font_{font_name}_{font_hash}.c")
    with open(source_file_path, "w") as source_file:
        source_file.write("\n".join(source_source))
        source_file.write("\n")
    print(f"Generated font asset: {header_file_path}")

def create_font_assets_common_header(output_dir):
    all_font_headers = os.listdir(output_dir + "/include")
    all_font_headers = [f for f in all_font_headers if f.endswith(".h") and f.startswith("avd_asset_font_")]
    all_font_names = sorted(list(set([f.split("_")[3] for f in all_font_headers])))

    common_header_source = [
        f"#ifndef AVD_ASSET_FONT_COMMON_H",
        f"#define AVD_ASSET_FONT_COMMON_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"#include <stdint.h>",
        f"#include <string.h>",
        f"#include <stddef.h>",
        f"#include \"font/avd_font.h\"",
        f"\n",
        f"// Asset Type: \"font\"",
        f"\n",
        *[f"#include \"{header}\"" for header in all_font_headers],
        "\n",
        "const uint8_t* avdAssetFontAtlas(const char* name, size_t* size); \n",
        "const AVD_FontAtlas* avdAssetFontMetrics(const char* name); \n",
        f"#endif // AVD_ASSET_FONT_COMMON_H"
    ]

    common_header_file_path = os.path.join(output_dir, "include", "avd_asset_font.h")
    with open(common_header_file_path, "w") as common_header_file:
        common_header_file.write("\n".join(common_header_source))
        common_header_file.write("\n")

    source_source = [
        f"#include \"avd_asset_font.h\"",
        f"\n",
        f"#include <stdio.h>",
        f"#include <string.h>",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"font\"",
        f"\n",
        f"const uint8_t* avdAssetFontAtlas(const char* name, size_t* size) {{",
        f"    if (size != NULL) {{",
        f"        *size = 0;",
        f"    }}",
        f"    if (name == NULL) return NULL;",
        f"    if (strcmp(name, \"\") == 0) return NULL;",
        *[f"    if (strcmp(name, \"{font_name}\") == 0) return avdAssetFontAtlas_{font_name}(size);" for font_name in all_font_names],
        f"    printf(\"Error: Font atlas asset \\\"%s\\\" not found.\\n\", name);",
        f"    return NULL;",
        f"}}",
        f"\n\n",
        f"const AVD_FontAtlas* avdAssetFontMetrics(const char* name) {{",
        f"    if (name == NULL) return NULL;",
        f"    if (strcmp(name, \"\") == 0) return NULL;",
        *[f"    if (strcmp(name, \"{font_name}\") == 0) return avdAssetFontMetrics_{font_name}();" for font_name in all_font_names],
        f"    printf(\"Error: Font metrics asset \\\"%s\\\" not found.\\n\", name);",
        f"    return NULL;",
        f"}}",
        f"\n",
    ]
    source_file_path = os.path.join(output_dir, "src", "avd_asset_font.c")
    with open(source_file_path, "w") as source_file:
        source_file.write("\n".join(source_source))
        source_file.write("\n")

    print(f"Generated common font header file: {common_header_file_path}")

def create_font_assets(git_root, output_dir, temp_dir, msdf_exe_path):
    asset_dir = os.path.join(git_root, "em_assets")
    all_asset_files = []
    for root_dir, _, files in os.walk(asset_dir):
        for file in files:
            file_path = os.path.join(root_dir, file)
            asset_type = get_asset_type(file_path)
            if asset_type == "font":
                all_asset_files.append(file_path)
    print(f"Found {len(all_asset_files)} font assets to generate.")
    
    for file_path in all_asset_files:
        create_font_asset(file_path, output_dir, temp_dir, msdf_exe_path, git_root)

    create_font_assets_common_header(output_dir)

def create_image_asset(file_path, output_dir):
    print(f"Generating image asset for {file_path} in {output_dir}")

    image_bytes = open(file_path, "rb").read()
    image_hash = hashlib.sha256(image_bytes).hexdigest()
    base_name_without_ext = os.path.splitext(os.path.basename(file_path))[0]
    image_name = to_correct_case(base_name_without_ext)

    all_files = os.listdir(output_dir + "/include") + os.listdir(output_dir + "/src")

    header_exists = any(f"avd_asset_image_{image_name}_{image_hash}" in file for file in all_files if file.endswith(".h"))
    source_exists = any(f"avd_asset_image_{image_name}_{image_hash}" in file for file in all_files if file.endswith(".c"))
    if header_exists and source_exists:
        print(f"Asset {image_name} already exists with same hash, skipping generation.")
        return
    
    for file in all_files:
        if file.startswith(f"avd_asset_image_{image_name}_") and file.endswith(".h"):
            os.remove(os.path.join(output_dir, "include", file))
        if file.startswith(f"avd_asset_image_{image_name}_") and file.endswith(".c"):
            os.remove(os.path.join(output_dir, "src", file))

    header_source = [
        f"#ifndef AVD_ASSET_IMAGE_{image_name}_{image_hash.upper()}_H",
        f"#define AVD_ASSET_IMAGE_{image_name}_{image_hash.upper()}_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"image\"",
        f"// Asset Path: {file_path}",
        f"// Hash: {image_hash}",
        f"// Name: {image_name}",
        f"\n", 
        f"#include <stdint.h>",
        f"#include <stddef.h>",
        f"\n",
        f"const uint8_t* avdAssetImage_{image_name}(size_t* size);",
        f"\n",
        f"#endif // AVD_ASSET_IMAGE_{image_name}_{image_hash.upper()}_H"
    ]

    header_file_path = os.path.join(output_dir, "include", f"avd_asset_image_{image_name}_{image_hash}.h")
    with open(header_file_path, "w") as header_file:
        header_file.write("\n".join(header_source))

    source_source = [
        f"#include \"avd_asset_image_{image_name}_{image_hash}.h\"",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"image\"",
        f"// Asset Path: {file_path}",
        f"// Hash: {image_hash}",
        f"// Name: {image_name}",
        f"\n", 
        generate_c_code_for_bytes(image_bytes, f"__avdAssetImage__{image_name}_{image_hash}"),
        f"\n",
        f"const uint8_t* avdAssetImage_{image_name}(size_t* size) {{",
        f"    if (size != NULL) {{",
        f"        *size = sizeof(__avdAssetImage__{image_name}_{image_hash});",
        f"    }}",
        f"   return &__avdAssetImage__{image_name}_{image_hash}[0];",
        f"}};",
        f"\n"
    ]

    source_file_path = os.path.join(output_dir, "src", f"avd_asset_image_{image_name}_{image_hash}.c")
    with open(source_file_path, "w") as source_file:
        source_file.write("\n".join(source_source))
        source_file.write("\n")
    print(f"Generated image asset: {header_file_path}")

def create_image_assets_common_header(output_dir):
    all_image_headers = os.listdir(output_dir + "/include")
    all_image_headers = [f for f in all_image_headers if f.endswith(".h") and f.startswith("avd_asset_image_")]
    all_image_names = [f.split("_")[3] for f in all_image_headers]

    common_header_source = [
        f"#ifndef AVD_ASSET_IMAGE_COMMON_H",
        f"#define AVD_ASSET_IMAGE_COMMON_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"#include <stdint.h>",
        f"#include <string.h>",
        f"#include <stddef.h>",
        f"\n"
        f"// Asset Type: \"image\"",
        f"\n",
        *[f"#include \"{header}\"" for header in all_image_headers],
        "\n",
        "const uint8_t* avdAssetImage(const char* name, size_t* size); \n",
        f"#endif // AVD_ASSET_IMAGE_COMMON_H"
    ]

    common_header_file_path = os.path.join(output_dir, "include", "avd_asset_image.h")
    with open(common_header_file_path, "w") as common_header_file:
        common_header_file.write("\n".join(common_header_source))
        common_header_file.write("\n")

    source_source = [
        f"#include \"avd_asset_image.h\"",
        f"\n",
        f"#include <stdio.h>",
        f"#include <string.h>",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"image\"",
        f"\n",
        f"const uint8_t* avdAssetImage(const char* name, size_t* size) {{",
        f"    if (size != NULL) {{",
        f"        *size = 0;",
        f"    }}",
        f"    if (name == NULL) return NULL;",
        f"    if (strcmp(name, \"\") == 0) return NULL;",
        *[f"    if (strcmp(name, \"{image_name}\") == 0) return avdAssetImage_{image_name}(size);" for image_name in all_image_names],
        f"    printf(\"Error: Image asset \\\"%s\\\" not found.\\n\", name);",
        f"    return NULL;",
        f"}}",
        f"\n",
    ]
    source_file_path = os.path.join(output_dir, "src", "avd_asset_image.c")
    with open(source_file_path, "w") as source_file:
        source_file.write("\n".join(source_source))
        source_file.write("\n")

    print(f"Generated common header file: {common_header_file_path}")

def create_image_assets(git_root, output_dir):
    asset_dir = os.path.join(git_root, "em_assets")
    all_asset_files = []
    for root, _, files in os.walk(asset_dir):
        for file in files:
            file_path = os.path.join(root, file)
            asset_type = get_asset_type(file_path)
            if asset_type == "image":
                all_asset_files.append(file_path)
    print(f"Found {len(all_asset_files)} image assets to generate.")
    
    for file_path in all_asset_files:
        create_image_asset(file_path, output_dir)

    create_image_assets_common_header(output_dir)

def shader_stage_to_avd(shader_stage):
    if shader_stage == 'fragment':
        return 'AVD_SHADER_STAGE_FRAGMENT'
    elif shader_stage == 'vertex':
        return 'AVD_SHADER_STAGE_VERTEX'
    elif shader_stage == 'compute':
        return 'AVD_SHADER_STAGE_COMPUTE'
    else:
        return 'AVD_SHADER_STAGE_HEADER'

def shader_language_to_avd(shader_language):
    if shader_language == 'glsl':
        return 'AVD_SHADER_LANGUAGE_GLSL'
    elif shader_language == 'hlsl':
        return 'AVD_SHADER_LANGUAGE_HLSL'
    elif shader_language == 'slang':
        return 'AVD_SHADER_LANGUAGE_SLANG'
    else:
        return 'AVD_SHADER_LANGUAGE_UNKNOWN'

def create_shader_asset(file_path, output_dir):
    print(f"Generating shader asset for {file_path} in {output_dir}")

    with open(file_path, "r") as f:
        shader_text = f.read()
    
    shader_hash = hashlib.sha256(shader_text.encode('utf-8')).hexdigest()

    extension = os.path.splitext(file_path)[1].lower()
    extension = re.sub(r'[^a-zA-Z0-9]', '', extension)

    base_name_without_ext = os.path.basename(file_path)
    if "Frag" in file_path:
        shader_stage = 'fragment'
    elif "Vert" in file_path:
        shader_stage = 'vertex'
    elif "Comp" in file_path:
        shader_stage = 'compute'
    else:
        shader_stage = 'header'

    extension_slugs_to_remove = ['.glsl', '.slang', '.hlsl']
    for slug in extension_slugs_to_remove:
        base_name_without_ext = base_name_without_ext.replace(slug, '')
    shader_name = to_correct_case(base_name_without_ext)

    all_files = os.listdir(output_dir + "/include") + os.listdir(output_dir + "/src")
    shader_c_file_name = f"avd_asset_shader_{shader_name}_{extension}_{shader_stage}_{shader_hash}"
    header_exists = any(shader_c_file_name in file for file in all_files if file.endswith(".h"))
    source_exists = any(shader_c_file_name in file for file in all_files if file.endswith(".c"))

    if header_exists and source_exists:
        print(f"Asset {shader_name} already exists with same hash, skipping generation.")
        return
    
    for file in all_files:
        if file.startswith(f"avd_asset_shader_{shader_name}_") and file.endswith(".h"):
            os.remove(os.path.join(output_dir, "include", file))
        if file.startswith(f"avd_asset_shader_{shader_name}_") and file.endswith(".c"):
            os.remove(os.path.join(output_dir, "src", file))

    header_source = [
        f"#ifndef AVD_ASSET_SHADER_{shader_name}_{shader_hash.upper()}_H",
        f"#define AVD_ASSET_SHADER_{shader_name}_{shader_hash.upper()}_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"#include \"shader/avd_shader_base.h\""
        f"\n",
        f"// Asset Type  : \"shader\"",
        f"// Asset Path  : {file_path}",
        f"// Hash        : {shader_hash}",
        f"// Name        : {shader_name}",
        f"// Stage       : {shader_stage}",
        f"// Language    : {extension}",
        f"\n", 
        f"// Return shader as a null-terminated string",
        f"const char* avdAssetShader_{shader_name}(void);",
        f"AVD_ShaderStage avdAssetShaderStage_{shader_name}(void);",
        f"AVD_ShaderLanguage avdAssetShaderLanguage_{shader_name}(void);",
        f"AVD_ShaderStage avdAssetShaderStage_{shader_name}(void);",
        f"\n",
        f"#endif // AVD_ASSET_SHADER_{shader_name}_{shader_hash.upper()}_H"
    ]

    header_file_path = os.path.join(output_dir, "include", f"{shader_c_file_name}.h")
    with open(header_file_path, "w") as header_file:
        header_file.write("\n".join(header_source))

    source_source = [
        f"#include \"{shader_c_file_name}.h\"",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type  : \"shader\"",
        f"// Asset Path  : {file_path}",
        f"// Hash        : {shader_hash}",
        f"// Name        : {shader_name}",
        f"// Stage       : {shader_stage}",
        f"// Language    : {extension}",
        f"\n", 
        generate_c_code_for_string(shader_text, f"__avdAssetShader__{shader_name}_{shader_hash}"),
        f"\n",
        f"const char* avdAssetShader_{shader_name}(void) {{",
        f"   return __avdAssetShader__{shader_name}_{shader_hash};",
        f"}};",
        f"\n",
        f"AVD_ShaderStage avdAssetShaderStage_{shader_name}(void) {{",
        f"   return {shader_stage_to_avd(shader_stage)};",
        f"}};",
        f"\n",
        f"AVD_ShaderLanguage avdAssetShaderLanguage_{shader_name}(void) {{",
        f"   return {shader_language_to_avd(extension)};",
        f"}};",
        f"\n"
    ]

    source_file_path = os.path.join(output_dir, "src", f"{shader_c_file_name}.c")
    with open(source_file_path, "w") as source_file:
        source_file.write("\n".join(source_source))
        source_file.write("\n")
    print(f"Generated shader asset: {header_file_path}")

def read_shader_source(output_dir, shader_name):
    all_file_names = os.listdir(output_dir + "/src")
    shader_file_name = f"avd_asset_shader_{shader_name}_"
    shader_file_name = next((f for f in all_file_names if f.startswith(shader_file_name) and f.endswith(".c")), None)
    if shader_file_name is None:
        print(f"Error: Shader source file for {shader_name} not found in {output_dir}/src")
        return ""
    shader_file_path = os.path.join(output_dir, "src", shader_file_name)
    with open(shader_file_path, "r") as shader_file:
        shader_source = shader_file.read()
    return shader_source

def calculate_shader_dependency_list(output_dir, shader, shader_names):
    current_source = read_shader_source(output_dir, shader)
    included_shader_names = []
    for shader_name in shader_names:
        if f"#include \\\"{shader_name}\\\"" in current_source:
            included_shader_names.append(shader_name)
    for included_shader_name in included_shader_names:
        included_shader_names.extend(calculate_shader_dependency_list(output_dir, included_shader_name, shader_names))
    return list(set(included_shader_names)) 

def calculate_shader_dependency_hash(output_dir, shader_name, shader_names):
    dependencies = calculate_shader_dependency_list(output_dir, shader_name, shader_names)
    dependencies.append(shader_name)  
    dependencies.sort()
    dependencies = [read_shader_source(output_dir, dep) for dep in dependencies]
    # return a 32 integer bit hash of the concatenated shader sources, RETURN A INTEGER HASH
    concatenated_sources = ''.join(dependencies) 
    return int(hashlib.sha256(concatenated_sources.encode('utf-8')).hexdigest()[:8], 16)


def create_shader_assets_common_header(output_dir):
    all_shader_headers = os.listdir(output_dir + "/include")
    all_shader_headers = [f for f in all_shader_headers if f.endswith(".h") and f.startswith("avd_asset_shader_")]
    all_shader_headers.sort()
    all_shader_names = [f.split("_")[3] for f in all_shader_headers]

    common_header_source = [
        f"#ifndef AVD_ASSET_SHADER_COMMON_H",
        f"#define AVD_ASSET_SHADER_COMMON_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"#include <string.h>",
        f"\n"
        f"// Asset Type: \"shader\"",
        f"\n",
        *[f"#include \"{header}\"" for header in all_shader_headers],
        "\n",
        "// Returns shader source as a null-terminated string",
        "const char* avdAssetShader(const char* name); \n",
        "// Returns shader stage as an AVD_ShaderStage enum",
        "AVD_ShaderStage avdAssetShaderStage(const char* name); \n",
        "// Returns shader language as an AVD_ShaderLanguage enum",
        "AVD_ShaderLanguage avdAssetShaderLanguage(const char* name); \n",
        "// Returns a list of shader names that this shader depends on",
        "const char** avdAssetShaderGetDependencies(const char* name, size_t* count); \n",
        "// Returns a hash of the shader dependencies",
        "unsigned int avdAssetShaderGetDependencyHash(const char* name); \n",
        f"\n",
        f"#endif // AVD_ASSET_SHADER_COMMON_H"
    ]

    common_header_file_path = os.path.join(output_dir, "include", "avd_asset_shader.h")
    with open(common_header_file_path, "w") as common_header_file:
        common_header_file.write("\n".join(common_header_source))
        common_header_file.write("\n")
        
    def shader_dependency_lambda(shader_name):
        dependencies = calculate_shader_dependency_list(output_dir, shader_name, all_shader_names)
        dependencies = sorted(list(set(dependencies)))
        if not dependencies:
            return f"    if (strcmp(name, \"{shader_name}\") == 0) {{\n" \
                   f"        *count = 0;\n" \
                   f"        return NULL;\n" \
                   f"    }}"
        dep_str = ', '.join([f'"{dep}"' for dep in dependencies])
        return f"    if (strcmp(name, \"{shader_name}\") == 0) {{\n" \
               f"        *count = {len(dependencies)};\n" \
               f"        static const char* deps[] = {{{dep_str}}};\n" \
               f"        return deps;\n" \
               f"    }}"
        
    source_source = [
        f"#include \"avd_asset_shader.h\"",
        f"\n",
        f"#include <stdio.h>",
        f"#include <string.h>",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"// Asset Type: \"shader\"",
        f"\n",
        f"const char* avdAssetShader(const char* name) {{",
        f"    if (name == NULL) return NULL;",
        f"    if (strcmp(name, \"\") == 0) return NULL;",
        *[f"    if (strcmp(name, \"{shader_name}\") == 0) return avdAssetShader_{shader_name}();" for shader_name in all_shader_names],
        f"    printf(\"Error: Shader asset \\\"%s\\\" not found.\\n\", name);",
        f"    return NULL;",
        f"}}",
        f"\n"
        f"AVD_ShaderStage avdAssetShaderStage(const char* name) {{",
        f"    if (name == NULL) return AVD_SHADER_STAGE_HEADER;",
        f"    if (strcmp(name, \"\") == 0) return AVD_SHADER_STAGE_HEADER;",
        *[f"    if (strcmp(name, \"{shader_name}\") == 0) return avdAssetShaderStage_{shader_name}();" for shader_name in all_shader_names],
        f"    printf(\"Error: Shader stage asset \\\"%s\\\" not found.\\n\", name);",
        f"    return AVD_SHADER_STAGE_HEADER;", 
        f"}}",
        f"\n"
        f"AVD_ShaderLanguage avdAssetShaderLanguage(const char* name) {{",
        f"    if (name == NULL) return AVD_SHADER_LANGUAGE_UNKNOWN;",
        f"    if (strcmp(name, \"\") == 0) return AVD_SHADER_LANGUAGE_UNKNOWN;",
        *[f"    if (strcmp(name, \"{shader_name}\") == 0) return avdAssetShaderLanguage_{shader_name}();" for shader_name in all_shader_names],
        f"    printf(\"Error: Shader language asset \\\"%s\\\" not found.\\n\", name);",
        f"    return AVD_SHADER_LANGUAGE_UNKNOWN;",
        f"}}",
        f"\n",
        f"// This function returns a list of shader names that this shader depends on.",
        f"// It is used to ensure that all dependencies are included in the build.",
        f"const char** avdAssetShaderGetDependencies(const char* name, size_t* count) {{",
        f"    if (name == NULL || count == NULL) {{",
        f"        return NULL;",
        f"    }}",
        f"    *count = 0;",
        f"    if (strcmp(name, \"\") == 0) {{",
        f"        return NULL;",
        f"    }}",
        *[shader_dependency_lambda(shader_name) for shader_name in all_shader_names],
        f"    printf(\"Error: Shader asset \\\"%s\\\" not found.\\n\", name);",
        f"    return NULL;",
        f"}}",
        f"\n"
        f"// This function returns a hash of the shader dependencies.",
        f"// It is used to ensure that all dependencies are included in the build.",
        f"unsigned int avdAssetShaderGetDependencyHash(const char* name) {{",
        f"    if (name == NULL) {{",
        f"        return 0;",
        f"    }}",
        f"    if (strcmp(name, \"\") == 0) {{",
        f"        return 0;",
        f"    }}",
        *[f"    if (strcmp(name, \"{shader_name}\") == 0) return {calculate_shader_dependency_hash(output_dir, shader_name, all_shader_names)};" for shader_name in all_shader_names],
        f"    printf(\"Error: Shader asset \\\"%s\\\" not found.\\n\", name);",
        f"    return 0;",
        f"}}",
        f"\n",
    ]
    source_file_path = os.path.join(output_dir, "src", "avd_asset_shader.c")
    with open(source_file_path, "w") as source_file:
        source_file.write("\n".join(source_source))
        source_file.write("\n")

    print(f"Generated common shader header file: {common_header_file_path}")

def create_shader_assets(git_root, output_dir):
    asset_dir = os.path.join(git_root, "em_assets")
    all_asset_files = []
    for root, _, files in os.walk(asset_dir):
        for file in files:
            file_path = os.path.join(root, file)
            asset_type = get_asset_type(file_path)
            if asset_type == "shader":
                all_asset_files.append(file_path)
    print(f"Found {len(all_asset_files)} shader assets to generate.")
    
    for file_path in all_asset_files:
        create_shader_asset(file_path, output_dir)

    create_shader_assets_common_header(output_dir)

def create_assets_common_header(output_dir):
    common_header_source = [
        f"#ifndef AVD_ASSET_COMMON_H",
        f"#define AVD_ASSET_COMMON_H",
        f"\n",
        f"// This file is auto-generated by the asset generator script.",
        f"// Do not edit this file directly.",
        f"\n",
        f"#include \"avd_asset_image.h\"",
        f"#include \"avd_asset_shader.h\"",
        f"#include \"avd_asset_text.h\"",
        f"#include \"avd_asset_font.h\"",
        f"\n",
        f"#endif // AVD_ASSET_COMMON_H"
    ]

    common_header_file_path = os.path.join(output_dir, "include", "avd_asset.h")
    with open(common_header_file_path, "w") as common_header_file:
        common_header_file.write("\n".join(common_header_source))
        common_header_file.write("\n")
    print(f"Generated common header file: {common_header_file_path}")

def create_text_asset(file_path, output_dir):
    print(f"Generating text asset for {file_path} in {output_dir}")
    with open(file_path, "r", encoding="utf-8") as f:
        text_data = f.read()
    text_hash = hashlib.sha256(text_data.encode('utf-8')).hexdigest()
    name = to_correct_case(os.path.splitext(os.path.basename(file_path))[0])

    all_files = os.listdir(output_dir+"/include") + os.listdir(output_dir+"/src")
    if any(f"avd_asset_text_{name}_{text_hash}" in fn for fn in all_files):
        return
    # remove old
    for fn in all_files:
        if fn.startswith(f"avd_asset_text_{name}_"):
            os.remove(os.path.join(output_dir, "include" if fn.endswith(".h") else "src", fn))

    # header
    hdr = [
        f"#ifndef AVD_ASSET_TEXT_{name.upper()}_{text_hash.upper()}_H",
        f"#define AVD_ASSET_TEXT_{name.upper()}_{text_hash.upper()}_H",
        "",
        "// Auto-generated. Do not edit.",
        "",
        "// Asset Type: \"text\"",
        f"// Asset Path: {file_path}",
        f"// Hash: {text_hash}",
        f"// Name: {name}",
        "",
        f"const char* avdAssetText_{name}(void);",
        "",
        f"#endif // AVD_ASSET_TEXT_{name.upper()}_{text_hash.upper()}_H"
    ]
    with open(os.path.join(output_dir, "include", f"avd_asset_text_{name}_{text_hash}.h"), "w") as f:
        f.write("\n".join(hdr)+"\n")

    # source
    src = [
        f"#include \"avd_asset_text_{name}_{text_hash}.h\"",
        "",
        "// Auto-generated. Do not edit.",
        "",
        generate_c_code_for_string(text_data, f"__avdAssetText__{name}_{text_hash}"),
        "",
        f"const char* avdAssetText_{name}(void) {{",
        f"    return __avdAssetText__{name}_{text_hash};",
        f"}}",
        ""
    ]
    with open(os.path.join(output_dir, "src", f"avd_asset_text_{name}_{text_hash}.c"), "w") as f:
        f.write("\n".join(src)+"\n")

def create_text_assets_common_header(output_dir):
    headers = [h for h in os.listdir(output_dir+"/include") if h.startswith("avd_asset_text_") and h.endswith(".h")]
    common = [
        "#ifndef AVD_ASSET_TEXT_COMMON_H",
        "#define AVD_ASSET_TEXT_COMMON_H",
        "",
        "// Auto-generated. Do not edit.",
        "",
        "#include <string.h>",
        "",
        "// Asset Type: \"text\"",
        "",
        *[f"#include \"{h}\"" for h in headers],
        "",
        "const char* avdAssetText(const char* name);",
        "",
        "#endif // AVD_ASSET_TEXT_COMMON_H"
    ]
    with open(os.path.join(output_dir, "include", "avd_asset_text.h"), "w") as f:
        f.write("\n".join(common)+"\n")

    src = [
        "#include \"avd_asset_text.h\"",
        "",
        "#include <stdio.h>",
        "#include <string.h>",
        "",
        "// Auto-generated. Do not edit.",
        "",
        "const char* avdAssetText(const char* name) {",
        "    if (!name || !*name) return NULL;",
        *[f"    if (strcmp(name, \"{to_correct_case(os.path.splitext(h)[0].split('_')[3])}\")==0) return avdAssetText_{to_correct_case(os.path.splitext(h)[0].split('_')[3])}();" for h in headers],
        "    printf(\"Error: Text asset \\\"%s\\\" not found.\\n\", name);",
        "    return NULL;",
        "}",
        ""
    ]
    with open(os.path.join(output_dir, "src", "avd_asset_text.c"), "w") as f:
        f.write("\n".join(src)+"\n")

def create_text_assets(git_root, output_dir):
    asset_dir = os.path.join(git_root, "em_assets")
    for root, _, files in os.walk(asset_dir):
        for fn in files:
            if fn.endswith(".txt"):
                create_text_asset(os.path.join(root, fn), output_dir)
    create_text_assets_common_header(output_dir)

def create_assets(git_root, output_dir, temp_dir, msdf_exe_path):
    asset_dir = os.path.join(git_root, "em_assets")
    all_asset_files = []
    for root_dir, _, files in os.walk(asset_dir):
        for file in files:
            file_path = os.path.join(root_dir, file)
            asset_type = get_asset_type(file_path)
            if asset_type == "font":
                all_asset_files.append(file_path)
    print(f"Found {len(all_asset_files)} font assets to generate.")
    
    for file_path in all_asset_files:
        create_font_asset(file_path, output_dir, temp_dir, msdf_exe_path, git_root)

    create_font_assets_common_header(output_dir)

    all_asset_files = []
    for root_dir, _, files in os.walk(asset_dir):
        for file in files:
            file_path = os.path.join(root_dir, file)
            asset_type = get_asset_type(file_path)
            if asset_type == "image":
                all_asset_files.append(file_path)
    print(f"Found {len(all_asset_files)} image assets to generate.")
    
    for file_path in all_asset_files:
        create_image_asset(file_path, output_dir)

    create_image_assets_common_header(output_dir)

    all_asset_files = []
    for root_dir, _, files in os.walk(asset_dir):
        for file in files:
            file_path = os.path.join(root_dir, file)
            asset_type = get_asset_type(file_path)
            if asset_type == "shader":
                all_asset_files.append(file_path)
    print(f"Found {len(all_asset_files)} shader assets to generate.")
    
    for file_path in all_asset_files:
        create_shader_asset(file_path, output_dir)

    create_shader_assets_common_header(output_dir)

    create_text_assets(git_root, output_dir)

    create_assets_common_header(output_dir)

def calculate_directory_hashes(directory):
    hashes = {}
    for root, _, files in os.walk(directory):
        for file in files:
            if file == "assets_manifest.json" or file == "assets_history.txt" or file == ".assetshash":
                continue
            file_path = os.path.join(root, file)
            rel_path = os.path.relpath(file_path, directory)
            rel_path = rel_path.replace("\\", "/")
            
            with open(file_path, "rb") as f:
                file_hash = hashlib.sha256(f.read()).hexdigest()
            hashes[rel_path] = file_hash
    return hashes

def update_asset_history(output_dir):
    manifest_path = os.path.join(output_dir, "assets_manifest.json")
    history_path = os.path.join(output_dir, "assets_history.txt")
    hash_file_path = os.path.join(output_dir, ".assetshash")
    
    current_hashes = calculate_directory_hashes(output_dir)
    
    combined_hash_input = ""
    for file_path in sorted(current_hashes.keys()):
        combined_hash_input += f"{file_path}:{current_hashes[file_path]};"
    combined_hash = hashlib.sha256(combined_hash_input.encode('utf-8')).hexdigest()
    
    with open(hash_file_path, "w") as f:
        f.write(combined_hash)
    
    previous_hashes = {}
    if os.path.exists(manifest_path):
        try:
            with open(manifest_path, "r") as f:
                previous_hashes = json.load(f)
        except:
            print("Failed to load existing manifest, assuming clean state.")
            
    changed_files = []
    
    for file_path, file_hash in current_hashes.items():
        if file_path not in previous_hashes:
            changed_files.append(f"NEW: {file_path}")
        elif previous_hashes[file_path] != file_hash:
            changed_files.append(f"MODIFIED: {file_path}")
            
    for file_path in previous_hashes:
        if file_path not in current_hashes:
            changed_files.append(f"DELETED: {file_path}")
            
    if changed_files:
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open(history_path, "a") as f:
            f.write(f"--- Build {timestamp} ---\n")
            f.write(f"Combined Hash: {combined_hash}\n")
            for change in changed_files:
                f.write(f"{change}\n")
            f.write("\n")
        print(f"Updated assets history with {len(changed_files)} changes.")
        
        # Save new manifest
        with open(manifest_path, "w") as f:
            json.dump(current_hashes, f, indent=4, sort_keys=True)
    else:
        print(f"No asset changes detected. Combined Hash: {combined_hash}")

def main():
    git_root = find_git_root()
    msdf_exe_path = ensure_msdf_atlas_gen(git_root)
    temp_dir = os.path.join(git_root, "temp")
    bins_dir = os.path.join(git_root, "bins")
    clean_directory(temp_dir)
    ensure_directory(bins_dir)
    
    output_dir = os.path.join(git_root, "avd_assets", "generated")
    ensure_directory(output_dir)
    ensure_directory(output_dir + "/include")
    ensure_directory(output_dir + "/src")   

    create_image_assets(git_root, output_dir)
    create_shader_assets(git_root, output_dir)
    create_text_assets(git_root, output_dir)
    create_font_assets(git_root, output_dir, temp_dir, msdf_exe_path)
    create_assets_common_header(output_dir)
    
    update_asset_history(output_dir)

if __name__ == "__main__":
    main()




