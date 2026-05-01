import torch
import pathlib
pathlib.PosixPath = pathlib.WindowsPath
from resemble_enhance.enhancer.inference import load_enhancer
import time

def test_vocoder():
    print("Loading enhancer...")
    enhancer = load_enhancer(None, "cpu")
    vocoder = enhancer.vocoder
    
    hp = enhancer.hp
    input_dim = hp.num_mels + hp.vocoder_extra_dim
    dummy_input = torch.randn(1, input_dim, 100)
    
    print("Running vocoder forward...")
    start = time.time()
    with torch.no_grad():
        out = vocoder(dummy_input)
    end = time.time()
    print(f"Forward took {end - start:.2f} seconds")
    print(f"Output shape: {out.shape}")

if __name__ == "__main__":
    test_vocoder()
