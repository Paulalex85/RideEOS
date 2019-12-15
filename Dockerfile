FROM ubuntu:18.04

RUN echo "INSTALLING EOSIO AND CDT"
RUN apt-get update && apt-get install -y wget sudo curl npm
RUN wget https://github.com/EOSIO/eosio.cdt/releases/download/v1.6.2/eosio.cdt_1.6.2-1-ubuntu-18.04_amd64.deb
RUN apt-get update && sudo apt install -y ./eosio.cdt_1.6.2-1-ubuntu-18.04_amd64.deb
RUN wget https://github.com/EOSIO/eos/releases/download/v1.8.6/eosio_1.8.6-1-ubuntu-18.04_amd64.deb
RUN apt-get update && sudo apt install -y ./eosio_1.8.6-1-ubuntu-18.04_amd64.deb

RUN echo "INSTALLING CONTRACTS"
RUN mkdir -p "/opt/eosio/bin/contracts"

RUN echo "INSTALLING EOSIO.CONTRACTS"
RUN wget https://github.com/EOSIO/eosio.contracts/archive/v1.7.0.tar.gz
RUN mkdir -p /eosio.contracts
RUN tar xvzf ./v1.7.0.tar.gz -C /eosio.contracts
RUN mv /eosio.contracts/eosio.contracts-1.7.0 /opt/eosio/bin/contracts
RUN mv /opt/eosio/bin/contracts/eosio.contracts-1.7.0 /opt/eosio/bin/contracts/eosio.contracts

RUN echo "INSTALLING EOSIO.ASSERT CONTRACT"
RUN wget https://github.com/EOSIO/eosio.assert/archive/v0.1.0.tar.gz
RUN mkdir -p /eosio.assert
RUN tar xvzf ./v0.1.0.tar.gz -C /eosio.assert
RUN mv /eosio.assert/eosio.assert-0.1.0 /opt/eosio/bin/contracts
RUN mv /opt/eosio/bin/contracts/eosio.assert-0.1.0 /opt/eosio/bin/contracts/eosio.assert