# Etap 1: budowanie aplikacji
FROM alpine:latest AS builder
LABEL org.opencontainers.image.title="weather-server"
LABEL org.opencontainers.image.description="Serwer HTTP w C wyświetlający pogodę"
HEALTHCHECK CMD curl --fail http://localhost:8081 || exit 1

# Zainstaluj kompilator i nagłówki do curl
RUN apk add --no-cache build-base curl-dev

WORKDIR /app
COPY main.c Makefile ./
RUN make

# Etap 2: minimalny runtime z libcurl
FROM alpine:latest
RUN apk add --no-cache libcurl

COPY --from=builder /app/server /server
EXPOSE 8081
ENTRYPOINT ["/server"]