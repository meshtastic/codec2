# Convert the linked hex to UF2 so the tool can be dragged onto a board, the
# same way the firmware repo does it in extra_scripts/nrf52_extra.py.
import sys
from os.path import basename

Import("env")


def hex_to_uf2(source, target, env):
    hex_path = target[0].get_abspath()
    uf2_path = hex_path.replace(".hex", ".uf2")
    conv = env.subst("$PROJECT_DIR") + "/../uf2conv.py"
    env.Execute(
        env.VerboseAction(
            f'"{sys.executable}" "{conv}" "{hex_path}" -c -f 0xADA52840 -o "{uf2_path}"',
            f"Generating UF2 from {basename(hex_path)}",
        )
    )


env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", hex_to_uf2)
