#!/usr/bin/env bash

set -eo pipefail

if  [[ ! -v CONTAINER_REPO ]]
then
	CONTAINER_REPO=docker.io/megarbelini/5ghoul
	# echo "CONTAINER_REPO not set. Using default: $CONTAINER_REPO"
else
	echo "CONTAINER_REPO set to: $CONTAINER_REPO"
fi

CONTAINER_NAME=5ghoul
ARCH=$(uname -m)

export DOCKER_BUILDKIT=1
export BUILDKIT_PROGRESS=plain
export BUILDKIT_INLINE_CACHE=1
export PROGRESS_NO_TRUNC=1

# Configure deploy token
if [[ -v CI_DEPLOY_USER ]]
then
    echo "Configuring credentials for CI/CD..."
    git config --global credential.helper store
    echo "https://$CI_DEPLOY_USER:$CI_DEPLOY_PASSWORD@gitlab.com" > ~/.git-credentials
fi


run_in_user(){
	GID_=$(id -g $USER)
	UID_=$(id -u $USER)

	# Receve command as arguments
	CMD_ARG=$1
	shift
	CMD_FULL="$CMD_ARG $@"
	echo $CMD_FULL

	mkdir -p /home/$USER &> /dev/null
	mkdir -p $PROJ_FOLDER
	chown $USER /home/$USER
	rm -d $PROJ_FOLDER
	ln -s /home/user $PROJ_FOLDER
	addgroup --gid $GID_ $USER &> /dev/null
	useradd --home /home/$USER --gid $GID_ --uid $UID_  $USER &> /dev/null
	cd $PROJ_FOLDER

	# Make sure /usr/bin/bash exists
	ln -s /bin/bash /usr/bin/bash &> /dev/null

	# Start shell or execute command as user
	if [ -z "$CMD_ARG" ]
	then
	        su $USER
	else
	        echo "CMake at $(which cmake)"
	        su $USER -c "$CMD_FULL"
	fi
}

gen_scripts(){
	mkdir -p scripts
	echo -e "#!/usr/bin/env bash\n $(type run_in_user | sed '1,3d;$d')" > ./scripts/docker_change_to_user.sh
	chmod +x ./scripts/docker_change_to_user.sh
}


copy_files_to_host(){
	# Copy config files from container to host on first startup
	if [[ ! -d configs ]] || [[ ! -d logs ]] || [[ ! -d modules ]]
	then
		echo "Copying runtime config. files to host..."
		sudo docker create --name=${CONTAINER_NAME}-$1 ${CONTAINER_REPO}:$1-$ARCH
		mkdir -p configs
		mkdir -p modules/exploits
		mkdir -p modules/reportsender
		mkdir -p logs
		sudo docker cp ${CONTAINER_NAME}-$1:/home/user/wdissector/container.sh ./container.sh
		sudo docker cp ${CONTAINER_NAME}-$1:/home/user/wdissector/configs .
		sudo docker cp ${CONTAINER_NAME}-$1:/home/user/wdissector/modules/exploits modules
		sudo docker cp ${CONTAINER_NAME}-$1:/home/user/wdissector/modules/reportsender modules
		sudo chown $USER:$USER . -R
		./container.sh stop $1
		echo "Done!"
	fi
}


start_container_dev(){
	sudo docker rm ${CONTAINER_NAME}-dev &> /dev/null
	sudo touch /home/root/.Xauthority # ensure we have xauthority file	
	sudo xhost local:root &> /dev/null # allow xhost on host root
	sudo docker run -ti -d --privileged --name ${CONTAINER_NAME}-dev \
	-e USER=$USER -e PROJ_FOLDER=$(pwd) \
	-e DISPLAY=$DISPLAY \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	-v /home/$USER/.Xauthority:/home/user/.Xauthority \
	-v /home/$USER/.Xauthority:/root/.Xauthority \
	-v /etc/passwd:/etc/passwd \
	-v /etc/shadow:/etc/shadow \
	-v /etc/sudoers:/etc/sudoers \
	--device-cgroup-rule='c 188:* rmw' \
	-v /dev/bus:/dev/bus:ro -v /dev/serial:/dev/serial:ro \
	-v /run/udev:/run/udev:ro \
	--user root \
	--entrypoint="/sbin/init" \
	--mount type=bind,source="$(pwd)"/,target=/home/user ${CONTAINER_REPO}:dev-$ARCH &> /dev/null

	gen_scripts
}


start_container_release(){
	copy_files_to_host $1

	sudo docker rm ${CONTAINER_NAME}-$1 &> /dev/null
	touch /home/$USER/.Xauthority # ensure we have xauthority file	
	sudo xhost local:root &> /dev/null # allow xhost on host root
	mkdir -p $(pwd)/logs
	sudo docker run -ti -d --privileged --name ${CONTAINER_NAME}-$1 \
	-e DISPLAY=$DISPLAY \
	--network=host \
	--user=root \
	--entrypoint="/sbin/init" \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	-v /home/$USER/.Xauthority:/home/user/.Xauthority \
	-v /home/$USER/.Xauthority:/root/.Xauthority \
	-v /run/udev:/run/udev:ro \
	-v /dev:/dev \
	-v $(pwd)/logs:/home/user/wdissector/logs \
	-v $(pwd)/configs:/home/user/wdissector/configs \
	-v $(pwd)/modules/exploits:/home/user/wdissector/modules/exploits \
	-v $(pwd)/modules/reportsender:/home/user/wdissector/modules/reportsender \
	${CONTAINER_REPO}:$1-$ARCH &> /dev/null
}

check_install_requirements(){
	# Make sure docker is installed
	if [[ -z $(which docker) ]]
	then
		echo "Docker not found, installing now..."
		./container.sh requirements
	else
		echo "Docker already installed."
	fi
}


if [ "$1" == "build" ]
then
	if [ "$2" == "dev" ]
	then
		TAG_NAME=${CONTAINER_REPO}:dev-$ARCH
		sudo docker build --progress=plain --compress -t $TAG_NAME --secret id=cred,src=.env \
		--secret id=my_env,src=.env .

	elif [ "$2" == "release" ]
	then
		TAG_NAME=${CONTAINER_REPO}:release-$ARCH
		sudo docker build --progress=plain --compress -t $TAG_NAME --secret id=cred,src=.env \
		-f scripts/docker_release.docker .

	elif [ "$2" == "release-desktop" ]
	then
		TAG_NAME=${CONTAINER_REPO}:release-desktop-$ARCH
		sudo docker build --progress=plain --compress -t $TAG_NAME --secret id=cred,src=.env \
		-f scripts/docker_release_desktop.docker .

	elif [ "$2" == "release-4g" ]
	then
		TAG_NAME=${CONTAINER_REPO}:release-4g-$ARCH
		# Clone 3rd-party tools and apply patches
		./scripts/apply_patches.sh 3rd-party
		sudo docker build --progress=plain --compress -t $TAG_NAME --secret id=cred,src=.env \
		--build-arg ARCH=$(uname -m) \
		-f scripts/docker_release_4g.docker .

	elif [ "$2" == "release-5g" ]
	then
		TAG_NAME=${CONTAINER_REPO}:release-5g-$ARCH
		# Clone 3rd-party tools and apply patches
		./scripts/apply_patches.sh 3rd-party
		sudo docker build --progress=plain --compress -t $TAG_NAME --secret id=cred,src=.env \
		--build-arg ARCH=$(uname -m) \
		-f scripts/docker_release_5g.docker .

	else
		echo "Missing build argument. use ./container.sh build <container tag name> [export]"
	fi

	if [ "$3" == "export" ]
	then
		TAG_NAME=${CONTAINER_REPO}:$4-$ARCH
		IMG_NAME=${CONTAINER_NAME}-$4-$ARCH
		mkdir -p release
		sudo docker image save $TAG_NAME | gzip -9 -c > release/$IMG_NAME.tar.gz
		chmod a+rw release/$IMG_NAME.tar.gz
		echo "Image release/$IMG_NAME.tar.gz created!"
	fi

elif [ "$1" == "stop" ]
then
	if [[ ! -z "$2" ]]
	then
		sudo docker rm --force ${CONTAINER_NAME}-$2
	fi

elif [ "$1" == "compile" ]
then
	start_container
	sudo docker exec ${CONTAINER_NAME}-dev scripts/docker_change_to_user.sh ./build.sh all # Compile library

elif [ "$1" == "pull" ]
then
	
	check_install_requirements

	if [[ ! -z "$2" ]]
	then
		sudo docker pull ${CONTAINER_REPO}:$2-$ARCH
		sudo docker system prune -f
	else
		echo "Missing push argument. use ./container.sh pull <container tag name>"
	fi

elif [ "$1" == "update" ]
then
	if [[ ! -z "$2" ]]
	then
		check_install_requirements
		echo "Removing config files"
		sudo rm configs host modules -rdf
		echo "Pulling latest image for $2-$ARCH"
		sudo docker pull ${CONTAINER_REPO}:$2-$ARCH
		echo "Stopping current image"
		./container.sh stop $2 &>/dev/null || true
		echo "Cleaning up old images"
		sudo docker system prune -f
		copy_files_to_host $2
		echo "Update finished"
	else
		echo "Missing push argument. use ./container.sh update <container tag name>"
	fi


elif [ "$1" == "push" ]
then
	if [[ ! -z "$2" ]]
	then
		mkdir -p .docker
		OLD_BASE=$(crane digest ${CONTAINER_REPO}:$2-$ARCH) || true
		if [[ -v OLD_BASE ]]
		then
			echo $OLD_BASE > ".docker/$2-$ARCH"
		fi

		sudo docker push ${CONTAINER_REPO}:$2-$ARCH

		if [[ -z $OLD_BASE ]]
		then
			OLD_BASE=$(crane digest ${CONTAINER_REPO}:$2-$ARCH) || true
			if [[ -v OLD_BASE ]]
			then
				echo $OLD_BASE > ".docker/$2-$ARCH"
			fi
		fi

	else
		echo "Missing push argument. use ./container.sh push <container tag name>"
		exit 1
	fi

elif [ "$1" == "run" ]
then

	if [[ -z $2 ]]
	then
		echo "Missing run argument. use ./container.sh run <container tag name>"
		exit 1
	fi

	check_install_requirements

	# Check if image exists and pull it if not found
	IMG_EXISTS="$(sudo docker inspect ${CONTAINER_REPO}:$2-$ARCH)" || true
	if [[ $IMG_EXISTS == "[]" ]]
	then
		echo "Image not found, pulling now..."
		./container.sh pull $2
	fi

	if [[ "$2" == "release"* ]]
	then
		echo "Starting $2"
		start_container_release $2 || true


		if [[ "$2" == "release-5g" || "$2" == "release-4g" ]]
		then
			# Regenerate udev rules in container
			if [[ ! -d host/${CONTAINER_NAME}-$2 ]]
			then
				echo "Regenerating udev rules..."
				sudo docker exec -ti --user=root ${CONTAINER_NAME}-$2 bash -c 'cd 3rd-party/ModemManager && ./build.sh &> /dev/null'
			fi

			echo "Starting MongoDB"
			# Start mongodb for 4g/5g releases
			sudo docker exec -ti --user=root ${CONTAINER_NAME}-$2 systemctl enable mongod --now || true
		fi

		# Install udev rules in the host if not installed before
		if [[ ! -d host/${CONTAINER_NAME}-$2 ]]
		then
			mkdir -p host/${CONTAINER_NAME}-$2
			echo "Installing udev rules to host"
			sudo docker cp ${CONTAINER_NAME}-$2:/lib/udev/rules.d host/${CONTAINER_NAME}-$2 &> /dev/null || true
			sudo chown $USER:$USER host -R
			sudo cp -rnv host/${CONTAINER_NAME}-$2/rules.d/* /lib/udev/rules.d &> /dev/null || true
			echo "Reloading udev daemon on the host"
			sudo udevadm control --reload-rules && sudo udevadm trigger
		fi

		# Run container terminal
		if [[ -z "$3" ]]
		then
			echo "Starting Container"
			sudo docker exec -ti --user=user ${CONTAINER_NAME}-$2 bash
			exit 0
		fi

		C_NAME=$2
		# Run container user provided input
		shift
		shift
		echo "Starting Container"
		sudo sudo docker exec -ti --user=user ${CONTAINER_NAME}-${C_NAME} $@

	# Development container
	elif [[ "$2" == "dev"  ]]
	then
		start_container || true
		sudo docker exec -ti ${CONTAINER_NAME}-dev scripts/docker_change_to_user.sh # Start container with bash and mount files
	fi

elif [ "$1" == "load" ]
then
	echo "Loading docker image"
	sudo docker load --input $2

elif [ "$1" == "clean" ]
then
	sudo docker rm --force ${CONTAINER_NAME}-$2
	sudo docker rmi -f ${CONTAINER_REPO}:$2-$ARCH # Remove container image
	sudo docker system prune --force

elif [ "$1" == "rebase" ]
then
	shift
	OLD_BASE=$(cat .docker/$1-$ARCH) || true
	if [[ -v OLD_BASE ]]
	then
		echo "Old Base:$OLD_BASE"
		crane rebase $CONTAINER_REPO:$2-$ARCH \
		  --old_base=$CONTAINER_REPO@$OLD_BASE \
		  --new_base=$CONTAINER_REPO:$1-$ARCH
	else
		echo "Old image of $CONTAINER_REPO:$1-$ARCH not found, create .docker_release file with $1-$ARCH base"
	fi

elif [[ "$1" == "requirements" || "$1" == "requirements-dev" ]]
then
	# Install Docker to run containers
	if [[ -z "$(which docker)" ]]
	then
		sudo apt-get install -y ca-certificates curl gnupg
		# Add Docker’s official GPG key
		sudo install -m 0755 -d /etc/apt/keyrings
		curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
		sudo chmod a+r /etc/apt/keyrings/docker.gpg
		echo \
		"deb [arch="$(dpkg --print-architecture)" signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
		"$(. /etc/os-release && echo "$VERSION_CODENAME")" stable" | \
		sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
		sudo apt-get update
		sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
		echo "Docker installed!"
	else
		echo "Docker already installed."
	fi

	# Install Docker to build container
	if [ "$1" == "requirements-dev" ]
	then
		# Install crane for container image rebasing
		sudo apt install -y ca-certificates gnupg curl jq
		VERSION=$(curl -s "https://api.github.com/repos/google/go-containerregistry/releases/latest" | jq -r '.tag_name')
		OS=Linux
		ARCH=$(uname -m)
		if [ "$ARCH" == "aarch64" ]
		then
			ARCH="arm64"
		fi
		curl -sL "https://github.com/google/go-containerregistry/releases/download/${VERSION}/go-containerregistry_${OS}_${ARCH}.tar.gz" > go-containerregistry.tar.gz
		sudo tar -zxvf go-containerregistry.tar.gz -C /usr/local/bin/ crane
	fi

else
	echo "-------------- HELP ------------------"
	echo "---------  USER Commands -------------"
	echo "./container.sh requirements           	 - Install system requirements to run container via docker"
	echo "./container.sh pull release            	 - Push $CONTAINER_NAME:release to $CONTAINER_REPO:release"
	echo "./container.sh pull release-4g             - Push $CONTAINER_NAME:release-4g to $CONTAINER_REPO:release-4g"
	echo "./container.sh pull release-5g             - Push $CONTAINER_NAME:release-4g to $CONTAINER_REPO:release-5g"
	echo "./container.sh run <release name>          - Start docker container shell"
	echo "./container.sh stop <release name>         - Stop docker container"
	echo "./container.sh update <release name>       - Update docker container (remove copied config files to host)"
	echo "./container.sh clean <release name>        - Stop and erase container and image"
	echo "---------  Dev. Commands -------------"
	echo "./container.sh dev-requirements            - Install system requirements to build container via docker"
	echo "./container.sh compile                     - Build project via container in current folder"
	echo "./container.sh build dev                   - Build development docker container"
	echo "./container.sh build release               - Build docker container and create compressed image for release"
	echo "./container.sh build release-4g            - Build docker container and create compressed image for release-4g"
	echo "./container.sh build release-5g            - Build docker container and create compressed image for release-5g"
	echo "./container.sh push  release            	 - Push $CONTAINER_NAME:release to $CONTAINER_REPO:release"
	echo "./container.sh push  release-4g            - Push $CONTAINER_NAME:release-4g to $CONTAINER_REPO:release-4g"
	echo "./container.sh push  release-5g            - Push $CONTAINER_NAME:release-4g to $CONTAINER_REPO:release-5g"
	echo "./container.sh load <image path>           - Load $CONTAINER_NAME docker image (.tar.gz)"
fi

# To add nvidia driver to container
# sudo add-apt-repository ppa:graphics-drivers/ppa
# sudo apt install nvidia-driver-450 -y
