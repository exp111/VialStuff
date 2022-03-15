#include <iostream>
#include <hidapi.h>
#include <vector>
#include <chrono>
#include <thread>

#define MSG_SIZE 33
#define CMD_VIA_LIGHTING_SET_VALUE 0x07
#define CMD_VIA_LIGHTING_GET_VALUE 0x08
#define QMK_RGBLIGHT_EFFECT 0x81

//TODO: add retries?
int hid_send(hid_device* handle, unsigned char* buf)
{
	int res = hid_write(handle, buf, MSG_SIZE);
	if (res < 0)
	{
		return res;
	}
	res = hid_read(handle, buf, MSG_SIZE);
	if (res < 0)
	{
		return res;
	}
	return 0;
}

int main()
{
	int res;
	const wchar_t* err;
	unsigned char buf[MSG_SIZE]; // 32 + 1 (for the report id)

	res = hid_init();

	// find vial devices
	const wchar_t* VialSerialNumberMagic = L"vial:f64c2b3c";
	std::vector<hid_device_info> devices = std::vector<hid_device_info>();
	hid_device_info* devs, * cur_dev;
	devs = hid_enumerate(0x0, 0x0);
	if (devs == nullptr)
	{
		printf("no hid devices. exiting...\n");
		return -1;
	}
	cur_dev = devs;
	do
	{
		//TODO: probably log stuff?
		// check if device serial number contains the magic
		if (std::wstring(cur_dev->serial_number).find(VialSerialNumberMagic) == std::wstring::npos)
			continue;

		// now check for the rawhid params
		if (cur_dev->usage_page != 0xFF60 || cur_dev->usage != 0x61)
			continue;

		hid_device* handle = hid_open_path(cur_dev->path);
		if (!handle)
			continue;
		//TODO: check extra stuff

		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("  Release:      %hx\n", cur_dev->release_number);
		printf("  Interface:    %d\n", cur_dev->interface_number);
		printf("  Usage (page): 0x%hx (0x%hx)\n", cur_dev->usage, cur_dev->usage_page);
		printf("\n");

		// copy the content over, cause cur_dev gets freed after the loop
		hid_device_info copy = hid_device_info();
		char buf[255];
		strncpy_s(buf, sizeof(buf), cur_dev->path, strlen(cur_dev->path));
		copy.path = buf;
		wchar_t wbuf[255];
		wcsncpy_s(wbuf, _countof(wbuf), cur_dev->manufacturer_string, wcslen(cur_dev->manufacturer_string));
		copy.manufacturer_string = wbuf;

		devices.push_back(copy);
	} while (cur_dev = cur_dev->next);
	hid_free_enumeration(devs);

	if (devices.size() == 0)
	{
		printf("no vial devices found\n");
		return 0;
	}

	hid_device_info selected = devices[0];
	printf("manufacturer: %ls\n", selected.manufacturer_string);
	hid_device* handle = hid_open_path(selected.path);

	// get current effect
	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x0; // report id
	buf[1] = CMD_VIA_LIGHTING_GET_VALUE;
	buf[2] = QMK_RGBLIGHT_EFFECT;
	res = hid_send(handle, buf);
	if (res < 0)
	{
		printf("failed at get, %i\n", res);
		err = hid_error(handle);
		printf("%ls", err);
	}

	int currentEffect = buf[2];
	printf("current effect: %i\n", currentEffect);

	// set effect to 0
	//TODO: do we need the memset? not really (except the initial) since we know here how much is written
	printf("setting effect to 0\n");
	memset(buf, 0x0, sizeof(buf));
	buf[1] = CMD_VIA_LIGHTING_SET_VALUE;
	buf[2] = QMK_RGBLIGHT_EFFECT;
	buf[3] = 0;
	res = hid_send(handle, buf);
	if (res < 0)
	{
		printf("failed at set, %i\n", res);
		err = hid_error(handle);
		printf("%ls", err);
	}

	// sleep for 2 seconds
	std::this_thread::sleep_for(std::chrono::seconds(2));

	// set effect back to previous
	printf("setting effect to %i\n", currentEffect);
	memset(buf, 0x0, sizeof(buf));
	buf[1] = CMD_VIA_LIGHTING_SET_VALUE;
	buf[2] = QMK_RGBLIGHT_EFFECT;
	buf[3] = currentEffect;
	res = hid_send(handle, buf);
	if (res < 0)
	{
		printf("failed at set, %i\n", res);
		err = hid_error(handle);
		printf("%ls", err);
	}

	res = hid_exit();
}
