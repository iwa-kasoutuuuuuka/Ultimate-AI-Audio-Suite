import os
import sys
import threading
import time
import re
from pathlib import Path

import customtkinter as ctk
from tkinterdnd2 import DND_FILES, TkinterDnD

from dependency_manager import DependencyManager
from enhancer_logic import EnhancerLogic, get_files_in_folder

# Set appearance mode and color theme
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

class ResembleEnhanceApp(TkinterDnD.Tk):
    def __init__(self):
        super().__init__()

        self.title("Resemble Enhance - 音声補正ツール")
        self.geometry("800x600")

        # Set Icon
        self._setup_icon()

        # Initialize Logic
        self.dm = DependencyManager()
        self.enhancer = EnhancerLogic(models_dir=self.dm.models_dir, bin_dir=self.dm.bin_dir)
        self.is_processing = False
        self.stop_requested = False

        self._build_sidebar()
        self._build_main_frame()
        self._update_device_info()
        self._check_dependencies_on_startup()

    def _setup_icon(self):
        icon_path = self.dm.exe_dir / "assets" / "icon.png"
        if icon_path.exists():
            try:
                from PIL import Image
                # Windows taskbar fix
                import ctypes
                myappid = 'antigravity.resemble.enhance.1.0'
                ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)
                
                # Load and set icon
                img = Image.open(icon_path)
                # Convert to something tkinter likes
                from ImageTk import PhotoImage # Wait, customtkinter might handle this
                # Actually, standard way for CTk:
                self.iconpath = PhotoImage(file=icon_path)
                self.wm_iconphoto(True, self.iconpath)
            except Exception as e:
                print(f"Icon load error: {e}")

    def _update_device_info(self):
        info = self.enhancer.get_device_info()
        self.log(f"システム情報: {info}")
        self.status_label.configure(text=f"準備完了 | デバイス: {info}")

    def _build_sidebar(self):
        self.sidebar_frame = ctk.CTkFrame(self, width=200, corner_radius=0)
        self.sidebar_frame.pack(side="left", fill="y", padx=0, pady=0)
        
        self.logo_label = ctk.CTkLabel(self.sidebar_frame, text="Resemble Enhance", font=ctk.CTkFont(size=20, weight="bold"))
        self.logo_label.pack(padx=20, pady=(20, 10))

        self.sidebar_button_1 = ctk.CTkButton(self.sidebar_frame, text="自動モード", command=self.set_auto_mode)
        self.sidebar_button_1.pack(padx=20, pady=10)

        self.sidebar_button_2 = ctk.CTkButton(self.sidebar_frame, text="手動モード", command=self.set_manual_mode)
        self.sidebar_button_2.pack(padx=20, pady=10)

        self.device_label = ctk.CTkLabel(self.sidebar_frame, text="使用デバイス:", anchor="w")
        self.device_label.pack(padx=20, pady=(20, 0))
        self.device_optionemenu = ctk.CTkOptionMenu(self.sidebar_frame, values=["Auto", "CUDA (GPU)", "CPU"],
                                                                        command=self.change_device_event)
        self.device_optionemenu.pack(padx=20, pady=(10, 10))
        self.device_optionemenu.set("Auto")

        self.appearance_mode_label = ctk.CTkLabel(self.sidebar_frame, text="外観モード:", anchor="w")
        self.appearance_mode_label.pack(padx=20, pady=(20, 0))
        self.appearance_mode_optionemenu = ctk.CTkOptionMenu(self.sidebar_frame, values=["Light", "Dark", "System"],
                                                                       command=self.change_appearance_mode_event)
        self.appearance_mode_optionemenu.pack(padx=20, pady=(10, 10))
        self.appearance_mode_optionemenu.set("Dark")

    def _build_main_frame(self):
        self.main_frame = ctk.CTkFrame(self, corner_radius=15, fg_color="transparent")
        self.main_frame.pack(side="left", fill="both", expand=True, padx=20, pady=20)

        # Input Path (Drag and Drop)
        self.input_label = ctk.CTkLabel(self.main_frame, text="入力フォルダ (ここにドラッグ＆ドロップ):", font=ctk.CTkFont(size=14))
        self.input_label.pack(padx=20, pady=(10, 0), anchor="w")
        
        self.input_entry = ctk.CTkEntry(self.main_frame, placeholder_text="フォルダまたはファイルをここにドラッグ＆ドロップ...")
        self.input_entry.pack(fill="x", padx=20, pady=(5, 10))
        self.input_entry.drop_target_register(DND_FILES)
        self.input_entry.dnd_bind('<<Drop>>', self.on_drop_input)

        # Output Path (Drag and Drop)
        self.output_label = ctk.CTkLabel(self.main_frame, text="出力フォルダ:", font=ctk.CTkFont(size=14))
        self.output_label.pack(padx=20, pady=(10, 0), anchor="w")
        
        self.output_row = ctk.CTkFrame(self.main_frame, fg_color="transparent")
        self.output_row.pack(fill="x", padx=20, pady=(5, 10))
        
        self.output_entry = ctk.CTkEntry(self.output_row, placeholder_text="出力先フォルダ...")
        self.output_entry.pack(side="left", fill="x", expand=True)
        self.output_entry.drop_target_register(DND_FILES)
        self.output_entry.dnd_bind('<<Drop>>', self.on_drop_output)
        
        self.open_folder_btn = ctk.CTkButton(self.output_row, text="開く", width=60, command=self.open_output_folder)
        self.open_folder_btn.pack(side="left", padx=(10, 0))

        # Manual Settings Frame
        self.settings_frame = ctk.CTkFrame(self.main_frame)
        self.settings_frame.pack(fill="x", padx=20, pady=10)
        self.settings_frame.pack_forget() # Hidden initially

        self.solver_label = ctk.CTkLabel(self.settings_frame, text="ソルバー:")
        self.solver_label.grid(row=0, column=0, padx=20, pady=5, sticky="w")
        self.solver_menu = ctk.CTkOptionMenu(self.settings_frame, values=["midpoint", "rk4", "euler"])
        self.solver_menu.grid(row=0, column=1, padx=20, pady=5)
        self.solver_menu.set("midpoint")

        self.nfe_label = ctk.CTkLabel(self.settings_frame, text="NFE (品質):")
        self.nfe_label.grid(row=1, column=0, padx=20, pady=5, sticky="w")
        self.nfe_slider = ctk.CTkSlider(self.settings_frame, from_=1, to=128, number_of_steps=127)
        self.nfe_slider.grid(row=1, column=1, padx=20, pady=5)
        self.nfe_slider.set(64)

        self.tau_label = ctk.CTkLabel(self.settings_frame, text="Tau (温度):")
        self.tau_label.grid(row=2, column=0, padx=20, pady=5, sticky="w")
        self.tau_slider = ctk.CTkSlider(self.settings_frame, from_=0, to=1.0)
        self.tau_slider.grid(row=2, column=1, padx=20, pady=5)
        self.tau_slider.set(0.5)

        self.lambd_label = ctk.CTkLabel(self.settings_frame, text="Denoise (強度):")
        self.lambd_label.grid(row=3, column=0, padx=20, pady=5, sticky="w")
        self.lambd_label_val = ctk.CTkLabel(self.settings_frame, text="0.9") # Initial value
        self.lambd_label_val.grid(row=3, column=2, padx=10, pady=5)
        self.lambd_slider = ctk.CTkSlider(self.settings_frame, from_=0, to=1.0, command=lambda v: self.lambd_label_val.configure(text=f"{v:.1f}"))
        self.lambd_slider.grid(row=3, column=1, padx=20, pady=5)
        self.lambd_slider.set(0.9)

        # Output Format and Suffix
        self.format_label = ctk.CTkLabel(self.settings_frame, text="出力形式:")
        self.format_label.grid(row=4, column=0, padx=20, pady=5, sticky="w")
        self.format_menu = ctk.CTkSegmentedButton(self.settings_frame, values=["mp3", "wav"])
        self.format_menu.grid(row=4, column=1, padx=20, pady=5, sticky="ew")
        self.format_menu.set("mp3")

        self.suffix_label = ctk.CTkLabel(self.settings_frame, text="末尾文字:")
        self.suffix_label.grid(row=5, column=0, padx=20, pady=5, sticky="w")
        self.suffix_entry = ctk.CTkEntry(self.settings_frame, placeholder_text="_enhanced")
        self.suffix_entry.grid(row=5, column=1, padx=20, pady=5, sticky="ew")
        self.suffix_entry.insert(0, "_enhanced")

        # Control Buttons
        self.btn_frame = ctk.CTkFrame(self.main_frame, fg_color="transparent")
        self.btn_frame.pack(fill="x", padx=20, pady=10)

        self.start_button = ctk.CTkButton(self.btn_frame, text="一括補正開始", command=self.start_processing)
        self.start_button.pack(side="left", expand=True, padx=5)

        self.preview_button = ctk.CTkButton(self.btn_frame, text="プレビュー (10秒)", command=self.start_preview, fg_color="green", hover_color="darkgreen")
        self.preview_button.pack(side="left", expand=True, padx=5)

        self.stop_button = ctk.CTkButton(self.btn_frame, text="停止", command=self.stop_processing, state="disabled")
        self.stop_button.pack(side="left", expand=True, padx=5)

        # Progress
        self.progress_bar = ctk.CTkProgressBar(self.main_frame)
        self.progress_bar.pack(fill="x", padx=20, pady=10)
        self.progress_bar.set(0)

        self.status_label = ctk.CTkLabel(self.main_frame, text="準備完了", font=ctk.CTkFont(size=12))
        self.status_label.pack(padx=20, pady=5)

        # Log Console
        self.log_textbox = ctk.CTkTextbox(self.main_frame, height=150, corner_radius=10)
        self.log_textbox.pack(fill="both", expand=True, padx=20, pady=10)
        self.log_textbox.configure(state="disabled")

    def on_drop_input(self, event):
        paths = self.parse_dnd_paths(event.data)
        if paths:
            path = paths[0] # Take the first one
            self.input_entry.delete(0, 'end')
            self.input_entry.insert(0, path)
            if os.path.isfile(path):
                self.log(f"ファイルを読み込みました: {os.path.basename(path)}")
            else:
                self.log(f"フォルダを読み込みました: {path}")

    def on_drop_output(self, event):
        paths = self.parse_dnd_paths(event.data)
        if paths:
            path = paths[0]
            if os.path.isdir(path):
                self.output_entry.delete(0, 'end')
                self.output_entry.insert(0, path)

    def parse_dnd_paths(self, data):
        # tkinterdnd2 handles paths with spaces by wrapping them in { }
        # Find all content inside { } or non-space sequences
        paths = re.findall(r'\{(.*?)\}|(\S+)', data)
        return [p[0] if p[0] else p[1] for p in paths]

    def open_output_folder(self):
        path = self.output_entry.get()
        if os.path.isdir(path):
            os.startfile(path)
        else:
            self.log("エラー: 有効な出力フォルダが指定されていません。")

    def set_auto_mode(self):
        self.settings_frame.pack_forget()
        self.status_label.configure(text="モード: 自動")

    def set_manual_mode(self):
        self.settings_frame.pack(fill="x", padx=20, pady=10)
        self.status_label.configure(text="モード: 手動")

    def log(self, message):
        self.log_textbox.configure(state="normal")
        self.log_textbox.insert("end", f"[{time.strftime('%H:%M:%S')}] {message}\n")
        self.log_textbox.see("end")
        self.log_textbox.configure(state="disabled")



    def change_device_event(self, new_device: str):
        self.log(f"使用デバイス設定を変更しました: {new_device}")

    def start_preview(self):
        self.start_processing(is_preview=True)

    def start_processing(self, is_preview=False):
        input_path = self.input_entry.get()
        output_dir = self.output_entry.get()

        if not os.path.exists(input_path) or not os.path.isdir(output_dir):
            self.status_label.configure(text="エラー: 入力または出力パスが無効です。")
            return

        if not self.dm.check_ffmpeg() or not self.dm.check_models():
            self.status_label.configure(text="エラー: 依存関係が不足しています。")
            return

        files = get_files_in_folder(input_path)
        if not files:
            self.status_label.configure(text="サポートされているファイルが見つかりません。")
            return

        self.is_processing = True
        self.stop_requested = False
        self.start_button.configure(state="disabled")
        self.preview_button.configure(state="disabled")
        self.stop_button.configure(state="normal")
        
        threading.Thread(target=self._process_thread, args=(files, output_dir, is_preview, input_path), daemon=True).start()

    def _process_thread(self, files, output_dir, is_preview, base_input):
        total = len(files)
        mode = "manual" if self.settings_frame.winfo_viewable() else "auto"
        params = {
            "solver": self.solver_menu.get(),
            "nfe": self.nfe_slider.get(),
            "tau": self.tau_slider.get(),
            "lambd": self.lambd_slider.get() if mode == "manual" else 0.9
        }
        
        output_format = self.format_menu.get()
        suffix = self.suffix_entry.get()
        device_choice = self.device_optionemenu.get()
        force_device = None
        if device_choice == "CUDA (GPU)": force_device = "cuda"
        elif device_choice == "CPU": force_device = "cpu"

        self.log(f"処理を開始します。プレビュー: {'ON' if is_preview else 'OFF'}, モード: {mode}")
        self.log(f"設定: 形式={output_format}, Suffix={suffix}, デバイス={device_choice}")
        self.log(f"AIパラメータ: {params}")

        for i, (f_abs, f_rel) in enumerate(files):
            if self.stop_requested:
                self.log("ユーザーによって停止されました。")
                break
            
            fname = os.path.basename(f_abs)
            self.status_label.configure(text=f"処理中 ({i+1}/{total}): {fname}")
            self.progress_bar.set((i) / total)
            self.log(f"[{i+1}/{total}] 処理中: {fname}...")
            
            start_t = time.time()
            success, result = self.enhancer.process_file(
                f_abs, output_dir, mode=mode, params=params,
                output_format=output_format, suffix=suffix, 
                is_preview=is_preview, relative_path=f_rel,
                force_device=force_device
            )
            elapsed = time.time() - start_t
            
            if success:
                self.log(f"完了 ({elapsed:.1f}秒): {os.path.basename(result)}")
            else:
                self.log(f"エラー: {fname} -> {result}")
            
            if is_preview:
                self.log("プレビュー生成が完了しました。")
                break

        self.progress_bar.set(1.0)
        self.status_label.configure(text="完了！" if not self.stop_requested else "停止しました。")
        self.log("すべての処理が終了しました。" if not self.stop_requested else "処理が中断されました。")
        self.is_processing = False
        self.start_button.configure(state="normal")
        self.preview_button.configure(state="normal")
        self.stop_button.configure(state="disabled")

    def stop_processing(self):
        self.stop_requested = True
        self.status_label.configure(text="停止中... 現在のファイルの処理が終わるまでお待ちください。")

    def _check_dependencies_on_startup(self):
        if not self.dm.check_ffmpeg() or not self.dm.check_models():
            self.status_label.configure(text="依存関係が不足しています！ダウンロードを開始します...")
            threading.Thread(target=self._download_dependencies, daemon=True).start()

    def _download_dependencies(self):
        try:
            if not self.dm.check_ffmpeg():
                self.status_label.configure(text="FFmpegをダウンロード中...")
                self.dm.setup_ffmpeg(progress_callback=self.progress_bar.set)
            
            if not self.dm.check_models():
                self.status_label.configure(text="AIモデルをダウンロード中 (時間がかかる場合があります)...")
                self.dm.setup_models(progress_callback=self.progress_bar.set)
            
            self.status_label.configure(text="すべての依存関係の準備ができました。")
            self.progress_bar.set(0)
        except Exception as e:
            self.status_label.configure(text=f"ダウンロードエラー: {str(e)}")

    def change_appearance_mode_event(self, new_appearance_mode: str):
        ctk.set_appearance_mode(new_appearance_mode)

if __name__ == "__main__":
    app = ResembleEnhanceApp()
    app.mainloop()
