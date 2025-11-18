cd dir-containing-dockerfile
sudo docker build -t downloader:latest .
sudo docker run -v /volume1/docker/bot_shared/:/home/apprunner/bot_shared -d --name downloader downloader:latest