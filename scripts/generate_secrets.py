Import("env")

from pathlib import Path
import re

project_dir = Path(env.subst("$PROJECT_DIR"))
credentials_path = project_dir / "credentials.txt"
output_path = project_dir / "include" / "secrets.h"

if not credentials_path.exists():
    raise RuntimeError("credentials.txt is required for a private build")

values = {}
for raw_line in credentials_path.read_text(encoding="utf-8").splitlines():
    parts = raw_line.split()
    if len(parts) >= 2:
        values[parts[0].lower()] = parts[1]

required = ("wifi", "device", "api", "unifi")
missing = [key for key in required if key not in values]
if missing:
    raise RuntimeError("credentials.txt is missing required fields")

def c_string(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'

device = values["device"].replace(":", "").replace("-", "")
if not re.fullmatch(r"[0-9A-Fa-f]{12}", device):
    raise RuntimeError("Device must be a MAC address")

output_path.parent.mkdir(parents=True, exist_ok=True)
# The Wi-Fi line is the only two-value line: `Wifi <ssid> <password>`.
wifi_line = next((line.split() for line in credentials_path.read_text(encoding="utf-8").splitlines()
                  if line.split() and line.split()[0].lower() == "wifi"), [])
if len(wifi_line) < 3:
    raise RuntimeError("Wifi must contain SSID and password")

output_path.write_text(
    "// Generated from credentials.txt. Do not commit or distribute.\n"
    "#pragma once\n"
    f"#define WIFI_SSID {c_string(wifi_line[1])}\n"
    f"#define WIFI_PASSWORD {c_string(wifi_line[2])}\n"
    f"#define DEVICE_MAC {c_string(values['device'])}\n"
    f"#define UNIFI_API_KEY {c_string(values['api'])}\n"
    f"#define UNIFI_HOST {c_string(values['unifi'])}\n",
    encoding="utf-8",
)
