FROM ubuntu:bionic as intermediate

WORKDIR /NFSmartPlayerEnv

# copy the bare minimum to get linux.sh to execute
COPY ./ci ./ci
COPY ./tools ./tools
RUN ./ci/linux.sh --help

# Rely on the copy that will be VOLUME-ed in at runtime
RUN rm -rf ./ci ./tools

ENV CC=clang
ENV CXX=clang++

# https://pythonspeed.com/articles/activate-virtualenv-dockerfile/
ENV VIRTUAL_ENV=/NFSmartPlayerEnv/smartplayer_env
ENV PATH="$VIRTUAL_ENV/bin:$PATH"