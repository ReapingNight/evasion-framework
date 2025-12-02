#/bin/bash
SOURCEDIR=/home/nox/GitHub/updated-evasion/evasion-kernels/linux-6.1.129-evasion
BUILDDIR=/home/nox/GitHub/updated-evasion/evasion-kernels/linux-6.1.129-evasion

./bin/syz-extract -os linux -arch arm64 -sourcedir $SOURCEDIR -builddir $BUILDDIR $1
./tools/syz-env TARGETARCH=arm64 make generate
./tools/syz-env TARGETARCH=arm64 make
