import whisper
import os

os.environ["WHISPER_MODELS_DIR"] = os.path.expanduser("~/.cache/whisper")
model = whisper.load_model("large-v3-turbo")

