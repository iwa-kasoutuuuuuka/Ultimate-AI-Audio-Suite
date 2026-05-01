import sys
from pathlib import Path

# Add src to path to find main_gui and other modules
src_path = Path(__file__).parent / "src"
sys.path.append(str(src_path))

try:
    from main_gui import ResembleEnhanceApp
except ImportError as e:
    print(f"Error: Could not find required modules. Make sure you are running from the project root. {e}")
    sys.exit(1)

if __name__ == "__main__":
    app = ResembleEnhanceApp()
    app.mainloop()
