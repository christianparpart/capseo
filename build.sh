#! /bin/bash

function die() {
	echo "${*}" 1>&2
	exit 1
}

PREFIX=${PREFIX:-/opt/sandbox}
INSTALL=no
VERBOSE=0

function print_help() {
	echo "syntax: ./build.sh [arguments]"
	echo
	echo " --help               prints this help."
	echo " --prefix PATH        specifies where to install, when ./configure is executed."
	echo " --install            performs install action on successfull built."
	echo " --clean              cleans tree"
	echo " -v, --verbose        increases verbosity."
	echo
}

function clean_tree() {
	echo "Cleaning tree"
	for ABI in $(get_install_abis); do
		rm -rf abi-${ABI}
	done
	./autogen.sh clean
}

function get_install_abis() {
	local ABIS
	case $(uname -m) in
		x86_64)
			if [[ -d /lib32 ]]; then
				ABIS=(amd64 x86)
			else
				ABIS=(amd64) # no multilib-arch
			fi
			;;
		i?86)
			ABIS=(x86)
			;;
		*)
			die "Unsupported ABI"
			;;
	esac
	echo "${ABIS[@]}"
}

function get_libdir() {
	if [[ -n $ABI ]]; then
		case $ABI in
			amd64)
				echo "lib64"
				;;
			x86)
				if $($(get_install_abis) | wc -w) -eq 1; then
					echo "lib"
				else
					echo "lib32"
				fi
				;;
			*)
				echo "lib"
				;;
		esac
	else
		echo "lib"
	fi
}

while test -n "$*"; do
	case $1 in
		--help)
			print_help
			exit 0
			;;
		--prefix)
			PREFIX="$2"
			shift 2
			;;
		-v)
			VERBOSE=$[VERBOSE + 1]
			;;
		--install|install)
			INSTALL=yes
			shift
			;;
		--clean|clean)
			clean_tree
			exit 0
			;;
		*)
			die "Unknown parameter."
			;;
	esac
done

# some envvar backups
O_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"
O_CFLAGS="${CFLAGS:--O0 -ggdb3 -pipe}"
O_CXXFLAGS="${CXXFLAGS:--O0 -ggdb3 -pipe}"
O_LDFLAGS="${LDFLAGS}"

function setup_env() {
	export CFLAGS="${O_CFLAGS} ${ABI_CFLAGS}"
	export CXXFLAGS="${O_CXXFLAGS} ${ABI_CXXFLAGS}"

	case ${ABI} in
		amd64)
			export CFLAGS="${O_CFLAGS}"
			export CXXFLAGS="${O_CXXFLAGS}"
			export LD_LIBRARY_PATH="${O_LD_LIBRARY_PATH}"
			export LDFLAGS="${O_LDFLAGS}"
			;;
		x86)
			export CFLAGS="${O_CFLAGS} -m32"
			export CXXFLAGS="${O_CXXFLAGS} -m32"

			# solves link error when linking src/example program
			export LDFLAGS="-L/usr/lib32" 
			export LD_LIBRARY_PATH="/usr/lib32"
			;;
	esac
}

function get_accel_type() {
	case ${ABI} in
		amd64) echo amd64 ;;
		x86) echo x86 ;;
		*) echo generic ;;
	esac
}

print_env() {
	echo " * ENVIRONMENT FOR ABI: ${ABI}"
	echo " * "
	echo " * CFLAGS=\"$CFLAGS\""
	echo " * CXXFLAGS=\"$CXXFLAGS\""
	echo " * LDFLAGS=\"$LDFLAGS\""
	echo " * LD_LIBRARY_PATH=\"$LD_LIBRARY_PATH\""
	echo
}

if [[ ! -f configure ]]; then
	./autogen.sh || die "./autogen failed."
fi

for ABI in $(get_install_abis); do
	export ABI

	echo "Building for ${ABI} ABI"

	setup_env
	print_env

	confopts=""
	confopts="${confopts} --prefix=${PREFIX}"
	confopts="${confopts} --libdir=${PREFIX}/$(get_libdir)"
	confopts="${confopts} --with-accel=$(get_accel_type)"
	confopts="${confopts} --enable-theora"
	confopts="${confopts} --enable-debug"

	mkdir -p "abi-${ABI}"
	pushd "abi-${ABI}"
		if [[ ! -f config.h ]]; then
			if [[ "${ABI}" = "x86" ]] && [[ "${ABI}" != "$(get_install_abis)" ]]; then
				# do not compile capseo tools when building multilib and for a prior ABI
				confopts="${confopts} --disable-tools"
			fi

			../configure ${confopts} \
				|| die "configure failed for ${ABI} ABI"
		fi

		make || die "make failed for ${ABI} ABI"

		if [[ ${INSTALL} = "yes" ]]; then
			make install || "make install failed for ${ABI} ABI"
		fi
	popd
done

echo
if [[ ${INSTALL} != "yes" ]]; then
	echo "Now run \`$0 --install\` to actually install the software."
else
	echo "SUCCESS!"
fi
