"""Prepare a local DirectMusic Producer 5.3.0.900 copy for ordinary startup."""

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import struct
import tempfile


EXE = "DMUSProd.exe"
ORIGINAL_SHA256 = "fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011"
TEXT_RVA = 0x1000
TEXT_OFFSET = 0x400
TEXT_VIRTUAL_SIZE_FIELD = 0x210
PROFILE_CODE_RVA = 0x43702  # First byte of .text file-alignment padding.


def rel32(opcode, rva, target):
    instruction = bytes.fromhex(opcode)
    return instruction + struct.pack("<i", target - rva - len(instruction) - 4)


def patch_original(original, *, local_profile=True):
    if hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
        raise ValueError(f"{EXE} is not the supported 5.3.0.900 executable (SHA-256 mismatch)")
    image = bytearray(original)

    def replace(rva, expected, replacement):
        offset = TEXT_OFFSET + rva - TEXT_RVA
        if len(expected) != len(replacement) or image[offset:offset + len(expected)] != expected:
            raise ValueError(f"Unexpected executable bytes at RVA {rva:#x}")
        image[offset:offset + len(expected)] = replacement

    # CJazzApp::InitInstance calls RegisterShellFileTypes(FALSE), then removes
    # and registers Producer file associations in HKCR. Keep EnableShellOpen
    # immediately before this sequence and command-line processing after it.
    associations = bytes.fromhex("53 8b ce e8 4d 40 01 00 8b ce e8 a3 c5 ff ff 8b ce e8 2e c0 ff ff")
    replace(0x24387, associations, b"\x90" * len(associations))

    # COleTemplateServer::UpdateRegistry reports the warning (MFC string
    # 0xF18A). Skip it and COleObjectFactory::UpdateRegistryAll. Earlier OLE
    # initialization and factory creation are still needed by the editor.
    ole_registration = bytes.fromhex("53 53 6a 03 8d 8e 3c 01 00 00 e8 01 40 01 00 57 e8 f5 3f 01 00")
    replace(0x243C6, ole_registration, b"\x90" * len(ole_registration))

    if not local_profile:
        return image

    # Before any profile lookup, use the existing GetModuleFileNameA result
    # (length in EAX, path at EBP-0x198) to put the INI beside the copy.
    # MFC owns m_pszProfileName, so allocate and release it using its CRT.
    hook_rva = 0x241B3
    original_setup = bytes.fromhex("53 53 8d 85 68 fd ff ff")
    code = bytearray()

    def branch(opcode, target):
        code.extend(rel32(opcode, PROFILE_CODE_RVA + len(code), target))

    abort_init = 0x240CD
    code.extend(bytes.fromhex("83 f8 04"))  # cmp eax, 4
    branch("0f 86", abort_init)            # jbe: short path
    code.extend(bytes.fromhex("3d 04 01 00 00"))  # cmp eax, MAX_PATH
    branch("0f 83", abort_init)                  # jae: truncated path
    code.extend(bytes.fromhex(
        "50 8d 85 68 fe ff ff 50"       # Save length; push the existing path buffer.
        " ff 15 98 1c 00 01 59 59"     # _strdup(path); discard argument; pop length into ECX.
        " 85 c0"                      # test eax, eax
    ))
    branch("0f 84", abort_init)         # je: allocation failed
    code.extend(bytes.fromhex(
        "c7 44 08 fd 69 6e 69 00"       # Replace the last three characters with 'ini'.
        " 50 ff b6 94 00 00 00"        # Save new pointer; push old m_pszProfileName.
        " ff 15 50 1c 00 01 59 58"     # free(old); discard argument; pop new pointer.
        " 89 86 94 00 00 00"           # Store new m_pszProfileName.
    ))
    code.extend(original_setup)
    branch("e9", hook_rva + len(original_setup))

    replace(hook_rva, original_setup, rel32("e9", hook_rva, PROFILE_CODE_RVA) + b"\x90" * 3)
    replace(PROFILE_CODE_RVA, bytes(len(code)), code)
    struct.pack_into("<I", image, TEXT_VIRTUAL_SIZE_FIELD, PROFILE_CODE_RVA - TEXT_RVA + len(code))
    return image


def main():
    default_source = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-directory", type=Path,
                        default=default_source / "Microsoft DirectMusic Producer")
    args = parser.parse_args()
    source = args.original_directory.resolve()
    root = Path(__file__).resolve().parents[1]
    work = root / "work"
    destination = work / "producer-no-registry-warning"

    try:
        if destination.resolve() != destination:
            raise ValueError(f"Expected a local destination under {root}")
        original = (source / EXE).read_bytes()
        installed_backup = work / "installed-producer-startup" / "DMUSProd.original.exe"
        if hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256 and installed_backup.is_file():
            backup = installed_backup.read_bytes()
            if original == patch_original(backup, local_profile=False):
                original = backup
        executable = patch_original(original)
        if destination.exists():
            if (destination / EXE).read_bytes() != executable:
                raise ValueError(f"Existing copy differs. Move it aside before regenerating: {destination}")
        else:
            work.mkdir(exist_ok=True)
            with tempfile.TemporaryDirectory(dir=work) as staging:
                app = Path(staging) / "app"
                app.mkdir()
                for file in source.iterdir():
                    if (file.is_file() and file.suffix.lower() in {".dll", ".ocx", ".bmp", ".chm", ".txt"}
                            and file.name.lower() != "uninst.dll"):
                        shutil.copyfile(file, app / file.name)
                (app / EXE).write_bytes(executable)
                app.rename(destination)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")

    print(destination / EXE)


if __name__ == "__main__":
    main()
