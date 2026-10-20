FROM emscripten/emsdk:6.0.12

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ninja-build ccache \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -m 777 /ccache \
    && cp -a /emsdk/upstream/emscripten/cache /emcache \
    && chmod -R a+rwX /emcache
ENV CCACHE_DIR=/ccache
ENV EM_CACHE=/emcache

WORKDIR /src
