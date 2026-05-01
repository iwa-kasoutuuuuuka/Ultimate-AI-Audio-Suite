import torch
from resemble_enhance.enhancer.lcfm.wn import WN
import torch.nn as nn

try:
    model = WN(input_dim=64, output_dim=64)
    x = torch.randn(1, 64, 100)
    out = model(x)
    print("Success: WN model forward pass works!")
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
