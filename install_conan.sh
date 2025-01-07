#!/usr/bin/env bash

for type in Debug Release RelWithDebInfo; do
	conan install .  -s build_type=${type} -c tools.system.package_manager:mode=install -c tools.system.package_manager:sudo=true --build=missing || {
		echo "failed to install required packages"
		exit 1
	}
done
