FROM redis:5.0.9

COPY ./entrypoint.sh /usr/local/bin/docker-entrypoint.sh

RUN apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y dnsutils \
  && rm -rf /var/lib/apt/lists/* \
  && chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
