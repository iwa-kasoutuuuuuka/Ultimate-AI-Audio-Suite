import os
import torch
import torchaudio
from pathlib import Path
from resemble_enhance.enhancer.inference import load_enhancer, load_denoiser
from resemble_enhance.data.utils import mel_spectrogram
from moviepy import VideoFileClip, AudioFileClip
from concurrent.futures import ThreadPoolExecutor

class EnhancerLogic:
    def __init__(self, models_dir=None, bin_dir=None):
        self.models_dir = Path(models_dir) if models_dir else Path("./models")
        self.bin_dir = Path(bin_dir) if bin_dir else Path("./bin")
        
        # Configure Resemble Enhance to find models in the local folder
        self.hf_cache = self.models_dir.parent / "hf_cache"
        os.environ["HF_HOME"] = str(self.hf_cache)
        
        # Set FFmpeg path for moviepy and others
        ffmpeg_exe = self.bin_dir / "ffmpeg.exe"
        if ffmpeg_exe.exists():
            os.environ["IMAGEIO_FFMPEG_EXE"] = str(ffmpeg_exe)
            # Also add to PATH just in case
            os.environ["PATH"] = str(self.bin_dir) + os.pathsep + os.environ.get("PATH", "")
        
        self.enhancer_model = None
        self.denoiser_model = None
        self.current_device = None

    def _load_models(self, device):
        # Force reload if device changed
        if self.current_device != device:
            self.enhancer_model = None
            self.denoiser_model = None
            self.current_device = device

        try:
            if self.enhancer_model is None:
                # We point to self.models_dir. The library expects enhancer_stage1/model.pt etc. inside it.
                model_path = self.models_dir / "enhancer_stage1" / "model.pt"
                if not model_path.exists():
                    raise FileNotFoundError(f"Enhancer model not found at {model_path}")
                self.enhancer_model = load_enhancer(str(self.models_dir), device)
            
            if self.denoiser_model is None:
                model_path = self.models_dir / "denoiser" / "model.pt"
                if not model_path.exists():
                    raise FileNotFoundError(f"Denoiser model not found at {model_path}")
                self.denoiser_model = load_denoiser(str(self.models_dir), device)
            
            return self.enhancer_model, self.denoiser_model
        except Exception as e:
            raise RuntimeError(f"AIモデルの読み込みに失敗しました: {str(e)}")

    def get_device_info(self):
        if torch.cuda.is_available():
            return f"GPU (CUDA): {torch.cuda.get_device_name(0)}"
        return "CPU"

    def process_file(self, input_path, output_folder, mode="auto", params=None, 
                     output_format="mp3", suffix="_enhanced", is_preview=False, 
                     relative_path="", force_device=None):
        """
        Process a single audio or video file.
        params: dict with solver, nfe, tau, lambd
        """
        input_path = Path(input_path)
        output_folder = Path(output_folder)
        
        # Determine output path (handle mirroring)
        target_dir = output_folder / relative_path
        target_dir.mkdir(parents=True, exist_ok=True)
        
        ext = ".mp3" if output_format == "mp3" else ".wav"
        output_file = target_dir / (input_path.stem + suffix + ext)
        
        # 1. Extract audio if video
        temp_wav = target_dir / f"temp_{input_path.stem}.wav"
        is_video = input_path.suffix.lower() in [".mp4", ".mkv", ".mov", ".avi"]
        
        try:
            if is_video:
                video = VideoFileClip(str(input_path))
                if video.audio is None:
                    return False, "動画に音声トラックが含まれていません。"
                
                # If preview, only take 10 seconds
                if is_preview and video.duration > 10:
                    video = video.subclip(0, 10)
                
                video.audio.write_audiofile(str(temp_wav), codec='pcm_s16le', verbose=False, logger=None)
                audio_path = temp_wav
                video.close()
            else:
                audio_path = input_path

            # 2. Load audio
            dwaveform, dsr = torchaudio.load(str(audio_path))
            
            # If preview and not video, trim to 10 seconds
            if is_preview and not is_video:
                max_samples = 10 * dsr
                dwaveform = dwaveform[:, :max_samples]
            
            # 3. Apply Resemble Enhance
            # For "auto", we use default or balanced settings
            if mode == "auto":
                # Default parameters for 'enhance'
                solver = "midpoint"
                nfe = 64
                tau = 0.5
                lambd = 0.9 # Full denoise + enhance
            else:
                solver = params.get("solver", "midpoint")
                nfe = params.get("nfe", 64)
                tau = params.get("tau", 0.5)
                lambd = params.get("lambd", 0.9)

            # Resemble Enhance logic
            device = force_device if force_device else ("cuda" if torch.cuda.is_available() else "cpu")
            enhancer, denoiser = self._load_models(device)

            # Manual implementation of 'enhance' to use our cached models and local paths
            # 1. Denoise
            denoised_waveform, sr = denoiser(dwaveform, dsr, device)
            
            # 2. Enhance
            # The enhancer takes the denoised waveform or original? 
            # Usually it takes the denoised one if lambd > 0
            enhanced_waveform, new_sr = enhancer(denoised_waveform, sr, device, nfe=int(nfe), solver=solver, tau=float(tau))
            
            # 3. Blend (lambd)
            # lambd=0 means only enhance, lambd=1 means only denoise (resampled)
            if lambd > 0:
                # Resample denoised to match enhanced (44.1kHz)
                resampler = torchaudio.transforms.Resample(dsr, new_sr).to(device)
                denoised_resampled = resampler(denoised_waveform)
                
                # Match lengths (they might differ slightly due to framing)
                min_len = min(enhanced_waveform.shape[1], denoised_resampled.shape[1])
                final_waveform = (1 - lambd) * enhanced_waveform[:, :min_len] + lambd * denoised_resampled[:, :min_len]
            else:
                final_waveform = enhanced_waveform

            # 4. Save
            enhanced_wav = target_dir / f"enhanced_{input_path.stem}.wav"
            torchaudio.save(str(enhanced_wav), final_waveform.cpu(), new_sr)
            
            if output_format == "wav":
                # Move/Rename to final destination
                if os.path.exists(output_file): os.remove(output_file)
                os.rename(enhanced_wav, output_file)
            else:
                # Convert to MP3
                audio_clip = AudioFileClip(str(enhanced_wav))
                audio_clip.write_audiofile(str(output_file), bitrate="320k", verbose=False, logger=None)
                audio_clip.close()
                if os.path.exists(enhanced_wav): os.remove(enhanced_wav)

            # Cleanup
            if os.path.exists(temp_wav): os.remove(temp_wav)
            if os.path.exists(enhanced_wav): os.remove(enhanced_wav)

            return True, str(output_file)
        
        except Exception as e:
            return False, str(e)

def get_files_in_folder(path):
    extensions = [".wav", ".mp3", ".flac", ".m4a", ".mp4", ".mkv", ".mov", ".avi"]
    files = []
    path = Path(path)
    
    if path.is_file():
        if path.suffix.lower() in extensions:
            files.append((str(path), ""))
        return files
        
    for root, _, filenames in os.walk(path):
        for f in filenames:
            if any(f.lower().endswith(ext) for ext in extensions):
                abs_p = os.path.join(root, f)
                rel_p = os.path.relpath(root, path)
                if rel_p == ".": rel_p = ""
                files.append((abs_p, rel_p))
    return files
