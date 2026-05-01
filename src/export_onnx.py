import pathlib
import os
import torch
import torch.nn as nn
from pathlib import Path
import sys

# Monkeypatch PosixPath for Windows compatibility
if os.name == 'nt':
    pathlib.PosixPath = pathlib.WindowsPath

# Add src to path
sys.path.append(str(Path(__file__).parent))

from resemble_enhance.enhancer.inference import load_enhancer
from resemble_enhance.inference import remove_weight_norm_recursively
from resemble_enhance.denoiser.denoiser import Denoiser

# Monkeypatch Denoiser to avoid complex tensors (unsupported in ONNX)
def patched_stft(self, x):
    dtype = x.dtype
    device = x.device
    window = torch.hann_window(self.stft_cfg["win_length"], device=x.device)
    # Use return_complex=False for ONNX compatibility
    s = torch.stft(x.float(), **self.stft_cfg, window=window, return_complex=False)  # (b f t+1 2)
    s = s[..., :-1, :]  # (b f t 2)
    
    real = s[..., 0]
    imag = s[..., 1]
    
    mag = torch.sqrt(real**2 + imag**2 + 1e-7)
    phi = torch.atan2(imag, real)
    cos = torch.cos(phi)
    sin = torch.sin(phi)
    
    return mag.to(dtype=dtype, device=device), cos.to(dtype=dtype, device=device), sin.to(dtype=dtype, device=device)

def patched_istft(self, mag, cos, sin):
    device = mag.device
    dtype = mag.dtype
    real = mag * cos
    imag = mag * sin
    # Combine real and imag into (b f t 2)
    s = torch.stack([real, imag], dim=-1)
    s = torch.nn.functional.pad(s, (0, 0, 0, 1), "replicate")  # (b f t+1 2)
    window = torch.hann_window(self.stft_cfg["win_length"], device=s.device)
    x = torch.istft(s, **self.stft_cfg, window=window, return_complex=False)
    return x.to(dtype=dtype, device=device)

Denoiser._stft = patched_stft
Denoiser._istft = patched_istft

def export_denoiser(output_dir, device="cpu"):
    print("Exporting Denoiser to ONNX...")
    enhancer = load_enhancer(None, device)
    denoiser = enhancer.denoiser
    denoiser.eval()
    
    # Remove weight norm for ONNX compatibility
    remove_weight_norm_recursively(denoiser)
    
    # Dummy input: [batch, samples]
    dummy_input = torch.randn(1, 44100)
    
    output_path = output_dir / "denoiser.onnx"
    
    torch.onnx.export(
        denoiser,
        (dummy_input,),
        str(output_path),
        input_names=["input_waveform"],
        output_names=["denoised_waveform"],
        dynamic_axes={
            "input_waveform": {1: "samples"},
            "denoised_waveform": {1: "samples"}
        },
        opset_version=18
    )
    print(f"Denoiser exported to {output_path}")

def export_enhancer(output_dir, device="cpu"):
    print("Exporting Enhancer to ONNX...")
    enhancer = load_enhancer(None, device)
    enhancer.eval()
    
    # Configure for inference
    enhancer.configurate_(nfe=32, solver="midpoint", lambd=0.5, tau=0.5)
    
    # Remove weight norm
    remove_weight_norm_recursively(enhancer)

    # Dummy inputs for enhancer
    dummy_waveform = torch.randn(1, 44100)
    
    output_path = output_dir / "enhancer.onnx"
    
    print("Attempting Enhancer export (this may take a while)...")
    
    try:
        torch.onnx.export(
            enhancer,
            (dummy_waveform,),
            str(output_path),
            input_names=["input_waveform"],
            output_names=["enhanced_waveform"],
            dynamic_axes={
                "input_waveform": {1: "samples"},
                "enhanced_waveform": {1: "samples"}
            },
            opset_version=14,
            do_constant_folding=True
        )
        print(f"Enhancer exported to {output_path}")
    except Exception as e:
        print(f"Enhancer export failed: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    # Setup paths
    base_dir = Path(__file__).parent.parent
    onnx_dir = base_dir / "models_onnx"
    onnx_dir.mkdir(exist_ok=True)
    
    export_denoiser(onnx_dir)
    export_enhancer(onnx_dir)
