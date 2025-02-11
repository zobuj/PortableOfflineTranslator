FROM python:3.9-slim

WORKDIR /app

RUN apt update
RUN apt install sudo
RUN sudo apt install ffmpeg
RUN pip install -U openai-whisper


CMD ["/bin/bash"]