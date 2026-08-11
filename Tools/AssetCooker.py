#!/usr/bin/env python3
"""Cooks the character, item and enemy payload into a playable build.

Textures are re-encoded from PNG/JPG into QOI (https://qoiformat.org) as a
compact storage/caching format. Cooked builds ship .qoi files instead of the
original images; the runtime decodes them straight to RGBA and uploads to the
GPU, so the PNGs never need to be copied into the build.

glTF files that reference textures by name have their image URIs rewritten to
the cooked .qoi files (enemy models embed their images, so they are untouched).
"""

from pathlib import Path
import re
import shutil
import struct
import sys

try:
    from PIL import Image as PILImage
except ImportError:
    PILImage = None


# ---------------------------------------------------------------------------
# QOI encoder (spec: https://qoiformat.org/qoi-specification.html)
# A small pure-Python implementation, kept intentionally simple and dependency
# free: the cooked .qoi files only need to be produced once per source change.
# ---------------------------------------------------------------------------

QOI_OP_INDEX = 0x00
QOI_OP_DIFF = 0x40
QOI_OP_LUMA = 0x80
QOI_OP_RUN = 0xC0
QOI_OP_RGB = 0xFE
QOI_OP_RGBA = 0xFF


def qoi_encode(data: bytes, width: int, height: int, channels: int = 4) -> bytes:
    out = bytearray()
    out += b"qoif"
    out += struct.pack(">II", width, height)
    out.append(channels)
    out.append(0)  # colorspace: sRGB with linear alpha

    index = [-1] * 64
    append = out.append

    run = 0
    pr = pg = pb = 0
    pa = 255
    stride = channels
    size = width * height * stride
    for off in range(0, size, stride):
        r = data[off]
        g = data[off + 1]
        b = data[off + 2]
        a = data[off + 3] if channels == 4 else 255

        if r == pr and g == pg and b == pb and a == pa:
            run += 1
            if run == 62:
                append(QOI_OP_RUN | (run - 1))
                run = 0
            continue
        if run:
            append(QOI_OP_RUN | (run - 1))
            run = 0

        px = (r << 24) | (g << 16) | (b << 8) | a
        idx = ((r * 3 + g * 5 + b * 7 + a * 11) & 63)
        if index[idx] == px:
            append(QOI_OP_INDEX | idx)
        else:
            index[idx] = px
            if a == pa:
                dr = r - pr
                dg = g - pg
                db = b - pb
                if -2 <= dr <= 1 and -2 <= dg <= 1 and -2 <= db <= 1:
                    append(QOI_OP_DIFF | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2))
                elif -32 <= dg <= 31 and -8 <= dr - dg <= 7 and -8 <= db - dg <= 7:
                    append(QOI_OP_LUMA | (dg + 32))
                    append(((dr - dg + 8) << 4) | (db - dg + 8))
                elif a == 255:
                    append(QOI_OP_RGB)
                    append(r)
                    append(g)
                    append(b)
                else:
                    append(QOI_OP_RGBA)
                    append(r)
                    append(g)
                    append(b)
                    append(a)
            else:
                append(QOI_OP_RGBA)
                append(r)
                append(g)
                append(b)
                append(a)
        pr, pg, pb, pa = r, g, b, a

    if run:
        append(QOI_OP_RUN | (run - 1))
    out += b"\x00\x00\x00\x00\x00\x00\x00\x01"
    return bytes(out)


TEXTURE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".bmp", ".tga")


def copy_if_changed(source: Path, destination: Path) -> bool:
    if not source.is_file():
        raise FileNotFoundError(f"Required asset was not found: {source}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.is_file():
        source_stat = source.stat()
        destination_stat = destination.stat()
        if (source_stat.st_size == destination_stat.st_size and
                source_stat.st_mtime_ns <= destination_stat.st_mtime_ns):
            return False

    shutil.copy2(source, destination)
    return True


def convert_texture_if_changed(source: Path, destination: Path) -> bool:
    """Re-encodes a texture into a .qoi file and returns True if (re)written.

    `destination` is the path the PNG would have had; the cooked file is the
    same path with a .qoi extension, so the PNG is never copied into the build.
    """
    qoi_destination = destination.with_suffix(".qoi")
    qoi_destination.parent.mkdir(parents=True, exist_ok=True)

    if qoi_destination.is_file() and source.stat().st_mtime_ns <= qoi_destination.stat().st_mtime_ns:
        return False

    if PILImage is None:
        # No decoder available: keep the original image so the build still runs.
        return copy_if_changed(source, destination)

    with PILImage.open(source) as image:
        rgba = image.convert("RGBA")
        width, height = rgba.size
        pixels = rgba.tobytes()

    qoi_destination.write_bytes(qoi_encode(pixels, width, height, 4))
    return True


_URI_PATTERN = re.compile(r'("uri"\s*:\s*")([^"]+)(")')


def cook_gltf_if_changed(source: Path, destination: Path,
                         converted_names: set, force: bool = False) -> bool:
    """Copies a glTF file, rewriting image URIs that point to cooked .qoi files.

    The URI rewrite is a pure function of the source text and the converted
    texture names, so an up-to-date destination only needs re-cooking when the
    source changed, or when a brand-new texture appeared (`force=True`) that a
    glTF may now reference.
    """
    destination.parent.mkdir(parents=True, exist_ok=True)
    if PILImage is None:
        return copy_if_changed(source, destination)

    if not force and destination.is_file() and \
            destination.stat().st_mtime_ns >= source.stat().st_mtime_ns:
        return False

    text = source.read_text(encoding="utf-8", errors="ignore")

    def replace_uri(match: re.Match) -> str:
        uri = match.group(2)
        name = uri.rsplit("/", 1)[-1]
        if name in converted_names:
            return match.group(1) + Path(name).stem + ".qoi" + match.group(3)
        return match.group(0)

    cooked = _URI_PATTERN.sub(replace_uri, text)
    if cooked == text:
        # Nothing to rewrite: the plain copy is fresh if the destination is at
        # least as new as the source (copy2 preserves the source mtime).
        if destination.is_file() and \
                destination.stat().st_mtime_ns >= source.stat().st_mtime_ns:
            return False
        shutil.copy2(source, destination)
        return True

    if destination.is_file():
        # The source may be unchanged while its rewritten form already sits on
        # disk (e.g. a forced re-cook after a new texture appeared): only write
        # when the cooked content actually differs from the destination.
        try:
            if destination.read_text(encoding="utf-8", errors="ignore") == cooked:
                return False
        except OSError:
            pass
    destination.write_text(cooked, encoding="utf-8")
    return True


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: AssetCooker.py <configuration> <output-directory>", file=sys.stderr)
        return 2

    configuration = sys.argv[1]
    output_directory = Path(sys.argv[2]).resolve()
    project_root = Path(__file__).resolve().parents[1]
    kit_assets = project_root / "Game" / "Assets" / "Kit assets"
    character_root = (kit_assets / "Universal Base Characters[Standard]" /
                      "Base Characters")
    animation_root = kit_assets / "Universal Animation Library[Standard]" / "Unity"
    cooked_character_root = output_directory / "Assets" / "Characters"
    cooked_outfit_root = cooked_character_root / "Outfits"
    item_root = project_root / "Game" / "Assets" / "UltimateRPg Items Pack"
    cooked_item_root = output_directory / "Assets" / "Items"
    monster_root = kit_assets / "Ultimate Monsters"
    cooked_enemy_root = output_directory / "Assets" / "Enemies"
    nature_root = kit_assets / "Stylized Nature MegaKit[Standard]" / "glTF"
    cooked_nature_root = output_directory / "Assets" / "Nature"
    terrain_root = project_root / "Game" / "Assets" / "Terrain"
    if not terrain_root.is_dir():
        terrain_root = project_root / "Game" / "Assets" / "Terrian"
    cooked_terrain_root = output_directory / "Assets" / "Terrain"
    outfit_root = (kit_assets / "Modular Character Outfits - Fantasy[Standard]" /
                   "Exports" / "FBX (Unity)" / "Outfits")

    files = []          # plain copies (fbx, bin, dll, levels, ...)
    textures = []       # images re-encoded to .qoi
    gltf_files = []     # glTF JSON copied with image URIs rewritten

    files.append((character_root / "Unity" / "Superhero_Male_FullBody.fbx",
                  cooked_character_root / "PlayerMale.fbx"))
    files.append((character_root / "Unity" / "Superhero_Female_FullBody.fbx",
                  cooked_character_root / "PlayerFemale.fbx"))
    files.append((animation_root / "UAL1_Standard.fbx",
                  cooked_character_root / "PlayerAnimations.fbx"))

    for texture in (character_root / "Textures").glob("*.png"):
        textures.append((texture, cooked_character_root / texture.name))

    for outfit_name in ("Male_Peasant.fbx", "Female_Peasant.fbx",
                        "Male_Ranger.fbx", "Female_Ranger.fbx"):
        files.append((outfit_root / outfit_name, cooked_outfit_root / outfit_name))

    outfit_textures = (kit_assets / "Modular Character Outfits - Fantasy[Standard]" / "Textures")
    for texture_name in ("Peasant/T_Peasant_BaseColor.png", "Peasant/T_Peasant_Normal.png",
                         "Ranger/T_Ranger_BaseColor.png", "Ranger/T_Ranger_Normal.png"):
        source = outfit_textures / texture_name
        textures.append((source, cooked_outfit_root / source.name))

    item_files = (
        "Potion1_Filled_Red.png", "Axe_Double.png", "Armor_Leather.png", "Armor_Metal.png",
        "Potion1_Filled.fbx", "Axe_Double.fbx", "Armor_Leather.fbx", "Armor_Metal.fbx",
    )
    for item_file in item_files:
        source_root = item_root / ("Icons" if item_file.endswith(".png") else "FBX")
        destination = cooked_item_root / item_file
        if item_file.endswith(".png"):
            textures.append((source_root / item_file, destination))
        else:
            files.append((source_root / item_file, destination))

    enemy_models = {
        "Blob": ("GreenBlob", "Cactoro", "Orc", "Yeti"),
        "Big": ("BlueDemon", "Dino", "MushroomKing", "Orc_Skull"),
        "Flying": ("Dragon", "Ghost", "Armabee", "Squidle"),
    }
    for category, model_names in enemy_models.items():
        for model_name in model_names:
            gltf_files.append((monster_root / category / "glTF" / f"{model_name}.gltf",
                               cooked_enemy_root / category / f"{model_name}.gltf"))
        atlas = monster_root / category / "glTF" / "Atlas_Monsters.png"
        if atlas.is_file():
            textures.append((atlas, cooked_enemy_root / category / atlas.name))

    # glTF keeps geometry in companion .bin files and references the shared
    # texture files by name, so the complete pack is copied as one unit. The
    # textures are re-encoded and the glTF image URIs rewritten to .qoi.
    for source in nature_root.iterdir():
        if not source.is_file():
            continue
        suffix = source.suffix.lower()
        if suffix == ".gltf":
            gltf_files.append((source, cooked_nature_root / source.name))
        elif suffix in TEXTURE_EXTENSIONS:
            textures.append((source, cooked_nature_root / source.name))
        elif suffix == ".bin":
            files.append((source, cooked_nature_root / source.name))

    imported_prop_root = project_root / "Game" / "Assets" / "ImportedProps"
    if imported_prop_root.is_dir():
        for source in imported_prop_root.rglob("*"):
            if not source.is_file():
                continue
            destination = (output_directory / "Assets" / "ImportedProps" /
                           source.relative_to(imported_prop_root))
            if source.suffix.lower() in TEXTURE_EXTENSIONS:
                textures.append((source, destination))
            else:
                files.append((source, destination))

    for texture_name in ("grass.png", "dirt.png", "sand.png", "pavement.png"):
        textures.append((terrain_root / texture_name, cooked_terrain_root / texture_name))

    level_root = project_root / "Game" / "Assets" / "Levels"
    for level_file in level_root.glob("*.level"):
        files.append((level_file, output_directory / "Assets" / "Levels" / level_file.name))

    assimp_dll = (project_root / "Game" / "ThirdParty" / "assimp" / "bin" / "x64" /
                  "assimp-vc145-mt.dll")
    files.append((assimp_dll, output_directory / assimp_dll.name))

    copied = 0
    try:
        for source, destination in files:
            copied += int(copy_if_changed(source, destination))

        converted_names = set()
        new_texture_appeared = False
        for source, destination in textures:
            qoi_destination = destination.with_suffix(".qoi")
            existed = qoi_destination.is_file()
            copied += int(convert_texture_if_changed(source, destination))
            # A texture that did not exist before may now be referenced by a
            # glTF, so its URIs need re-checking even if the glTF itself is
            # unchanged.
            if not existed and qoi_destination.is_file():
                new_texture_appeared = True
            converted_names.add(destination.name)
            # Cooked builds ship the .qoi; the original image is redundant.
            # Remove it (also cleans stale PNGs from previous cooker versions).
            if qoi_destination.is_file():
                try:
                    destination.unlink()
                except OSError:
                    pass

        for source, destination in gltf_files:
            copied += int(cook_gltf_if_changed(
                source, destination, converted_names, force=new_texture_appeared))
    except (OSError, FileNotFoundError) as error:
        print(f"AssetCooker error: {error}", file=sys.stderr)
        return 1

    print(f"AssetCooker [{configuration}]: {copied} file(s) updated in {output_directory}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
