@echo off

FOR /f "tokens=*" %%i IN ('docker ps -aq') DO docker rm %%i
FOR /f "tokens=*" %%i IN ('docker images --format "{{.ID}}"') DO docker rmi %%i

docker build -t downloader .
docker run --name xxx -v bot_shared:/home/apprunner/bot_shared -d downloader

pause