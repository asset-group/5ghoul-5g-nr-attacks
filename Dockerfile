FROM ubuntu:18.04 as base

ENV HOME /home/user
ENV TERM xterm-256color

WORKDIR $HOME

# Fix timezone for tzdata
RUN ln -snf /usr/share/zoneinfo/$CONTAINER_TIMEZONE /etc/localtime && echo $CONTAINER_TIMEZONE > /etc/timezone

# Default system packages
RUN apt-get update && apt-get install sudo x11-xserver-utils git -y

# Create default non-root user
RUN groupadd -r user && useradd -m -r -g user user && chown user:user /home/user -R  && echo "user ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Workaround for CVE-2022-24765
RUN git config --system --add safe.directory '*'

USER user

# Copy all relevant project files
COPY --chown=user:user . $HOME/wdissector

# Install development requirements
RUN --mount=type=secret,id=cred,mode=0666 /bin/bash -c "source /run/secrets/cred && cd $HOME/wdissector && \
	./requirements.sh dev && ./requirements.sh 3gpp && mkdir -p modules/ && cd modules && \
	wget https://github.com/indygreg/python-build-standalone/releases/download/20230116/cpython-3.8.16+20230116-$(uname -m)-unknown-linux-gnu-lto-full.tar.zst \
	-O python.tar.zst && tar -I zstd -xf python.tar.zst && cd ../ && \
	wget https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py \
	-O get-platformio.py && ./modules/python/install/bin/python3 get-platformio.py && \
	ln -sf modules/python/install/bin/python3 /$HOME/.platformio/penv/bin/python3 && \
	./modules/python/install/bin/python3 src/drivers/firmware_bluetooth/firmware.py build || true && \
	sudo rm ./* -rdf && \
	sudo rm -rf \
            ~/.git-credentials \
            /tmp/* \
            /var/lib/apt/lists/* \
            /var/tmp/*"

ENV PATH $PATH:$HOME/.platformio/penv/bin

CMD sleep infinity
