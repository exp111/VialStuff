import hid, sys, time, struct

# sends a msg to a open hid device
def hid_send(dev, msg, retries=1):
    MSG_LEN = 32
    if len(msg) > MSG_LEN:
        raise RuntimeError("message must be less than 32 bytes")
    msg += b"\x00" * (MSG_LEN - len(msg))

    data = b""
    first = True

    while retries > 0:
        retries -= 1
        if not first:
            time.sleep(0.5)
        first = False
        try:
            # add 00 at start for hidapi report id
            if dev.write(b"\x00" + msg) != MSG_LEN + 1:
                continue

            data = bytes(dev.read(MSG_LEN, timeout_ms=500))
            if not data:
                continue
        except OSError:
            continue
        break

    if not data:
        raise RuntimeError("failed to communicate with the device")
    return data

# checks if the device is the valid? this usually filters from multiple devices to 1
def is_rawhid(device):
    if device["usage_page"] != 0xFF60 or device["usage"] != 0x61:
        return False
    
    # try to open the path
    dev = hid.device()
    try:
        dev.open_path(device["path"])
    except OSError as e:
        return False
    
    #TODO: add those version checks
    dev.close()
    return True

# finds all compatible vial devices
def find_devices():
    VIAL_SERIAL_NUMBER_MAGIC = "vial:f64c2b3c"
    devices = []
    for dev in hid.enumerate():
        if VIAL_SERIAL_NUMBER_MAGIC in dev["serial_number"]:
            if is_rawhid(dev):
                devices.append(dev)
    return devices

devices = find_devices()
#print(devices)
if len(devices) == 0:
    print("no vial devices found")
    sys.exit(0)

device = devices[0]
print("using device: " + str(device))

dev = hid.device()
dev.open_path(device["path"])
# constants we need
CMD_VIA_LIGHTING_SET_VALUE = 0x07
CMD_VIA_LIGHTING_GET_VALUE = 0x08
QMK_RGBLIGHT_EFFECT = 0x81
# try to get rgb effect
effect = hid_send(dev, 
    struct.pack(">BB", CMD_VIA_LIGHTING_GET_VALUE, QMK_RGBLIGHT_EFFECT), retries=20)[2]
print("RGB Effect: " + str(effect))
# set effect to 0 (disable lightning)
print("Setting effect to 0")
hid_send(dev, 
    struct.pack(">BBB", CMD_VIA_LIGHTING_SET_VALUE, QMK_RGBLIGHT_EFFECT, 0), retries=20)
# wait for 2 seconds
time.sleep(2)
# set effect back to the previous value
print("Setting effect back to " + str(effect))
hid_send(dev, 
    struct.pack(">BBB", CMD_VIA_LIGHTING_SET_VALUE, QMK_RGBLIGHT_EFFECT, effect), retries=20)
dev.close()