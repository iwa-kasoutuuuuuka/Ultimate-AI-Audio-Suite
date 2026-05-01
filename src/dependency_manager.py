import os
import sys
import urllib.request
import zipfile
import shutil
from pathlib import Path

# Configuration
FFMPEG_URL = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
MODELS = {
    "denoiser": "https://huggingface.co/resemble-ai/resemble-enhance/resolve/main/denoiser/model.pt",
    "enhancer_stage1": "https://huggingface.co/resemble-ai/resemble-enhance/resolve/main/enhancer_stage1/model.pt",
    "enhancer_stage2": "https://huggingface.co/resemble-ai/resemble-enhance/resolve/main/enhancer_stage2/model.pt",
}

class DependencyManager:
    def __init__(self, base_dir=None):
        if base_dir is None:
            if getattr(sys, 'frozen', False):
                self.base_dir = Path(sys._MEIPASS)
                self.exe_dir = Path(sys.executable).parent
            else:
                self.base_dir = Path(__file__).parent
                # If we are in 'src' folder, the binaries and models are in the parent
                if self.base_dir.name == "src":
                    self.exe_dir = self.base_dir.parent
                else:
                    self.exe_dir = self.base_dir
        else:
            self.base_dir = Path(base_dir)
            self.exe_dir = self.base_dir

        self.bin_dir = self.exe_dir / "bin"
        self.models_dir = self.exe_dir / "models"
        
        self.bin_dir.mkdir(exist_ok=True)
        self.models_dir.mkdir(exist_ok=True)

    def check_ffmpeg(self):
        ffmpeg_exe = self.bin_dir / "ffmpeg.exe"
        return ffmpeg_exe.exists()

    def check_models(self):
        for name in MODELS.keys():
            model_path = self.models_dir / name / "model.pt"
            if not model_path.exists():
                return False
        return True

    def download_file(self, url, dest_path, progress_callback=None):
        dest_path.parent.mkdir(parents=True, exist_ok=True)
        
        def report_hook(count, block_size, total_size):
            if progress_callback:
                progress = count * block_size / total_size
                progress_callback(min(progress, 1.0))

        urllib.request.urlretrieve(url, dest_path, reporthook=report_hook if progress_callback else None)

    def setup_ffmpeg(self, progress_callback=None):
        zip_path = self.bin_dir / "ffmpeg.zip"
        self.download_file(FFMPEG_URL, zip_path, progress_callback)
        
        # Extract
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            # Find ffmpeg.exe in the zip and extract it to bin/
            for member in zip_ref.namelist():
                if member.endswith("ffmpeg.exe") or member.endswith("ffprobe.exe"):
                    filename = os.path.basename(member)
                    source = zip_ref.open(member)
                    target = open(self.bin_dir / filename, "wb")
                    with source, target:
                        shutil.copyfileobj(source, target)
        
        os.remove(zip_path)

    def setup_models(self, progress_callback=None):
        total_models = len(MODELS)
        for i, (name, url) in enumerate(MODELS.items()):
            model_path = self.models_dir / name / "model.pt"
            # Sub-progress for each model
            def sub_callback(p):
                if progress_callback:
                    overall_p = (i + p) / total_models
                    progress_callback(overall_p)
            
            self.download_file(url, model_path, sub_callback)

if __name__ == "__main__":
    # Test
    dm = DependencyManager()
    print(f"FFmpeg exists: {dm.check_ffmpeg()}")
    print(f"Models exist: {dm.check_models()}")
