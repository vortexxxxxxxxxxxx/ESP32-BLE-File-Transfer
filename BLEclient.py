###########################################
"""
               /$$      
              | $$      
 /$$$$$$/$$$$ | $$   /$$
| $$_  $$_  $$| $$  /$$/
| $$ \ $$ \ $$| $$$$$$/ 
| $$ | $$ | $$| $$_  $$ 
| $$ | $$ | $$| $$ \  $$
|__/ |__/ |__/|__/  \__/                     

-Uses Asyncio Library for I/O
-Uses Bleak for BLE communication

"""
###########################################
import asyncio
import os
from bleak import BleakClient, BleakScanner, BleakError

DEVICE_NAME = "NanoESP32-BLE"
CHAR_UUID    = "19b10001-e8f2-537e-4f6c-d104768a1214"
CHUNK_SIZE   = 244  # match your MTU - 3

def notification_handler(sender, data):
    text = data.decode(errors='ignore')
    # filter markers
    if text == "[DIR_START]":
        print("\n--- Directory Listing Start ---")
    elif text == "[DIR_END]":
        print("--- Directory Listing End ---\n")
    else:
        print(text)

async def find_device(name: str):
    while True:
        print(f"Scanning for '{name}'...")
        try:
            devices = await BleakScanner.discover(timeout=3.0)
            for d in devices:
                if d.name == name:
                    print(f"→ Found {d.name} [{d.address}]")
                    return d
            print("…retrying\n")
        except Exception as e:
            print("Scan error:", e)
        await asyncio.sleep(1.0)

async def send_file(client: BleakClient, path: str):
    if not os.path.isfile(path):
        print("File not found:", path)
        return

    fname = os.path.basename(path)
    size  = os.path.getsize(path)
    print(f"Sending '{fname}' ({size} bytes)…")

    # start marker with filename
    start = f"[FILE_START:{fname}]".encode()
    await client.write_gatt_char(CHAR_UUID, start, response=False)
    # send file chunks
    with open(path, "rb") as f:
        while chunk := f.read(CHUNK_SIZE):
            await client.write_gatt_char(CHAR_UUID, chunk, response=False)
            await asyncio.sleep(0)
    # end marker
    await client.write_gatt_char(CHAR_UUID, b"[FILE_END]", response=False)
    print("File transfer complete.")

async def request_list(client: BleakClient):
    await client.write_gatt_char(CHAR_UUID, b"[LIST]", response=False)

async def main():
    dev = await find_device(DEVICE_NAME)
    async with BleakClient(dev.address) as client:
        print("Connected; discovering services…")
        await client.get_services()
        for svc in client.services:
            print(f"Service {svc.uuid}:")
            for char in svc.characteristics:
                print("  •", char.uuid, char.properties)

        await client.start_notify(CHAR_UUID, notification_handler)
        print("\nCommands:\n"
              "  <text>           send a text message\n"
              "  sendfile <path>  transfer a file\n"
              "  list             list SD directory\n"
              "  exit             quit\n")

        while True:
            try:
                cmd = await asyncio.to_thread(input, "> ")
            except (EOFError, KeyboardInterrupt):
                break
            cmd = cmd.strip()
            if cmd.lower() == "exit":
                break
            elif cmd.lower() == "list":
                await request_list(client)
            elif cmd.startswith("sendfile "):
                _, path = cmd.split(" ", 1)
                await send_file(client, path.strip())
            elif cmd:
                await client.write_gatt_char(CHAR_UUID, cmd.encode(), response=False)

        await client.stop_notify(CHAR_UUID)
        print("Disconnected.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nUser exit")



