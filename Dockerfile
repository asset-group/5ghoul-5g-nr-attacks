FROM ubuntu:26.04 AS base
ENV HOME=/home/user TERM=xterm-256color
RUN apt-get update && apt-get install -y --no-install-recommends sudo git ca-certificates && \
    useradd -m -u 1000 user && echo 'user ALL=(ALL) NOPASSWD: ALL' > /etc/sudoers.d/user
USER user
WORKDIR /home/user/wdissector
COPY --chown=user:user . .
RUN ./requirements.sh
ENV PATH="/home/user/miniforge3/envs/5ghoul23/bin:${PATH}"
CMD ["sleep", "infinity"]
