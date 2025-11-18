FROM ubuntu:24.04

RUN echo 'APT::Install-Suggests "0";' >> /etc/apt/apt.conf.d/00-docker
RUN echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get update \
  && apt-get install -y nano git cmake build-essential qt6-base-dev ffmpeg wget ca-certificates python3 python3-pip python3-venv zip make pandoc python3-pytest \
  && rm -rf /var/lib/apt/lists/*

# we create a user with a certain uid (1030) to match in the mounted volume
RUN useradd --create-home -g 100 -u 1030 apprunner

USER apprunner

WORKDIR /home/apprunner/

RUN git clone https://github.com/crystalidea/telegram-media-downloader-bot

WORKDIR /home/apprunner/telegram-media-downloader-bot

ENTRYPOINT ["/home/apprunner/telegram-media-downloader-bot/_build.sh"]