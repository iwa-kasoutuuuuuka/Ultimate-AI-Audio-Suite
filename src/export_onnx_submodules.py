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

def export_denoiser_unet(output_dir, device="cpu"):
    print("Exporting Denoiser UNet to ONNX...")
    enhancer = load_enhancer(None, device)
    denoiser = enhancer.denoiser
    unet = denoiser.net
    unet.eval()
    remove_weight_norm_recursively(unet)
    
    # Input: (b, 3, f, t)
    # f is n_fft // 2 + 1. For hop_size 512, n_fft is 2048, so f is 1025.
    f_bins = denoiser.stft_cfg["n_fft"] // 2 + 1
    dummy_input = torch.randn(1, 3, f_bins, 100)
    
    output_path = output_dir / "denoiser_unet.onnx"
    torch.onnx.export(
        unet,
        dummy_input,
        str(output_path),
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={"input": {3: "time"}, "output": {3: "time"}},
        opset_version=17
    )
    print(f"Denoiser UNet exported to {output_path}")

def export_enhancer_vocoder(output_dir, device="cpu"):
    print("Exporting Enhancer Vocoder (UnivNet) to ONNX...")
    enhancer = load_enhancer(None, device)
    vocoder = enhancer.vocoder
    vocoder.eval()
    remove_weight_norm_recursively(vocoder)
    
    # Input: (b, c, t)
    # c is vocoder_input_dim (usually num_mels + extra)
    hp = enhancer.hp
    input_dim = hp.num_mels + hp.vocoder_extra_dim
    dummy_input = torch.randn(1, input_dim, 100)
    
    class UnivNetWrapper(nn.Module):
        def __init__(self, vocoder):
            super().__init__()
            self.vocoder = vocoder
        def forward(self, x):
            return self.vocoder(x)
            
    print("Exporting Vocoder to ONNX...")
    wrapper = UnivNetWrapper(vocoder)
    # Use a small dummy input for faster tracing, but with dynamic axes
    dummy_input = torch.randn(1, input_dim, 8) 
    
    output_path = output_dir / "enhancer_vocoder.onnx"
    torch.onnx.export(
        wrapper,
        (dummy_input,),
        str(output_path),
        input_names=["mel"],
        output_names=["waveform"],
        dynamic_axes={"mel": {2: "time"}, "waveform": {1: "time"}},
        opset_version=17,
        do_constant_folding=True
    )
    print(f"Vocoder exported to {output_path}")

def export_enhancer_encoder(output_dir, device="cpu"):
    print("Exporting IRMAE Encoder to ONNX...")
    enhancer = load_enhancer(None, device)
    encoder = enhancer.lcfm.ae.encoder
    encoder.eval()
    remove_weight_norm_recursively(encoder)
    
    dummy_input = torch.randn(1, enhancer.hp.num_mels, 100)
    output_path = output_dir / "enhancer_encoder.onnx"
    torch.onnx.export(
        encoder,
        dummy_input,
        str(output_path),
        input_names=["mel"],
        output_names=["latent"],
        dynamic_axes={"mel": {2: "time"}, "latent": {2: "time"}},
        opset_version=17
    )
    print(f"Encoder exported to {output_path}")

def export_enhancer_decoder(output_dir, device="cpu"):
    print("Exporting IRMAE Decoder to ONNX...")
    enhancer = load_enhancer(None, device)
    decoder = enhancer.lcfm.ae.decoder
    decoder.eval()
    remove_weight_norm_recursively(decoder)
    
    dummy_input = torch.randn(1, enhancer.hp.lcfm_latent_dim, 100)
    output_path = output_dir / "enhancer_decoder.onnx"
    torch.onnx.export(
        decoder,
        dummy_input,
        str(output_path),
        input_names=["latent"],
        output_names=["mel"],
        dynamic_axes={"latent": {2: "time"}, "mel": {2: "time"}},
        opset_version=17
    )
    print(f"Decoder exported to {output_path}")

def export_cfm_wn(output_dir, device="cpu"):
    print("Exporting CFM WN to ONNX...")
    enhancer = load_enhancer(None, device)
    cfm_net = enhancer.lcfm.cfm.net
    cfm_net.eval()
    remove_weight_norm_recursively(cfm_net)
    
    # forward(self, x, l=None, g=None)
    # x: ψt (b, latent_dim, t)
    # l: cond (b, mel_dim, t)
    # g: time_emb (b, emb_dim)
    latent_dim = enhancer.hp.lcfm_latent_dim
    mel_dim = enhancer.hp.num_mels
    emb_dim = enhancer.lcfm.cfm.time_emb_dim
    
    dummy_x = torch.randn(1, latent_dim, 100)
    dummy_l = torch.randn(1, mel_dim, 100)
    dummy_g = torch.randn(1, emb_dim)
    
    output_path = output_dir / "cfm_wn.onnx"
    torch.onnx.export(
        cfm_net,
        (dummy_x, dummy_l, dummy_g),
        str(output_path),
        input_names=["psi_t", "cond", "time_emb"],
        output_names=["velocity"],
        dynamic_axes={
            "psi_t": {2: "time"},
            "cond": {2: "time"},
            "velocity": {2: "time"}
        },
        opset_version=14
    )
    print(f"CFM WN exported to {output_path}")

if __name__ == "__main__":
    onnx_dir = Path("models_onnx")
    onnx_dir.mkdir(exist_ok=True)
    
    device = "cpu"
    
    tasks = [
        ("Denoiser UNet", export_denoiser_unet),
        ("Vocoder", export_enhancer_vocoder),
        ("Encoder", export_enhancer_encoder),
        ("Decoder", export_enhancer_decoder),
        ("CFM WN", export_cfm_wn),
    ]
    
    for name, func in tasks:
        try:
            func(onnx_dir, device)
        except Exception as e:
            print(f"Failed to export {name}: {e}")
    
    # Save some metadata/hparams for C++ implementation
    try:
        enhancer = load_enhancer(None, "cpu")
        hp = enhancer.hp
        with open(onnx_dir / "hparams.txt", "w") as f:
            f.write(f"wav_rate: {hp.wav_rate}\n")
            f.write(f"num_mels: {hp.num_mels}\n")
            f.write(f"hop_size: {hp.hop_size}\n")
            f.write(f"win_size: {hp.win_size}\n")
            f.write(f"n_fft: {hp.n_fft}\n")
            f.write(f"latent_dim: {hp.lcfm_latent_dim}\n")
            f.write(f"z_scale: {enhancer.lcfm.z_scale}\n")
            f.write(f"stft_magnitude_min: {hp.stft_magnitude_min}\n")
        print("HParams saved to hparams.txt")
    except Exception as e:
        print(f"Failed to save hparams: {e}")
