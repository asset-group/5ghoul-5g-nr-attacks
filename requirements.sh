#!/usr/bin/env bash

set -eo pipefail

CLANG_VERSION=15
ARCH="$(uname -m)"
KERNEL="$(uname -r)"

# Configure deploy token
if [[ -v CI_DEPLOY_USER ]]
then
    echo "Configuring credentials for CI/CD..."
    git config --global credential.helper store
    echo "https://$CI_DEPLOY_USER:$CI_DEPLOY_PASSWORD@gitlab.com" > ~/.git-credentials
fi

if [ "$1" == "dev" ]
then
	# Make sure that .config is used
	git config --local include.path ../.gitconfig || true
	# Ubuntu dev. requirements
	sudo apt install software-properties-common kmod bc gzip curl git wget zstd python3-dev flex bison pkg-config swig graphviz libglib2.0-dev libgcrypt-dev libnl-genl-3-200 \
	libgraphviz-dev liblz4-dev libsnappy-dev libgnutls28-dev libxml2-dev libnghttp2-dev libkrb5-dev libnl-3-dev libspandsp-dev libxrandr-dev libxinerama-dev libxcursor-dev \
	libxi-dev libnl-genl-3-dev libnl-route-3-dev libnl-nf-3-dev libcap-dev libbrotli-dev libsmi2-dev liblua5.2-dev libc-ares-dev libsbc-dev libspeexdsp-dev libfreetype6-dev libxss1 \
	libtbb-dev libnss3-dev libudev-dev libpulse-dev libpcre2-dev libasound2-dev libgl1-mesa-dev libssh-dev libmaxminddb-dev libopus-dev libusb-1.0-0 psmisc sshpass tcpdump -y
	# folly requirements
	sudo apt install libgoogle-glog-dev libzstd-dev libevent-dev libunwind-dev libdouble-conversion-dev libgflags-dev -y

	# Install clang-15 for Ubuntu
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y # add missing libgcc-s1 on Ubuntu18.04
	sudo mkdir -p /usr/lib/python3.6/dist-packages/
	sudo ./scripts/install_llvm.sh $CLANG_VERSION
	CLANG_VERSION_FULL=$(clang-$CLANG_VERSION --version | grep -o -i -E "([0-9]+.){2}[0-9]")
	# Fix lldb-15 on ubuntu18.04
	sudo ln -sf /usr/lib/llvm-$CLANG_VERSION/lib/python3.6/site-packages/lldb /usr/lib/python3.6/dist-packages/lldb || true

	# Install gcc-9 headers and libraries for use with Clang 15
	sudo apt install gcc-9 g++-9 libstdc++-9-dev -y

	# Install qt 5.12.2 on x86 arch
	if [ $ARCH == "x86_64" ]
	then
		sudo add-apt-repository ppa:beineri/opt-qt-5.12.2-bionic -y
		sudo apt install qt512base qt512tools qt512svg qt512multimedia mesa-common-dev -y
	fi

	# Install nodejs
	if ! which node > /dev/null;
	then
		echo "nodejs not found, installing now..."
		curl -sL https://deb.nodesource.com/setup_16.x | sudo -E bash -
		sudo apt update
		sudo apt install nodejs -y
	else
		echo "nodejs found!"
	fi

	# Install quicktype
	if ! which quicktype > /dev/null; 
	then
	echo "quicktype not found, installing now..."
	sudo npm install -g quicktype || echo -e "\nError. Make sure to install nodejs"
	else
	echo "quicktype found!"
	fi

	# Install python pip
	if ! which pip3 > /dev/null;
	then
	echo "pip3 not found, installing now..."
	wget https://bootstrap.pypa.io/pip/3.6/get-pip.py
	sudo python3 get-pip.py
	rm get-pip.py
	else
	echo "pip3 found!"
	fi

	# Python3 packages
	sudo python3 -m pip install \
	cmake==3.24.1 -U \
	ninja==1.11.1 -U \
	meson==0.53.0 --use-feature=2020-resolver

elif [ "$1" == "doc" ]
then
	sudo apt install python build-essential fonts-noto-color-emoji -y

	# Install nodejs
	if ! which node > /dev/null;
	then
		echo "nodejs not found, installing now..."
		curl -sL https://deb.nodesource.com/setup_16.x | sudo -E bash -
		sudo apt update
		sudo apt install nodejs -y
	else
		echo "nodejs found!"
	fi
	# Install local nodejs documentation requirements
	npm install
	cd docs/old_greyhound/
	npm install
	cd ../../

elif [ "$1" == "3gpp" ]
then
	if [ ! -f ".3gpp" ] # 
	then
		# Install basic runtime requirements
		./requirements.sh

		# Install extra requirements
		sudo apt install -y wget xxd libc-bin

		# Clone 3rd-party tools and apply patches
		./scripts/apply_patches.sh 3rd-party

		# Install python pip
		if ! which pip3 > /dev/null;
		then
			echo "pip3 not found, installing now..."
			wget https://bootstrap.pypa.io/pip/3.6/get-pip.py
			sudo python3 get-pip.py
			rm get-pip.py
		else
			echo "pip3 found!"
		fi

		# Python3 packages
		sudo python3 -m pip install \
		cmake==3.24.1 -U \
		ninja==1.11.1 -U \
		meson==0.53.0 --use-feature=2020-resolver || true

		sudo python3 -m pip install \
		cmake==3.24.1 -U \
		ninja==1.11.1 -U \
		meson==0.53.0 || true

		# Install ModemManager requirements
		cd 3rd-party/ModemManager/
		./requirements.sh
		./build.sh
		cd ../../
		cd 3rd-party/open5gs-core/
		./requirements.sh
		./build.sh
		cd ../

		# Build and install misc requirements (TODO: remove)
		# libzmq
		git clone https://github.com/zeromq/libzmq.git --depth=1 || true
		cd libzmq
		./autogen.sh
		./configure
		make -j
		sudo make install
		sudo ldconfig
		cd ../

		# libczmq
		git clone https://github.com/zeromq/czmq.git --depth=1 || true
		cd czmq
		./autogen.sh
		./configure
		make -j
		sudo make install
		sudo ldconfig
		cd ../

		# libjson-c
		git clone https://github.com/json-c/json-c.git --depth=1 || true
		cd json-c
		mkdir -p build
		cd build
		cmake ../
		make -j
		sudo make install
		sudo ldconfig

		cd ../../../
		touch .3gpp
	fi
	echo -e "3GPP requirements built and installed"

elif [ "$1" == "4g" ]
then
	./requirements.sh 3gpp
	cd 3rd-party/oai_4g_lte/openair-ran
	./build_all.sh

elif [ "$1" == "5g" ]
then
	./requirements.sh 3gpp
	cd 3rd-party/oai_5g_sa
	source oaienv
	cd cmake_targets
	./build_oai -I -w USRP --gNB --nrUE --ninja --noavx512
	# TODO: add --noavx512 later

elif [ "$1" == "all" ]
then
	./requirements.sh 3gpp 4g
	./requirements.sh 3gpp 5g
	# TODO

elif [ "$1" == "wifi" ]
then
	# Install basic runtime requirements
	./requirements.sh
	# Wi-Fi requirements
	sudo apt install linux-headers-${KERNEL} -y || true
	sudo apt install dnsmasq net-tools iptables -y
	sudo cp src/drivers/wifi/rtl8812au/85-nm-unmanaged.rules /etc/udev/rules.d/85-nm-unmanaged.rules
	sudo udevadm control --reload-rules && sudo udevadm trigger
	cd src/drivers/wifi/rtl8812au/
	make -j4
	cd ../../../../

elif [ "$1" == "evaluation" ]
then
	# Evaluation packages
	sudo python3 -m pip install numpy pandas python-pcapng==1.0 matplotlib
	sudo apt install expect

else
	# Minimal Ubuntu Packages to run binary WDissector and Wireshark distribution
	sudo apt update
	sudo apt install -y g++ software-properties-common kmod libglib2.0-dev libsnappy1v5 libsmi2ldbl liblua5.2-0 libc-ares2 \
	libnl-route-3-200 libnl-genl-3-200 libfreetype6 graphviz libtbb2 libxss1 libnss3 libspandsp2 libsbc1 libbrotli1 libnghttp2-14 \
	libasound2 psmisc sshpass libpulse0 libasound2 libpcre2-dev libmaxminddb0 libopus0 libspeex1 bc tcpdump libgoogle-glog0v5 \
	libgflags2.2 libzstd1 libunwind8 libcap2 libspeexdsp1 libxtst6 libatk-bridge2.0-0 libusb-1.0-0

	# Install updated c++ glibcxx
	sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
	sudo apt install libstdc++-9-dev -y

	# Install qt 5.12.2 runtime for ubuntu
	if [ "$(lsb_release -sr)" == "18.04" ]
	then
		if [ $ARCH == "x86_64" ]
		then
			sudo add-apt-repository ppa:beineri/opt-qt-5.12.2-bionic -y || true
			sudo apt install qt512base qt512multimedia -y || true
		fi
	else
		# For ubuntu 20.04 and beyound
		if [ $ARCH == "x86_64" ] # Only install qt for x86_64
		then
			sudo apt install qtbase5-dev libqt5multimedia5 -y || true
		fi
		# For ubuntu 22.04 and beyound
		if [  $(echo "$(lsb_release -sr) >= 22.04" | bc ) == "1" ]
		then
			# Install missing libssl1.1.1 for ubuntu 22.04 and beyound
			if [ -z "$(dpkg -l | grep libssl1.1)" ]
			then
				if [ $ARCH == "x86_64" ]
				then
                    wget http://launchpadlibrarian.net/367327833/libssl1.1_1.1.0g-2ubuntu4_amd64.deb
                    sudo apt install -y ./libssl1.1_1.1.0g-2ubuntu4_amd64.deb
                    rm libssl1.1_1.1.0g-2ubuntu4_amd64.deb
				else
                    wget http://launchpadlibrarian.net/367327970/libssl1.1_1.1.0g-2ubuntu4_arm64.deb
                    sudo apt install -y ./libssl1.1_1.1.0g-2ubuntu4_arm64.deb
                    rm libssl1.1_1.1.0g-2ubuntu4_arm64.deb
				fi
			fi
		fi
	fi
fi
